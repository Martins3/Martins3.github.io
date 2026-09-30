#!/usr/bin/env python3
"""Build a source-level FD API index, not a list of runtime-supported commands.

No compiler, kernel build, third-party packages, or device access is required.
See fd-api-enumeration.md for coverage and deliberate limitations.
"""

import argparse
import csv
import json
import os
import re
import subprocess
from collections import defaultdict, deque
from collections.abc import Iterator
from dataclasses import dataclass
from pathlib import Path


ROOTS = (
    "arch",
    "block",
    "crypto",
    "drivers",
    "fs",
    "include",
    "init",
    "io_uring",
    "ipc",
    "kernel",
    "lib",
    "mm",
    "net",
    "security",
    "sound",
    "virt",
)
ENCODERS = {"_IO", "_IOR", "_IOW", "_IOWR", "_IOC"}
LEXICAL = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|/\*.*?\*/|//[^\n]*', re.S)
DEFINE = re.compile(r"^\s*#\s*define\s+(\w+)(\([^\n]*?\))?[^\S\n]+([^\n]+)", re.M)
ENUM = re.compile(r"\benum\s+(?:\w+\s*)?\{")
TABLE = re.compile(
    r"\bstruct\s+(file_operations|block_device_operations|proto_ops|"
    r"v4l2_file_operations|v4l2_ioctl_ops|vfio_device_ops|tty_operations|proc_ops)"
    r"\s+(?:const\s+)?(\w+)\s*(?:\[[^\]]*\]\s*)?=\s*\{"
)
TOKEN = re.compile(r"\b[A-Za-z_]\w*\b")
FIELD = re.compile(r"\.([A-Za-z_]\w*)\s*=\s*(.*)", re.S)


@dataclass(frozen=True)
class Definition:
    path: str
    name: str
    parameters: str
    expression: str
    form: str


def compact(value: str) -> str:
    return " ".join(value.split())


def clean_source(source: str) -> str:
    # C line splicing precedes comment removal. Keep strings and character literals.
    source = re.sub(r"\\\r?\n", "", source)
    return LEXICAL.sub(
        lambda m: re.sub(r"[^\n]", " ", m[0]) if m[0].startswith("/") else m[0],
        source,
    )


def mask_literals(source: str) -> str:
    return LEXICAL.sub(lambda m: " " * len(m[0]), source)


def remove_directives(source: str) -> str:
    return re.sub(r"^\s*#[^\n]*", "", source, flags=re.M)


def braced_body(source: str, start: int) -> str:
    """start is immediately after an opening brace; source is comment-free."""
    masked = mask_literals(source[start:])
    depth = 1
    for i, char in enumerate(masked):
        if char == "{":
            depth += 1
        elif char == "}":
            depth -= 1
            if depth == 0:
                return source[start : start + i]
    return ""


def split_members(body: str) -> Iterator[str]:
    """Split commas at top level, preserving nested calls and initializers."""
    depth = 0
    start = 0
    for i, char in enumerate(mask_literals(body)):
        if char in "([{":
            depth += 1
        elif char in ")]}":
            depth -= 1
        elif char == "," and depth == 0:
            yield body[start:i].strip()
            start = i + 1
    yield body[start:].strip()


def definitions(path: str, source: str) -> Iterator[Definition]:
    for match in DEFINE.finditer(source):
        name, parameters, expression = match.groups()
        yield Definition(path, name, parameters or "", compact(expression), "macro")
    for match in ENUM.finditer(source):
        body = remove_directives(braced_body(source, match.end()))
        for member in split_members(body):
            item = re.fullmatch(r"(\w+)\s*(?:=\s*(.*))?", member, re.S)
            if item:
                name, expression = item.groups()
                yield Definition(
                    path, name, "", compact(expression or "<implicit>"), "enum"
                )


def operation_rows(path: str, source: str) -> Iterator[list[str]]:
    for match in TABLE.finditer(source):
        table_type, name = match.groups()
        body = remove_directives(braced_body(source, match.end()))
        for member in split_members(body):
            field = FIELD.fullmatch(member)
            if field:
                yield [path, table_type, name, field[1], compact(field[2])]


def source_paths(root: Path) -> Iterator[Path]:
    for directory in ROOTS:
        for current, dirs, files in os.walk(root / directory):
            dirs[:] = sorted(
                d for d in dirs if not d.startswith(".") and d != "generated"
            )
            for name in sorted(files):
                path = Path(current) / name
                if path.suffix in {".h", ".c"} and not path.is_symlink():
                    yield path


