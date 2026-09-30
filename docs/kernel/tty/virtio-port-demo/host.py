#!/usr/bin/env python3
"""Host side: relay an interactive Bash shell over vport or HVC0."""

import argparse
import base64
import os
import select
import socket
import sys
import termios
import tty

from wire import Channel, write_all


def relay_hvc(path: str) -> int:
    if not os.isatty(0):
        raise ValueError("HVC mode needs a local terminal")
    with open(path, "rb+", buffering=0) as backend:
        backend_fd = backend.fileno()
        local_settings = termios.tcgetattr(0)
        backend_settings = termios.tcgetattr(backend_fd)
        try:
            print(
                f"Connected to guest /dev/hvc0 via {path}; Ctrl+] disconnects.",
                file=sys.stderr,
            )
            tty.setraw(0, termios.TCSANOW)
            tty.setraw(backend_fd, termios.TCSANOW)
            while True:
                readable, _, _ = select.select([0, backend_fd], [], [])
                if backend_fd in readable:
                    data = os.read(backend_fd, 16384)
                    if not data:
                        return 0
                    write_all(1, data)
                if 0 in readable:
                    data = os.read(0, 16384)
                    if not data or b"\x1d" in data:
                        return 0
                    write_all(backend_fd, data)
        finally:
            termios.tcsetattr(0, termios.TCSANOW, local_settings)
            termios.tcsetattr(backend_fd, termios.TCSANOW, backend_settings)


def relay_vport(path: str) -> int:
    if not os.isatty(0):
        raise ValueError("virtio-port mode needs a local terminal")
    with socket.socket(socket.AF_UNIX, socket.SOCK_STREAM) as client:
        client.connect(path)
        channel = Channel(client.fileno())
        size = os.get_terminal_size()
        channel.send("shell", rows=size.lines, cols=size.columns)
        settings = termios.tcgetattr(0)
        try:
            print(
                "Connected; Ctrl+] disconnects. Ctrl+C goes to guest Bash.",
                file=sys.stderr,
            )
            tty.setraw(0, termios.TCSANOW)
            ready = False
            while True:
                inputs = [channel.fd, 0] if ready else [channel.fd]
                readable, _, _ = select.select(inputs, [], [])
                if channel.fd in readable:
                    for message in channel.receive():
                        if message["op"] == "ready":
                            ready = True
                        elif message["op"] == "data":
                            write_all(
                                1, base64.b64decode(message["data"], validate=True)
                            )
                        elif message["op"] == "exit":
                            status = int(message["status"])
                            return min(255, status if status >= 0 else 128 - status)
                if 0 in readable:
                    data = os.read(0, 16384)
                    if not data or b"\x1d" in data:
                        channel.send("close")
                        return 0
                    channel.send("data", data=base64.b64encode(data).decode())
        finally:
            termios.tcsetattr(0, termios.TCSANOW, settings)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    transports = parser.add_mutually_exclusive_group()
    transports.add_argument(
        "--socket",
        default="/home/martins3/data/hack/vm/virtme-29/t/vport.sock",
        help="QEMU virtserialport socket (default: %(default)s)",
    )
    transports.add_argument(
        "--hvc",
        metavar="HOST_PTY",
        help="QEMU host PTY backing guest /dev/hvc0",
    )
    args = parser.parse_args()
    try:
        if args.hvc:
            return relay_hvc(args.hvc)
        return relay_vport(args.socket)
    except ValueError as error:
        parser.error(str(error))


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (EOFError, BrokenPipeError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
