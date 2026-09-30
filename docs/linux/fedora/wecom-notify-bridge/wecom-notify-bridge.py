from __future__ import annotations

import argparse
import ctypes
import os
import subprocess
import sys
import time
from ctypes import wintypes
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Final


def require_windows() -> None:
    if sys.platform != "win32":
        raise SystemExit("wecom-notify-bridge.py only runs on Windows")


require_windows()


@dataclass(frozen=True)
class WindowInfo:
    handle: int
    process_id: int
    process_name: str
    class_name: str
    title: str


class LastInputInfo(ctypes.Structure):
    _fields_ = (("size", wintypes.UINT), ("time", wintypes.DWORD))


WECOM_PROCESS_NAMES: Final = frozenset({"wxwork", "wxworkweb", "wechatappex"})
IGNORED_WINDOW_CLASSES: Final = frozenset(
    {
        "TitleBarWindow",
        "PerryShadowWnd",
        "Tencent.WXWork.WedocHostWindow",
        "WeWorkWindow",
    }
)
PROCESS_QUERY_LIMITED_INFORMATION: Final = 0x1000
MAX_PATH_LENGTH: Final = 32768
NOTIFICATION_INTERVAL_SECONDS: Final = 15.0
ACTIVE_IDLE_SECONDS: Final = 10.0
POLL_INTERVAL_SECONDS: Final = 0.5
CREATE_NO_WINDOW: Final = 0x08000000

WIN_DLL = getattr(ctypes, "WinDLL")
WIN_FUNCTION_TYPE = getattr(ctypes, "WINFUNCTYPE")
GET_LAST_ERROR = getattr(ctypes, "get_last_error")
WIN_ERROR = getattr(ctypes, "WinError")

USER32 = WIN_DLL("user32", use_last_error=True)
KERNEL32 = WIN_DLL("kernel32", use_last_error=True)

ENUM_WINDOWS_CALLBACK = WIN_FUNCTION_TYPE(wintypes.BOOL, wintypes.HWND, wintypes.LPARAM)

USER32.EnumWindows.argtypes = (ENUM_WINDOWS_CALLBACK, wintypes.LPARAM)
USER32.EnumWindows.restype = wintypes.BOOL
USER32.IsWindowVisible.argtypes = (wintypes.HWND,)
USER32.IsWindowVisible.restype = wintypes.BOOL
USER32.GetWindowTextW.argtypes = (
    wintypes.HWND,
    wintypes.LPWSTR,
    ctypes.c_int,
)
USER32.GetWindowTextW.restype = ctypes.c_int
USER32.GetClassNameW.argtypes = (
    wintypes.HWND,
    wintypes.LPWSTR,
    ctypes.c_int,
)
USER32.GetClassNameW.restype = ctypes.c_int
USER32.GetWindowThreadProcessId.argtypes = (
    wintypes.HWND,
    ctypes.POINTER(wintypes.DWORD),
)
USER32.GetWindowThreadProcessId.restype = wintypes.DWORD
USER32.GetForegroundWindow.argtypes = ()
USER32.GetForegroundWindow.restype = wintypes.HWND
USER32.GetLastInputInfo.argtypes = (ctypes.POINTER(LastInputInfo),)
USER32.GetLastInputInfo.restype = wintypes.BOOL

KERNEL32.OpenProcess.argtypes = (wintypes.DWORD, wintypes.BOOL, wintypes.DWORD)
KERNEL32.OpenProcess.restype = wintypes.HANDLE
KERNEL32.QueryFullProcessImageNameW.argtypes = (
    wintypes.HANDLE,
    wintypes.DWORD,
    wintypes.LPWSTR,
    ctypes.POINTER(wintypes.DWORD),
)
KERNEL32.QueryFullProcessImageNameW.restype = wintypes.BOOL
KERNEL32.CloseHandle.argtypes = (wintypes.HANDLE,)
KERNEL32.CloseHandle.restype = wintypes.BOOL
KERNEL32.GetTickCount.argtypes = ()
KERNEL32.GetTickCount.restype = wintypes.DWORD


def get_process_name(process_id: int) -> str | None:
    process = KERNEL32.OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, False, process_id)
    if not process:
        return None

    try:
        buffer = ctypes.create_unicode_buffer(MAX_PATH_LENGTH)
        size = wintypes.DWORD(len(buffer))
        if not KERNEL32.QueryFullProcessImageNameW(
            process, 0, buffer, ctypes.byref(size)
        ):
            return None
        return Path(buffer.value).stem
    finally:
        KERNEL32.CloseHandle(process)


def get_window_class(handle: int) -> str:
    buffer = ctypes.create_unicode_buffer(256)
    USER32.GetClassNameW(handle, buffer, len(buffer))
    return buffer.value


def get_window_title(handle: int) -> str:
    buffer = ctypes.create_unicode_buffer(512)
    USER32.GetWindowTextW(handle, buffer, len(buffer))
    return buffer.value