def ioctl_candidates(items: list[Definition]) -> set[int]:
    # Follow aliases/wrappers across files, without pretending to resolve #if or
    # include scopes. Same-name definitions in other scopes can cause false positives.
    users: dict[str, list[int]] = defaultdict(list)
    for i, item in enumerate(items):
        parameters = set(TOKEN.findall(item.parameters))
        for dependency in (
            set(TOKEN.findall(mask_literals(item.expression))) - parameters
        ):
            users[dependency].append(i)
    pending = deque(sorted(ENCODERS))
    seen = set(ENCODERS)
    selected: set[int] = set()
    while pending:
        for i in users[pending.popleft()]:
            item = items[i]
            if item.name in ENCODERS:
                continue
            selected.add(i)
            if item.name not in seen:
                seen.add(item.name)
                pending.append(item.name)
    return selected


def write_tsv(path: Path, header: list[str], rows: list[list[str]]) -> None:
    with path.open("w", encoding="utf-8", newline="") as stream:
        writer = csv.writer(stream, delimiter="\t", lineterminator="\n")
        writer.writerow(header)
        writer.writerows(sorted(rows))


def git_info(root: Path, *args: str) -> str:
    try:
        return subprocess.check_output(
            ["git", "-C", str(root), *args], stderr=subprocess.DEVNULL, text=True
        ).strip()
    except (OSError, subprocess.CalledProcessError):
        return "unknown"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "kernel", type=Path, help="Linux source tree (all architectures scanned)"
    )
    parser.add_argument("--out", required=True, type=Path, help="new output directory")
    args = parser.parse_args()
    root = args.kernel.resolve()
    if not (root / "include/uapi").is_dir() or not (root / "Makefile").is_file():
        parser.error(f"not a Linux source tree: {root}")
    # Refuse existing destinations, so no previous scan or user files are overwritten.
    if args.out.exists():
        parser.error(f"output directory already exists: {args.out}")

    items: list[Definition] = []
    operations: list[list[str]] = []
    review_paths: set[str] = set()
    files = 0
    for path in source_paths(root):
        files += 1
        relative = path.relative_to(root).as_posix()
        raw_source = path.read_text(encoding="utf-8", errors="replace")
        source = clean_source(raw_source)
        items.extend(definitions(relative, source))
        operations.extend(operation_rows(relative, source))
        # Legacy headers may mention ioctl only in comments (e.g. scsi/sg.h).
        if "ioctl" in relative.lower() or "ioctl" in raw_source.lower():
            review_paths.add(relative)

    selected = ioctl_candidates(items)
    review_paths.update(items[i].path for i in selected)
    ioctl_rows: list[list[str]] = []
    review_rows: list[list[str]] = []
    for i, item in enumerate(items):
        scope = "uapi" if "uapi" in Path(item.path).parts else "non-uapi"
        row = [item.path, scope, item.form, item.name, item.parameters, item.expression]
        if i in selected:
            ioctl_rows.append(row)
        elif item.path in review_paths:
            review_rows.append(row)

    args.out.mkdir(parents=True)
    header = ["path", "scope", "form", "name", "parameters", "expression"]
    write_tsv(args.out / "ioctl-candidates.tsv", header, ioctl_rows)
    write_tsv(args.out / "review-definitions.tsv", header, review_rows)
    write_tsv(
        args.out / "operations.tsv",
        ["path", "table_type", "table", "member", "expression"],
        operations,
    )
    metadata = {
        "kernel": str(root),
        "git_head": git_info(root, "rev-parse", "HEAD"),
        "git_describe": git_info(root, "describe", "--always", "--dirty"),
        "roots": ROOTS,
        "source_files": files,
        "counts": {
            "ioctl_candidate_definitions": len(ioctl_rows),
            "review_definitions": len(review_rows),
            "operation_members": len(operations),
            "operation_tables": len({tuple(row[:3]) for row in operations}),
        },
        "limitations": [
            "Lexical source index, not a C parser or runtime support probe.",
            "All #if branches/architectures retained; no config/include-scope resolution.",
            "Aliases, encoder wrappers and parameterized families are also candidates.",
            "Review definitions include flags, capabilities and unrelated constants.",
            "No evaluated ioctl numbers, type layouts, call graph or FD-to-command mapping.",
            "Macro-generated/dynamic operation tables, Rust and out-of-tree code not covered.",
            "tools, samples, generated directories and symlinks not scanned.",
            "git_describe dirty marker does not account for untracked source files.",
        ],
    }
    (args.out / "metadata.json").write_text(
        json.dumps(metadata, indent=2, ensure_ascii=False) + "\n", encoding="utf-8"
    )
    print(json.dumps(metadata, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
