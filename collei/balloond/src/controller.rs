use std::collections::{BTreeSet, HashMap};
use std::fs::{self, File, OpenOptions};
use std::path::PathBuf;
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

use anyhow::{Context, Result, anyhow, bail};
use fs2::FileExt;
use serde::Serialize;
use serde_json::{Value, json};
use tokio::signal::unix::{SignalKind, signal};
use tokio::time::{MissedTickBehavior, interval, sleep};
use tracing::{debug, info, trace, warn};

use crate::config::{Config, PolicyConfig};
use crate::discovery::{VmCandidate, VmRuntime, discover_vms};
use crate::policy::{ActionKind, MIB, decide, thresholds};
use crate::qmp::QmpClient;

const BALLOON_PATH: &str = "/machine/peripheral/balloon0";
const ACTUAL_TOLERANCE: u64 = 4 * MIB;

#[derive(Debug, Clone, Serialize)]
pub struct VmReport {
    pub name: String,
    pub pid: Option<u32>,
    pub capacity_mib: Option<u64>,
    pub current_mib: Option<u64>,
    /// QEMU host resident memory, excluding swapped-out pages.
    pub rss_mib: Option<u64>,
    pub available_mib: Option<u64>,
    pub reserve_mib: Option<u64>,
    pub last_update: Option<u64>,
    pub manageable: bool,
    pub reason: String,
}

#[derive(Debug, Clone, Serialize)]
pub struct RestoreResult {
    pub name: String,
    pub before_mib: u64,
    pub target_mib: u64,
    pub actual_mib: u64,
}

pub struct InstanceLock {
    _file: File,
}

impl InstanceLock {
    /// Acquires the process-wide balloon controller lock.
    ///
    /// # Errors
    ///
    /// Returns an error when the runtime directory is unavailable, the lock
    /// file cannot be opened, or another controller already owns the lock.
    pub fn acquire() -> Result<Self> {
        let runtime_dir = std::env::var_os("XDG_RUNTIME_DIR")
            .map(PathBuf::from)
            .context("XDG_RUNTIME_DIR is not set")?;
        let path = runtime_dir.join("collei-balloond.lock");
        let file = OpenOptions::new()
            .create(true)
            .truncate(false)
            .read(true)
            .write(true)
            .open(&path)
            .with_context(|| format!("cannot open {}", path.display()))?;
        file.try_lock_exclusive().with_context(|| {
            format!(
                "another collei-balloond instance holds {}; stop it before continuing",
                path.display()
            )
        })?;
        Ok(Self { _file: file })
    }
}

#[derive(Debug)]
struct PendingCommand {
    target: u64,
    action: ActionKind,
    started: Instant,
}

#[derive(Debug)]
struct VmState {
    pid: u32,
    last_update: u64,
    high_samples: u32,
    invalid_samples: u32,
    pending: Option<PendingCommand>,
    reclaim_backoff_until: Option<Instant>,
}

#[derive(Debug)]
struct Snapshot {
    capacity: u64,
    actual: u64,
    available: Option<u64>,
    last_update: u64,
}

/// Runs the reconciliation loop until interrupted.
///
/// # Errors
///
/// Returns an error when the singleton lock, VM discovery, or signal handling
/// fails. Per-VM QMP failures are logged and do not terminate the daemon.
pub async fn run_daemon(config: &Config, only: &BTreeSet<String>, dry_run: bool) -> Result<()> {
    let _lock = InstanceLock::acquire()?;
    let mut states: HashMap<String, VmState> = HashMap::new();
    let mut ticker = interval(Duration::from_secs(config.daemon.scan_interval_seconds));
    ticker.set_missed_tick_behavior(MissedTickBehavior::Skip);
    let mut terminate =
        signal(SignalKind::terminate()).context("cannot install SIGTERM handler")?;
    info!(dry_run, "collei balloon daemon started");

    loop {
        tokio::select! {
            _ = ticker.tick() => {
                reconcile_all(config, only, dry_run, &mut states).await?;
            }
            signal = tokio::signal::ctrl_c() => {
                signal.context("cannot install Ctrl-C handler")?;
                info!("shutdown requested; balloon state is preserved");
                return Ok(());
            }
            _ = terminate.recv() => {
                info!("SIGTERM received; balloon state is preserved");
                return Ok(());
            }
        }
    }
}

