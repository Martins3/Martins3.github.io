from __future__ import annotations

import shutil
from dataclasses import dataclass
from pathlib import Path
from typing import Literal, Sequence

from errors import ColleiError, UnsupportedNativeConfiguration
from runtime import VmRuntime

NetworkBackend = Literal["user", "passt"]
Protocol = Literal["tcp", "udp"]


@dataclass(frozen=True)
class PortForward:
    protocol: Protocol
    host_port: int
    guest_port: int


def network_backend(vm: VmRuntime) -> NetworkBackend:
    configured = vm.config.options.get("net_backend") or "user"
    if configured == "user":
        return "user"
    if configured == "passt":
        return "passt"
    raise UnsupportedNativeConfiguration(
        f"unsupported net_backend={configured}; expected user or passt"
    )


def user_mode_netdev(
    vm: VmRuntime,
    forwards: Sequence[PortForward],
    *,
    smb: str | None = None,
) -> str:
    backend = network_backend(vm)
    if backend == "user":
        options = ["user", "id=net1"]
        options.extend(
            f"hostfwd={forward.protocol}:127.0.0.1:{forward.host_port}-:{forward.guest_port}"
            for forward in forwards
        )
        options.append(f"hostname={vm.config.name}")
        if smb is not None:
            options.append(f"smb={smb}")
        return ",".join(options)

    raise AssertionError("passt uses the external vhost-user backend")


def passt_socket(vm: VmRuntime) -> Path:
    return vm.qemu_directory / "passt.sock"


def user_mode_network_arguments(
    vm: VmRuntime,
    forwards: Sequence[PortForward],
    *,
    smb: str | None = None,
) -> tuple[str, ...]:
    if network_backend(vm) == "user":
        return ("-netdev", user_mode_netdev(vm, forwards, smb=smb))

    socket = passt_socket(vm)
    return (
        "-chardev",
        f"socket,id=passt_net_chr,path={socket}",
        "-netdev",
        "vhost-user,id=net1,chardev=passt_net_chr,vhostforce=on",
    )


def passt_daemon_arguments(
    vm: VmRuntime, forwards: Sequence[PortForward]
) -> tuple[str, ...]:
    executable = shutil.which("passt")
    if executable is None:
        raise ColleiError("net_backend=passt requires passt in PATH")

    options = [
        executable,
        "--foreground",
        "--one-off",
        "--vhost-user",
        "--socket",
        str(passt_socket(vm)),
    ]
    for protocol in ("tcp", "udp"):
        protocol_forwards = [
            forward for forward in forwards if forward.protocol == protocol
        ]
        for index, forward in enumerate(protocol_forwards):
            address = "127.0.0.1/" if index == 0 else ""
            options.extend(
                (
                    f"--{protocol}-ports",
                    f"{address}{forward.host_port}:{forward.guest_port}",
                )
            )
    options.extend(("--fqdn", vm.config.name))
    return tuple(options)
