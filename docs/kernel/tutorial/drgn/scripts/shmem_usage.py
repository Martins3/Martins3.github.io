#!/usr/bin/env drgn
"""
分析当前系统 shmem(tmpfs) 内存的占用情况: 谁在用、memfd 是什么、/dev/shm 里有什么。

Usage:
    sudo drgn -c /proc/kcore ./shmem_usage.py
    sudo drgn -c /proc/kcore ./shmem_usage.py --top 80
    sudo drgn -c /proc/kcore ./shmem_usage.py --category memfd,devshm

方法:
    遍历所有 task 的 fd 表 (for_each_file) 和 VMA (for_each_vma),
    找出所有引用 tmpfs/shmem inode 的 struct file, 按 inode 聚合:
      - 常驻页数: inode.i_mapping.nrpages (不含换出到 swap 的部分)
      - swap 页数: shmem_inode_info.swapped (best-effort)
      - 逻辑大小: inode.i_size
    每个 inode 记录持有者: 进程 comm/pid, 以及通过 fd 还是 mmap 引用。

    分类规则 (按 dentry 名和解析出的路径):
      - memfd:   memfd_create() 创建, dentry 名形如 "memfd:<name>"
      - sysv:    SysV shmget() 创建, dentry 名形如 "SYSV<key>"
      - devshm:  POSIX shm_open() 等落在 /dev/shm 下的具名文件
      - anonshm: MAP_SHARED|MAP_ANONYMOUS 共享匿名映射, dentry 名 "dev/zero"
      - tmpfs:   其他 tmpfs 文件 (/run, /tmp 等), 严格说不是 shmem 语义

    与 /proc/meminfo 的 Shmem 对比: Shmem 只统计常驻部分, 且包含已经
    unlink 但仍有页残留的孤儿 inode; 这类 inode 没有任何进程引用,
    本脚本看不到, 可以用 page_cache_usage.py --fs tmpfs 兜底。

注意:
    遍历是无锁的, 结果是活系统的尽力快照。共享文件被多个进程引用时,
    Per-process 统计会把同一个 inode 完整算到每个持有者头上, 不能直接相加。


如果不用 drgn 脚本:

   总量 — /proc/meminfo
   ```
     Shmem:          23486972 kB   # ≈ 22.4 GiB，和 drgn 脚本的 21.7 GiB
   resident 基本吻合
     ShmemHugePages:   927744 kB
   ```

   谁在用 — 按进程，两个接口：

   • /proc/<pid>/smaps_rollup 里的 Pss_Shmem：该进程按比例分摊的 shmem 常驻量。
     全系统所有进程的 Pss_Shmem 之和 ≈ meminfo 的 Shmem，这是最准确的"谁在用"口
     径。比如 photoshop：Pss_Shmem: 654 kB——比 drgn 脚本里的 92.9 MiB 小很多，因为
     Pss 要把共享页平摊给所有持有者进程，而脚本是全额记账。
   • 粗一点可以用 Rss 减去匿名/文件部分，但没这个直接。

   memfd — /proc/<pid>/fd 和 /proc/<pid>/maps，不需要任何特权工具：

   ```
     $ ls -l /proc/3939557/fd | grep memfd
     104 -> /memfd:system.flash0 (deleted)
     105 -> /memfd:system.flash1 (deleted)
     107 -> /memfd:0000:00:04.0/virtio-net-pci.rom (deleted)
   ```

   QEMU 的 guest RAM 就是那根 memfd:memory-backend-memfd fd，/proc/<pid>/maps
   里也能看到对应的映射区间和大小。

   /dev/shm 里有什么：直接 ls -la /dev/shm、du -sh /dev/shm/*；df -h -t tmpfs
   看各 tmpfs 挂载点的总量（/dev/shm 用了 1.6G，/tmp 用了 779M）。

   SysV shm — ipcs -m：每段的 key、bytes、nattch，加 -p 还能看创建者/最后操作的
   pid。比 drgn 脚本按 dentry 名猜 SYSV* 更权威。

   drgn 脚本不可替代的只有三点：每个 inode 的常驻 vs 已换出拆分
   （shmem_inode_info.swapped，procfs 完全不暴露）、已 unlink 且无引用的孤儿
   inode、以及跨进程按 inode 聚合的持有者视图。日常排查"谁吃了 shmem"用
   Pss_Shmem + ipcs -m + ls /dev/shm 就够了。
   Resumed session (session_634c89e0-7625-434d-8e7a-9f317bf360a0).

"""

import argparse
import sys

