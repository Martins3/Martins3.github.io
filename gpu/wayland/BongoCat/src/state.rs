use std::time::{Duration, Instant};

pub const HOLD: Duration = Duration::from_millis(80);

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Visual {
    pub keys: u8,
    pub mouse: [i32; 2],
}

// Bit 0 controls the keyboard paw, bit 1 controls the mouse paw.
pub fn side(code: u16) -> Option<usize> {
    match code {
        1..=255 => Some(0),
        272..=274 => Some(1),
        _ => None,
    }
}

#[derive(Default)]
pub struct Pose {
    devices: Vec<[u64; 5]>,
    held: [usize; 2],
    until: [Option<Instant>; 2],
    shown: u8,
    unseen: [bool; 2],
    motion: [i32; 2],
}

impl Pose {
    pub fn relative(&mut self, axis: u16, delta: i32) {
        if axis > 1 {
            return;
        }
        let limit = [320, 192][axis as usize];
        self.motion[axis as usize] = self.motion[axis as usize]
            .saturating_add(delta)
            .clamp(-limit, limit);
    }

    pub fn visual(&self, now: Instant) -> Visual {
        Visual {
            keys: self.current(now),
            mouse: self.motion.map(|v| v / 8),
        }
    }
    pub fn new(devices: usize) -> Self {
        Self {
            devices: vec![[0; 5]; devices],
            ..Self::default()
        }
    }

    pub fn key(&mut self, device: usize, code: u16, value: i32, now: Instant) {
        let Some(side) = side(code) else { return };
        let word = &mut self.devices[device][code as usize / 64];
        let mask = 1 << (code % 64);
        match (value, *word & mask != 0) {
            (1, false) => {
                *word |= mask;
                self.held[side] += 1;
                self.until[side] = Some(now + HOLD);
                self.unseen[side] |= self.shown & (1 << side) == 0;
            }
            (0, true) => {
                *word &= !mask;
                self.held[side] -= 1;
            }
            _ => (), // Autorepeat and duplicate edges must not affect counts.
        }
    }

    pub fn disconnect(&mut self, device: usize) {
        let keys = std::mem::take(&mut self.devices[device]);
        for code in 1..=274 {
            if keys[code as usize / 64] & (1 << (code % 64)) == 0 {
                continue;
            }
            let side = side(code).unwrap();
            self.held[side] -= 1;
            if self.held[side] == 0 {
                self.until[side] = None;
                self.unseen[side] = false;
            }
        }
    }

    pub fn current(&self, now: Instant) -> u8 {
        (0..2).fold(0, |bits, side| {
            bits | (u8::from(
                self.held[side] > 0
                    || self.unseen[side]
                    || self.until[side].is_some_and(|t| t > now),
            ) << side)
        })
    }

    pub fn presented(&mut self, frame: u8, now: Instant) {
        for side in 0..2 {
            if frame & (1 << side) != 0 && self.shown & (1 << side) == 0 {
                self.until[side] = Some(now + HOLD);
            }
            if frame & (1 << side) != 0 {
                self.unseen[side] = false;
            }
        }
        self.shown = frame;
    }

    pub fn deadline(&self, now: Instant) -> Option<Instant> {
        (0..2)
            .filter(|&s| self.held[s] == 0)
            .filter_map(|s| self.until[s])
            .filter(|&t| t > now)
            .min()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn mouse_motion_accumulates_clamps_and_ignores_wheels() {
        let mut p = Pose::new(1);
        let now = Instant::now();
        p.relative(0, 4);
        assert_eq!(p.visual(now).mouse, [0, 0]);
        p.relative(0, 4);
        p.relative(1, -16);
        assert_eq!(p.visual(now).mouse, [1, -2]);
        p.relative(8, 100); // REL_WHEEL
        p.key(0, 30, 1, now);
        assert_eq!(p.visual(now).mouse, [1, -2]);
        p.relative(0, i32::MAX);
        p.relative(1, i32::MIN);
        assert_eq!(p.visual(now).mouse, [40, -24]);
        p.relative(0, -8);
        assert_eq!(p.visual(now).mouse, [39, -24]);
    }

    #[test]
    fn a_short_tap_survives_a_delayed_compositor_frame() {
        let now = Instant::now();
        let mut p = Pose::new(1);
        p.key(0, 30, 1, now);
        p.key(0, 30, 0, now);
        let later = now + Duration::from_secs(2);
        assert_eq!(p.current(later), 1);
        p.presented(1, later);
        assert_eq!(p.current(later + HOLD), 0);
    }

    #[test]
    fn chords_and_multiple_devices_do_not_release_each_other() {
        let now = Instant::now();
        let mut p = Pose::new(2);
        for (device, code) in [(0, 30), (0, 31), (1, 30)] {
            p.key(device, code, 1, now);
        }
        p.key(0, 30, 2, now);
        p.key(0, 30, 0, now);
        p.key(0, 31, 0, now);
        assert_eq!(p.current(now + HOLD), 1);
        p.disconnect(1);
        assert_eq!(p.current(now + HOLD), 0);
    }

    #[test]
    fn short_click_is_visible_and_combines_with_keyboard() {
        let now = Instant::now();
        let mut p = Pose::new(1);
        p.key(0, 30, 1, now);
        p.key(0, 272, 1, now);
        p.key(0, 272, 0, now);
        assert_eq!(p.current(now), 3);
        let presented = now + Duration::from_millis(50);
        p.presented(3, presented);
        assert_eq!(p.current(now + HOLD), 3);
        assert_eq!(p.current(presented + HOLD), 1);
        assert_eq!(p.deadline(presented + HOLD), None);
    }

    #[test]
    fn repeats_unmatched_releases_and_non_keys_are_ignored() {
        let now = Instant::now();
        let mut p = Pose::new(1);
        for (code, value) in [(30, 2), (30, 0), (0, 1), (275, 1)] {
            p.key(0, code, value, now);
        }
        assert_eq!(p.current(now), 0);
        assert_eq!(p.deadline(now), None);
        p.key(0, 30, 1, now);
        p.key(0, 30, 1, now);
        p.key(0, 30, 0, now);
        p.presented(1, now);
        assert_eq!(p.current(now + HOLD), 0);
    }
}
