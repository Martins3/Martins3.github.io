#!/usr/bin/env drgn
"""按进程匿名页和 tmpfs/shmem inode 分析 swap 占用（只读）。

Usage:
    sudo drgn -k ./swap_usage.py
    sudo drgn -k ./swap_usage.py --top 50
    sudo drgn -k ./swap_usage.py --all --no-holders
    drgn -c vmcore -s vmlinux ./swap_usage.py

需要提供 task_rss 和 linux.swap helpers 的 drgn（本机使用 0.2.0）。
进程按 mm 去重，MM_SWAPENTS 不包含 tmpfs/shmem 文件中的 swap entries；
遍历所有 tmpfs superblock 的 s_inodes，覆盖 memfd、SysV SHM、共享匿名映射，
以及没有打开者的普通 tmpfs 文件。仅对显示的 inode 查找 fd/mmap 持有者。

这不是 swap slot 的精确归属审计：fork 后不同 mm 可引用同一匿名 swap slot，
因此匿名合计可能重复。SwapCached 与上述归属有重叠，不能直接相加。
zram 的 USED 是未压缩逻辑页大小，不是压缩后 RAM 占用；zswap 也不会额外
增加一份 swap 配额。差额不等于泄漏。活系统无锁遍历，各计数不在同一时刻，
读失败会明确报告；vmcore 中缺失的页也可能导致不完整结果。
"""

import argparse
from collections import Counter
from dataclasses import dataclass, field
from pathlib import PurePosixPath

from drgn import FaultError, Program, container_of
from drgn.helpers.linux.fs import d_path, for_each_file
from drgn.helpers.linux.list import hlist_for_each_entry, list_for_each_entry
from drgn.helpers.linux.mm import for_each_vma, task_rss
from drgn.helpers.linux.pid import for_each_task
from drgn.helpers.linux.swap import (
    for_each_swap_info,
    swap_file_path,
    swap_total_usage,
    swap_usage_in_pages,
    total_swapcache_pages,
)

READ_ERRORS = (FaultError, LookupError, AttributeError, ValueError)
prog: Program  # Injected by the drgn launcher.


@dataclass
class Process:
    mm: int
    swapped: int
    owners: set[str] = field(default_factory=set)


@dataclass
class Shmem:
    inode: int
    ino: int
    sb: str
    swapped: int
    resident: int
    size: int
    path: str
    holders: set[str] = field(default_factory=set)


class Warnings:
    def __init__(self):
        self.counts = Counter()
        self.examples = {}

    def add(self, context, error):
        self.counts[context] += 1
        self.examples.setdefault(context, str(error).splitlines()[0])

    def report(self):
        if self.counts:
            print("\nWARNING: 读取不完整，以下项目不能视为零占用：")
            for context, count in self.counts.items():
                print(f"  {context}: {count} failure(s); {self.examples[context]}")


def human_size(size):
    value = float(size)
    for unit in ("B", "KiB", "MiB", "GiB", "TiB", "PiB"):
        if abs(value) < 1024 or unit == "PiB":
            return f"{value:.2f} {unit}"
        value /= 1024
    raise AssertionError("unreachable")


def c_string(obj):
    # repr() at display sites prevents control characters in names affecting the terminal.
    return obj.string_().decode("utf-8", errors="replace")


def owner(task):
    return f"{int(task.tgid)}:{c_string(task.group_leader.comm)!r}"


def collect_processes(tasks, warnings):
    processes = {}
    for task in tasks:
        try:
            mm = int(task.mm)
            if not mm:
                continue
            if mm not in processes:
                processes[mm] = Process(mm, max(0, task_rss(task).swap))
            processes[mm].owners.add(owner(task))
        except READ_ERRORS as error:
            warnings.add("task MM_SWAPENTS", error)
    return sorted(processes.values(), key=lambda entry: entry.swapped, reverse=True)


def inode_path(inode):
    # d_path() reports only '[tmpfs]' for dynamic dentry names (e.g. memfd).
    # Preserve the actual name in those cases, including unlinked memfds.
    for member in ("d_alias", "d_u.d_alias"):
        try:
            aliases = hlist_for_each_entry("struct dentry", inode.i_dentry.address_of_(), member)
            dentry = next(aliases, None)
        except LookupError:
            continue
        if dentry is None:
            return "(no dentry)"
        name = c_string(dentry.d_name.name)
        try:
            path = d_path(dentry).decode("utf-8", errors="replace")
        except (ValueError, LookupError):
            path = name
        if not path.startswith("/"):
            path = name
        if int(inode.i_nlink) == 0:
            path += " (unlinked)"
        return path
    raise LookupError("no supported dentry alias member")


