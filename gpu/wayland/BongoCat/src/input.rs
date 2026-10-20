use std::{
    io,
    os::fd::{AsFd, OwnedFd},
    path::{Path, PathBuf},
    time::Instant,
};

use calloop::{Interest, LoopHandle, Mode, PostAction, generic::Generic};
use evdev::{Device, EventType};
use inotify::{Inotify, WatchMask};

use crate::{App, Result, state::Pose};

pub struct Input {
    paths: Vec<PathBuf>,
    devices: Vec<Option<Device>>,
    watch: Inotify,
    pub rescan: bool,
}

// EVIOCSMASK is per open fd: it never changes another application's input.
// Keep keys and X/Y motion; suppress wheel, scan-code and other unused events.
fn filter_events(device: &Device) -> io::Result<()> {
    use rustix::ioctl::{Setter, ioctl, opcode};
    #[repr(C)]
    struct InputMask {
        kind: u32,
        size: u32,
        codes: u64,
    }
    const SET_MASK: rustix::ioctl::Opcode = opcode::write::<InputMask>(b'E', 0x93);
    // evdev's bits_from_user() requires a whole native unsigned long,
    // even though the event-type bitmap contains only 32 bits.
    // SAFETY: Linux input.h defines EVIOCSMASK as _IOW('E', 0x93, input_mask).
    // Both the repr(C) struct and its bitmap are valid for the entire ioctl;
    // the kernel copies the bitmap synchronously and does not retain pointers.
    for (kind, bits) in [(0, 0b111), (2, 0b11)] {
        // SYN/KEY/REL, then REL_X/REL_Y
        let types: std::ffi::c_ulong = bits;
        let mask = InputMask {
            kind,
            size: std::mem::size_of_val(&types) as u32,
            codes: (&types as *const std::ffi::c_ulong) as u64,
        };
        unsafe {
            ioctl(device, Setter::<SET_MASK, _>::new(mask))?;
        }
    }
    Ok(())
}

fn source(fd: impl AsFd) -> io::Result<Generic<OwnedFd>> {
    Ok(Generic::new(
        fd.as_fd().try_clone_to_owned()?,
        Interest::READ,
        Mode::Level,
    ))
}

impl Input {
    pub fn new(paths: Vec<PathBuf>, handle: &LoopHandle<'static, App>) -> Result<Self> {
        let watch = Inotify::init()?;
        // Watching the symlink parent also observes udev creating the stable
        // by-id link after the event node itself has appeared.
        let mut parents = vec![PathBuf::from("/dev/input")];
        parents.extend(
            paths
                .iter()
                .filter_map(|p| p.parent().map(Path::to_path_buf)),
        );
        parents.sort();
        parents.dedup();
        for parent in parents {
            watch.watches().add(
                parent,
                WatchMask::CREATE | WatchMask::DELETE | WatchMask::MOVED_TO | WatchMask::ATTRIB,
            )?;
        }
        handle.insert_source(source(&watch)?, |_, _, app| {
            let mut bytes = [0; 4096];
            loop {
                match app.input.watch.read_events(&mut bytes) {
                    Ok(events) => {
                        for _ in events {}
                        app.input.rescan = true;
                    }
                    Err(e) if e.kind() == io::ErrorKind::WouldBlock => break,
                    Err(e) => return Err(e),
                }
            }
            Ok(PostAction::Continue)
        })?;
        Ok(Self {
            devices: (0..paths.len()).map(|_| None).collect(),
            paths,
            watch,
            rescan: true,
        })
    }