def get_wecom_windows(*, include_titles: bool = False) -> list[WindowInfo]:
    windows: list[WindowInfo] = []

    @ENUM_WINDOWS_CALLBACK
    def collect_window(handle: int, _parameter: int) -> bool:
        if not USER32.IsWindowVisible(handle):
            return True

        process_id = wintypes.DWORD()
        USER32.GetWindowThreadProcessId(handle, ctypes.byref(process_id))
        process_name = get_process_name(process_id.value)
        if process_name is None or process_name.casefold() not in WECOM_PROCESS_NAMES:
            return True

        windows.append(
            WindowInfo(
                handle=handle,
                process_id=process_id.value,
                process_name=process_name,
                class_name=get_window_class(handle),
                title=get_window_title(handle) if include_titles else "",
            )
        )
        return True

    if not USER32.EnumWindows(collect_window, 0):
        error = GET_LAST_ERROR()
        if error:
            raise WIN_ERROR(error)
    return windows


def get_signal_window_handles() -> set[int]:
    return {
        window.handle
        for window in get_wecom_windows()
        if window.class_name not in IGNORED_WINDOW_CLASSES
    }


def get_idle_milliseconds() -> int:
    info = LastInputInfo()
    info.size = ctypes.sizeof(info)
    if not USER32.GetLastInputInfo(ctypes.byref(info)):
        return 0xFFFFFFFF
    return (KERNEL32.GetTickCount() - info.time) & 0xFFFFFFFF


def is_wecom_active() -> bool:
    handle = USER32.GetForegroundWindow()
    if not handle:
        return False

    process_id = wintypes.DWORD()
    USER32.GetWindowThreadProcessId(handle, ctypes.byref(process_id))
    process_name = get_process_name(process_id.value)
    return (
        process_name is not None
        and process_name.casefold() in WECOM_PROCESS_NAMES
        and get_idle_milliseconds() < ACTIVE_IDLE_SECONDS * 1000
    )


def append_log(log_file: Path, message: str) -> None:
    timestamp = datetime.now().astimezone().isoformat()
    with log_file.open("a", encoding="utf-8") as stream:
        stream.write(f"{timestamp} {message}\n")


def send_host_notification(state_directory: Path, log_file: Path) -> None:
    key = state_directory / "host_key"
    known_hosts = state_directory / "known_hosts"
    if not key.exists():
        append_log(log_file, "source=window error=missing_key")
        return

    ssh = Path(os.environ["WINDIR"]) / "System32" / "OpenSSH" / "ssh.exe"
    command = (
        str(ssh),
        "-T",
        "-i",
        str(key),
        "-o",
        "BatchMode=yes",
        "-o",
        "LogLevel=ERROR",
        "-o",
        "ConnectTimeout=5",
        "-o",
        "StrictHostKeyChecking=yes",
        "-o",
        f"UserKnownHostsFile={known_hosts}",
        "martins3@10.0.2.2",
    )
    try:
        completed = subprocess.run(
            command,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
            check=False,
            creationflags=CREATE_NO_WINDOW,
        )
    except OSError as error:
        append_log(
            log_file, f"source=window error=ssh_start_failed errno={error.errno}"
        )
        return
    append_log(log_file, f"source=window ssh_exit={completed.returncode}")


def dump_windows(state_directory: Path) -> None:
    dump_file = state_directory / "windows.txt"
    lines = [
        "\t".join(
            (
                str(window.handle),
                str(window.process_id),
                window.process_name,
                window.class_name,
                window.title,
            )
        )
        for window in get_wecom_windows(include_titles=True)
    ]
    output = "\n".join(lines)
    if output:
        output += "\n"
    dump_file.write_text(output, encoding="utf-8")
    print(output, end="")


def monitor(state_directory: Path) -> None:
    state_file = state_directory / "state.txt"
    log_file = state_directory / "bridge.log"
    previous_windows = get_signal_window_handles()
    state_file.write_text(
        f"pid={os.getpid()} started={datetime.now().astimezone().isoformat()} "
        f"windows={len(previous_windows)}\n",
        encoding="ascii",
    )

    last_sent = float("-inf")
    while True:
        current_windows = get_signal_window_handles()
        new_window = bool(current_windows - previous_windows)
        now = time.monotonic()

        if new_window:
            active = is_wecom_active()
            if not active and now - last_sent >= NOTIFICATION_INTERVAL_SECONDS:
                send_host_notification(state_directory, log_file)
                last_sent = now
            elif active:
                append_log(log_file, "source=window suppressed=active")

        previous_windows = current_windows
        time.sleep(POLL_INTERVAL_SECONDS)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--dump-once", action="store_true")
    return parser.parse_args()


def main() -> None:
    arguments = parse_arguments()
    state_directory = Path(os.environ["LOCALAPPDATA"]) / "WeComNotifyBridge"
    state_directory.mkdir(parents=True, exist_ok=True)
    if arguments.dump_once:
        dump_windows(state_directory)
        return
    monitor(state_directory)


if __name__ == "__main__":
    main()
