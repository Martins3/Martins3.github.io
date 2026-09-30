"""Declarative serial layout for Collei.

The VM config contains one ``tty`` option whose continuation lines have the
form ``<frontend> <backend>``. This module turns those lines into QEMU
chardev backends and guest frontends. Backend selection is independent from
the QEMU process lifecycle: ``bg`` controls where QEMU runs, while this file
controls how each channel is connected.
"""

from __future__ import annotations

import platform
import shlex
from dataclasses import dataclass
from pathlib import Path
from typing import Protocol

from errors import ColleiError

FRONTENDS = frozenset({"serial", "hvc", "virtserialport"})
# TODO 到时候添加上 pipe 的后端吧
BACKENDS = frozenset({"socket", "stdio", "pty"})
MAX_MUX_FRONTENDS = 4


class SerialOptions(Protocol):
    def get(self, name: str) -> str | None: ...


@dataclass(frozen=True, slots=True)
class SerialChannel:
    name: str
    frontend: str
    chardev_id: str
    backend: str
    endpoint: Path | None
    logfile: Path
    guest_name: str | None = None
    index: int = 0

    @property
    def console_name(self) -> str:
        if self.frontend == "serial":
            prefix = "ttyAMA" if platform.machine() == "aarch64" else "ttyS"
            return f"{prefix}{self.index}"
        if self.frontend == "hvc":
            return f"hvc{self.index}"
        return self.frontend

    @property
    def interactive(self) -> bool:
        return self.frontend in {"serial", "hvc"}


@dataclass(frozen=True, slots=True)
class SerialLayout:
    monitor_dir: Path
    channels: tuple[SerialChannel, ...]
    stdio_chardev_id: str | None = None

    def channel(self, name: str) -> SerialChannel:
        for channel in self.channels:
            if channel.name == name:
                return channel
        raise ColleiError(f"serial channel does not exist: {name}")

    def first(self, frontend: str) -> SerialChannel | None:
        return next(
            (channel for channel in self.channels if channel.frontend == frontend),
            None,
        )

    def backend_id(self, channel: SerialChannel) -> str:
        if channel.backend == "stdio" and self.stdio_chardev_id is not None:
            return self.stdio_chardev_id
        return channel.chardev_id

    def logfile(self, channel: SerialChannel) -> Path:
        if channel.backend == "stdio":
            return self.monitor_dir / "stdio.log"
        return channel.logfile

    @property
    def primary_console(self) -> str:
        for frontend in ("hvc", "serial"):
            channel = self.first(frontend)
            if channel is not None:
                return channel.console_name
        raise ColleiError("tty has no kernel console frontend")

    def kernel_console_args(self) -> tuple[str, ...]:
        """Infer standard kernel consoles from configured frontend order."""
        return tuple(
            f"console={channel.console_name},115200n8"
            for channel in self.channels
            if channel.frontend in {"serial", "hvc"}
        )

    def argv(self) -> tuple[str, ...]:
        return self.chardev_argv() + self.frontend_argv()

    def chardev_argv(self) -> tuple[str, ...]:
        argv: list[str] = []
        stdio = [channel for channel in self.channels if channel.backend == "stdio"]
        if stdio:
            if len(stdio) > MAX_MUX_FRONTENDS:
                raise ColleiError(
                    f"stdio backend has {len(stdio)} frontends; "
                    f"QEMU supports at most {MAX_MUX_FRONTENDS} mux frontends"
                )
            backend_id = self.stdio_chardev_id
            if backend_id is None:
                raise ColleiError("stdio backend ID is missing")
            mux = ",mux=on" if len(stdio) > 1 else ""
            logfile = self.monitor_dir / "stdio.log"
            argv.extend(
                (
                    "-chardev",
                    f"stdio,id={backend_id}{mux},signal=off,"
                    f"logfile={logfile},logappend=on,logtimestamp=on",
                )
            )
        for channel in self.channels:
            if channel.backend == "stdio":
                continue
            options = [channel.backend, f"id={channel.chardev_id}"]
            if channel.backend == "socket":
                if channel.endpoint is None:
                    raise ColleiError(f"socket path is missing for {channel.name}")
                options[0] = f"socket,path={channel.endpoint},server=on,wait=off"
            elif channel.backend == "pty":
                options[0] = "pty"
            if channel.interactive:
                options.extend(
                    (
                        f"logfile={channel.logfile}",
                        "logappend=on",
                        "logtimestamp=on",
                    )
                )
            argv.extend(("-chardev", ",".join(options)))
        return tuple(argv)

    def frontend_argv(self) -> tuple[str, ...]:
        argv: list[str] = []
        if any(
            channel.frontend in {"hvc", "virtserialport"} for channel in self.channels
        ):
            argv.extend(("-device", "virtio-serial"))
        for channel in self.channels:
            backend = self.backend_id(channel)
            if channel.frontend == "serial":
                argv.extend(("-serial", f"chardev:{backend}"))
            elif channel.frontend == "hvc":
                argv.extend(("-device", f"virtconsole,chardev={backend}"))
            elif channel.frontend == "virtserialport":
                if channel.guest_name is None:
                    raise ColleiError(f"guest port name is missing for {channel.name}")
                argv.extend(
                    (
                        "-device",
                        f"virtserialport,chardev={backend},name={channel.guest_name}",
                    )
                )
        return tuple(argv)