def collect_shmem(warnings):
    entries = {}
    scanned = 0
    try:
        for sb in list_for_each_entry("struct super_block", prog["super_blocks"].address_of_(), "s_list"):
            try:
                if c_string(sb.s_type.name) != "tmpfs":
                    continue
                for inode in list_for_each_entry("struct inode", sb.s_inodes.address_of_(), "i_sb_list"):
                    try:
                        scanned += 1
                        # Directories/symlinks cannot carry swapped file data.
                        if int(inode.i_mode) & 0o170000 != 0o100000:
                            continue
                        swapped = int(container_of(inode, "struct shmem_inode_info", "vfs_inode").swapped)
                        if swapped <= 0:
                            continue
                        entry = Shmem(
                            int(inode),
                            int(inode.i_ino),
                            c_string(sb.s_id),
                            swapped,
                            int(inode.i_mapping.nrpages),
                            int(inode.i_size),
                            "(path unavailable)",
                        )
                        entries[entry.inode] = entry
                        try:
                            entry.path = inode_path(inode)
                        except READ_ERRORS as error:
                            warnings.add("shmem path", error)
                    except READ_ERRORS as error:
                        warnings.add("shmem inode", error)
            except READ_ERRORS as error:
                warnings.add("tmpfs superblock/inode walk", error)
    except READ_ERRORS as error:
        warnings.add("superblock walk", error)
    return scanned, sorted(entries.values(), key=lambda entry: entry.swapped, reverse=True)


def collect_holders(tasks, entries, warnings):
    targets = {entry.inode: entry for entry in entries}
    if not targets:
        return
    # Cache by actual files_struct/mm, rather than assuming all threads share them.
    fd_cache = {}
    mm_cache = {}
    for task in tasks:
        try:
            label = owner(task)
            files = int(task.files)
            if files:
                if files not in fd_cache:
                    hits = fd_cache[files] = set()
                    try:
                        for fd, file in for_each_file(task):
                            try:
                                if not file:
                                    continue
                                key = int(file.f_inode)
                                if key in targets:
                                    hits.add((key, int(fd)))
                            except READ_ERRORS as error:
                                warnings.add("fd entry", error)
                    except READ_ERRORS as error:
                        warnings.add("fd walk", error)
                for key, fd in fd_cache[files]:
                    targets[key].holders.add(f"{label}:fd={fd}")
            mm = int(task.mm)
            if mm:
                if mm not in mm_cache:
                    mapped = mm_cache[mm] = set()
                    try:
                        for vma in for_each_vma(task.mm):
                            file = vma.vm_file.read_()
                            if file:
                                key = int(file.f_inode)
                                if key in targets:
                                    mapped.add(key)
                    except READ_ERRORS as error:
                        warnings.add("VMA walk", error)
                for key in mm_cache[mm]:
                    targets[key].holders.add(f"{label}:mmap")
        except READ_ERRORS as error:
            warnings.add("holder task", error)


def limited_labels(labels, limit=8):
    ordered = sorted(labels)
    result = " ".join(ordered[:limit])
    if len(ordered) > limit:
        result += f" ... (+{len(ordered) - limit} refs)"
    return result


def shmem_group(path):
    if path.startswith("memfd:"):
        return path.removesuffix(" (unlinked)")
    if path.startswith("SYSV"):
        return "SysV shared memory"
    if path.removesuffix(" (unlinked)").lstrip("/") == "dev/zero":
        return "shared anonymous mappings"
    if path.startswith("/"):
        # Aggregate the first two directory components, never a filename.
        parent = PurePosixPath(path).parent
        return str(PurePosixPath(*parent.parts[:3]))
    return "other / path unavailable"