async fn reconcile_all(
    config: &Config,
    only: &BTreeSet<String>,
    dry_run: bool,
    states: &mut HashMap<String, VmState>,
) -> Result<()> {
    let candidates = discover_vms(&config.collei.config, only)?;
    let live: BTreeSet<(String, u32)> = candidates
        .iter()
        .filter_map(|candidate| {
            candidate
                .runtime
                .as_ref()
                .map(|runtime| (runtime.name.clone(), runtime.pid))
        })
        .collect();
    states.retain(|name, state| live.contains(&(name.clone(), state.pid)));

    for candidate in candidates {
        let Some(runtime) = candidate.runtime else {
            warn!(vm = candidate.name, reason = candidate.reason, "VM skipped");
            continue;
        };
        let policy = config.policy_for(&runtime.name);
        if !policy.enabled {
            trace!(vm = runtime.name, "VM disabled by policy");
            continue;
        }
        if states
            .get(&runtime.name)
            .is_none_or(|state| state.pid != runtime.pid)
        {
            match attach_vm(config, &runtime).await {
                Ok(state) => {
                    info!(
                        vm = runtime.name,
                        pid = runtime.pid,
                        "VM attached; waiting for fresh stats"
                    );
                    states.insert(runtime.name.clone(), state);
                }
                Err(error) => warn!(vm = runtime.name, %error, "cannot attach VM"),
            }
            continue;
        }
        let state = states
            .get_mut(&runtime.name)
            .expect("state was checked above");
        if let Err(error) = reconcile_vm(config, &policy, &runtime, state, dry_run).await {
            warn!(vm = runtime.name, %error, "VM reconciliation failed");
        }
    }
    Ok(())
}

async fn attach_vm(config: &Config, runtime: &VmRuntime) -> Result<VmState> {
    let mut client = connect(config, runtime).await?;
    preflight(&mut client).await?;
    let baseline = query_guest_stats(&mut client).await?.1;
    client
        .execute(
            "qom-set",
            Some(json!({
                "path": BALLOON_PATH,
                "property": "guest-stats-polling-interval",
                "value": config.daemon.stats_poll_interval_seconds,
            })),
        )
        .await?;
    Ok(VmState {
        pid: runtime.pid,
        last_update: baseline,
        high_samples: 0,
        invalid_samples: 0,
        pending: None,
        reclaim_backoff_until: None,
    })
}

async fn reconcile_vm(
    config: &Config,
    policy: &PolicyConfig,
    runtime: &VmRuntime,
    state: &mut VmState,
    dry_run: bool,
) -> Result<()> {
    let mut client = connect(config, runtime).await?;
    preflight(&mut client).await?;
    let snapshot = query_snapshot(&mut client).await?;

    if runtime.vfio_passthrough {
        state.last_update = state.last_update.max(snapshot.last_update);
        state.pending = None;
        trace!(
            vm = runtime.name,
            current_mib = snapshot.actual / MIB,
            available_mib = snapshot.available.map(|value| value / MIB),
            "VFIO passthrough VM is observe-only"
        );
        return Ok(());
    }
    update_pending(config, runtime, state, snapshot.actual);

    let now = unix_now()?;
    let fresh = snapshot.last_update != 0
        && snapshot.last_update <= now.saturating_add(5)
        && now.saturating_sub(snapshot.last_update) <= config.daemon.stats_stale_after_seconds;
    let new_sample = snapshot.last_update > state.last_update;
    if !fresh || snapshot.available.is_none() {
        return handle_invalid_stats(config, runtime, state, &snapshot, &mut client, dry_run).await;
    }
    if !new_sample {
        trace!(vm = runtime.name, "waiting for a new balloon stats sample");
        return Ok(());
    }

    state.last_update = snapshot.last_update;
    state.invalid_samples = 0;
    let available = snapshot.available.expect("availability was checked above");
    let (reserve, _, hysteresis) = thresholds(policy, snapshot.capacity);
    if available > reserve.saturating_add(hysteresis) {
        state.high_samples = state.high_samples.saturating_add(1);
    } else {
        state.high_samples = 0;
    }

    let Some(decision) = decide(
        policy,
        snapshot.capacity,
        snapshot.actual,
        available,
        state.high_samples,
    ) else {
        trace!(
            vm = runtime.name,
            actual_mib = snapshot.actual / MIB,
            available_mib = available / MIB,
            reserve_mib = reserve / MIB,
            "no balloon change needed"
        );
        return Ok(());
    };

    if decision.action == ActionKind::Reclaim {
        if state.pending.is_some()
            || state
                .reclaim_backoff_until
                .is_some_and(|deadline| deadline > Instant::now())
        {
            return Ok(());
        }
    } else if state.pending.as_ref().is_some_and(|pending| {
        pending.action == ActionKind::Return && pending.target >= decision.target
    }) {
        return Ok(());
    }

    let reason = match decision.action {
        ActionKind::Reclaim => "available memory stayed above high watermark",
        ActionKind::Return => "available memory fell below reserve",
    };
    command_balloon(
        &mut client,
        runtime,
        state,
        decision.target,
        decision.action,
        dry_run,
        reason,
    )
    .await?;
    if decision.action == ActionKind::Reclaim {
        state.high_samples = 0;
    }
    Ok(())
}

