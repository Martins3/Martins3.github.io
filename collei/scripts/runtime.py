from __future__ import annotations

import os
import re
import subprocess
from dataclasses import dataclass
from pathlib import Path

from config import GlobalConfig, VmConfig
from errors import ColleiError

VM_PORT_RANGE = 20
PORT_ALLOCATE_START = 50000
PORT_ALLOCATE_END = 60000
VSOCK_CID_START = 1000
STORAGE_SLOT_FILE = "storage_slot"
STORAGE_SLOTS = frozenset({"s", "t"})


def _storage_slot(config: VmConfig) -> str:
    path = config.directory / STORAGE_SLOT_FILE
    try:
        slot = path.read_text().strip()
    except FileNotFoundError:
        return "s"
    if slot not in STORAGE_SLOTS:
        raise ColleiError(f"invalid storage slot in {path}: {slot!r}")
    return slot


def _is_firecracker_command(argv: tuple[str, ...], config_file: Path) -> bool:
    if not argv or Path(argv[0]).name != "firecracker":
        return False
    try:
        index = argv.index("--config-file")
        configured = Path(argv[index + 1]).resolve()
    except (ValueError, IndexError, OSError):
        return False
    return configured == config_file.resolve()


def _firecracker_pid(config: VmConfig) -> int | None:
    """Firecracker 没有 QEMU 的 -pidfile，按独立 config path 识别实例。"""
    config_file = config.directory / "fire.json"
    for process in Path("/proc").glob("[0-9]*"):
        try:
            fields = (process / "cmdline").read_bytes().split(b"\0")
            argv = tuple(
                field.decode(errors="surrogateescape") for field in fields if field
            )
        except (FileNotFoundError, PermissionError, ProcessLookupError, OSError):
            continue
        if _is_firecracker_command(argv, config_file):
            return int(process.name)
    return None


@dataclass(frozen=True)
class VmRuntime:
    """Immutable snapshot; reload it after moving a VM or changing runtime state."""

    config: VmConfig
    which_qemu: str
    live_pids: tuple[int, ...]
    storage_slot: str = "s"

    @classmethod
    def inspect(cls, config: VmConfig) -> VmRuntime:
        if config.options.enabled("fire"):
            pid = _firecracker_pid(config)
            return cls(
                config=config,
                which_qemu="s",
                live_pids=(pid,) if pid is not None else (),
                storage_slot="s",
            )
        live: list[tuple[str, int]] = []
        for name, pid_file in (
            ("s", config.directory / "s" / "pid"),
            ("t", config.directory / "t" / "pid"),
            ("s", config.directory / "pid"),
        ):
            try:
                pid = int(pid_file.read_text().strip())
            except (FileNotFoundError, ValueError):
                continue
            if Path(f"/proc/{pid}/status").is_file():
                live.append((name, pid))
        storage_slot = _storage_slot(config)
        which = live[-1][0] if live else storage_slot
        return cls(
            config=config,
            which_qemu=which,
            live_pids=tuple(pid for _, pid in live),
            storage_slot=storage_slot,
        )

    @property
    def directory(self) -> Path:
        """Canonical VM directory owned by the immutable config snapshot."""
        return self.config.directory

    @property
    def active(self) -> bool:
        return bool(self.live_pids)

    @property
    def image_directory(self) -> Path:
        name = "img" if self.storage_slot == "s" else "img-t"
        return self.directory / name

    def persist_storage_slot(self, slot: str) -> None:
        if slot not in STORAGE_SLOTS:
            raise ColleiError(f"invalid storage slot: {slot!r}")
        (self.directory / STORAGE_SLOT_FILE).write_text(f"{slot}\n")

    @property
    def qemu_index(self) -> int:
        return 1 if self.which_qemu == "t" else 0

    @property
    def vsock_cid(self) -> int:
        return self.config.guest_id + VSOCK_CID_START + self.qemu_index

    @property
    def pid(self) -> int:
        if not self.live_pids:
            raise ColleiError(f"{self.config.name} is not running")
        return self.live_pids[0]

    def tcp_port(self, service: str, disk: int = 0) -> int:
        base = PORT_ALLOCATE_START + self.config.guest_id * VM_PORT_RANGE
        if service == "vnc":
            return base + self.qemu_index * 2
        if service == "ssh":
            return base + 4 + self.qemu_index
        if service == "nbd":
            port = base + 6 + disk
            if port > PORT_ALLOCATE_END:
                raise ColleiError("too many vm")
            return port
        raise ColleiError(f"unsupported port service: {service}")


@dataclass(frozen=True)
class ColleiContext:
    repo: Path
    scripts: Path
    global_config: GlobalConfig

    @classmethod
    def load(cls) -> ColleiContext:
        scripts = Path(__file__).resolve().parent
        repo = scripts.parent
        config_path = Path.home() / ".config" / "collei"
        return cls(
            repo=repo, scripts=scripts, global_config=GlobalConfig.load(config_path)
        )

    def vm(self, name: str | None = None) -> VmRuntime:
        if name:
            directory = self.global_config.vm_root / name
        else:
            link = self.global_config.default_vm_link
            if not link.is_symlink() or not link.exists():
                raise ColleiError(f"is {link} a valid vm dir ?")
            directory = link.resolve()
        return VmRuntime.inspect(VmConfig.load(directory))

    def set_default(self, vm: VmRuntime) -> None:
        link = self.global_config.default_vm_link
        link.parent.mkdir(parents=True, exist_ok=True)
        link.unlink(missing_ok=True)
        os.symlink(vm.directory, link)

    def list_vms(self, active: bool | None = None) -> list[VmRuntime]:
        result: list[VmRuntime] = []
        for directory in sorted(self.global_config.vm_root.iterdir()):
            if not directory.is_dir() or not (directory / "opt").is_dir():
                continue
            runtime = VmRuntime.inspect(VmConfig.load(directory))
            if active is None or runtime.active == active:
                result.append(runtime)
        return result

    def master_ip(self) -> str:
        configured = self.global_config.directory.get("vnc")
        if configured is not None:
            return configured
        bridge = "br-in" if Path("/sys/class/net/br-in").exists() else "br9527"
        completed = subprocess.run(
            ["ip", "-4", "-o", "addr", "show", "dev", bridge],
            text=True,
            capture_output=True,
            check=False,
        )
        match = re.search(r"\binet ([0-9.]+)/", completed.stdout)
        return match.group(1) if match else self.global_config.master_ip
