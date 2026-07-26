from __future__ import annotations

import os
import platform
import signal
import subprocess
import sys
from collections.abc import Callable
from pathlib import Path

from commands import CommandRunner
from errors import ColleiError
from runtime import ColleiContext
from tasks import clean_task_backend
from ui import confirm, print_table

GlobalAction = Callable[[ColleiContext, CommandRunner], None]


def qemu_top(context: ColleiContext, runner: CommandRunner) -> None:
    del context
    completed = runner.run(["pgrep", "qemu"], check=False, capture=True)
    pids = completed.stdout.split()
    if not pids:
        raise ColleiError("no qemu process found")
    argv = ["top"]
    for pid in pids:
        argv.extend(["-p", pid])
    runner.exec(argv)


def config_edit(context: ColleiContext, runner: CommandRunner) -> None:
    runner.exec(["nvim"], cwd=context.global_config.directory.path)


def config_show(context: ColleiContext, runner: CommandRunner) -> None:
    del runner
    rows: list[tuple[str, str]] = []
    for path in sorted(context.global_config.directory.path.iterdir()):
        if path.is_file():
            content = " ".join(path.read_text().splitlines())[:50]
            rows.append((path.name, content))
    print_table(("Option", "Content"), rows)


def dashboard(context: ColleiContext, runner: CommandRunner) -> None:
    runner.exec([sys.executable, context.repo / "dashboard.py"])


def kill_all_qemu(context: ColleiContext, runner: CommandRunner) -> None:
    live = context.list_vms(active=True)
    if not live:
        print("no qemu process found")
        return
    for vm in live:
        print(vm.directory)
    if not confirm("Kill these machine?"):
        return
    for vm in live:
        os.kill(vm.pid, signal.SIGTERM)


def trace_qemu(context: ColleiContext, runner: CommandRunner) -> None:
    del context
    qemu = Path.home() / "data" / "qemu" / "build" / f"qemu-system-{platform.machine()}"
    symbols = runner.run(["objdump", "-t", qemu], capture=True).stdout
    selected = subprocess.run(
        ["fzf"], input=symbols, text=True, capture_output=True, check=False
    )
    if selected.returncode or not selected.stdout.strip():
        raise ColleiError("no QEMU symbol selected")
    symbol = selected.stdout.split()[-1]
    runner.exec(
        ["sudo", "bpftrace", "-e", f"uprobe:{qemu}:{symbol} {{ @[ustack] = count(); }}"]
    )


def _clear_ovs_config(runner: CommandRunner) -> None:
    ports = runner.run(
        ["sudo", "ovs-vsctl", "list-ports", "br-in"], capture=True
    ).stdout.splitlines()
    for port in ports:
        print(port)
        show = runner.run(["sudo", "ovs-vsctl", "show"], capture=True).stdout
        if port in show and "No such device" in show:
            runner.run(["sudo", "ovs-vsctl", "del-port", port])


def clean(context: ColleiContext, runner: CommandRunner) -> None:
    _clear_ovs_config(runner)
    clean_task_backend(context, runner)
    print("clean")


def setup_nat(context: ColleiContext, runner: CommandRunner) -> None:
    del context
    wifi = "wlan0"
    bridge = "br-in"
    commands = (
        [
            "sudo",
            "iptables",
            "-t",
            "nat",
            "-A",
            "POSTROUTING",
            "-s",
            "10.0.0.0/16",
            "-o",
            wifi,
            "-j",
            "MASQUERADE",
        ],
        ["sudo", "iptables", "-A", "FORWARD", "-i", bridge, "-o", wifi, "-j", "ACCEPT"],
        [
            "sudo",
            "iptables",
            "-A",
            "FORWARD",
            "-i",
            wifi,
            "-o",
            bridge,
            "-m",
            "state",
            "--state",
            "RELATED,ESTABLISHED",
            "-j",
            "ACCEPT",
        ],
    )
    for command in commands:
        runner.run(command)


