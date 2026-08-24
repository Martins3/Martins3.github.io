from __future__ import annotations

import hashlib
import json
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any, Sequence

from commands import CommandRunner
from errors import ColleiError
from monitor import QmpClient
from runtime import ColleiContext

MIRROR_TIMEOUT = 3600.0
CLEANUP_TIMEOUT = 60.0

# 整体流程
# - discover_migratable_disks()：通过源 QEMU 的 query-block 找出可迁移的可写磁盘。
# - prepare_target_images()：在目标 slot 创建同格式、同 virtual size 的空镜像。
# - BlockMirrorSession.start()：在目标 QEMU 上启动 NBD server，并把目标磁盘 export 出来。
# - BlockMirrorSession.start()：在源 QEMU 上添加 NBD client 节点，指向目标磁盘 export。
# - blockdev-mirror：源 QEMU 把源磁盘完整同步到目标磁盘，期间 guest 新写入也会同步。
# - wait_ready()：轮询 query-block-jobs，等所有 mirror job 达到 ready 和 actively-synced。
# - finish() / cleanup()：停止 mirror job、删除临时 NBD client/export/server。
#
# 核心结构体:
# - `MigratableDisk`：描述一块要迁移的磁盘，包括源路径、目标路径、格式、大小、QEMU block node 名。
# - `MirrorResource`：给每块磁盘生成 QMP 资源名：NBD export id、源端 NBD client node、mirror job id。
# - `BlockMirrorSession`：真正管理一次磁盘同步会话的状态机。


@dataclass(frozen=True)
class MigratableDisk:
    node_name: str
    source: Path
    target: Path
    image_format: str
    virtual_size: int


@dataclass(frozen=True)
class MirrorResource:
    disk: MigratableDisk
    export_id: str
    client_node: str
    job_id: str


def _qmp_list(value: object, command: str) -> list[dict[str, Any]]:
    if not isinstance(value, list) or not all(isinstance(item, dict) for item in value):
        raise ColleiError(f"{command} returned invalid data: {value!r}")
    return value


def qmp_request(command: str, arguments: dict[str, object] | None = None) -> str:
    request: dict[str, object] = {"execute": command}
    if arguments:
        request["arguments"] = arguments
    return json.dumps(request, separators=(",", ":"))


def qmp_execute(
    qmp: QmpClient,
    command: str,
    arguments: dict[str, object] | None = None,
) -> Any:
    print(f"QMP {qmp.path} <= {qmp_request(command, arguments)}", flush=True)
    return qmp.execute(command, arguments)


def _relative_image(filename: str, image_directory: Path) -> Path:
    source = Path(filename).resolve()
    try:
        relative = source.relative_to(image_directory.resolve())
    except ValueError as error:
        raise ColleiError(
            f"writable disk {source} is outside migration image directory "
            f"{image_directory}"
        ) from error
    if not relative.parts or ".." in relative.parts:
        raise ColleiError(f"invalid migration image path: {source}")
    return relative


