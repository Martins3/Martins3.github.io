#!/usr/bin/env python3
import os
import subprocess
import sys
import time
from pathlib import Path


def get_qemu_pid(vm_dir: Path) -> str:
    """Check s/pid, t/pid, or pid file and verify process exists."""
    pid_files = [vm_dir / "s" / "pid", vm_dir / "t" / "pid", vm_dir / "pid"]
    for pf in pid_files:
        if pf.exists():
            try:
                pid = pf.read_text().strip()
                if (Path("/proc") / pid / "status").exists():
                    return pid
            except (OSError, ValueError):
                pass
    return ""


def read_cpu_ticks(pid: str) -> tuple[int, float] | None:
    """Return (utime+stime, monotonic_time) for a process.

    /proc/<pid>/stat format: pid (comm) state ...  The comm field may contain
    spaces and parentheses, so we split after the final ')'.
    """
    try:
        text = (Path("/proc") / pid / "stat").read_text()
        idx = text.rfind(")")
        if idx == -1:
            return None
        fields = text[idx + 1 :].split()
        # Field indices are 0-based after the closing ')':
        # state=0, ppid=1, ..., utime=11, stime=12
        utime = int(fields[11])
        stime = int(fields[12])
        return (utime + stime, time.monotonic())
    except (OSError, ValueError, IndexError):
        return None


def format_cpu_percent(
    first: tuple[int, float], second: tuple[int, float]
) -> str:
    """Compute top-style %CPU from two /proc/<pid>/stat samples.

    Like top's default Irix mode, the result can exceed 100% for multi-threaded
    processes such as QEMU.
    """
    delta_ticks = second[0] - first[0]
    delta_wall = second[1] - first[1]
    if delta_wall <= 0:
        return ""
    clk_tck = os.sysconf("SC_CLK_TCK")
    if clk_tck <= 0:
        return ""
    percent = (delta_ticks / clk_tck) / delta_wall * 100
    return f"{percent:.1f}"


def get_mem(pid: str) -> str:
    """Return human-readable RSS for a pid, or empty string if unavailable."""
    if not pid:
        return ""
    try:
        status_path = Path("/proc") / pid / "status"
        for line in status_path.read_text().splitlines():
            if line.startswith("VmRSS:"):
                parts = line.split()
                kb = int(parts[1])
                return humanize_kb(kb)
    except (OSError, ValueError, IndexError):
        pass
    return ""


def humanize_kb(kb: int) -> str:
    """Convert kB to human-readable string."""
    if kb >= 1024 * 1024 * 1024:
        return f"{kb / (1024 * 1024 * 1024):.1f}T"
    elif kb >= 1024 * 1024:
        return f"{kb / (1024 * 1024):.1f}G"
    elif kb >= 1024:
        return f"{kb / 1024:.1f}M"
    else:
        return f"{kb}K"


def read_option(path: Path) -> str:
    """Read config file, skip comments and empty lines."""
    if not path.exists():
        return ""
    try:
        content = path.read_text()
        lines = []
        for line in content.splitlines():
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            lines.append(line)
        result = "\n".join(lines).strip()
        return result
    except OSError:
        return ""


def get_dir_size(vm_dir: Path) -> str:
    """Get human-readable directory size using du -sh."""
    try:
        result = subprocess.run(
            ["du", "-sh", str(vm_dir)],
            capture_output=True,
            text=True,
            check=True,
        )
        return result.stdout.split()[0]
    except (subprocess.CalledProcessError, FileNotFoundError):
        return ""


def format_snapshot_date(vm_dir: Path) -> str:
    """Return the date of the newest disk snapshot created by the backup action."""
    bak_files = list((vm_dir / "img").glob("boot*.bak"))
    if not bak_files:
        return ""
    newest = max(bak_files, key=lambda p: p.stat().st_mtime)
    ts = newest.stat().st_mtime
    return subprocess.run(
        ["date", "+%Y %m %d", "--date", f"@{int(ts)}"],
        capture_output=True,
        text=True,
        check=True,
    ).stdout.strip()


def main() -> int:
    global_config_dir = Path.home() / ".config" / "collei"
    vm_config = global_config_dir / "vm"
    all_vm_dir_str = read_option(vm_config)
    if not all_vm_dir_str:
        print("global config not setup or vm dir missing", file=sys.stderr)
        return 1

    all_vm_dir = Path(all_vm_dir_str)
    if not all_vm_dir.is_dir():
        print(f"invalid vm dir: {all_vm_dir}", file=sys.stderr)
        return 1

    cpu_interval = float(os.environ.get("COLLEI_DASHBOARD_CPU_INTERVAL", "1.0"))

    entries = []
    for d in sorted(all_vm_dir.iterdir()):
        if not d.is_dir():
            continue
        cmd_sh = d / "cmd.sh"
        if not cmd_sh.exists():
            continue

        try:
            mtime = cmd_sh.stat().st_mtime
        except OSError:
            continue

        name = d.name
        pid = get_qemu_pid(d)
        mem = get_mem(pid)

        id_path = d / "opt" / "id"
        vm_id = ""
        if id_path.exists():
            try:
                vm_id = id_path.read_text().strip()
            except OSError:
                pass

        snapshot = format_snapshot_date(d)
        size = get_dir_size(d)

        entries.append((mtime, name, pid, vm_id, snapshot, size, mem))

    # Sample CPU once for all running VMs so the total delay is ~1 second
    # instead of one second per VM.
    first_samples: dict[str, tuple[int, float]] = {}
    for *_, pid, _, _, _, _ in entries:
        if pid:
            sample = read_cpu_ticks(pid)
            if sample is not None:
                first_samples[pid] = sample
    if first_samples and cpu_interval > 0:
        time.sleep(cpu_interval)
    cpu_map: dict[str, str] = {}
    for pid, first in first_samples.items():
        second = read_cpu_ticks(pid)
        if second is not None:
            cpu_map[pid] = format_cpu_percent(first, second)

    rows = [
        (mtime, name, pid, vm_id, snapshot, size, cpu_map.get(pid, ""), mem)
        for (mtime, name, pid, vm_id, snapshot, size, mem) in entries
    ]

    if not rows:
        print("no vm found")
        return 0

    # Sort by mtime descending
    rows.sort(key=lambda x: x[0], reverse=True)

    dash = Path("/tmp/martins3/dashboard")
    dash.parent.mkdir(parents=True, exist_ok=True)
    dash_csv = dash.with_suffix(".csv")

    with dash_csv.open("w") as f:
        for _, name, pid, vm_id, snapshot, size, cpu, mem in rows:
            f.write(f"{name},{pid},{vm_id},{snapshot},{size},{cpu},{mem}\n")

    vm_num = len(rows)
    subprocess.run(
        [
            "gum",
            "table",
            "--height",
            str(vm_num),
            "-s",
            ",",
            "-c",
            "name",
            "-c",
            "pid",
            "-c",
            "id",
            "-c",
            "snapshot",
            "-c",
            "size",
            "-c",
            "cpu",
            "-c",
            "mem",
            "-f",
            str(dash_csv),
            "-p",
            "--border=rounded",
        ],
        check=False,
    )
    return 0


if __name__ == "__main__":
    sys.exit(main())
