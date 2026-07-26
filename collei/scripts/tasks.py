from __future__ import annotations

import os
import re
import uuid
from abc import ABC, abstractmethod
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence

from commands import CommandRunner
from errors import ColleiError
from runtime import ColleiContext, VmRuntime


@dataclass(frozen=True)
class TaskHandle:
    backend: str
    identifier: str


class TaskBackend(ABC):
    """后台任务运行后端抽象基类。"""

    @abstractmethod
    def add_task(
        self,
        runner: CommandRunner,
        command: Sequence[str | Path],
        *,
        vm: VmRuntime | None = None,
        group: str = "qemu",
        label: str = "task",
        record: bool = False,
        unit: str | None = None,
    ) -> TaskHandle:
        """提交一个后台任务并返回其句柄。"""

    @abstractmethod
    def exec_log(
        self,
        runner: CommandRunner,
        handle: TaskHandle,
        *,
        follow: bool,
    ) -> None:
        """查看指定任务日志。"""

    @abstractmethod
    def exec_follow(self, handle: TaskHandle) -> None:
        """持续跟踪指定任务输出。"""

    @abstractmethod
    def clean(self, runner: CommandRunner) -> None:
        """清理当前后端的残留状态。"""


class PueueBackend(TaskBackend):
    """基于 pueue 的后台任务实现。"""

    def add_task(
        self,
        runner: CommandRunner,
        command: Sequence[str | Path],
        *,
        vm: VmRuntime | None = None,
        group: str = "qemu",
        label: str = "task",
        record: bool = False,
        unit: str | None = None,
    ) -> TaskHandle:
        _ = label
        _ = unit
        if vm is not None and record:
            task_id = self._add_to_vm(runner, command, vm.directory, group=group)
        else:
            added = runner.run(
                ["pueue", "add", "-i", "-g", group, "--", *command],
                capture=True,
            )
            print(added.stdout, end="")
            match = re.search(r"\bid ([0-9]+)\b", added.stdout)
            task_id = match.group(1) if match is not None else ""
        if not task_id and runner.dry_run:
            task_id = "dry-run"
        if not task_id:
            raise ColleiError("cannot determine pueue task id")
        return TaskHandle("pueue", task_id)

    def _add_to_vm(
        self,
        runner: CommandRunner,
        command: Sequence[str | Path],
        vm_directory: Path,
        *,
        group: str,
    ) -> str:
        added = runner.run(
            ["pueue", "add", "-i", "-g", group, "--", *command],
            capture=True,
        )
        print(added.stdout, end="")
        match = re.search(r"\bid ([0-9]+)\b", added.stdout)
        if match is None:
            raise ColleiError("cannot determine pueue task id")
        task_id = match.group(1)
        (vm_directory / "pueue").write_text(f"{task_id}\n")
        return task_id

    def exec_log(
        self,
        runner: CommandRunner,
        handle: TaskHandle,
        *,
        follow: bool,
    ) -> None:
        command = ["pueue", "log", "-f", handle.identifier]
        if follow:
            command = ["pueue", "follow", handle.identifier]
        runner.exec(command)

    def exec_follow(self, handle: TaskHandle) -> None:
        os.execvp("pueue", ["pueue", "follow", handle.identifier])

    def clean(self, runner: CommandRunner) -> None:
        runner.run(["pueue", "clean"])


