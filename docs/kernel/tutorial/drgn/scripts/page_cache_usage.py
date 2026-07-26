#!/usr/bin/env drgn
"""
Summarize live page cache usage by inode.

Usage:
    sudo drgn -c /proc/kcore ./page_cache_by_file.py
    sudo drgn -c /proc/kcore ./page_cache_by_file.py --top 50
    sudo drgn -c /proc/kcore ./page_cache_by_file.py --fs xfs,ext4,tmpfs
    sudo drgn -c /proc/kcore ./page_cache_by_file.py --all --min-pages 16 --no-paths

算法非常简单，就是先找到所有的 superblock ，然后找到所有的 inodes ，然后
就可以找到所有的文件的 page cache 了:

    super_blocks -> super_block.s_inodes -> inode.i_mapping.nrpages

注意:
It reports inode-backed page cache. Swap cache is not inode-backed and is not
included. The walk is lockless, so treat the result as a live best-effort
snapshot.
"""

import argparse
import sys

from drgn.helpers.linux import hlist_for_each_entry, list_for_each_entry

PAGE_SIZE = prog["PAGE_SIZE"].value_()

MODE_TYPES = {
    0o010000: "fifo",
    0o020000: "chr",
    0o040000: "dir",
    0o060000: "blk",
    0o100000: "reg",
    0o120000: "lnk",
    0o140000: "sock",
}


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


def ptr(obj):
    return f"{obj_value(obj):#x}"


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


def inode_type(inode):
    mode = obj_value(inode.i_mode)
    return MODE_TYPES.get(mode & 0o170000, "unk")


def dentry_name(dentry):
    return c_string(dentry.d_name.name, default="?")


def dentry_path(dentry, max_depth=128):
    parts = []
    seen = set()
    current = dentry

    for _ in range(max_depth):
        addr = obj_value(current)
        if addr == 0 or addr in seen:
            break
        seen.add(addr)

        name = dentry_name(current)
        if name and name != "/":
            parts.append(name)

        parent = current.d_parent
        parent_addr = obj_value(parent)
        if parent_addr == 0 or parent_addr == addr:
            break
        current = parent[0]

    if not parts:
        return "/"

    prefix = "" if parts[-1].startswith("/") else "/"
    return prefix + "/".join(reversed(parts))


def inode_dentries(inode, max_aliases=8):
    try:
        if obj_value(inode.i_dentry.first) == 0:
            return []
    except Exception:
        return []

    for member in ("d_alias", "d_u.d_alias"):
        try:
            dentries = []
            for dentry in hlist_for_each_entry("struct dentry", inode.i_dentry.address_of_(), member):
                dentries.append(dentry)
                if len(dentries) >= max_aliases:
                    break
            return dentries
        except Exception:
            continue

    return []


def inode_paths(inode):
    paths = []
    for dentry in inode_dentries(inode):
        path = dentry_path(dentry)
        if path not in paths:
            paths.append(path)
    return paths


def superblock_fs_name(sb):
    return c_string(sb.s_type.name, default="?")


def superblock_id(sb):
    return c_string(sb.s_id, default="?")


def collect_entries(fs_filter):
    entries = []
    scanned_inodes = 0
    scanned_superblocks = 0

    for sb in list_for_each_entry("struct super_block", prog["super_blocks"].address_of_(), "s_list"):
        scanned_superblocks += 1
        fs_name = superblock_fs_name(sb)
        if fs_filter and fs_name not in fs_filter:
            continue

        s_id = superblock_id(sb)
        try:
            inode_iter = list_for_each_entry("struct inode", sb.s_inodes.address_of_(), "i_sb_list")
        except Exception:
            continue

        for inode in inode_iter:
            scanned_inodes += 1
            try:
                pages = inode.i_mapping.nrpages.value_()
            except Exception:
                continue
            if pages == 0:
                continue

            entries.append(
                {
                    "pages": pages,
                    "bytes": pages * PAGE_SIZE,
                    "fs": fs_name,
                    "s_id": s_id,
                    "ino": obj_value(inode.i_ino),
                    "type": inode_type(inode),
                    "aliases": None,
                    "path": None,
                    "inode_obj": inode,
                    "inode": ptr(inode),
                    "mapping": ptr(inode.i_mapping),
                }
            )

    entries.sort(key=lambda item: item["pages"], reverse=True)
    return scanned_superblocks, scanned_inodes, entries


def resolve_entry_path(entry):
    if entry["path"] is not None:
        return

    paths = inode_paths(entry["inode_obj"])
    entry["aliases"] = len(paths)
    entry["path"] = paths[0] if paths else f"(no dentry) ino={entry['ino']}"


def parse_args():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--top", type=int, default=30, help="number of largest cached files to print")
    parser.add_argument("--all", action="store_true", help="print all cached inodes")
    parser.add_argument("--min-pages", type=int, default=1, help="hide inodes with fewer cached pages")
    parser.add_argument("--fs", help="comma-separated filesystem type filter, e.g. xfs,ext4,tmpfs")
    parser.add_argument("--no-paths", action="store_true", help="skip dentry path resolution")
    parser.add_argument("--no-header", action="store_true", help="only print the table")
    return parser.parse_args()


def main():
    args = parse_args()
    if args.top < 0:
        print("error: --top must be >= 0", file=sys.stderr)
        sys.exit(2)

    fs_filter = None
    if args.fs:
        fs_filter = {item.strip() for item in args.fs.split(",") if item.strip()}

    scanned_superblocks, scanned_inodes, entries = collect_entries(fs_filter)
    entries = [entry for entry in entries if entry["pages"] >= args.min_pages]

    total_pages = sum(entry["pages"] for entry in entries)
    total_bytes = total_pages * PAGE_SIZE

    if args.all:
        shown = entries
    elif args.top == 0:
        shown = []
    else:
        shown = entries[: args.top]

    if args.no_paths:
        for entry in shown:
            entry["aliases"] = 0
            entry["path"] = f"ino={entry['ino']} inode={entry['inode']} mapping={entry['mapping']}"
    else:
        for entry in shown:
            resolve_entry_path(entry)

    if not args.no_header:
        print("=== Page Cache By File ===")
        print(f"page_size: {PAGE_SIZE} bytes")
        print(f"scanned_superblocks: {scanned_superblocks}")
        print(f"scanned_inodes: {scanned_inodes}")
        print(f"cached_inodes: {len(entries)}")
        print(f"cached_total: {total_pages} pages ({human_size(total_bytes)})")
        if fs_filter:
            print(f"fs_filter: {','.join(sorted(fs_filter))}")
        print("note: inode-backed page cache only; swap cache is not included.")
        print()

    print(f"{'PAGES':>12} {'SIZE':>10} {'FS':<10} {'SB':<16} {'INO':>10} {'TYPE':<4} {'A':>2} PATH")
    for entry in shown:
        print(
            f"{entry['pages']:12d} "
            f"{human_size(entry['bytes']):>10} "
            f"{entry['fs'][:10]:<10} "
            f"{entry['s_id'][:16]:<16} "
            f"{entry['ino']:10d} "
            f"{entry['type']:<4} "
            f"{entry['aliases']:2d} "
            f"{entry['path']}"
        )


if __name__ == "__main__":
    main()