async fn handle_invalid_stats(
    config: &Config,
    runtime: &VmRuntime,
    state: &mut VmState,
    snapshot: &Snapshot,
    client: &mut QmpClient,
    dry_run: bool,
) -> Result<()> {
    state.invalid_samples = state.invalid_samples.saturating_add(1);
    state.high_samples = 0;
    if state.invalid_samples >= config.daemon.invalid_samples_before_restore {
        let should_restore = snapshot.actual < snapshot.capacity
            && state
                .pending
                .as_ref()
                .is_none_or(|pending| pending.action == ActionKind::Reclaim);
        if should_restore {
            command_balloon(
                client,
                runtime,
                state,
                snapshot.capacity,
                ActionKind::Return,
                dry_run,
                "stats unavailable; restoring full capacity",
            )
            .await?;
        }
    }
    debug!(
        vm = runtime.name,
        last_update = snapshot.last_update,
        invalid_samples = state.invalid_samples,
        "balloon stats are unavailable or stale"
    );
    Ok(())
}

fn update_pending(config: &Config, runtime: &VmRuntime, state: &mut VmState, actual: u64) {
    let Some(pending) = &state.pending else {
        return;
    };
    let complete = match pending.action {
        ActionKind::Reclaim => actual <= pending.target.saturating_add(ACTUAL_TOLERANCE),
        ActionKind::Return => actual.saturating_add(ACTUAL_TOLERANCE) >= pending.target,
    };
    if complete {
        info!(
            vm = runtime.name,
            actual_mib = actual / MIB,
            target_mib = pending.target / MIB,
            "balloon command completed"
        );
        state.pending = None;
    } else if pending.started.elapsed() > Duration::from_secs(config.daemon.command_timeout_seconds)
    {
        warn!(
            vm = runtime.name,
            actual_mib = actual / MIB,
            target_mib = pending.target / MIB,
            "balloon command did not complete before timeout"
        );
        if pending.action == ActionKind::Reclaim {
            state.reclaim_backoff_until =
                Some(Instant::now() + Duration::from_secs(config.daemon.command_timeout_seconds));
        }
        state.pending = None;
    }
}

async fn command_balloon(
    client: &mut QmpClient,
    runtime: &VmRuntime,
    state: &mut VmState,
    target: u64,
    action: ActionKind,
    dry_run: bool,
    reason: &str,
) -> Result<()> {
    info!(
        vm = runtime.name,
        target_mib = target / MIB,
        ?action,
        dry_run,
        reason,
        "balloon target selected"
    );
    if dry_run {
        return Ok(());
    }
    client
        .execute("balloon", Some(json!({"value": target})))
        .await?;
    state.pending = Some(PendingCommand {
        target,
        action,
        started: Instant::now(),
    });
    Ok(())
}

/// Returns a read-only status snapshot for active Collei VMs.
///
/// # Errors
///
/// Returns an error when Collei configuration or VM discovery fails. Per-VM
/// QMP failures are represented in the returned report.
pub async fn status(config: &Config, only: &BTreeSet<String>) -> Result<Vec<VmReport>> {
    let candidates = discover_vms(&config.collei.config, only)?;
    let mut reports = Vec::new();
    for candidate in candidates {
        reports.push(report_candidate(config, candidate).await);
    }
    Ok(reports)
}

