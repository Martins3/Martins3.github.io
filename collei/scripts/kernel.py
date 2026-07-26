"""内核构建树相关的辅助函数。"""

from __future__ import annotations

import platform
import re
import subprocess
from pathlib import Path

from errors import UnsupportedNativeConfiguration


def kernel_image(kernel_dir: Path) -> Path:
    # Normal kernel debug boots point opt/kernel at a build tree.  Installer
    # boots (Fedora Anaconda, etc.) point it directly at an extracted vmlinuz.
    if kernel_dir.is_file():
        return kernel_dir
    if platform.machine() == "x86_64":
        return kernel_dir / "arch" / "x86" / "boot" / "bzImage"
    if platform.machine() == "aarch64":
        return kernel_dir / "arch" / "arm64" / "boot" / "Image"
    raise UnsupportedNativeConfiguration(
        f"unsupported architecture: {platform.machine()}"
    )


def kernel_release(kernel_dir: Path) -> str:
    """返回实际启动的内核版本。

    不能直接信任 include/config/kernel.release: 后续任何一次 make 调用都可能
    重新生成它，而 bzImage 和模块还是旧构建的，两者会不一致。优先从将要启动的
    内核镜像本身提取版本，失败时才回退到 release 文件。
    """
    image = kernel_image(kernel_dir)
    if image.is_file():
        release = _release_from_image(image, kernel_dir)
        if release:
            return release
    release_file = kernel_dir / "include" / "config" / "kernel.release"
    if release_file.is_file():
        return release_file.read_text().strip()
    return ""


def _release_from_image(image: Path, kernel_dir: Path) -> str:
    # file(1) 从 bzImage setup header 中读出嵌入的版本字符串。
    completed = subprocess.run(
        ["file", str(image)], text=True, capture_output=True, check=False
    )
    match = re.search(r"version ([\w.+-]+)", completed.stdout)
    if match:
        return match.group(1)
    # 回退: 解压 vmlinux 后搜索 "Linux version" 字符串。
    extractor = kernel_dir / "scripts" / "extract-vmlinux"
    if extractor.is_file():
        extracted = subprocess.run(
            [str(extractor), str(image)], capture_output=True, check=False
        )
        if extracted.returncode == 0:
            match = re.search(rb"Linux version ([\w.+-]+)", extracted.stdout)
            if match:
                return match.group(1).decode()
    return ""
