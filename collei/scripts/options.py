from __future__ import annotations

import os
import re
import tempfile
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path

from errors import ColleiError

_KEY = re.compile(r"[A-Za-z0-9_.-]+")
_SECTION = re.compile(r"\[([^]]+)]")


@dataclass(frozen=True)
class _Entry:
    name: str
    value: str | None
    start: int
    end: int


def _effective_value(lines: list[str]) -> str | None:
    values = [
        line.rstrip()
        for line in lines
        if line.strip() and not line.lstrip().startswith("#")
    ]
    return "\n".join(values) or None


def read_legacy_option(path: Path) -> tuple[str | None, tuple[str, ...]]:
    """Read an opt/ file and retain its full-line comments for migration."""
    lines = path.read_text().splitlines()
    comments = tuple(line.strip() for line in lines if line.lstrip().startswith("#"))
    return _effective_value(lines), comments


class IniConfig:
    """A single-section INI file with comment-preserving targeted writes."""

    def __init__(self, path: Path, section: str) -> None:
        self.path = path
        self.section = section

    @classmethod
    def create(
        cls,
        path: Path,
        section: str,
        values: Mapping[str, str | None],
        *,
        comments: Mapping[str, tuple[str, ...]] | None = None,
    ) -> IniConfig:
        config = cls(path, section)
        config._validate_names(values)
        lines = [f"[{section}]\n"]
        for name, value in values.items():
            if comments:
                for comment in comments.get(name, ()):
                    marker = (
                        comment if comment.startswith(("#", ";")) else f"# {comment}"
                    )
                    lines.append(f"{marker}\n")
            if value is None:
                continue
            lines.extend(config._render(name, value))
        config._atomic_write(lines)
        config.validate()
        return config

    def validate(self) -> None:
        self._read()

    def get(self, name: str) -> str | None:
        self._validate_name(name)
        _, entries = self._read()
        entry = entries.get(name)
        return entry.value if entry else None

    def require(self, name: str) -> str:
        value = self.get(name)
        if value is None:
            raise ColleiError(
                f"{self.path} [{self.section}] {name} is missing or invalid"
            )
        return value

    def integer(self, name: str, default: int | None = None) -> int:
        value = self.get(name)
        if value is None:
            if default is None:
                raise ColleiError(
                    f"{self.path} [{self.section}] {name} is missing or invalid"
                )
            return default
        try:
            return int(value)
        except ValueError as error:
            raise ColleiError(
                f"{self.path} [{self.section}] {name} must be an integer: {value}"
            ) from error

    def enabled(self, name: str) -> bool:
        return self.get(name) is not None

    def enabled_names(self) -> frozenset[str]:
        _, entries = self._read()
        return frozenset(
            name for name, entry in entries.items() if entry.value is not None
        )

    def set(self, name: str, value: str) -> None:
        self.set_many({name: value})

    def set_many(self, values: Mapping[str, str]) -> None:
        self._validate_names(values)
        lines, entries = self._read()
        replacements: list[tuple[int, int, list[str]]] = []
        additions: list[str] = []
        for name, value in values.items():
            rendered = self._render(name, value)
            entry = entries.get(name)
            if entry:
                replacements.append((entry.start, entry.end, rendered))
            else:
                additions.extend(rendered)
        for start, end, rendered in sorted(replacements, reverse=True):
            lines[start:end] = rendered
        if additions:
            if lines and lines[-1].strip():
                lines.append("\n")
            lines.extend(additions)
        self._atomic_write(lines)

    def append(self, name: str, value: str) -> None:
        current = self.get(name)
        self.set(name, value if current is None else f"{current}\n{value}")

    def remove(self, name: str) -> None:
        self._validate_name(name)
        lines, entries = self._read()
        entry = entries.get(name)
        if entry is None:
            return
        del lines[entry.start : entry.end]
        self._atomic_write(lines)

    def _read(self) -> tuple[list[str], dict[str, _Entry]]:
        if not self.path.is_file():
            raise ColleiError(f"config not found: {self.path}")
        try:
            lines = self.path.read_text().splitlines(keepends=True)
        except UnicodeDecodeError as error:
            raise ColleiError(f"config is not UTF-8: {self.path}") from error
        entries: dict[str, _Entry] = {}
        found_section = False
        current_name: str | None = None
        current_start = 0
        current_values: list[str] = []

        def finish(end: int) -> None:
            nonlocal current_name, current_values
            if current_name is None:
                return
            entries[current_name] = _Entry(
                current_name,
                "\n".join(current_values) or None,
                current_start,
                end,
            )
            current_name = None
            current_values = []

        for index, raw in enumerate(lines):
            line = raw.rstrip("\r\n")
            stripped = line.strip()
            section = _SECTION.fullmatch(stripped)
            if section:
                finish(index)
                if found_section or section.group(1) != self.section:
                    raise ColleiError(f"{self.path} must contain only [{self.section}]")
                found_section = True
                continue
            if not found_section:
                if stripped and not stripped.startswith(("#", ";")):
                    raise ColleiError(
                        f"invalid content before [{self.section}] in {self.path}"
                    )
                continue
            if line[:1].isspace():
                if current_name is None:
                    if stripped and not stripped.startswith(("#", ";")):
                        raise ColleiError(f"orphan continuation in {self.path}: {line}")
                    continue
                if line.startswith("    "):
                    current_values.append(line[4:].rstrip())
                elif line.startswith("\t"):
                    current_values.append(line[1:].rstrip())
                else:
                    raise ColleiError(
                        f"continuation must use four spaces or one tab in {self.path}"
                    )
                continue
            finish(index)
            if not stripped or stripped.startswith(("#", ";")):
                continue
            if "=" not in line:
                raise ColleiError(f"invalid option in {self.path}: {line}")
            name, first = line.split("=", 1)
            name = name.strip()
            self._validate_name(name)
            if name in entries:
                raise ColleiError(f"duplicate option in {self.path}: {name}")
            current_name = name
            current_start = index
            first = first[1:] if first.startswith(" ") else first
            current_values = [first.rstrip()] if first else []
        finish(len(lines))
        if not found_section:
            raise ColleiError(f"missing [{self.section}] in {self.path}")
        return lines, entries

    def _render(self, name: str, value: str) -> list[str]:
        self._validate_name(name)
        value = value.rstrip("\r\n")
        if "\n" not in value and not value[:1].isspace():
            return [f"{name} = {value}\n"]
        lines = [f"{name} =\n"]
        lines.extend(f"    {line}\n" for line in value.split("\n"))
        return lines

    def _atomic_write(self, lines: list[str]) -> None:
        self.path.parent.mkdir(parents=True, exist_ok=True)
        previous_mode = (
            self.path.stat().st_mode & 0o777 if self.path.exists() else 0o644
        )
        descriptor, temporary = tempfile.mkstemp(
            prefix=f".{self.path.name}.", dir=self.path.parent, text=True
        )
        temporary_path = Path(temporary)
        try:
            with os.fdopen(descriptor, "w") as output:
                output.writelines(lines)
                output.flush()
                os.fsync(output.fileno())
            temporary_path.chmod(previous_mode)
            os.replace(temporary_path, self.path)
        finally:
            temporary_path.unlink(missing_ok=True)

    @staticmethod
    def _validate_name(name: str) -> None:
        if _KEY.fullmatch(name) is None:
            raise ColleiError(f"invalid INI option name: {name}")

    @classmethod
    def _validate_names(cls, values: Mapping[str, object]) -> None:
        for name in values:
            cls._validate_name(name)