async fn report_candidate(config: &Config, candidate: VmCandidate) -> VmReport {
    let Some(runtime) = candidate.runtime else {
        return VmReport {
            name: candidate.name,
            pid: None,
            capacity_mib: None,
            current_mib: None,
            rss_mib: None,
            available_mib: None,
            reserve_mib: None,
            last_update: None,
            manageable: false,
            reason: candidate.reason.unwrap_or_else(|| "not running".to_owned()),
        };
    };
    let policy = config.policy_for(&runtime.name);
    let rss_mib = match resident_mib(runtime.pid) {
        Ok(value) => Some(value),
        Err(error) => {
            debug!(vm = runtime.name, %error, "host RSS unavailable");
            None
        }
    };
    match inspect_runtime(config, &runtime).await {
        Ok(snapshot) => {
            let now = unix_now().unwrap_or_default();
            let available = snapshot.available.filter(|_| {
                snapshot.last_update != 0
                    && snapshot.last_update <= now.saturating_add(5)
                    && now.saturating_sub(snapshot.last_update)
                        <= config.daemon.stats_stale_after_seconds
            });
            let (reserve, _, _) = thresholds(&policy, snapshot.capacity);
            let (manageable, reason) = if runtime.vfio_passthrough {
                (false, "observe-only: VFIO passthrough".to_owned())
            } else if !policy.enabled {
                (false, "disabled by configuration".to_owned())
            } else if available.is_none() {
                (false, "balloon stats unsupported or stale".to_owned())
            } else {
                (true, "ready".to_owned())
            };
            VmReport {
                name: runtime.name,
                pid: Some(runtime.pid),
                capacity_mib: Some(snapshot.capacity / MIB),
                current_mib: Some(snapshot.actual / MIB),
                rss_mib,
                available_mib: available.map(|value| value / MIB),
                reserve_mib: Some(reserve / MIB),
                last_update: Some(snapshot.last_update),
                manageable,
                reason,
            }
        }
        Err(error) => VmReport {
            name: runtime.name,
            pid: Some(runtime.pid),
            capacity_mib: None,
            current_mib: None,
            rss_mib,
            available_mib: None,
            reserve_mib: None,
            last_update: None,
            manageable: false,
            reason: error.to_string(),
        },
    }
}

fn resident_mib(pid: u32) -> Result<u64> {
    let path = format!("/proc/{pid}/statm");
    let content = fs::read_to_string(&path).with_context(|| format!("cannot read {path}"))?;
    let pages = content
        .split_whitespace()
        .nth(1)
        .context("statm omitted resident pages")?
        .parse::<u64>()
        .context("invalid statm resident pages")?;
    let page_size = u64::try_from(rustix::param::page_size())?;
    let bytes = pages.checked_mul(page_size).context("RSS size overflow")?;
    Ok(bytes / MIB)
}

/// Restores selected active VMs to their full QEMU memory capacity.
///
/// # Errors
///
/// Returns an error when another daemon owns the lock, discovery fails, a VM
/// is migrating, or any selected VM cannot reach full capacity before timeout.
pub async fn restore(config: &Config, only: &BTreeSet<String>) -> Result<Vec<RestoreResult>> {
    let _lock = InstanceLock::acquire()?;
    let candidates = discover_vms(&config.collei.config, only)?;
    let mut results = Vec::new();
    let mut errors = Vec::new();
    for candidate in candidates {
        let Some(runtime) = candidate.runtime else {
            errors.push(format!(
                "{}: {}",
                candidate.name,
                candidate.reason.unwrap_or_else(|| "not running".to_owned())
            ));
            continue;
        };
        match restore_runtime(config, &runtime).await {
            Ok(result) => results.push(result),
            Err(error) => errors.push(format!("{}: {error}", runtime.name)),
        }
    }
    if errors.is_empty() {
        Ok(results)
    } else {
        bail!("restore incomplete: {}", errors.join("; "))
    }
}

