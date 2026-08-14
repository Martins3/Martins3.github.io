from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from errors import ColleiError
from options import IniConfig


@dataclass(frozen=True)
class GlobalConfig:
    directory: IniConfig

    @classmethod
    def load(cls, path: Path) -> GlobalConfig:
        if not path.is_dir():
            raise ColleiError(f"global config not setup: {path}")
        config = IniConfig(path / "config.ini", "collei")
        config.validate()
        return cls(config)

    @property
    def vm_root(self) -> Path:
        return Path(self.directory.require("vm")).expanduser()

    @property
    def default_vm_name(self) -> str:
        name = self.directory.require("default_vm")
        if re.fullmatch(r"[A-Za-z0-9_.-]+", name) is None:
            raise ColleiError(f"invalid default_vm in {self.directory.path}: {name}")
        return name

    @property
    def master_ip(self) -> str:
        return self.directory.require("ip")

    @property
    def task_backend(self) -> str:
        backend = self.directory.get("task_backend") or "pueue"
        aliases = {
            "pueue": "pueue",
            "systemd": "systemd",
        }
        try:
            return aliases[backend.strip().lower()]
        except KeyError as error:
            raise ColleiError(
                f"{self.directory.path / 'task_backend'} must be pueue or systemd"
            ) from error


@dataclass(frozen=True)
class VmConfig:
    directory: Path
    options: IniConfig

    @classmethod
    def load(cls, directory: Path) -> VmConfig:
        directory = directory.resolve()
        if not directory.is_dir():
            raise ColleiError(f"VM not found: {directory}")
        options = IniConfig(directory / "config.ini", "vm")
        options.validate()
        return cls(directory, options)

    @property
    def name(self) -> str:
        return self.directory.name

    @property
    def guest_id(self) -> int:
        return self.options.integer("id")
