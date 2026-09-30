#!/usr/bin/env python3
"""Guest side: run Bash over a virtserialport or the HVC0 console."""
import argparse
import base64
import errno
import fcntl
import os
import pty
import pwd
import select
import signal
import socket
import sys
import termios
import time

from wire import Channel, write_all


def reap_shell(pid: int) -> None:
    if os.waitpid(pid, os.WNOHANG)[0]:
        return
    try:
        os.killpg(pid, signal.SIGHUP)
    except ProcessLookupError:
        pass
    deadline = time.monotonic() + 2
    while time.monotonic() < deadline:
        if os.waitpid(pid, os.WNOHANG)[0]:
            return
        time.sleep(0.05)
    try:
        os.killpg(pid, signal.SIGKILL)
    except ProcessLookupError:
        pass
    os.waitpid(pid, 0)


def shell_environment(account: pwd.struct_passwd) -> dict[str, str]:
    return {
        "PATH": "/usr/local/bin:/usr/bin:/bin",
        "HOME": account.pw_dir,
        "USER": account.pw_name,
        "TERM": "xterm",
        "PS1": "vport-demo$ ",
    }


def drop_to_user(account: pwd.struct_passwd) -> None:
    if os.geteuid() == 0:
        os.initgroups(account.pw_name, account.pw_gid)
        os.setgid(account.pw_gid)
        os.setuid(account.pw_uid)
    elif os.geteuid() != account.pw_uid:
        raise PermissionError("cannot switch to requested shell user")


def exec_shell(channel_fd: int, account: pwd.struct_passwd) -> None:
    os.close(channel_fd)
    drop_to_user(account)
    os.chdir("/tmp")
    os.execve(
        "/bin/bash",
        ["bash", "--noprofile", "--norc", "-i"],
        shell_environment(account),
    )


def shell(channel: Channel, user: str, rows: int, cols: int) -> None:
    account = pwd.getpwnam(user)
    pid, master = pty.fork()
    if pid == 0:
        try:
            exec_shell(channel.fd, account)
        except Exception as error:
            print(f"cannot start Bash: {error}", file=sys.stderr, flush=True)
            os._exit(126)
    reaped = False
    try:
        termios.tcsetwinsize(master, (rows, cols))
        channel.send(
            "ready",
            mode="shell",
            transport="pty",
            hostname=socket.gethostname(),
            user=user,
        )
        while True:
            readable, _, _ = select.select([channel.fd, master], [], [])
            if master in readable:
                try:
                    data = os.read(master, 16384)
                except OSError as error:
                    if error.errno != errno.EIO:
                        raise
                    data = b""
                if not data:
                    _, status = os.waitpid(pid, 0)
                    reaped = True
                    channel.send("exit", status=os.waitstatus_to_exitcode(status))
                    return
                channel.send("data", data=base64.b64encode(data).decode())
            if channel.fd in readable:
                for message in channel.receive():
                    if message["op"] == "close":
                        return
                    if message["op"] != "data":
                        raise ValueError("expected terminal data")
                    write_all(master, base64.b64decode(message["data"], validate=True))
    finally:
        os.close(master)
        if not reaped:
            reap_shell(pid)


