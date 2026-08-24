from __future__ import annotations

import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any
from urllib.parse import quote

from block_migration import discover_migratable_disks, qmp_execute
from errors import ColleiError
from monitor import QmpClient
from runtime import VmRuntime

EXPORT_ID = "fio-read"
SOCKET_NAME = "fio.nbd"
STOP_TIMEOUT = 30.0


@dataclass(frozen=True)
class NbdBenchmarkExport:
    node_name: str
    export_id: str
    socket: Path

    @property
    def uri(self) -> str:
        export = quote(self.node_name, safe="")
        socket_path = quote(str(self.socket), safe="/")
        return f"nbd+unix:///{export}?socket={socket_path}"


def _monitor_directory(vm: VmRuntime) -> Path:
    return vm.directory / vm.which_qemu


def _socket_path(vm: VmRuntime) -> Path:
    return _monitor_directory(vm) / SOCKET_NAME


def _qmp_path(vm: VmRuntime) -> Path:
    return _monitor_directory(vm) / "qmp-no-pretty"


def _qmp_records(value: object, command: str) -> list[dict[str, Any]]:
    if not isinstance(value, list) or not all(isinstance(item, dict) for item in value):
        raise ColleiError(f"{command} returned invalid data: {value!r}")
    return value


def _query_exports(qmp_path: Path) -> list[dict[str, Any]]:
    with QmpClient(qmp_path) as qmp:
        return _qmp_records(
            qmp_execute(qmp, "query-block-exports"), "query-block-exports"
        )


def _benchmark_export(
    vm: VmRuntime, exports: list[dict[str, Any]]
) -> NbdBenchmarkExport | None:
    for export in exports:
        if export.get("id") != EXPORT_ID:
            continue
        node_name = export.get("node-name")
        if not isinstance(node_name, str) or not node_name:
            raise ColleiError(f"QEMU block export {EXPORT_ID!r} has no node-name")
        return NbdBenchmarkExport(node_name, EXPORT_ID, _socket_path(vm))
    return None


def _socket_is_listening(path: Path) -> bool:
    try:
        lines = Path("/proc/net/unix").read_text().splitlines()[1:]
    except OSError as error:
        raise ColleiError(f"cannot inspect Unix sockets: {error}") from error
    for line in lines:
        fields = line.split(maxsplit=7)
        if len(fields) != 8 or fields[7] != str(path):
            continue
        try:
            flags = int(fields[3], 16)
        except ValueError as error:
            raise ColleiError(f"invalid /proc/net/unix entry: {line!r}") from error
        if flags & 0x10000:
            return True
    return False


def _remove_stale_socket(path: Path) -> None:
    if not path.exists():
        return
    if _socket_is_listening(path):
        raise ColleiError(f"NBD Unix socket is already accepting connections: {path}")
    path.unlink()


def _wait_export_removed(
    qmp_path: Path, export_id: str, timeout: float = STOP_TIMEOUT
) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        exports = _query_exports(qmp_path)
        if not any(export.get("id") == export_id for export in exports):
            return
        time.sleep(0.1)
    raise ColleiError(f"QEMU block export {export_id!r} did not stop")


def _select_node(vm: VmRuntime, requested: str | None) -> str:
    disks = discover_migratable_disks(
        _qmp_path(vm), vm.image_directory, vm.image_directory
    )
    available = {disk.node_name for disk in disks}
    if requested is not None:
        if requested not in available:
            raise ColleiError(
                f"unknown writable image-slot block node {requested!r}; "
                f"available: {', '.join(sorted(available))}"
            )
        return requested
    if "boot1" in available:
        return "boot1"
    return min(available)


def start_nbd_benchmark(vm: VmRuntime, node_name: str | None) -> NbdBenchmarkExport:
    qmp_path = _qmp_path(vm)
    exports = _query_exports(qmp_path)
    existing = _benchmark_export(vm, exports)
    if existing is not None:
        if node_name is not None and node_name != existing.node_name:
            raise ColleiError(
                f"QEMU fio NBD export already uses {existing.node_name!r}; "
                f"stop it before exporting {node_name!r}"
            )
        return existing

    selected = _select_node(vm, node_name)
    socket_path = _socket_path(vm)
    if len(str(socket_path).encode()) >= 108:
        raise ColleiError(f"NBD Unix socket path is too long: {socket_path}")
    _remove_stale_socket(socket_path)
    benchmark = NbdBenchmarkExport(selected, EXPORT_ID, socket_path)

    server_started = False
    export_added = False
    try:
        with QmpClient(qmp_path) as qmp:
            qmp_execute(
                qmp,
                "nbd-server-start",
                {
                    "addr": {
                        "type": "unix",
                        "data": {"path": str(socket_path)},
                    },
                    "max-connections": 1,
                },
            )
            server_started = True
            qmp_execute(
                qmp,
                "block-export-add",
                {
                    "type": "nbd",
                    "id": EXPORT_ID,
                    "node-name": selected,
                    "name": selected,
                    "writable": False,
                },
            )
            export_added = True
    except BaseException:
        if server_started:
            try:
                with QmpClient(qmp_path) as qmp:
                    if export_added:
                        qmp_execute(
                            qmp,
                            "block-export-del",
                            {"id": EXPORT_ID, "mode": "hard"},
                        )
                if export_added:
                    _wait_export_removed(qmp_path, EXPORT_ID)
                with QmpClient(qmp_path) as qmp:
                    qmp_execute(qmp, "nbd-server-stop")
            except (ColleiError, OSError):
                pass
        socket_path.unlink(missing_ok=True)
        raise
    return benchmark


def nbd_benchmark_status(
    vm: VmRuntime,
) -> tuple[NbdBenchmarkExport | None, bool]:
    export = _benchmark_export(vm, _query_exports(_qmp_path(vm)))
    return export, _socket_is_listening(_socket_path(vm))


def stop_nbd_benchmark(vm: VmRuntime) -> NbdBenchmarkExport | None:
    qmp_path = _qmp_path(vm)
    socket_path = _socket_path(vm)
    exports = _query_exports(qmp_path)
    benchmark = _benchmark_export(vm, exports)
    listening = _socket_is_listening(socket_path)

    if benchmark is None:
        if listening:
            with QmpClient(qmp_path) as qmp:
                qmp_execute(qmp, "nbd-server-stop")
        socket_path.unlink(missing_ok=True)
        return None

    with QmpClient(qmp_path) as qmp:
        qmp_execute(
            qmp,
            "block-export-del",
            {"id": benchmark.export_id, "mode": "safe"},
        )
    _wait_export_removed(qmp_path, benchmark.export_id)

    remaining = _query_exports(qmp_path)
    if remaining:
        ids = ", ".join(str(export.get("id")) for export in remaining)
        raise ColleiError(f"cannot stop NBD server with other exports present: {ids}")
    with QmpClient(qmp_path) as qmp:
        qmp_execute(qmp, "nbd-server-stop")
    socket_path.unlink(missing_ok=True)
    return benchmark
