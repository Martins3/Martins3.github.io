from __future__ import annotations

import getopt
from dataclasses import dataclass
from typing import Sequence

from errors import ColleiError, ColleiHelp


# TODO 可能这个还是需要慢慢想一想
# 现在让 actions 和 collei.py 都是可以用相同的参数
# 似乎直接这么实现就可以的了，但是实际上这个问题我们是一直都没有仔细思考过
# 也就是: rk 可以携带参数
# qd 也可以携带参数，而且每一个 action 都可以吃携带参数
# 并且 qd 中携带的参数中
#
# 也就是说，这个时候，rk 是收敛到 qd 的一个特殊命令而已，当然 run 是最特殊的
# 也就是有了这个思路，很多 run 的命令可以收敛掉
@dataclass(frozen=True)
class LaunchOptions:
    """collei.sh getopts 的 Python 对应；短选项语义保持不变。"""

    dry_run: bool = False
    migration: str | None = None
    gdb: bool = False
    efi_application: bool = False
    debug_kernel: bool = False
    foreground: bool = False
    nbd_target: bool = False

    @classmethod
    def parse(cls, arguments: Sequence[str]) -> LaunchOptions:
        try:
            options, remaining = getopt.getopt(
                list(arguments),
                "alTLdEhixvstwV",
                ["dry-run", "foreground", "help", "nbd-target"],
            )
        except getopt.GetoptError as error:
            raise ColleiError(str(error)) from error
        if remaining:
            raise ColleiError(f"unexpected arguments: {' '.join(remaining)}")

        dry_run = False
        migration: str | None = None
        migration_requests: set[str] = set()
        gdb = False
        efi_application = False
        debug_kernel = False
        foreground = False
        nbd_target = False
        for option, _ in options:
            if option in {"-h", "--help"}:
                raise ColleiHelp
            if option == "--dry-run":
                dry_run = True
            elif option == "-a":
                migration_requests.add("defer")
            elif option == "-l":
                migration_requests.add("file")
            elif option == "-L":
                migration_requests.add("cpr")
            elif option == "-T":
                migration_requests.add("cpr-transfer")
            elif option == "-d":
                gdb = True
            elif option == "-E":
                efi_application = True
            elif option in {"-i", "-x", "-v", "-V"}:
                raise ColleiError(
                    "VM installation moved to collei-install.py; "
                    f"use collei-install.py {option}"
                )
            elif option == "-s":
                debug_kernel = True
            elif option == "--foreground":
                foreground = True
            elif option == "--nbd-target":
                migration_requests.add("defer")
                nbd_target = True
            else:
                # -t/-w 在原脚本中也没有实现，不能静默忽略。
                raise ColleiError(f"unsupported option: {option}")
        # 原脚本按 a -> l -> L -> T 的 if/elif 顺序决定组合选项的优先级。
        migration = next(
            (
                mode
                for mode in ("defer", "file", "cpr", "cpr-transfer")
                if mode in migration_requests
            ),
            None,
        )
        return cls(
            dry_run=dry_run,
            migration=migration,
            gdb=gdb,
            efi_application=efi_application,
            debug_kernel=debug_kernel,
            foreground=foreground,
            nbd_target=nbd_target,
        )
