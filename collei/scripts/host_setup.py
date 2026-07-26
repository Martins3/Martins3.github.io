from __future__ import annotations

import os
import shutil
import socket
import time
from pathlib import Path

from commands import CommandRunner
from errors import ColleiError, UnsupportedNativeConfiguration
from runtime import ColleiContext, VmRuntime
from tasks import add_background_task
from vfio import pci_bind_to_vfio


# 2026-07-08 发现了一个问题，如果直接 kill 掉 qemu ，那么
# 会留下一个 active 的 tap 设备，这其实相当烦人，之前是没有这个问题的
# 这导致很多时候，我们都需要使用 sudo
def remove_vm_taps(
    vm: VmRuntime,
    runner: CommandRunner,
    network_root: Path = Path("/sys/class/net"),
) -> None:
    if not vm.active:
        return
    prefix = f"vif_{vm.which_qemu}_{vm.config.guest_id}_"
    for interface in sorted(network_root.glob(f"{prefix}*")):
        runner.run(["sudo", "ip", "link", "delete", "dev", interface.name])


def _tcp_port_is_listening(port: int) -> bool:
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as connection:
        connection.settimeout(0.1)
        return connection.connect_ex(("127.0.0.1", port)) == 0


def prepare_novnc(context: ColleiContext, vm: VmRuntime, runner: CommandRunner) -> None:
    qemu_port = vm.tcp_port("vnc")
    novnc_port = qemu_port + 1
    url = f"http://{context.master_ip()}:{novnc_port}/vnc.html"
    if _tcp_port_is_listening(novnc_port):
        print(url)
        return

    executable = shutil.which("novnc") or shutil.which("novnc_server")
    if executable is None:
        raise ColleiError("novnc is not installed")
    add_background_task(
        context,
        runner,
        [
            executable,
            "--vnc",
            f"localhost:{qemu_port}",
            "--listen",
            str(novnc_port),
        ],
        vm=vm,
        label="novnc",
    )
    for _ in range(20):
        if _tcp_port_is_listening(novnc_port):
            print(url)
            return
        time.sleep(0.05)
    raise ColleiError(f"novnc did not listen on port {novnc_port}")


def prepare_native_host(
    context: ColleiContext, vm: VmRuntime, runner: CommandRunner
) -> None:
    if vm.config.options.get("bios") == "ovmf_binary":
        ovmf = (
            context.repo.parent.parent
            / "bios"
            / "ovmf_binary"
            / "usr"
            / "share"
            / "edk2"
            / "ovmf"
        )
        code = ovmf / "OVMF_CODE.fd"
        variables = ovmf / "OVMF_VARS.fd"
        if not code.is_file() or not variables.is_file():
            raise UnsupportedNativeConfiguration(f"OVMF firmware is incomplete: {ovmf}")
        local_variables = vm.directory / "OVMF_VARS.fd"
        if not local_variables.exists():
            shutil.copy2(variables, local_variables)

    for device in (vm.config.options.get("vfio") or "").splitlines():
        pci_bind_to_vfio(device, runner)

    monitor = vm.directory / vm.which_qemu
    monitor.mkdir(parents=True, exist_ok=True)
    for counter in ("hp_mm_counter", "hp_disk_counter", "vif_counter"):
        (monitor / counter).write_text("0\n")
    if context.global_config.directory.get("bridge") != "no":
        prepare_ovs_tap(context, vm, runner)


def prepare_ovs_tap(
    context: ColleiContext, vm: VmRuntime, runner: CommandRunner
) -> tuple[str, str]:
    counter_file = vm.directory / vm.which_qemu / "vif_counter"
    counter = int(counter_file.read_text())
    if counter >= 10:
        raise ColleiError("too many nic")
    counter_file.write_text(f"{counter + 1}\n")
    tap = f"vif_{vm.which_qemu}_{vm.config.guest_id}_{counter}"
    level = context.global_config.directory.integer("level", 0)
    mac = f"52:54:00:{level:02x}:{vm.config.guest_id:02x}:{counter:02x}"

    if runner.run(
        ["ip", "link", "show", "dev", "br-in"], check=False, capture=True
    ).returncode:
        raise UnsupportedNativeConfiguration("OVS bridge br-in is not prepared")
    addresses = runner.run(["ip", "-4", "addr", "show", "br-in"], capture=True).stdout
    if "10.0." not in addresses:
        runner.run(
            [
                "sudo",
                "ip",
                "address",
                "add",
                f"{context.global_config.master_ip}/16",
                "dev",
                "br-in",
            ]
        )
        runner.run(["sudo", "ip", "link", "set", "br-in", "up"])

    exists = (
        runner.run(
            ["ip", "link", "show", "dev", tap], check=False, capture=True
        ).returncode
        == 0
    )
    if not exists:
        runner.run(
            [
                "sudo",
                "ip",
                "tuntap",
                "add",
                "mode",
                "tap",
                "user",
                os.environ["USER"],
                "dev",
                tap,
            ]
        )
    link = runner.run(["ip", "link", "show", tap], capture=True).stdout
    if ",UP" not in link:
        runner.run(["sudo", "ip", "link", "set", tap, "up"])
    ports = runner.run(
        ["sudo", "ovs-vsctl", "list-ports", "br-in"], capture=True
    ).stdout.splitlines()
    if tap not in ports:
        runner.run(["sudo", "ovs-vsctl", "add-port", "br-in", tap])
    return tap, mac
