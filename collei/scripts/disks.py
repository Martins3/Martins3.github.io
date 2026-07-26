from __future__ import annotations

from pathlib import Path

from commands import CommandRunner
from errors import ColleiError

DEFAULT_BOOT_SIZE = "350G"
DEFAULT_RAW_BOOT_SIZE = "40G"


def create_disk_image(
    runner: CommandRunner,
    path: Path,
    size: str,
    fmt: str = "qcow2",
) -> None:
    """统一封装 qemu-img create；镜像父目录不存在时自动创建。"""
    path.parent.mkdir(parents=True, exist_ok=True)
    runner.run(["qemu-img", "create", "-f", fmt, path, size])


def create_missing_disk(
    runner: CommandRunner,
    path: Path,
    size: str,
    fmt: str = "qcow2",
) -> None:
    """幂等创建：只有文件不存在时才创建。"""
    if not path.is_file():
        create_disk_image(runner, path, size, fmt)

# TODO 这里的逻辑还是非常奇怪的，将太多逻辑都放到这里了
def create_standard_boot_disks(
    vm_dir: Path,
    runner: CommandRunner,
    *,
    disk_count: int,
    raw: bool = False,
    boot_size: str | None = None,
) -> None:
    """创建标准 boot 盘；不负责写 opt/disk。"""
    if disk_count not in {1, 2, 3}:
        raise ColleiError("disk_count must be 1, 2 or 3")
    if raw and disk_count != 1:
        raise ColleiError("raw boot image requires exactly one disk")
    image_format = "raw" if raw else "qcow2"
    size = boot_size if boot_size is not None else (
        DEFAULT_RAW_BOOT_SIZE if raw else DEFAULT_BOOT_SIZE
    )
    for index in range(1, disk_count + 1):
        image = vm_dir / f"img/boot{index}"
        create_disk_image(runner, image, size, image_format)
