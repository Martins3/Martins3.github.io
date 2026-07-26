from __future__ import annotations

import shlex
import stat
from dataclasses import dataclass
from pathlib import Path

from errors import ColleiError


@dataclass(frozen=True)
class QemuCommand:
    argv: tuple[str, ...]

    @classmethod
    def from_script(cls, path: Path) -> QemuCommand:
        try:
            lines = path.read_text().splitlines()
        except FileNotFoundError as error:
            raise ColleiError(f"generated command is missing: {path}") from error
        body = "\n".join(
            line
            for line in lines
            if not line.startswith("#!") and not line.startswith("set ")
        )
        body = body.replace("\\\n", " ")
        try:
            argv = tuple(shlex.split(body, posix=True))
        except ValueError as error:
            raise ColleiError(
                f"cannot parse generated command {path}: {error}"
            ) from error
        if not argv:
            raise ColleiError(f"generated command is empty: {path}")
        return cls(argv)

    def shell_text(self) -> str:
        return shlex.join(self.argv)

    def write_script(self, path: Path) -> None:
        lines = ["#!/usr/bin/env bash", "set -E -e -u -o pipefail"]
        index = 0
        # 如果当前参数以 - 开头且下一个参数不以 - 开头，就把它们合并成一行。
        # 不过，为什么以前用 bash 实现就没有这么复杂啊，不想看了，就这样吧
        while index < len(self.argv):
            argument = self.argv[index]
            is_first = index == 0
            parts = [shlex.quote(argument)]
            # Keep option flags and their values on the same line so that
            # generated scripts are easier to read and edit by hand.
            if (
                argument.startswith("-")
                and index + 1 < len(self.argv)
                and not self.argv[index + 1].startswith("-")
            ):
                index += 1
                parts.append(shlex.quote(self.argv[index]))
            line = " ".join(parts)
            if index < len(self.argv) - 1:
                line += " \\"
            prefix = "" if is_first else "\t"
            lines.append(f"{prefix}{line}")
            index += 1
        path.write_text("\n".join(lines) + "\n")
        path.chmod(path.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)
