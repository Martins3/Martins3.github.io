from __future__ import annotations

import json
import socket
from collections.abc import Iterable
from pathlib import Path
from typing import Any

from errors import ColleiError


def hmp_commands(path: Path, commands: Iterable[str], timeout: float = 0.2) -> str:
    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    client.settimeout(timeout)
    try:
        client.connect(str(path))
        try:
            client.recv(65536)
        except TimeoutError:
            pass
        for command in commands:
            client.sendall(f"{command}\n".encode())
        chunks: list[bytes] = []
        while True:
            try:
                data = client.recv(65536)
            except TimeoutError:
                break
            if not data:
                break
            chunks.append(data)
        return b"".join(chunks).decode(errors="replace")
    except OSError as error:
        raise ColleiError(f"HMP command failed via {path}: {error}") from error
    finally:
        client.close()


def hmp_command(path: Path, command: str, timeout: float = 0.2) -> str:
    return hmp_commands(path, (command,), timeout)


class QmpClient:
    def __init__(self, path: Path, timeout: float = 2.0) -> None:
        self.path = path
        self.socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.socket.settimeout(timeout)
        self.buffer = ""

    def __enter__(self) -> QmpClient:
        try:
            self.socket.connect(str(self.path))
            self._read()
            self._send({"execute": "qmp_capabilities"})
            self._response()
            return self
        except OSError as error:
            self.socket.close()
            raise ColleiError(
                f"QMP connection failed via {self.path}: {error}"
            ) from error

    def __exit__(self, *_: object) -> None:
        self.socket.close()

    def _send(self, message: dict[str, Any]) -> None:
        self.socket.sendall(
            (json.dumps(message, separators=(",", ":")) + "\n").encode()
        )

    def _read(self) -> dict[str, Any]:
        decoder = json.JSONDecoder()
        while True:
            stripped = self.buffer.lstrip()
            try:
                value, end = decoder.raw_decode(stripped)
            except json.JSONDecodeError:
                data = self.socket.recv(65536)
                if not data:
                    raise ColleiError(f"QMP socket closed: {self.path}")
                self.buffer += data.decode()
                continue
            self.buffer = stripped[end:]
            if not isinstance(value, dict):
                raise ColleiError(f"invalid QMP response: {value!r}")
            return value

    def _response(self) -> Any:
        while True:
            response = self._read()
            if "event" in response:
                continue
            if "error" in response:
                raise ColleiError(f"QMP error: {response['error']}")
            if "return" in response:
                return response["return"]

    def execute(self, command: str, arguments: dict[str, Any] | None = None) -> Any:
        request: dict[str, Any] = {"execute": command}
        if arguments:
            request["arguments"] = arguments
        self._send(request)
        return self._response()