def _auto_channel(
    monitor_dir: Path,
    index: int,
    frontend: str,
    backend: str,
    *,
    force_socket: bool = False,
) -> SerialChannel:
    suffix = f"_{index}" if index else ""
    if frontend == "serial":
        name = f"serial_console{suffix}"
        guest_name = None
    elif frontend == "hvc":
        name = f"virtio_console{suffix}"
        guest_name = None
    else:
        name = f"test_port_{index}"
        guest_name = f"org.qemu.vport.{index}"
    effective_backend = "socket" if backend == "stdio" and force_socket else backend
    endpoint_name = f"{frontend}{index}.socket"
    endpoint = monitor_dir / endpoint_name if effective_backend == "socket" else None
    return SerialChannel(
        name=name,
        frontend=frontend,
        chardev_id=name,
        backend=effective_backend,
        endpoint=endpoint,
        logfile=monitor_dir / f"{name}.log",
        guest_name=guest_name,
        index=index,
    )


def _parse_tty(value: str | None) -> tuple[tuple[str, str], ...]:
    if value is None or not value.strip():
        raise ColleiError("config.ini tty is required")
    records: list[tuple[str, str]] = []
    for number, line in enumerate(value.splitlines(), 1):
        if not line.strip():
            continue
        fields = shlex.split(line)
        if len(fields) != 2:
            raise ColleiError(
                f"config.ini tty line {number} must be '<frontend> <backend>'"
            )
        frontend, backend = fields
        if frontend not in FRONTENDS:
            values = ", ".join(sorted(FRONTENDS))
            raise ColleiError(f"unknown tty frontend {frontend}; expected {values}")
        if backend not in BACKENDS:
            values = ", ".join(sorted(BACKENDS))
            raise ColleiError(f"unknown tty backend {backend}; expected {values}")
        records.append((frontend, backend))
    if not records:
        raise ColleiError("config.ini tty must contain at least one channel")
    return tuple(records)


def build_serial_layout_for_options(
    monitor_dir: Path,
    options: SerialOptions,
    *,
    force_socket_stdio: bool = False,
) -> SerialLayout:
    records = _parse_tty(options.get("tty"))
    indices = dict.fromkeys(FRONTENDS, 0)
    channels: list[SerialChannel] = []
    for frontend, backend in records:
        index = indices[frontend]
        channels.append(
            _auto_channel(
                monitor_dir,
                index,
                frontend,
                backend,
                force_socket=force_socket_stdio,
            )
        )
        indices[frontend] += 1
    stdio = [channel for channel in channels if channel.backend == "stdio"]
    stdio_id = "stdio" if stdio else None
    return SerialLayout(monitor_dir, tuple(channels), stdio_id)
