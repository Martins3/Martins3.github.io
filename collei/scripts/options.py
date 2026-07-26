from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

from errors import ColleiError


def _option_value(path: Path) -> str | None:
    lines = [
        line.rstrip()
        for line in path.read_text().splitlines()
        if line.strip() and not line.lstrip().startswith("#")
    ]
    return "\n".join(lines) or None


@dataclass(frozen=True)
class OptionDirectory:
    """Read collei's one-file-per-option configuration format."""

    path: Path

    def get(self, name: str) -> str | None:
        option = self.path / name
        if not option.is_file():
            return None
        return _option_value(option)

    def require(self, name: str) -> str:
        value = self.get(name)
        if value is None:
            raise ColleiError(f"{self.path / name} is missing or invalid")
        return value

    def integer(self, name: str, default: int | None = None) -> int:
        value = self.get(name)
        if value is None:
            if default is None:
                raise ColleiError(f"{self.path / name} is missing or invalid")
            return default
        try:
            return int(value)
        except ValueError as error:
            raise ColleiError(
                f"{self.path / name} must be an integer: {value}"
            ) from error

    def enabled(self, name: str) -> bool:
        return self.get(name) is not None

    def enabled_names(self) -> frozenset[str]:
        if not self.path.is_dir():
            return frozenset()
        return frozenset(
            option.name
            for option in self.path.iterdir()
            if option.is_file() and _option_value(option) is not None
        )
