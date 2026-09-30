# ============================================
# initramfs 打包共用逻辑（virtme / init_run 等）
# ============================================
from __future__ import annotations

import os
import subprocess
from pathlib import Path

from errors import ColleiError


def is_statically_linked(binary: Path) -> bool:
    """Return whether ``file`` identifies a binary as statically linked."""
    try:
        completed = subprocess.run(
            ["file", "-L", str(binary)],
            check=False,
            capture_output=True,
            text=True,
        )
    except OSError as error:
        raise ColleiError(
            f"cannot inspect {binary}: the file command is unavailable"
        ) from error

    if completed.returncode != 0:
        return False
    description = f"{completed.stdout}\n{completed.stderr}"
    return "statically linked" in description or "static-pie linked" in description


def create_device_nodes(root: Path) -> None:
    # 如果 devtmpfs 不可用，需要这些基本设备。普通用户无法 mknod 时，
    # initramfs 启动后仍可由 devtmpfs 提供，所以保持容错行为。
    (root / "dev").mkdir(parents=True, exist_ok=True)
    for name, mode, major, minor in (
        ("null", 0o666, 1, 3),
        ("zero", 0o666, 1, 5),
        ("random", 0o666, 1, 8),
        ("urandom", 0o666, 1, 9),
        ("console", 0o622, 5, 1),
        ("kmsg", 0o660, 1, 11),
    ):
        try:
            os.mknod(root / f"dev/{name}", 0o20000 | mode, os.makedev(major, minor))
        except PermissionError:
            pass


def pack_cpio_zst(root: Path, archive: Path) -> None:
    # 打包为 cpio.zst。走外部 zstd 而不是 Python gzip:
    # 19.5MB 的 initramfs 用 gzip level 9 要 ~3.4s (level 1 也要 0.13s)，
    # zstd -1 只需 ~30ms，内核 CONFIG_RD_ZSTD 直接支持。
    archive.parent.mkdir(parents=True, exist_ok=True)
    find = subprocess.Popen(["find", ".", "-print0"], cwd=root, stdout=subprocess.PIPE)
    if find.stdout is None:
        raise ColleiError("cannot read find output")
    cpio = subprocess.Popen(
        ["cpio", "--null", "-o", "--format=newc"],
        cwd=root,
        stdin=find.stdout,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
    )
    find.stdout.close()
    if cpio.stdout is None:
        raise ColleiError("cannot read cpio output")
    with archive.open("wb") as output:
        zstd = subprocess.Popen(
            ["zstd", "-q", "-1", "-T0"],
            stdin=cpio.stdout,
            stdout=output,
        )
    cpio.stdout.close()
    if cpio.wait() or find.wait() or zstd.wait():
        raise ColleiError(f"failed to create initramfs: {archive}")
