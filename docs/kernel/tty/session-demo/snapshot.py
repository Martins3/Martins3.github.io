#!/usr/bin/env python3
"""采集一次 ps，按 session → 进程组 → 进程展示控制终端关系。"""

import subprocess
from dataclasses import dataclass
from datetime import datetime


@dataclass
class Process:
    user: str
    pid: int  # 进程 ID
    ppid: int  # 父进程 ID；父子关系可以跨 session
    sid: int  # session ID
    pgid: int  # 进程组 ID
    tpgid: int  # 控制终端当前的前台进程组 ID；无控制终端时通常为 -1
    tty: str  # 控制终端名称；? 表示没有
    stat: str  # 进程状态；s 表示 session leader，+ 表示属于前台进程组
    command: str  # 命令名，不包含命令行参数


def read_processes() -> list[Process]:
    """只采集一次，让汇总和详细结构使用同一份数据。"""
    # -e：所有可见进程；-o：指定列；列名后的 =：省略表头。
    # ps 读取各进程信息有先后，因此仍不是内核提供的原子快照。
    result = subprocess.run(
        [
            "ps",
            "-eo",
            "user=,pid=,ppid=,sid=,pgid=,tpgid=,tty=,stat=,comm=",
            "--sort=sid,pgid,pid",
        ],
        check=True,
        capture_output=True,
        text=True,
    )

    processes = []
    for line in result.stdout.splitlines():
        # 只切分前 8 个分隔处，保留命令名内部的空格，例如 tmux: server。
        user, pid, ppid, sid, pgid, tpgid, tty, stat, command = line.split(maxsplit=8)
        processes.append(
            Process(
                user=user,
                pid=int(pid),
                ppid=int(ppid),
                sid=int(sid),
                pgid=int(pgid),
                tpgid=int(tpgid),
                tty=tty,
                stat=stat,
                command=command,
            )
        )
    return processes


def print_summary(sessions: dict[int, list[Process]], kernel_tasks: int) -> None:
    """每个 session 输出一行：进程数、进程组数、控制终端和 leader。"""
    attached = 0
    for members in sessions.values():
        if any(process.tty != "?" for process in members):
            attached += 1

    print(
        f"Sessions (SID != 0): {len(sessions)}; with controlling tty: {attached}; "
        f"without: {len(sessions) - attached}"
    )
    print(f"SID 0 processes (kernel tasks, separate): {kernel_tasks}\n")
    print(f"{'SID':>9} {'PROCS':>6} {'GROUPS':>6} {'TTY':<10} LEADER")

    for sid, members in sorted(sessions.items()):
        # set 去重：同一 PGID 下即使有多个进程，也只计作一个进程组。
        group_ids = {process.pgid for process in members}
        tty = "?"
        leader = "[leader absent from snapshot]"
        for process in members:
            if process.tty != "?":
                tty = process.tty
            # PID == SID 的进程是 session leader。
            # leader 可能已退出，但 session 内仍有其他进程。
            if process.pid == sid:
                leader = process.command
        print(f"{sid:>9} {len(members):>6} {len(group_ids):>6} {tty:<10} {leader}")


def print_terminal_tree(sessions: dict[int, list[Process]]) -> None:
    """只展开有控制终端的记录，缩进表示 SID → PGID → PID。"""
    print("\nSessions with a controlling terminal, grouped by SID and PGID:")
    for sid, members in sorted(sessions.items()):
        terminal_members = [process for process in members if process.tty != "?"]
        if not terminal_members:
            continue

        terminal = terminal_members[0]
        print(f"\nSID {sid} / TTY {terminal.tty} / foreground PGID {terminal.tpgid}")
        groups: dict[int, list[Process]] = {}
        for process in terminal_members:
            groups.setdefault(process.pgid, []).append(process)

        for pgid, group_members in sorted(groups.items()):
            # 前后台按进程组判断：比较 PGID 和 TPGID，而不是 PID 和 TPGID。
            foreground = pgid == group_members[0].tpgid
            state = "foreground" if foreground else "background"
            print(f"  PGID {pgid} [{state}]")
            for process in sorted(group_members, key=lambda process: process.pid):
                print(
                    f"    PID {process.pid:<8} PPID {process.ppid:<8} "
                    f"{process.user:<10} {process.stat:<5} {process.command}"
                )


def main() -> None:
    processes = read_processes()
    sessions: dict[int, list[Process]] = {}
    kernel_tasks = 0
    for process in processes:
        # SID 0 的内核任务单独计数，不混入普通 session 的统计。
        if process.sid == 0:
            kernel_tasks += 1
        else:
            # 每个 SID 对应一个进程列表；首次遇到时创建空列表，再添加成员。
            sessions.setdefault(process.sid, []).append(process)

    print(datetime.now().astimezone().isoformat(timespec="seconds"))
    print("SID > PGID > process snapshot (command names only; no command arguments)")
    print_summary(sessions, kernel_tasks)
    print_terminal_tree(sessions)


if __name__ == "__main__":
    main()