def pipe_shell(channel: Channel, user: str) -> None:
    account = pwd.getpwnam(user)
    child_stdin, parent_stdin = os.pipe()
    parent_stdout, child_stdout = os.pipe()
    pid = os.fork()
    if pid == 0:
        try:
            os.close(parent_stdin)
            os.close(parent_stdout)
            os.setsid()
            os.dup2(child_stdin, 0)
            os.dup2(child_stdout, 1)
            os.dup2(child_stdout, 2)
            os.close(child_stdin)
            os.close(child_stdout)
            exec_shell(channel.fd, account)
        except Exception as error:
            print(f"cannot start Bash: {error}", file=sys.stderr, flush=True)
            os._exit(126)
    os.close(child_stdin)
    os.close(child_stdout)
    reaped = False
    try:
        channel.send(
            "ready",
            mode="shell",
            transport="pipe",
            hostname=socket.gethostname(),
            user=user,
        )
        while True:
            readable, _, _ = select.select([channel.fd, parent_stdout], [], [])
            if parent_stdout in readable:
                data = os.read(parent_stdout, 16384)
                if not data:
                    _, status = os.waitpid(pid, 0)
                    reaped = True
                    channel.send("exit", status=os.waitstatus_to_exitcode(status))
                    return
                channel.send("data", data=base64.b64encode(data).decode())
            if channel.fd in readable:
                for message in channel.receive():
                    if message["op"] == "close":
                        return
                    if message["op"] != "data":
                        raise ValueError("expected terminal data")
                    write_all(
                        parent_stdin,
                        base64.b64decode(message["data"], validate=True),
                    )
    finally:
        os.close(parent_stdin)
        os.close(parent_stdout)
        if not reaped:
            reap_shell(pid)


def hvc_shell(path: str, user: str) -> None:
    account = pwd.getpwnam(user)
    pid = os.fork()
    if pid == 0:
        try:
            os.setsid()
            fd = os.open(path, os.O_RDWR)
            fcntl.ioctl(fd, termios.TIOCSCTTY, 0)
            os.dup2(fd, 0)
            os.dup2(fd, 1)
            os.dup2(fd, 2)
            if fd > 2:
                os.close(fd)
            drop_to_user(account)
            os.chdir("/tmp")
            os.execve(
                "/bin/bash",
                ["bash", "--noprofile", "--norc", "-i"],
                shell_environment(account),
            )
        except Exception as error:
            print(f"cannot start Bash on {path}: {error}", file=sys.stderr, flush=True)
            os._exit(126)
    reaped = False
    try:
        print(f"READY hvc={path} user={user}", flush=True)
        os.waitpid(pid, 0)
        reaped = True
    finally:
        if not reaped:
            reap_shell(pid)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", default="/dev/virtio-ports/org.qemu.vport.0")
    parser.add_argument("--user", default=pwd.getpwuid(os.getuid()).pw_name)
    parser.add_argument(
        "--mode",
        choices=["pty", "pipe", "hvc"],
        default="pty",
        help="shell transport: PTY (default), plain pipes, or guest /dev/hvc0",
    )
    parser.add_argument("--hvc", default="/dev/hvc0", help="HVC device for --mode hvc")
    args = parser.parse_args()
    if args.mode == "hvc":
        hvc_shell(args.hvc, args.user)
        return
    fd = os.open(args.port, os.O_RDWR | os.O_NOCTTY)
    try:
        channel = Channel(fd)
        print(
            f"READY port={args.port} isatty={os.isatty(fd)} mode={args.mode}",
            flush=True,
        )
        # An unopened host backend returns EOF. Wait for the first request.
        messages = []
        while not messages:
            try:
                messages = channel.receive()
            except EOFError:
                time.sleep(0.1)
        if len(messages) != 1:
            raise ValueError("send one initial request and wait for its reply")
        request = messages[0]
        if request["op"] == "echo":
            try:
                termios.tcgetattr(fd)
                terminal_error = None
            except termios.error as error:
                terminal_error = error.args[0]
            channel.send(
                "echo",
                data=request["data"],
                hostname=socket.gethostname(),
                port_isatty=os.isatty(fd),
                tcgetattr_errno=terminal_error,
            )
        elif request["op"] == "shell":
            if args.mode == "pty":
                shell(channel, args.user, int(request["rows"]), int(request["cols"]))
            else:
                pipe_shell(channel, args.user)
        else:
            raise ValueError("expected echo or shell")
    finally:
        os.close(fd)


if __name__ == "__main__":
    try:
        main()
    except (EOFError, BrokenPipeError):
        print("host disconnected", file=sys.stderr)