from drgn.helpers.linux import for_each_task
from drgn.helpers.linux.fs import d_path, for_each_file
from drgn.helpers.linux.mm import for_each_vma

PAGE_SIZE = prog["PAGE_SIZE"].value_()

CATEGORIES = ["memfd", "sysv", "devshm", "anonshm", "tmpfs"]


def obj_value(obj, default=0):
    try:
        value = obj.value_()
        if not isinstance(value, dict):
            return value
    except Exception:
        pass

    try:
        return obj.address_
    except Exception:
        return default


def c_string(obj, default="?"):
    try:
        return obj.string_().decode("utf-8", errors="replace")
    except Exception:
        return default


def human_size(size):
    units = ["B", "KiB", "MiB", "GiB", "TiB", "PiB"]
    value = float(size)

    for unit in units:
        if abs(value) < 1024.0 or unit == units[-1]:
            if unit == "B":
                return f"{int(value)} {unit}"
            return f"{value:.1f} {unit}"
        value /= 1024.0


def file_fs_name(file):
    return c_string(file.f_inode.i_sb.s_type.name, default="?")


def classify(name, path):
    if name.startswith("memfd:"):
        return "memfd"
    if name.startswith("SYSV"):
        return "sysv"
    if name == "dev/zero":
        # MAP_SHARED|MAP_ANONYMOUS 共享匿名映射, 内核内部用 shmem 实现,
        # dentry 名固定为 "dev/zero" (典型用户: postgres 动态共享内存)。
        return "anonshm"
    if path.startswith("/dev/shm/"):
        return "devshm"
    return "tmpfs"


def file_path(file):
    try:
        path = d_path(file.f_path.address_of_()).decode("utf-8", errors="replace")
        if path.startswith("/"):
            return path
    except Exception:
        pass
    # 内部 shmem 挂载 (memfd/SysV) 上的文件 d_path 得不到真实路径,
    # 显示 dentry 名, 形如 "memfd:<name>" / "SYSV<key>".
    return c_string(file.f_path.dentry.d_name.name, default="?")


def shmem_swapped_pages(inode):
    try:
        from drgn import container_of

        info = container_of(inode, "struct shmem_inode_info", "vfs_inode")
        return info.swapped.value_()
    except Exception:
        return 0


def add_ref(inodes, task, file, via):
    inode = file.f_inode
    key = obj_value(inode)
    entry = inodes.get(key)
    if entry is None:
        name = c_string(file.f_path.dentry.d_name.name, default="?")
        path = file_path(file)
        entry = {
            "inode": key,
            "ino": obj_value(inode.i_ino),
            "name": name,
            "path": path,
            "category": classify(name, path),
            "size": obj_value(inode.i_size),
            "pages": obj_value(inode.i_mapping.nrpages),
            "swapped": shmem_swapped_pages(inode),
            "holders": {},
        }
        inodes[key] = entry

    pid = obj_value(task.pid)
    holder = entry["holders"].get(pid)
    if holder is None:
        holder = {"comm": c_string(task.comm), "pid": pid, "fd": [], "mmap": False}
        entry["holders"][pid] = holder

    if via[0] == "fd":
        holder["fd"].append(via[1])
    else:
        holder["mmap"] = True


def collect():
    inodes = {}

    for task in for_each_task(prog):
        # 只看线程组 leader: 线程共享 fd 表和 mm, 遍历每个线程会把
        # 同一个引用重复统计很多次 (QEMU/浏览器会有几百个线程)。
        if obj_value(task.pid) != obj_value(task.tgid):
            continue
        try:
            for fd, file in for_each_file(task):
                try:
                    if file_fs_name(file) == "tmpfs":
                        add_ref(inodes, task, file, ("fd", fd))
                except Exception:
                    continue
        except Exception:
            pass

        try:
            if obj_value(task.mm) == 0:
                continue
            for vma in for_each_vma(task.mm):
                try:
                    file = vma.vm_file
                    if obj_value(file) == 0:
                        continue
                    if file_fs_name(file) == "tmpfs":
                        add_ref(inodes, task, file, ("mmap",))
                except Exception:
                    continue
        except Exception:
            continue

    return inodes