class SystemdBackend(TaskBackend):
    """基于 systemd-run 的后台任务实现。"""

    @staticmethod
    def _safe_unit_part(value: str) -> str:
        safe = re.sub(r"[^A-Za-z0-9_.-]+", "-", value.strip())
        safe = safe.strip(".-")
        return safe or "task"

    def vm_unit(self, vm: VmRuntime) -> str:
        suffix = (
            "" if vm.which_qemu == "s" else f"-{self._safe_unit_part(vm.which_qemu)}"
        )
        return f"collei-vm-{self._safe_unit_part(vm.config.name)}{suffix}.service"

    def transient_unit(self, vm: VmRuntime | None, label: str) -> str:
        parts = ["collei", self._safe_unit_part(label)]
        if vm is not None:
            parts.append(self._safe_unit_part(vm.config.name))
        parts.append(uuid.uuid4().hex[:12])
        return "-".join(parts) + ".service"

    def add_task(
        self,
        runner: CommandRunner,
        command: Sequence[str | Path],
        *,
        vm: VmRuntime | None = None,
        group: str = "qemu",
        label: str = "task",
        record: bool = False,
        unit: str | None = None,
    ) -> TaskHandle:
        _ = group
        unit_name = unit or self.transient_unit(vm, label)
        started = runner.run(
            [
                "systemd-run",
                "--user",
                f"--unit={unit_name}",
                "--collect",
                "--",
                *command,
            ],
            capture=True,
        )
        print(started.stdout, end="")
        return TaskHandle("systemd", unit_name)

    def exec_log(
        self,
        runner: CommandRunner,
        handle: TaskHandle,
        *,
        follow: bool,
    ) -> None:
        command = ["journalctl", "--user", "-u", handle.identifier, "-o", "cat"]
        if follow:
            command.append("-f")
        runner.exec(command)

    def exec_follow(self, handle: TaskHandle) -> None:
        CommandRunner().exec(
            ["journalctl", "--user", "-u", handle.identifier, "-f", "-o", "cat"]
        )

    def clean(self, runner: CommandRunner) -> None:
        runner.run(["systemctl", "--user", "reset-failed", "collei-*"], check=False)


def _get_backend(context: ColleiContext) -> TaskBackend:
    backend = context.global_config.task_backend
    if backend == "pueue":
        return PueueBackend()
    if backend == "systemd":
        return SystemdBackend()
    raise ColleiError(f"unsupported task backend: {backend}")


def _backend_for(handle: TaskHandle) -> TaskBackend:
    if handle.backend == "pueue":
        return PueueBackend()
    if handle.backend == "systemd":
        return SystemdBackend()
    raise ColleiError(f"unsupported task backend: {handle.backend}")


def _record_task(vm_directory: Path, handle: TaskHandle) -> None:
    (vm_directory / "task_backend").write_text(f"{handle.backend}\n")
    (vm_directory / "task_id").write_text(f"{handle.identifier}\n")
    if handle.backend == "pueue":
        (vm_directory / "pueue").write_text(f"{handle.identifier}\n")
    elif handle.backend == "systemd":
        (vm_directory / "systemd-unit").write_text(f"{handle.identifier}\n")


def read_recorded_task(context: ColleiContext, vm_directory: Path) -> TaskHandle:
    del context
    backend_file = vm_directory / "task_backend"
    task_file = vm_directory / "task_id"
    if backend_file.is_file() and task_file.is_file():
        return TaskHandle(
            backend_file.read_text().strip(),
            task_file.read_text().strip(),
        )

    if (vm_directory / "pueue").is_file():
        return TaskHandle("pueue", (vm_directory / "pueue").read_text().strip())
    if (vm_directory / "systemd-unit").is_file():
        return TaskHandle(
            "systemd", (vm_directory / "systemd-unit").read_text().strip()
        )
    raise ColleiError(f"no recorded background task for {vm_directory}")


def add_background_task(
    context: ColleiContext,
    runner: CommandRunner,
    command: Sequence[str | Path],
    *,
    vm: VmRuntime | None = None,
    group: str = "qemu",
    label: str = "task",
    record: bool = False,
) -> TaskHandle:
    backend = _get_backend(context)
    unit: str | None = None
    if record and vm is not None and isinstance(backend, SystemdBackend):
        unit = backend.vm_unit(vm)
    handle = backend.add_task(
        runner,
        command,
        vm=vm,
        group=group,
        label=label,
        record=record,
        unit=unit,
    )
    if vm is not None and record:
        _record_task(vm.directory, handle)
    return handle


def exec_task_log(
    context: ColleiContext,
    runner: CommandRunner,
    vm_directory: Path,
    *,
    follow: bool,
) -> None:
    handle = read_recorded_task(context, vm_directory)
    _backend_for(handle).exec_log(runner, handle, follow=follow)


def exec_task_follow(handle: TaskHandle) -> None:
    _backend_for(handle).exec_follow(handle)


def clean_task_backend(context: ColleiContext, runner: CommandRunner) -> None:
    _get_backend(context).clean(runner)
