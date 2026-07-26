from __future__ import annotations

import os
import shlex
import subprocess
from dataclasses import dataclass
from pathlib import Path
from typing import IO, Mapping, Sequence

from errors import CommandError


@dataclass(frozen=True)
class CommandResult:
    argv: tuple[str, ...]
    returncode: int
    stdout: str
    stderr: str


class CommandRunner:
    def __init__(self, dry_run: bool = False) -> None:
        self.dry_run = dry_run

    def run(
        self,
        argv: Sequence[str | Path],
        *,
        check: bool = True,
        capture: bool = False,
        cwd: Path | None = None,
        env: Mapping[str, str] | None = None,
        stdin: IO[str] | int | None = None,
        input_text: str | None = None,
    ) -> CommandResult:
        command = tuple(str(item) for item in argv)
        if self.dry_run:
            print(shlex.join(command))
            return CommandResult(command, 0, "", "")
        process_env = os.environ.copy()
        if env:
            process_env.update(env)
        completed = subprocess.run(
            command,
            check=False,
            cwd=cwd,
            env=process_env,
            stdin=stdin,
            input=input_text,
            text=True,
            capture_output=capture,
        )
        result = CommandResult(
            command,
            completed.returncode,
            completed.stdout or "",
            completed.stderr or "",
        )
        if check and completed.returncode:
            detail = result.stderr.strip()
            suffix = f": {detail}" if detail else ""
            raise CommandError(
                f"command failed ({completed.returncode}): {shlex.join(command)}{suffix}"
            )
        return result

    def exec(
        self,
        argv: Sequence[str | Path],
        *,
        cwd: Path | None = None,
        env: Mapping[str, str] | None = None,
    ) -> None:
        command = [str(item) for item in argv]
        if self.dry_run:
            print(shlex.join(command))
            return
        if cwd is not None:
            os.chdir(cwd)
        process_env = os.environ.copy()
        if env:
            process_env.update(env)
        os.execvpe(command[0], command, process_env)