def discover_migratable_disks(
    qmp_path: Path,
    source_directory: Path,
    target_directory: Path,
) -> tuple[MigratableDisk, ...]:
    with QmpClient(qmp_path) as qmp:
        blocks = _qmp_list(qmp_execute(qmp, "query-block"), "query-block")

    disks: list[MigratableDisk] = []
    seen: set[str] = set()
    for block in blocks:
        inserted = block.get("inserted")
        if not isinstance(inserted, dict):
            continue
        device = block.get("device")
        if (
            isinstance(device, str)
            and device.startswith("pflash")
            and device.removeprefix("pflash").isdigit()
        ):
            # OVMF pflash is shared by both local QEMU slots, not an image-slot disk.
            continue
        qdev = block.get("qdev")
        if not isinstance(qdev, str) or not qdev:
            continue
        if inserted.get("ro") is True:
            continue

        node_name = inserted.get("node-name")
        image_format = inserted.get("drv")
        image = inserted.get("image")
        if not isinstance(node_name, str) or not node_name:
            raise ColleiError(f"writable block device {qdev!r} has no node-name")
        if node_name in seen:
            raise ColleiError(f"duplicate writable block node: {node_name}")
        if image_format not in {"qcow2", "raw"}:
            raise ColleiError(
                f"block migration does not support {node_name} format {image_format!r}"
            )
        if not isinstance(image, dict):
            raise ColleiError(f"block node {node_name} has no image metadata")
        filename = image.get("filename")
        virtual_size = image.get("virtual-size")
        if not isinstance(filename, str) or not isinstance(virtual_size, int):
            raise ColleiError(f"block node {node_name} has invalid image metadata")
        if inserted.get("encrypted") is True:
            raise ColleiError(f"encrypted block node is not supported: {node_name}")

        relative = _relative_image(filename, source_directory)
        target = (target_directory / relative).resolve()
        try:
            target.relative_to(target_directory.resolve())
        except ValueError as error:
            raise ColleiError(f"invalid target image path: {target}") from error
        disks.append(
            MigratableDisk(
                node_name=node_name,
                source=Path(filename).resolve(),
                target=target,
                image_format=image_format,
                virtual_size=virtual_size,
            )
        )
        seen.add(node_name)

    if not disks:
        raise ColleiError("QEMU has no writable file-backed disks to migrate")
    return tuple(sorted(disks, key=lambda disk: disk.node_name))


def prepare_target_images(
    context: ColleiContext,
    runner: CommandRunner,
    disks: Sequence[MigratableDisk],
) -> None:
    qemu_img = context.repo.parent.parent / "qemu/build/qemu-img"
    if not qemu_img.is_file():
        raise ColleiError(f"qemu-img not found: {qemu_img}")
    for disk in disks:
        print(
            f"prepare {disk.node_name}: {disk.image_format} "
            f"{disk.virtual_size} bytes -> {disk.target}"
        )
        if not runner.dry_run:
            disk.target.parent.mkdir(parents=True, exist_ok=True)
            disk.target.unlink(missing_ok=True)
        runner.run(
            [
                qemu_img,
                "create",
                "-f",
                disk.image_format,
                disk.target,
                str(disk.virtual_size),
            ]
        )


def _resource_id(prefix: str, node_name: str, limit: int = 31) -> str:
    safe = "".join(character if character.isalnum() else "-" for character in node_name)
    digest = hashlib.sha256(node_name.encode()).hexdigest()[:8]
    suffix = f"-{digest}"
    available = limit - len(prefix) - len(suffix)
    return f"{prefix}{safe[:available]}{suffix}"


def _resources(disks: Sequence[MigratableDisk]) -> tuple[MirrorResource, ...]:
    return tuple(
        MirrorResource(
            disk=disk,
            export_id=_resource_id("exp-", disk.node_name, 48),
            client_node=_resource_id("mig-", disk.node_name),
            job_id=_resource_id("mirror-", disk.node_name, 48),
        )
        for disk in disks
    )


