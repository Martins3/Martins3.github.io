from __future__ import annotations

import sys
from collections.abc import Callable, Sequence
from typing import TextIO

from errors import ColleiError


def confirm(
    message: str,
    *,
    default: bool = False,
    reader: Callable[[str], str] | None = None,
) -> bool:
    read = reader or input
    suffix = "[Y/n]" if default else "[y/N]"
    while True:
        try:
            answer = read(f"{message} {suffix} ").strip().lower()
        except EOFError:
            return default
        if not answer:
            return default
        if answer in {"y", "yes"}:
            return True
        if answer in {"n", "no"}:
            return False
        print("please answer y or n", file=sys.stderr)


def choose(
    items: Sequence[str],
    *,
    prompt: str = "Choose",
    reader: Callable[[str], str] | None = None,
    output: TextIO | None = None,
) -> str:
    choices = list(items)
    if not choices:
        raise ColleiError("no choices available")
    stream = output or sys.stdout
    for index, item in enumerate(choices, 1):
        print(f"{index:>2}. {item}", file=stream)
    read = reader or input
    try:
        answer = read(f"{prompt} [1-{len(choices)}]: ").strip()
    except EOFError as error:
        raise ColleiError("no selection") from error
    if answer in choices:
        return answer
    try:
        index = int(answer)
    except ValueError as error:
        raise ColleiError(f"invalid selection: {answer}") from error
    if not 1 <= index <= len(choices):
        raise ColleiError(f"invalid selection: {answer}")
    return choices[index - 1]


def print_table(headers: Sequence[str], rows: Sequence[Sequence[str]]) -> None:
    table = [list(headers), *(list(row) for row in rows)]
    if not table[0]:
        return
    widths = [max(len(row[column]) for row in table) for column in range(len(headers))]
    for index, row in enumerate(table):
        print(
            "  ".join(value.ljust(widths[column]) for column, value in enumerate(row))
        )
        if index == 0:
            print("  ".join("-" * width for width in widths))


def print_banner(
    text: str, *, color: int | None = None, output: TextIO | None = None
) -> None:
    stream = output or sys.stdout
    border = "=" * (len(text) + 4)
    colored = text
    if color is not None and stream.isatty():
        colored = f"\033[38;5;{color}m{text}\033[0m"
    print(border, file=stream)
    print(f"| {colored} |", file=stream)
    print(border, file=stream)
