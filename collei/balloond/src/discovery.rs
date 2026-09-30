use std::collections::BTreeSet;
use std::ffi::OsStr;
use std::fs;
use std::os::unix::fs::FileTypeExt;
use std::path::{Path, PathBuf};

use anyhow::{Context, Result, bail};

use crate::config::expand_tilde;

#[derive(Debug, Clone)]
pub struct VmRuntime {
    pub name: String,
    pub pid: u32,
    pub qmp_path: PathBuf,
    pub vfio_passthrough: bool,
}

#[derive(Debug, Clone)]
pub struct VmCandidate {
    pub name: String,
    pub runtime: Option<VmRuntime>,
    pub reason: Option<String>,
}

/// Reads the Collei VM root from the `[collei]` section.
///
/// # Errors
///
/// Returns an error when the configuration cannot be read or has no usable
/// `vm` entry.
pub fn collei_vm_root(config_path: &Path) -> Result<PathBuf> {
    let config_path = expand_tilde(config_path)?;
    let content = fs::read_to_string(&config_path)
        .with_context(|| format!("cannot read {}", config_path.display()))?;
    let mut in_collei = false;
    for raw_line in content.lines() {
        let line = raw_line.trim();
        if line.starts_with('[') && line.ends_with(']') {
            in_collei = line == "[collei]";
            continue;
        }
        if !in_collei || line.is_empty() || line.starts_with(['#', ';']) {
            continue;
        }
        let Some((key, value)) = line.split_once('=') else {
            continue;
        };
        if key.trim() == "vm" {
            let value = value.trim();
            if value.is_empty() {
                bail!("empty vm path in {}", config_path.display());
            }
            return expand_tilde(Path::new(value));
        }
    }
    bail!("missing [collei] vm in {}", config_path.display())
}

/// Discovers live QEMU processes with validated Collei PID files and QMP sockets.
///
/// # Errors
///
/// Returns an error when the Collei configuration or VM root cannot be read.
pub fn discover_vms(config_path: &Path, only: &BTreeSet<String>) -> Result<Vec<VmCandidate>> {
    let root = collei_vm_root(config_path)?;
    let entries = fs::read_dir(&root)
        .with_context(|| format!("cannot list Collei VM root {}", root.display()))?;
    let mut result = Vec::new();
    for entry in entries {
        let entry = entry?;
        let vm_dir = entry.path();
        if !vm_dir.is_dir() || !vm_dir.join("config.ini").is_file() {
            continue;
        }
        let Some(name) = vm_dir.file_name().and_then(OsStr::to_str) else {
            continue;
        };
        if !only.is_empty() && !only.contains(name) {
            continue;
        }
        let mut runtimes = Vec::new();
        for monitor_dir in [vm_dir.join("s"), vm_dir.join("t"), vm_dir.clone()] {
            let pid_path = monitor_dir.join("pid");
            let Ok(pid_text) = fs::read_to_string(&pid_path) else {
                continue;
            };
            let Ok(pid) = pid_text.trim().parse::<u32>() else {
                continue;
            };
            let qmp_path = monitor_dir.join("qmp-no-pretty");
            if let Some(vfio_passthrough) = process_details(pid, &pid_path)
                && is_socket(&qmp_path)
            {
                runtimes.push(VmRuntime {
                    name: name.to_owned(),
                    pid,
                    qmp_path,
                    vfio_passthrough,
                });
            }
        }
        match runtimes.len() {
            0 => {}
            1 => result.push(VmCandidate {
                name: name.to_owned(),
                runtime: runtimes.pop(),
                reason: None,
            }),
            count => result.push(VmCandidate {
                name: name.to_owned(),
                runtime: None,
                reason: Some(format!(
                    "{count} live QEMU processes detected; possible migration"
                )),
            }),
        }
    }
    result.sort_by(|left, right| left.name.cmp(&right.name));
    Ok(result)
}

fn is_socket(path: &Path) -> bool {
    fs::metadata(path).is_ok_and(|metadata| metadata.file_type().is_socket())
}

fn process_details(pid: u32, pid_path: &Path) -> Option<bool> {
    let Ok(command) = fs::read(format!("/proc/{pid}/cmdline")) else {
        return None;
    };
    let arguments: Vec<&[u8]> = command
        .split(|byte| *byte == 0)
        .filter(|argument| !argument.is_empty())
        .collect();
    let executable = arguments.first()?;
    let executable = Path::new(OsStr::from_bytes(executable));
    if !executable
        .file_name()
        .and_then(OsStr::to_str)
        .is_some_and(|name| name.starts_with("qemu-system-"))
    {
        return None;
    }
    let pid_file_matches = arguments
        .windows(2)
        .any(|pair| pair[0] == b"-pidfile" && Path::new(OsStr::from_bytes(pair[1])) == pid_path);
    pid_file_matches.then(|| has_vfio_passthrough(&arguments))
}

fn has_vfio_passthrough(arguments: &[&[u8]]) -> bool {
    arguments
        .iter()
        .any(|argument| *argument == b"vfio-pci" || argument.starts_with(b"vfio-pci,"))
}

use std::os::unix::ffi::OsStrExt;

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Write;

    #[test]
    fn parses_collei_vm_root() {
        let directory = tempfile::tempdir().unwrap();
        let path = directory.path().join("config.ini");
        let mut file = fs::File::create(&path).unwrap();
        writeln!(file, "[other]\nvm = /wrong\n[collei]\nvm = /right").unwrap();
        assert_eq!(collei_vm_root(&path).unwrap(), PathBuf::from("/right"));
    }

    #[test]
    fn detects_vfio_pci_device_arguments() {
        assert!(has_vfio_passthrough(&[
            b"qemu-system-x86_64",
            b"-device",
            b"vfio-pci,host=0000:01:00.0,iommufd=iommufd0",
        ]));
        assert!(!has_vfio_passthrough(&[
            b"qemu-system-x86_64",
            b"-device",
            b"virtio-net-pci",
        ]));
    }
}