async fn restore_runtime(config: &Config, runtime: &VmRuntime) -> Result<RestoreResult> {
    let mut client = connect(config, runtime).await?;
    preflight(&mut client).await?;
    let capacity = query_capacity(&mut client).await?;
    let before = query_actual(&mut client).await?;
    if before < capacity {
        client
            .execute("balloon", Some(json!({"value": capacity})))
            .await?;
    }
    let deadline = Instant::now() + Duration::from_secs(config.daemon.command_timeout_seconds);
    let mut actual = before;
    while actual.saturating_add(ACTUAL_TOLERANCE) < capacity && Instant::now() < deadline {
        sleep(Duration::from_millis(500)).await;
        actual = query_actual(&mut client).await?;
    }
    if actual.saturating_add(ACTUAL_TOLERANCE) < capacity {
        bail!(
            "restore timed out: actual={} MiB target={} MiB",
            actual / MIB,
            capacity / MIB
        );
    }
    Ok(RestoreResult {
        name: runtime.name.clone(),
        before_mib: before / MIB,
        target_mib: capacity / MIB,
        actual_mib: actual / MIB,
    })
}

async fn inspect_runtime(config: &Config, runtime: &VmRuntime) -> Result<Snapshot> {
    let mut client = connect(config, runtime).await?;
    preflight(&mut client).await?;
    query_snapshot(&mut client).await
}

async fn connect(config: &Config, runtime: &VmRuntime) -> Result<QmpClient> {
    QmpClient::connect(
        &runtime.qmp_path,
        Duration::from_secs(config.daemon.qmp_timeout_seconds),
    )
    .await
    .map_err(Into::into)
}

async fn preflight(client: &mut QmpClient) -> Result<()> {
    let status = client.execute("query-status", None).await?;
    let run_state = status
        .get("status")
        .and_then(Value::as_str)
        .context("query-status omitted status")?;
    if run_state != "running" {
        bail!("VM run state is {run_state}");
    }
    let migration = client.execute("query-migrate", None).await?;
    if let Some(migration_state) = migration.get("status").and_then(Value::as_str)
        && !matches!(
            migration_state,
            "none" | "completed" | "failed" | "cancelled"
        )
    {
        bail!("migration state is {migration_state}");
    }
    Ok(())
}

async fn query_snapshot(client: &mut QmpClient) -> Result<Snapshot> {
    let capacity = query_capacity(client).await?;
    let actual = query_actual(client).await?;
    let (available, last_update) = query_guest_stats(client).await?;
    Ok(Snapshot {
        capacity,
        actual,
        available,
        last_update,
    })
}

async fn query_capacity(client: &mut QmpClient) -> Result<u64> {
    let memory = client.execute("query-memory-size-summary", None).await?;
    let base = required_u64(&memory, "base-memory")?;
    let plugged = optional_u64(&memory, "plugged-memory")?.unwrap_or_default();
    base.checked_add(plugged)
        .context("VM memory capacity overflow")
}

async fn query_actual(client: &mut QmpClient) -> Result<u64> {
    let balloon = client.execute("query-balloon", None).await?;
    required_u64(&balloon, "actual")
}

async fn query_guest_stats(client: &mut QmpClient) -> Result<(Option<u64>, u64)> {
    let response = client
        .execute(
            "qom-get",
            Some(json!({"path": BALLOON_PATH, "property": "guest-stats"})),
        )
        .await?;
    let last_update = required_u64(&response, "last-update")?;
    let available = response
        .get("stats")
        .and_then(|stats| stats.get("stat-available-memory"))
        .and_then(stat_value);
    Ok((available, last_update))
}

fn stat_value(value: &Value) -> Option<u64> {
    if value.as_i64() == Some(-1) || value.as_u64() == Some(u64::MAX) {
        None
    } else {
        value.as_u64()
    }
}

fn required_u64(value: &Value, field: &str) -> Result<u64> {
    optional_u64(value, field)?.ok_or_else(|| anyhow!("QMP response omitted {field}"))
}

fn optional_u64(value: &Value, field: &str) -> Result<Option<u64>> {
    value
        .get(field)
        .map(|field_value| {
            field_value
                .as_u64()
                .ok_or_else(|| anyhow!("QMP field {field} is not an unsigned integer"))
        })
        .transpose()
}

fn unix_now() -> Result<u64> {
    Ok(SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .context("system clock is before the Unix epoch")?
        .as_secs())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn unsupported_stat_sentinels_are_rejected() {
        assert_eq!(stat_value(&json!(-1)), None);
        assert_eq!(stat_value(&json!(u64::MAX)), None);
        assert_eq!(stat_value(&json!(123)), Some(123));
    }
}