def holder_str(entry, max_holders=8):
    parts = []
    holders = sorted(entry["holders"].values(), key=lambda h: h["pid"])
    for holder in holders[:max_holders]:
        via = []
        if holder["fd"]:
            via.append("fd" + ",".join(str(fd) for fd in sorted(holder["fd"])))
        if holder["mmap"]:
            via.append("mmap")
        parts.append(f"{holder['comm']}({holder['pid']}):{'+'.join(via)}")
    if len(holders) > max_holders:
        parts.append(f"... +{len(holders) - max_holders} procs")
    return " ".join(parts)


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--top", type=int, default=40, help="number of largest shmem inodes to print")
    parser.add_argument("--all", action="store_true", help="print all shmem inodes")
    parser.add_argument("--category", help="comma-separated filter: memfd,sysv,devshm,tmpfs")
    parser.add_argument("--min-pages", type=int, default=0, help="hide inodes with fewer resident pages")
    parser.add_argument("--no-procs", action="store_true", help="skip the per-process summary")
    return parser.parse_args()


def main():
    args = parse_args()
    if args.top < 0:
        print("error: --top must be >= 0", file=sys.stderr)
        sys.exit(2)

    category_filter = None
    if args.category:
        category_filter = {item.strip() for item in args.category.split(",") if item.strip()}

    inodes = collect()
    entries = list(inodes.values())
    entries = [e for e in entries if e["pages"] + e["swapped"] >= args.min_pages]
    if category_filter:
        entries = [e for e in entries if e["category"] in category_filter]
    entries.sort(key=lambda e: e["pages"] + e["swapped"], reverse=True)

    shown = entries if args.all else entries[: args.top]

    print("=== Shmem Usage ===")
    print(f"page_size: {PAGE_SIZE} bytes")
    print(f"referenced_inodes: {len(entries)}")
    total_pages = sum(e["pages"] for e in entries)
    total_swapped = sum(e["swapped"] for e in entries)
    print(f"resident_total: {total_pages} pages ({human_size(total_pages * PAGE_SIZE)})")
    print(f"swapped_total: {total_swapped} pages ({human_size(total_swapped * PAGE_SIZE)})")
    for category in CATEGORIES:
        cat = [e for e in entries if e["category"] == category]
        pages = sum(e["pages"] for e in cat)
        swapped = sum(e["swapped"] for e in cat)
        print(
            f"  {category:<7} inodes={len(cat):<5} "
            f"resident={human_size(pages * PAGE_SIZE):>10} "
            f"swapped={human_size(swapped * PAGE_SIZE):>10}"
        )
    print("note: resident 不含已换出页; 无进程引用的孤儿 shmem inode 不在统计内。")
    print()

    print(f"{'RESIDENT':>10} {'SWAPPED':>10} {'SIZE':>10} {'CATEGORY':<8} {'INO':>10} PATH [HOLDERS]")
    for entry in shown:
        print(
            f"{human_size(entry['pages'] * PAGE_SIZE):>10} "
            f"{human_size(entry['swapped'] * PAGE_SIZE):>10} "
            f"{human_size(entry['size']):>10} "
            f"{entry['category']:<8} "
            f"{entry['ino']:10d} "
            f"{entry['path']} [{holder_str(entry)}]"
        )
    if not args.all and len(entries) > args.top:
        print(f"... and {len(entries) - args.top} more (use --all)")

    if args.no_procs:
        return

    print()
    print("=== Per Process (shared inodes are counted under every holder) ===")
    procs = {}
    for entry in entries:
        for holder in entry["holders"].values():
            key = holder["pid"]
            proc = procs.setdefault(
                key,
                {
                    "comm": holder["comm"],
                    "pid": key,
                    "fd_pages": 0,
                    "mmap_pages": 0,
                    "swapped": 0,
                    "files": 0,
                },
            )
            proc["files"] += 1
            if holder["fd"]:
                proc["fd_pages"] += entry["pages"]
            if holder["mmap"]:
                proc["mmap_pages"] += entry["pages"]
            proc["swapped"] += entry["swapped"]

    print(f"{'RESIDENT':>10} {'SWAPPED':>10} {'FILES':>5} {'VIA':<10} {'PID':>7} COMM")
    for proc in sorted(procs.values(), key=lambda p: p["fd_pages"] + p["mmap_pages"], reverse=True)[: args.top]:
        via = []
        if proc["fd_pages"]:
            via.append("fd")
        if proc["mmap_pages"]:
            via.append("mmap")
        resident = max(proc["fd_pages"], proc["mmap_pages"]) * PAGE_SIZE
        print(
            f"{human_size(resident):>10} "
            f"{human_size(proc['swapped'] * PAGE_SIZE):>10} "
            f"{proc['files']:5d} "
            f"{'+'.join(via):<10} "
            f"{proc['pid']:7d} "
            f"{proc['comm']}"
        )


if __name__ == "__main__":
    main()
