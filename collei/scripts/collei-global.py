#!/usr/bin/env python3
from __future__ import annotations

import getopt
import subprocess
import sys
from collections.abc import Sequence

from commands import CommandRunner
from errors import ColleiError
from global_actions import ACTIONS
from runtime import ColleiContext


def _choose(items: Sequence[str]) -> str:
    completed = subprocess.run(
        ["fzf"],
        input="\n".join(items) + "\n",
        text=True,
        capture_output=True,
        check=False,
    )
    if completed.returncode or not completed.stdout.strip():
        raise ColleiError("no selection")
    return completed.stdout.strip().splitlines()[0]


def _show_help() -> None:
    print("usage: collei-global.py [-a action]")
    print("actions:")
    for action in sorted(ACTIONS):
        print(f"  {action}")


def main(argv: Sequence[str] | None = None) -> int:
    raw = list(sys.argv[1:] if argv is None else argv)
    try:
        options, _ = getopt.getopt(raw, "ha:")
        action = "none"
        for option, value in options:
            if option == "-a":
                action = value
            elif option == "-h":
                _show_help()
                return 0
        if action == "none":
            action = _choose(sorted(ACTIONS))
        function = ACTIONS.get(action)
        if function is None:
            raise ColleiError(f"unsupported action: {action}")
        function(ColleiContext.load(), CommandRunner())
        return 0
    except (ColleiError, getopt.GetoptError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