    pub fn connect(
        &mut self,
        handle: &LoopHandle<'static, App>,
        pose: &mut Pose,
        startup: bool,
    ) -> Result<()> {
        self.rescan = false;
        for index in 0..self.paths.len() {
            if self.devices[index].is_some() {
                continue;
            }
            let path = &self.paths[index];
            let opened = (|| -> io::Result<Device> {
                let device = Device::from_fd(std::fs::File::open(path)?.into())?;
                if !device
                    .supported_keys()
                    .is_some_and(|keys| keys.iter().any(|k| crate::state::side(k.code()).is_some()))
                {
                    return Err(io::Error::new(
                        io::ErrorKind::InvalidInput,
                        "device has no supported keys/buttons",
                    ));
                }
                device.set_nonblocking(true)?;
                filter_events(&device)
                    .map_err(|e| io::Error::new(e.kind(), format!("EVIOCSMASK: {e}")))?;
                Ok(device)
            })();
            let device = match opened {
                Ok(device) => device,
                Err(e) if startup => {
                    let hint = if e.kind() == io::ErrorKind::PermissionDenied {
                        "; see README for device permissions"
                    } else {
                        ""
                    };
                    return Err(format!("{}: {e}{hint}", path.display()).into());
                }
                Err(_) => continue, // Retry only on the next filesystem notification.
            };
            let now = Instant::now();
            for key in device.get_key_state()?.iter() {
                pose.key(index, key.code(), 1, now);
            }
            handle.insert_source(source(&device)?, move |_, _, app| {
                match app.input.read(index, &mut app.pose) {
                    Ok(()) => Ok(PostAction::Continue),
                    Err(e) => {
                        eprintln!("{}: disconnected ({e})", app.input.paths[index].display());
                        app.input.devices[index] = None;
                        app.pose.disconnect(index);
                        app.input.rescan = true;
                        Ok(PostAction::Remove)
                    }
                }
            })?;
            eprintln!(
                "input: {} ({})",
                path.display(),
                device.name().unwrap_or("unknown")
            );
            self.devices[index] = Some(device);
        }
        Ok(())
    }

    fn read(&mut self, index: usize, pose: &mut Pose) -> io::Result<()> {
        let device = self.devices[index].as_mut().unwrap();
        // Bound work per dispatch so pathological devices cannot starve Wayland.
        for _ in 0..32 {
            match device.fetch_events() {
                Ok(events) => {
                    let now = Instant::now();
                    // evdev::Device synthesizes recovery edges after SYN_DROPPED.
                    for event in events {
                        if event.event_type() == EventType::KEY {
                            pose.key(index, event.code(), event.value(), now);
                        } else if event.event_type() == EventType::RELATIVE {
                            pose.relative(event.code(), event.value());
                        }
                    }
                }
                Err(e) if e.kind() == io::ErrorKind::WouldBlock => return Ok(()),
                Err(e) => return Err(e),
            }
        }
        Ok(())
    }
}

pub fn list() -> Result<()> {
    let mut entries = std::fs::read_dir("/dev/input")?.collect::<io::Result<Vec<_>>>()?;
    entries.sort_by_key(|e| e.path());
    for entry in entries {
        if !entry.file_name().to_string_lossy().starts_with("event") {
            continue;
        }
        match std::fs::File::open(entry.path()).and_then(|file| Device::from_fd(file.into())) {
            Ok(device) => println!(
                "{}  {}",
                entry.path().display(),
                device.name().unwrap_or("unknown")
            ),
            Err(e) => println!("{}  {e}", entry.path().display()),
        }
    }
    println!("Use stable /dev/input/by-id/* paths with --device.");
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use rustix::ioctl::{Updater, ioctl, opcode};

    fn event_mask(device: &Device, kind: u32) -> std::ffi::c_ulong {
        #[repr(C)]
        struct Mask {
            kind: u32,
            size: u32,
            codes: u64,
        }
        const GET_MASK: rustix::ioctl::Opcode = opcode::read::<Mask>(b'E', 0x92);
        let mut bits: std::ffi::c_ulong = 0;
        let mut mask = Mask {
            kind,
            size: std::mem::size_of_val(&bits) as u32,
            codes: (&mut bits as *mut std::ffi::c_ulong) as u64,
        };
        // SAFETY: EVIOCGMASK copies at most size bytes to the live, writable
        // bitmap. Both the bitmap and repr(C) argument outlive the syscall.
        unsafe {
            ioctl(device, Updater::<GET_MASK, _>::new(&mut mask)).unwrap();
        }
        bits
    }

    #[test]
    #[ignore = "requires BONGOCAT_TEST_DEVICE pointing to a readable evdev device"]
    fn live_evdev_mask_round_trip() {
        let path = std::env::var_os("BONGOCAT_TEST_DEVICE").expect("set BONGOCAT_TEST_DEVICE");
        let open = || Device::from_fd(std::fs::File::open(&path).unwrap().into()).unwrap();
        let filtered = open();
        let independent = open();
        let before = event_mask(&independent, 0);
        let before_rel = event_mask(&independent, 2);
        filter_events(&filtered).unwrap();
        assert_eq!(
            event_mask(&filtered, 0),
            0b111,
            "only SYN, KEY, REL are enabled"
        );
        assert_eq!(event_mask(&filtered, 2), 0b11, "only X/Y motion, not wheel");
        assert_eq!(
            event_mask(&independent, 0),
            before,
            "other readers must not be affected"
        );
        assert_eq!(event_mask(&independent, 2), before_rel);
    }
}
