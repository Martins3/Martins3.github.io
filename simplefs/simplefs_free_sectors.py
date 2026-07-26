#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-2.0
"""Dump SimpleFS free block ranges as sector ranges.

用法: simplefs_free_sectors.py DEVICE SECTORS_PER_BLOCK

SimpleFS 的空闲块位图位于 (1 + istore + ifree) 块之后，bit=1 表示空闲。
输出与 xfstests generic/746 的 get_free_sectors 期望格式一致：
每个空闲段一行 "start_sector end_sector"。
"""
import struct
import sys

SIMPLEFS_MAGIC = 0xDEADCE


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit(f"usage: {sys.argv[0]} DEVICE SECTORS_PER_BLOCK")

    dev, spb = sys.argv[1], int(sys.argv[2])
    with open(dev, "rb") as img:
        header = img.read(4096)
        magic, nr_blocks, _nr_inodes, istore, ifree, bfree = struct.unpack_from(
            "<6I", header, 0)
        if magic != SIMPLEFS_MAGIC:
            raise SystemExit(f"{dev}: not a SimpleFS image (magic {magic:#x})")
        img.seek((1 + istore + ifree) * 4096)
        bitmap = img.read(bfree * 4096)

    def is_free(block: int) -> bool:
        return (bitmap[block // 8] >> (block % 8)) & 1 == 1

    run_start = None
    for block in range(nr_blocks + 1):
        free = block < nr_blocks and is_free(block)
        if free and run_start is None:
            run_start = block
        elif not free and run_start is not None:
            print(spb * run_start, spb * block - 1)
            run_start = None


if __name__ == "__main__":
    main()