def show_devices(page_size, warnings):
    print("=== Swap devices (logical, uncompressed bytes) ===")
    print(f"{'USED':>12} {'TOTAL':>12} {'PRIO':>6} PATH")
    try:
        for si in for_each_swap_info(prog):
            try:
                used = swap_usage_in_pages(si)
                print(
                    f"{human_size(used * page_size):>12} {human_size(int(si.pages) * page_size):>12} "
                    f"{int(si.prio):6} {swap_file_path(si).decode('utf-8', errors='replace')!r}"
                )
            except READ_ERRORS as error:
                warnings.add("swap device", error)
    except READ_ERRORS as error:
        warnings.add("swap device walk", error)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--top", type=int, default=30, help="每张排行显示多少项（默认 30，0 只看总量）")
    parser.add_argument("--all", action="store_true", help="显示所有非零占用项")
    parser.add_argument("--no-holders", action="store_true", help="跳过 fd/VMA 持有者扫描")
    args = parser.parse_args()
    if args.top < 0:
        parser.error("--top must be >= 0")
    return args


def main():
    args = parse_args()
    warnings = Warnings()
    page_size = int(prog["PAGE_SIZE"])
    # A missing global counter is fatal: do not misreport it as zero usage.
    usage = swap_total_usage(prog)
    show_devices(page_size, warnings)
    tasks = []
    try:
        tasks.extend(for_each_task(prog))
    except READ_ERRORS as error:
        warnings.add("task list", error)
    processes = collect_processes(tasks, warnings)
    scanned, shmem = collect_shmem(warnings)
    shown_shmem = shmem if args.all else shmem[: args.top]
    if not args.no_holders:
        collect_holders(tasks, shown_shmem, warnings)

    anonymous = sum(entry.swapped for entry in processes)
    shared = sum(entry.swapped for entry in shmem)
    print("\n=== Attribution summary ===")
    for label, pages in (
        ("Swap used (global)", usage.used_pages),
        ("Anonymous swap references (unique mm)", anonymous),
        ("Shmem/tmpfs swap entries (unique inode)", shared),
        ("Used - anonymous references - shmem", usage.used_pages - anonymous - shared),
    ):
        print(f"{label:43} {human_size(pages * page_size):>12} ({pages} pages)")
    try:
        cached = total_swapcache_pages(prog)
        print(f"{'SwapCached (overlaps attribution)':43} {human_size(cached * page_size):>12} ({cached} pages)")
    except READ_ERRORS as error:
        warnings.add("SwapCached", error)
    print(f"Scanned: {len(processes)} unique mm, {scanned} tmpfs inodes; {len(shmem)} swapped inodes")
    print("匿名项跨 mm 可能共享同一 slot；SwapCached 也有重叠。差额不是精确的未归属量或泄漏量。")

    print("\n=== Shmem groups (path prefixes: at most two directory components) ===")
    groups = Counter()
    counts = Counter()
    for entry in shmem:
        group = shmem_group(entry.path)
        groups[group] += entry.swapped
        counts[group] += 1
    print(f"{'SWAP':>12} {'INODES':>8} GROUP")
    for group, pages in groups.most_common(None if args.all else args.top):
        print(f"{human_size(pages * page_size):>12} {counts[group]:8} {group!r}")

    print("\n=== Anonymous swap by mm (excludes shmem/tmpfs) ===")
    print(f"{'SWAP':>12} {'MM':>18} TGID:COMM (tasks sharing mm)")
    nonzero = [entry for entry in processes if entry.swapped]
    for entry in nonzero if args.all else nonzero[: args.top]:
        print(f"{human_size(entry.swapped * page_size):>12} {entry.mm:#18x} {limited_labels(entry.owners)}")

    print("\n=== Shmem/tmpfs by inode (holders are references, not exclusive owners) ===")
    print(f"{'SWAP':>12} {'RESIDENT':>12} {'SIZE':>12} INODE / SB / INO / PATH")
    for entry in shown_shmem:
        print(
            f"{human_size(entry.swapped * page_size):>12} {human_size(entry.resident * page_size):>12} "
            f"{human_size(entry.size):>12} {entry.inode:#x} / {entry.sb!r} / {entry.ino} / {entry.path!r}"
        )
        if not args.no_holders:
            print(f"    {limited_labels(entry.holders) or '(no fd/mmap holder found)'}")
    if not args.all:
        print(f"\nShowing top {args.top} per table; totals include all entries. Use --all for the full list.")
    print("路径可能来自其他 mount namespace；无持有者的具名 tmpfs 文件仍可占用 swap。")
    warnings.report()


if __name__ == "__main__":
    main()