def clear_hugetlb(context: ColleiContext, runner: CommandRunner) -> None:
    del context
    root = Path("/sys/kernel/mm/hugepages")
    for target in sorted(root.glob("hugepages-*/nr_hugepages")):
        runner.run(["sudo", "tee", target], input_text="0\n")


def _read(path: Path, default: str = "") -> str:
    try:
        return path.read_text().strip()
    except OSError:
        return default


def _driver(device: Path) -> str:
    try:
        return (device / "driver" / "module").resolve(strict=True).name
    except OSError:
        return "unknown"


def _vf_in_use(group: str, runner: CommandRunner) -> bool:
    if not group:
        return False
    return (
        runner.run(["lsof", f"/dev/vfio/{group}"], check=False, capture=True).returncode
        == 0
    )


def _print_sriov_metrics(device: Path) -> None:
    for metric in sorted(device.glob("sriov_*")):
        if metric.is_file():
            print(f"\t{metric.name}:{_read(metric)}")


def sriov(context: ColleiContext, runner: CommandRunner) -> None:
    del context
    for device in sorted(Path("/sys/class/net").glob("*/device")):
        nic = device.parent.name
        if not (device / "sriov_totalvfs").is_file():
            continue
        speed = runner.run(["ethtool", nic], check=False, capture=True).stdout
        speed_line = next(
            (line.strip() for line in speed.splitlines() if "Speed" in line), ""
        )
        if "Unknown" in speed_line:
            continue
        print(nic)
        print(_driver(device))
        print(speed_line)
        _print_sriov_metrics(device)
        print(" ")
        have_vf = False
        for vf in sorted(device.glob("virtfn*")):
            target = vf.resolve()
            group_path = target / "iommu_group"
            group = group_path.resolve().name if group_path.exists() else ""
            suffix = " **" if _vf_in_use(group, runner) else ""
            print(f"\t{vf.name} {target.name}{suffix}")
            have_vf = True
        if have_vf:
            runner.run(["ip", "link", "show", nic])
        print()


def intel_gpu(context: ColleiContext, runner: CommandRunner) -> None:
    del context
    for device in sorted(Path("/sys/class/drm").glob("card[0-9]/device")):
        if (device / "physfn").is_symlink() or _read(device / "vendor") != "0x8086":
            continue
        print(device.parent.name)
        print(_driver(device))
        print(
            f"vendor={_read(device / 'vendor')} device={_read(device / 'device')} "
            f"revision={_read(device / 'revision')}"
        )
        if not (device / "sriov_totalvfs").is_file():
            print()
            continue
        print(device / "sriov_numvfs")
        _print_sriov_metrics(device)
        print(" ")
        vfs = sorted(device.glob("virtfn*"))
        for vf in vfs:
            target = vf.resolve()
            group_path = target / "iommu_group"
            group = group_path.resolve().name if group_path.exists() else ""
            suffix = " **" if _vf_in_use(group, runner) else ""
            print(f"\t{vf.name} {target.name}{suffix}")
        admin = device / "sriov_admin"
        if vfs and admin.is_dir():
            print("\tsriov_admin:")
            for item in sorted(admin.iterdir()):
                print(f"\t\t{item.name}")
        print()


def check_env(context: ColleiContext, runner: CommandRunner) -> None:
    del runner
    print("当前一共存在如下虚拟机:")
    for vm in context.list_vms():
        if (vm.directory / "cmd.sh").is_file():
            print(vm.config.name)


ACTIONS: dict[str, GlobalAction] = {
    "top": qemu_top,
    "dashboard": dashboard,
    "check_env": check_env,
    "config_show": config_show,
    "config_edit": config_edit,
    "kill_all_qemu": kill_all_qemu,
    "clean": clean,
    "nat": setup_nat,
    "trace": trace_qemu,
    "sriov": sriov,
    "intel_gpu": intel_gpu,
    "clear_hugetlb": clear_hugetlb,
}
