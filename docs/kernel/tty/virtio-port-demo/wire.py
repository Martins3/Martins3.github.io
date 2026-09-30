"""Newline-delimited JSON frames over a byte stream; binary data uses base64."""

import json
import os
from typing import Any


def write_all(fd: int, data: bytes) -> None:
    while data:
        written = os.write(fd, data)
        if written == 0:
            raise BrokenPipeError("zero-length write")
        data = data[written:]


class Channel:
    def __init__(self, fd: int):
        self.fd = fd
        self.buffer = b""

    def send(self, op: str, **fields: Any) -> None:
        write_all(self.fd, json.dumps({"op": op, **fields}).encode() + b"\n")

    def receive(self) -> list[dict[str, Any]]:
        data = os.read(self.fd, 65536)
        if not data:
            raise EOFError("peer disconnected")
        self.buffer += data
        if len(self.buffer) > 1024 * 1024:
            raise ValueError("frame too large")
        messages = []
        while b"\n" in self.buffer:
            line, self.buffer = self.buffer.split(b"\n", 1)
            message = json.loads(line)
            if not isinstance(message, dict):
                raise ValueError("expected a JSON object")
            messages.append(message)
        return messages
