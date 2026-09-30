use std::collections::BTreeMap;
use std::env;
use std::fs;
use std::path::{Path, PathBuf};

use anyhow::{Context, Result, bail};
use serde::Deserialize;

const MIB: u64 = 1024 * 1024;

#[derive(Debug, Clone, Default, Deserialize)]
#[serde(default, deny_unknown_fields)]
pub struct Config {
    pub collei: ColleiConfig,
    pub daemon: DaemonConfig,
    pub defaults: PolicyConfig,
    pub vms: BTreeMap<String, VmPolicyOverride>,
}

#[derive(Debug, Clone, Deserialize)]
#[serde(default, deny_unknown_fields)]
pub struct ColleiConfig {
    pub config: PathBuf,
}

impl Default for ColleiConfig {
    fn default() -> Self {
        Self {
            config: PathBuf::from("~/.config/collei/config.ini"),
        }
    }
}

#[derive(Debug, Clone, Deserialize)]
#[serde(default, deny_unknown_fields)]
pub struct DaemonConfig {
    pub scan_interval_seconds: u64,
    pub stats_poll_interval_seconds: u64,
    pub stats_stale_after_seconds: u64,
    pub invalid_samples_before_restore: u32,
    pub qmp_timeout_seconds: u64,
    pub command_timeout_seconds: u64,
}

impl Default for DaemonConfig {
    fn default() -> Self {
        Self {
            scan_interval_seconds: 10,
            stats_poll_interval_seconds: 5,
            stats_stale_after_seconds: 20,
            invalid_samples_before_restore: 3,
            qmp_timeout_seconds: 2,
            command_timeout_seconds: 60,
        }
    }
}

#[derive(Debug, Clone, Deserialize)]
#[serde(default, deny_unknown_fields)]
pub struct PolicyConfig {
    pub enabled: bool,
    pub reserve_mib: u64,
    pub reserve_percent: u8,
    pub min_logical_mib: u64,
    pub min_logical_percent: u8,
    pub hysteresis_mib: u64,
    pub reclaim_step_mib: u64,
    pub confirm_samples: u32,
}

impl Default for PolicyConfig {
    fn default() -> Self {
        Self {
            enabled: true,
            reserve_mib: 2048,
            reserve_percent: 20,
            min_logical_mib: 2048,
            min_logical_percent: 25,
            hysteresis_mib: 512,
            reclaim_step_mib: 512,
            confirm_samples: 3,
        }
    }
}

#[derive(Debug, Clone, Default, Deserialize)]
#[serde(default, deny_unknown_fields)]
pub struct VmPolicyOverride {
    pub enabled: Option<bool>,
    pub reserve_mib: Option<u64>,
    pub reserve_percent: Option<u8>,
    pub min_logical_mib: Option<u64>,
    pub min_logical_percent: Option<u8>,
    pub hysteresis_mib: Option<u64>,
    pub reclaim_step_mib: Option<u64>,
    pub confirm_samples: Option<u32>,
}

impl Config {
    /// Loads and validates a balloon daemon configuration.
    ///
    /// # Errors
    ///
    /// Returns an error when the path cannot be expanded or read, TOML parsing
    /// fails, or a value violates the configuration invariants.
    pub fn load(path: &Path) -> Result<Self> {
        let path = expand_tilde(path)?;
        let content =
            fs::read_to_string(&path).with_context(|| format!("cannot read {}", path.display()))?;
        let config: Self =
            toml::from_str(&content).with_context(|| format!("cannot parse {}", path.display()))?;
        config.validate()?;
        Ok(config)
    }

    /// Validates all global and per-VM configuration values.
    ///
    /// # Errors
    ///
    /// Returns an error for zero intervals, invalid percentages, overflows, or
    /// empty VM names.
    pub fn validate(&self) -> Result<()> {
        let daemon = &self.daemon;
        if daemon.scan_interval_seconds == 0
            || daemon.stats_poll_interval_seconds == 0
            || daemon.stats_stale_after_seconds == 0
            || daemon.invalid_samples_before_restore == 0
            || daemon.qmp_timeout_seconds == 0
            || daemon.command_timeout_seconds == 0
        {
            bail!("daemon intervals, timeouts and sample counts must be non-zero");
        }
        if daemon.stats_stale_after_seconds <= daemon.stats_poll_interval_seconds {
            bail!("stats_stale_after_seconds must exceed stats_poll_interval_seconds");
        }
        self.defaults.validate("defaults")?;
        for (name, override_config) in &self.vms {
            self.policy_for(name).validate(&format!("vms.{name:?}"))?;
            if name.is_empty() {
                bail!("VM override name cannot be empty");
            }
            let _ = override_config;
        }
        Ok(())
    }

    #[must_use]
    pub fn policy_for(&self, name: &str) -> PolicyConfig {
        let mut policy = self.defaults.clone();
        if let Some(overrides) = self.vms.get(name) {
            macro_rules! replace {
                ($field:ident) => {
                    if let Some(value) = overrides.$field {
                        policy.$field = value;
                    }
                };
            }
            replace!(enabled);
            replace!(reserve_mib);
            replace!(reserve_percent);
            replace!(min_logical_mib);
            replace!(min_logical_percent);
            replace!(hysteresis_mib);
            replace!(reclaim_step_mib);
            replace!(confirm_samples);
        }
        policy
    }
}

impl PolicyConfig {
    fn validate(&self, location: &str) -> Result<()> {
        if self.reserve_percent > 100 || self.min_logical_percent > 100 {
            bail!("{location}: percentages must be between 0 and 100");
        }
        if self.reserve_mib == 0
            || self.min_logical_mib == 0
            || self.hysteresis_mib == 0
            || self.reclaim_step_mib == 0
            || self.confirm_samples == 0
        {
            bail!("{location}: memory values and confirm_samples must be non-zero");
        }
        for value in [
            self.reserve_mib,
            self.min_logical_mib,
            self.hysteresis_mib,
            self.reclaim_step_mib,
        ] {
            if value > u64::MAX / MIB {
                bail!("{location}: memory value is too large");
            }
        }
        Ok(())
    }
}

/// Expands a leading `~/` using `HOME`.
///
/// # Errors
///
/// Returns an error when expansion is requested but `HOME` is unset.
pub fn expand_tilde(path: &Path) -> Result<PathBuf> {
    let text = path.to_string_lossy();
    if text == "~" || text.starts_with("~/") {
        let home = env::var_os("HOME").context("HOME is not set")?;
        let mut expanded = PathBuf::from(home);
        if text.len() > 2 {
            expanded.push(&text[2..]);
        }
        Ok(expanded)
    } else {
        Ok(path.to_path_buf())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn vm_override_only_replaces_selected_fields() {
        let mut config = Config::default();
        config.vms.insert(
            "guest".to_owned(),
            VmPolicyOverride {
                enabled: Some(false),
                reserve_mib: Some(4096),
                ..VmPolicyOverride::default()
            },
        );
        let policy = config.policy_for("guest");
        assert!(!policy.enabled);
        assert_eq!(policy.reserve_mib, 4096);
        assert_eq!(policy.reserve_percent, 20);
    }

    #[test]
    fn rejects_unknown_fields() {
        let error = toml::from_str::<Config>("[daemon]\nunknown = 1").unwrap_err();
        assert!(error.to_string().contains("unknown field"));
    }
}
