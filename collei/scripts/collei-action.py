#!/usr/bin/env python3
from __future__ import annotations

import getopt
import subprocess
import sys
from pathlib import Path
from typing import Sequence

from actions import ActionContext, VmRequirement, registry, validate_requirement
from commands import CommandRunner
from errors import ColleiError
from runtime import ColleiContext, VmRuntime
from ui import print_banner


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


def _choose_vm(context: ColleiContext, requirement: VmRequirement) -> VmRuntime:
    active = None
    if requirement is VmRequirement.ACTIVE:
        active = True
    elif requirement is VmRequirement.INACTIVE:
        active = False
    candidates = context.list_vms(active)
    if not candidates:
        raise ColleiError("no matching VM found")
    if len(candidates) == 1:
        return candidates[0]
    selected = _choose([str(vm.directory) for vm in candidates])
    return context.vm(Path(selected).name)


def _show_vm(vm: VmRuntime) -> None:
    color = 112 if vm.which_qemu == "t" else 212
    print_banner(vm.config.name, color=color)


def _show_help() -> None:
    print("usage: collei-action.py [-a ACTION] [-n VM] [-s] [-y] [--dry-run]")
    print()
    print("actions:")
    print("  " + "\n  ".join(sorted(registry())))


def main(argv: Sequence[str] | None = None) -> int:
    raw = list(sys.argv[1:] if argv is None else argv)
    try:
        options, remainder = getopt.getopt(raw, "a:n:syh", ["dry-run"])
        action_name = "none"
        vm_name: str | None = None
        choose = False
        auto_yes = False
        dry_run = False
        for option, value in options:
            if option == "-a":
                action_name = value
            elif option == "-n":
                vm_name = value
            elif option == "-s":
                choose = True
            elif option == "-y":
                auto_yes = True
            elif option == "--dry-run":
                dry_run = True
            elif option == "-h":
                _show_help()
                return 0

        actions = registry()
        if action_name == "none":
            action_name = _choose(sorted(actions))
        action = actions.get(action_name)
        if action is None or action.function is None:
            raise ColleiError(f"unsupported action: {action_name}")

        context = ColleiContext.load()
        if choose:
            vm = _choose_vm(context, action.requirement)
        else:
            vm = context.vm(vm_name)
            if action_name == "ssh" and not vm.active and vm_name is None:
                vm = _choose_vm(context, VmRequirement.ACTIVE)
        validate_requirement(action, vm)
        if vm_name is not None or choose:
            context.set_default(vm)
        _show_vm(vm)
        action_context = ActionContext(context, vm, CommandRunner(dry_run), auto_yes)
        action.function(action_context, remainder)
        return 0
    except (ColleiError, getopt.GetoptError, OSError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