class BlockMirrorSession:
    def __init__(
        self,
        source_qmp: Path,
        target_qmp: Path,
        nbd_socket: Path,
        disks: Sequence[MigratableDisk],
    ) -> None:
        if len(str(nbd_socket).encode()) >= 108:
            raise ColleiError(f"NBD Unix socket path is too long: {nbd_socket}")
        self.source_qmp = source_qmp
        self.target_qmp = target_qmp
        self.nbd_socket = nbd_socket
        self.resources = _resources(disks)
        self.server_started = False
        self.exports: list[MirrorResource] = []
        self.clients: list[MirrorResource] = []
        self.jobs: list[MirrorResource] = []

    def print_plan(self) -> None:
        print(
            qmp_request(
                "nbd-server-start",
                {
                    "addr": {
                        "type": "unix",
                        "data": {"path": str(self.nbd_socket)},
                    },
                    "max-connections": len(self.resources),
                },
            )
        )
        for resource in self.resources:
            print(
                qmp_request(
                    "block-export-add",
                    {
                        "type": "nbd",
                        "id": resource.export_id,
                        "node-name": resource.disk.node_name,
                        "name": resource.disk.node_name,
                        "writable": True,
                    },
                )
            )
            print(
                qmp_request(
                    "blockdev-add",
                    {
                        "driver": "nbd",
                        "node-name": resource.client_node,
                        "server": {"type": "unix", "path": str(self.nbd_socket)},
                        "export": resource.disk.node_name,
                    },
                )
            )
            print(qmp_request("blockdev-mirror", self._mirror_arguments(resource)))

    @staticmethod
    def _mirror_arguments(resource: MirrorResource) -> dict[str, object]:
        return {
            "job-id": resource.job_id,
            "device": resource.disk.node_name,
            "target": resource.client_node,
            "sync": "full",
            "copy-mode": "write-blocking",
            "on-source-error": "report",
            "on-target-error": "report",
            "auto-dismiss": False,
        }

    def start(self) -> None:
        self.nbd_socket.unlink(missing_ok=True)
        with QmpClient(self.target_qmp) as qmp:
            qmp_execute(
                qmp,
                "nbd-server-start",
                {
                    "addr": {
                        "type": "unix",
                        "data": {"path": str(self.nbd_socket)},
                    },
                    "max-connections": len(self.resources),
                },
            )
            self.server_started = True
            for resource in self.resources:
                qmp_execute(
                    qmp,
                    "block-export-add",
                    {
                        "type": "nbd",
                        "id": resource.export_id,
                        "node-name": resource.disk.node_name,
                        "name": resource.disk.node_name,
                        "writable": True,
                    },
                )
                self.exports.append(resource)

        with QmpClient(self.source_qmp) as qmp:
            for resource in self.resources:
                qmp_execute(
                    qmp,
                    "blockdev-add",
                    {
                        "driver": "nbd",
                        "node-name": resource.client_node,
                        "server": {"type": "unix", "path": str(self.nbd_socket)},
                        "export": resource.disk.node_name,
                    },
                )
                self.clients.append(resource)
                qmp_execute(qmp, "blockdev-mirror", self._mirror_arguments(resource))
                self.jobs.append(resource)

    def _query_jobs(self) -> dict[str, dict[str, Any]]:
        with QmpClient(self.source_qmp) as qmp:
            jobs = _qmp_list(qmp_execute(qmp, "query-block-jobs"), "query-block-jobs")
        return {
            str(job["device"]): job
            for job in jobs
            if isinstance(job.get("device"), str)
        }

    def wait_ready(self, timeout: float = MIRROR_TIMEOUT) -> None:
        deadline = time.monotonic() + timeout
        last_report = 0.0
        while time.monotonic() < deadline:
            jobs = self._query_jobs()
            all_ready = True
            progress: list[str] = []
            for resource in self.jobs:
                job = jobs.get(resource.job_id)
                if job is None:
                    raise ColleiError(f"mirror job disappeared: {resource.job_id}")
                status = job.get("status")
                io_status = job.get("io-status")
                error = job.get("error")
                if error is not None or io_status not in {None, "ok"}:
                    raise ColleiError(
                        f"mirror {resource.disk.node_name} failed: "
                        f"status={status}, io-status={io_status}, error={error}"
                    )
                ready = job.get("ready") is True
                actively_synced = job.get("actively-synced") is True
                all_ready = all_ready and ready and actively_synced
                offset = int(job.get("offset", 0))
                length = int(job.get("len", 0))
                percent = 100.0 if length == 0 else offset * 100.0 / length
                progress.append(f"{resource.disk.node_name}={percent:.1f}%/{status}")
            now = time.monotonic()
            if now - last_report >= 2.0 or all_ready:
                print("mirror status: " + ", ".join(progress), flush=True)
                last_report = now
            if all_ready:
                return
            time.sleep(0.2)
        raise ColleiError(f"block mirror timed out after {timeout:g} seconds")

    def finish(self, timeout: float = CLEANUP_TIMEOUT) -> None:
        jobs = self._query_jobs()
        with QmpClient(self.source_qmp) as qmp:
            for resource in self.jobs:
                job = jobs.get(resource.job_id)
                if job is not None and job.get("status") != "concluded":
                    qmp_execute(qmp, "block-job-cancel", {"device": resource.job_id})
        self._wait_jobs_concluded(timeout)

    def _wait_jobs_concluded(self, timeout: float) -> None:
        pending = {resource.job_id for resource in self.jobs}
        deadline = time.monotonic() + timeout
        job_errors: list[str] = []
        while pending and time.monotonic() < deadline:
            jobs = self._query_jobs()
            with QmpClient(self.source_qmp) as qmp:
                for job_id in tuple(pending):
                    job = jobs.get(job_id)
                    if job is None:
                        pending.remove(job_id)
                        continue
                    if job.get("status") != "concluded":
                        continue
                    error = job.get("error")
                    qmp_execute(qmp, "job-dismiss", {"id": job_id})
                    pending.remove(job_id)
                    if error is not None:
                        job_errors.append(f"{job_id}: {error}")
            if pending:
                time.sleep(0.1)
        if pending:
            raise ColleiError(f"mirror jobs did not stop: {sorted(pending)}")
        self.jobs.clear()
        if job_errors:
            raise ColleiError("mirror job failed: " + "; ".join(job_errors))

    def cleanup(self, *, best_effort: bool = False) -> None:
        errors: list[str] = []

        if self.jobs:
            try:
                jobs = self._query_jobs()
                with QmpClient(self.source_qmp) as qmp:
                    for resource in self.jobs:
                        job = jobs.get(resource.job_id)
                        if job is not None and job.get("status") != "concluded":
                            qmp_execute(
                                qmp,
                                "block-job-cancel",
                                {"device": resource.job_id, "force": True},
                            )
                self._wait_jobs_concluded(CLEANUP_TIMEOUT)
            except (ColleiError, OSError) as error:
                errors.append(str(error))

        if self.clients:
            try:
                with QmpClient(self.source_qmp) as qmp:
                    for resource in reversed(self.clients):
                        qmp_execute(
                            qmp,
                            "blockdev-del",
                            {"node-name": resource.client_node},
                        )
                self.clients.clear()
            except (ColleiError, OSError) as error:
                errors.append(str(error))

        if self.exports:
            try:
                with QmpClient(self.target_qmp) as qmp:
                    for resource in reversed(self.exports):
                        qmp_execute(
                            qmp,
                            "block-export-del",
                            {"id": resource.export_id, "mode": "safe"},
                        )
                self._wait_exports_removed(CLEANUP_TIMEOUT)
                self.exports.clear()
            except (ColleiError, OSError) as error:
                errors.append(str(error))

        if self.server_started:
            try:
                with QmpClient(self.target_qmp) as qmp:
                    qmp_execute(qmp, "nbd-server-stop")
                self.server_started = False
            except (ColleiError, OSError) as error:
                errors.append(str(error))
        if not self.server_started:
            self.nbd_socket.unlink(missing_ok=True)

        if errors:
            detail = "; ".join(errors)
            if best_effort:
                print(f"warning: block migration cleanup incomplete: {detail}")
            else:
                raise ColleiError(f"block migration cleanup failed: {detail}")

    def _wait_exports_removed(self, timeout: float) -> None:
        ids = {resource.export_id for resource in self.exports}
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            with QmpClient(self.target_qmp) as qmp:
                exports = _qmp_list(
                    qmp_execute(qmp, "query-block-exports"),
                    "query-block-exports",
                )
            present = {
                str(export["id"])
                for export in exports
                if isinstance(export.get("id"), str)
            }
            if not ids & present:
                return
            time.sleep(0.1)
        raise ColleiError(f"block exports did not stop: {sorted(ids)}")
