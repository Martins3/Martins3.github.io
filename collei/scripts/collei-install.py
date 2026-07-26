#!/usr/bin/env python3
from __future__ import annotations

import getopt
import sys
from dataclasses import dataclass
from typing import Sequence

from commands import CommandRunner
from errors import ColleiError
from install import (
    choose_disk_layout,
    choose_iso,
    choose_virtme_vm_name,
    choose_vm_name,
    install_vm,
)
from runtime import ColleiContext
from ui import choose


class InstallHelp(Exception):
    pass


@dataclass(frozen=True)
class InstallOptions:
    mode: str

    @classmethod
    def parse(cls, arguments: Sequence[str]) -> InstallOptions:
        try:
            options, remaining = getopt.getopt(list(arguments), "hfioxvVS", ["help"])
        except getopt.GetoptError as error:
            raise ColleiError(str(error)) from error
        if remaining:
            raise ColleiError(f"unexpected arguments: {' '.join(remaining)}")

        modes: list[str] = []
        for option, _ in options:
            if option in {"-h", "--help"}:
                raise InstallHelp
            if option == "-f":
                modes.append("fedora")
            elif option == "-i":
                modes.append("iso")
            elif option == "-o":
                modes.append("openeuler")
            elif option == "-x":
                modes.append("nixos")
            elif option == "-v":
                modes.append("vmtest")
            elif option == "-V":
                modes.append("virtme")
            elif option == "-S":
                modes.append("kitty")
        if not modes:
            raise InstallHelp
        if len(modes) != 1:
            raise ColleiError("choose exactly one install mode")
        return cls(mode=modes[0])


def print_help() -> None:
    print(
        """usage: collei-install.py MODE

创建 VM 目录并更新 ~/.config/collei/last；不会生成 cmd.sh，也不会启动 QEMU。
启动已有 VM 使用 collei.py。

模式：
  -i  从 ISO 创建 VM
  -f  从 Fedora Server ISO 创建无人值守安装 VM
  -o  从 openEuler ISO 创建无人值守安装 VM
  -S  从 kitty ISO 创建无人值守安装 VM（默认双盘 RAID1）
  -x  创建 NixOS VM
  -v  创建 vmtest VM，使用 host / 作为 9p rootfs
  -V  创建 virtme VM，使用 virtio-fs 共享 host rootfs
  -h, --help  显示此帮助"""
    )


def install_selected(options: InstallOptions) -> None:
    context = ColleiContext.load()
    runner = CommandRunner()
    if options.mode == "virtme":
        name = choose_virtme_vm_name(context, runner)
        disk_count = int(choose(("1", "3"), prompt="Disk count"))
        install_vm(context, "virtme", runner, name=name, disk_count=disk_count)
    elif options.mode == "vmtest":
        name = choose_vm_name(context, runner, "vmtest")
        disk_count, raw = choose_disk_layout(runner)
        # 原 collei-disk.sh 允许 raw；Python builder 同样按 magic 识别格式。
        install_vm(
            context,
            "vmtest",
            runner,
            name=name,
            disk_count=disk_count,
            raw=raw,
        )
    elif options.mode == "iso":
        iso = choose_iso(context, runner)
        name = choose_vm_name(context, runner, iso.stem)
        disk_count, raw = choose_disk_layout(runner)
        install_vm(
            context,
            "iso",
            runner,
            name=name,
            iso=iso,
            disk_count=disk_count,
            raw=raw,
        )
    elif options.mode == "fedora":
        install_vm(context, "fedora", runner)
    elif options.mode == "openeuler":
        install_vm(context, "openeuler", runner)
    elif options.mode == "kitty":
        from kitty import install as kitty_install

        kitty_install(context, runner)
    elif options.mode == "nixos":
        name = choose_vm_name(context, runner, "nixos")
        install_vm(context, "nixos", runner, name=name)
    else:
        raise ColleiError(f"unsupported install mode: {options.mode}")


def main(argv: Sequence[str] | None = None) -> int:
    try:
        options = InstallOptions.parse(sys.argv[1:] if argv is None else argv)
        install_selected(options)
        return 0
    except InstallHelp:
        print_help()
        return 0
    except (ColleiError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
