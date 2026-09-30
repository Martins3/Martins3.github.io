use crate::config::PolicyConfig;

pub const MIB: u64 = 1024 * 1024;

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ActionKind {
    Reclaim,
    Return,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Decision {
    pub action: ActionKind,
    pub target: u64,
    pub reserve: u64,
    pub min_logical: u64,
}

#[must_use]
pub fn percentage(capacity: u64, percent: u8) -> u64 {
    u64::try_from((u128::from(capacity) * u128::from(percent)) / 100).unwrap_or(u64::MAX)
}

#[must_use]
pub fn thresholds(policy: &PolicyConfig, capacity: u64) -> (u64, u64, u64) {
    let reserve = (policy.reserve_mib * MIB).max(percentage(capacity, policy.reserve_percent));
    let min_logical = (policy.min_logical_mib * MIB)
        .max(percentage(capacity, policy.min_logical_percent))
        .min(capacity);
    (
        reserve.min(capacity),
        min_logical,
        policy.hysteresis_mib * MIB,
    )
}

#[must_use]
pub fn decide(
    policy: &PolicyConfig,
    capacity: u64,
    actual: u64,
    available: u64,
    confirmed_high_samples: u32,
) -> Option<Decision> {
    let (reserve, min_logical, hysteresis) = thresholds(policy, capacity);
    if actual < min_logical {
        return Some(Decision {
            action: ActionKind::Return,
            target: min_logical,
            reserve,
            min_logical,
        });
    }
    if available < reserve {
        let shortage = reserve.saturating_add(hysteresis).saturating_sub(available);
        let target = actual.saturating_add(shortage).min(capacity);
        if target > actual {
            return Some(Decision {
                action: ActionKind::Return,
                target,
                reserve,
                min_logical,
            });
        }
        return None;
    }
    if available > reserve.saturating_add(hysteresis)
        && confirmed_high_samples >= policy.confirm_samples
    {
        let excess = available.saturating_sub(reserve);
        let step = (policy.reclaim_step_mib * MIB).min(excess);
        let target = actual.saturating_sub(step).max(min_logical);
        if target < actual {
            return Some(Decision {
                action: ActionKind::Reclaim,
                target,
                reserve,
                min_logical,
            });
        }
    }
    None
}

#[cfg(test)]
mod tests {
    use super::*;

    fn gib(value: u64) -> u64 {
        value * 1024 * MIB
    }

    #[test]
    fn balanced_thresholds_use_percentage_for_large_vm() {
        let (reserve, minimum, hysteresis) = thresholds(&PolicyConfig::default(), gib(24));
        assert_eq!(reserve, percentage(gib(24), 20));
        assert_eq!(minimum, gib(6));
        assert_eq!(hysteresis, 512 * MIB);
    }

    #[test]
    fn reclaim_is_confirmed_and_step_limited() {
        let policy = PolicyConfig::default();
        assert_eq!(decide(&policy, gib(8), gib(8), gib(7), 2), None);
        let decision = decide(&policy, gib(8), gib(8), gib(7), 3).unwrap();
        assert_eq!(decision.action, ActionKind::Reclaim);
        assert_eq!(decision.target, gib(8) - 512 * MIB);
    }

    #[test]
    fn return_is_immediate_and_aims_above_reserve() {
        let policy = PolicyConfig::default();
        let decision = decide(&policy, gib(8), gib(4), gib(1), 0).unwrap();
        assert_eq!(decision.action, ActionKind::Return);
        assert_eq!(decision.target, gib(5) + 512 * MIB);
    }

    #[test]
    fn logical_floor_is_always_enforced() {
        let policy = PolicyConfig::default();
        let decision = decide(&policy, gib(24), gib(4), gib(3), 0).unwrap();
        assert_eq!(decision.action, ActionKind::Return);
        assert_eq!(decision.target, gib(6));
    }
}
