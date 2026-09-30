from __future__ import annotations

import os
import platform
import shutil
import socket
import time
from pathlib import Path

from commands import CommandRunner
from errors import ColleiError, UnsupportedNativeConfiguration
from network import PortForward, network_backend, passt_daemon_arguments, passt_socket
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
        bios_root = (
            context.repo.parent.parent
            / "bios"
            / "ovmf_binary"
            / "usr"
            / "share"
            / "edk2"
        )
        if platform.machine() == "aarch64":
            code = bios_root / "aarch64" / "QEMU_EFI.fd"
            if not code.is_file():
                raise UnsupportedNativeConfiguration(
                    f"AArch64 OVMF firmware is missing: {code}"
                )
        else:
            ovmf = bios_root / "ovmf"
            code = ovmf / "OVMF_CODE.fd"
            variables = ovmf / "OVMF_VARS.fd"
            if not code.is_file() or not variables.is_file():
                raise UnsupportedNativeConfiguration(
                    f"OVMF firmware is incomplete: {ovmf}"
                )
            local_variables = vm.directory / "OVMF_VARS.fd"
            if not local_variables.exists():
                shutil.copy2(variables, local_variables)

    for device in (vm.config.options.get("vfio") or "").splitlines():
        pci_bind_to_vfio(device, runner)

    monitor = vm.qemu_directory
    monitor.mkdir(parents=True, exist_ok=True)
    for counter in ("hp_mm_counter", "hp_disk_counter", "vif_counter"):
        (monitor / counter).write_text("0\n")
    prepare_passt_network(context, vm, runner)
    prepare_vhost_user_net(context, vm, runner)
    if context.global_config.options.get("bridge") != "no":
        prepare_ovs_tap(context, vm, runner)


def prepare_passt_network(
    context: ColleiContext, vm: VmRuntime, runner: CommandRunner
) -> None:
    if network_backend(vm) != "passt":
        return
    socket_path = passt_socket(vm)
    socket_path.unlink(missing_ok=True)
    repair_socket = Path(f"{socket_path}.repair")
    repair_socket.unlink(missing_ok=True)
    add_background_task(
        context,
        runner,
        passt_daemon_arguments(vm, [PortForward("tcp", vm.tcp_port("ssh"), 22)]),
        vm=vm,
        group="qemu",
        label="passt",
    )
    for _ in range(50):
        if socket_path.exists():
            return
        time.sleep(0.1)
    raise ColleiError(f"passt socket was not created: {socket_path}")


# 2026-08-25 总体来说，这个东西没玩太明白，不过此时此刻，我在调查 vhost 重连的问题
def prepare_vhost_user_net(
    context: ColleiContext, vm: VmRuntime, runner: CommandRunner
) -> None:
    if not vm.config.options.enabled("vhost_user_net"):
        return
    binary = (
        context.repo.parent.parent
        / "qemu/build/contrib/vhost-user-bridge/vhost-user-bridge"
    )
    if not binary.is_file() or not os.access(binary, os.X_OK):
        raise ColleiError(f"vhost-user-bridge is not built: {binary}")
    socket_path = vm.qemu_directory / "vhost-user-net.sock"
    socket_path.unlink(missing_ok=True)
    local_port = vm.tcp_port("vhost_user")
    add_background_task(
        context,
        runner,
        [
            binary,
            "-u",
            socket_path,
            "-l",
            f"127.0.0.1:{local_port}",
            "-r",
            f"127.0.0.1:{local_port + 1}",
        ],
        vm=vm,
        group="qemu",
        label="vhost-user-net",
    )
    for _ in range(50):
        if socket_path.exists():
            return
        time.sleep(0.1)
    raise ColleiError(f"vhost-user-bridge socket was not created: {socket_path}")


def prepare_ovs_tap(
    context: ColleiContext, vm: VmRuntime, runner: CommandRunner
) -> tuple[str, str]:
    counter_file = vm.qemu_directory / "vif_counter"
    counter = int(counter_file.read_text())
    if counter >= 10:
        raise ColleiError("too many nic")
    counter_file.write_text(f"{counter + 1}\n")
    tap = f"vif_{vm.which_qemu}_{vm.config.guest_id}_{counter}"
    level = context.global_config.options.integer("level", 0)
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
