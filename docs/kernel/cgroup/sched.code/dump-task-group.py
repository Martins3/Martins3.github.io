#!/usr/bin/env python3
"""解析 bpftrace 输出，打印 task_group 层级树。"""
import signal
import sys
from collections import defaultdict

signal.signal(signal.SIGPIPE, signal.SIG_DFL)

nodes = {}
children = defaultdict(list)

for line in sys.stdin:
    line = line.strip()
    if not line.startswith("NODE "):
        continue
    parts = line.split()
    if len(parts) < 6:
        continue
    addr, parent, shares, cfs_rq, se = parts[1:6]
    nodes[addr] = {
        "parent": parent,
        "shares": shares,
        "cfs_rq0": cfs_rq,
        "se0": se,
    }
    if parent != "0":
        children[parent].append(addr)

root = None
for addr, info in nodes.items():
    if info["parent"] == "0":
        root = addr
        break


def print_tree_inline(node, prefix):
    if node not in nodes:
        return
    info = nodes[node]
    print(f"{node} shares={info['shares']} cfs_rq[0]={info['cfs_rq0']} se[0]={info['se0']}")
    kids = children.get(node, [])
    for i, kid in enumerate(kids):
        is_last = (i == len(kids) - 1)
        connector = "└── " if is_last else "├── "
        print(f"{prefix}{connector}", end="")
        grandchild_prefix = prefix + ("    " if is_last else "│   ")
        print_tree_inline(kid, grandchild_prefix)


if root:
    print(f"{root} shares={nodes[root]['shares']} cfs_rq[0]={nodes[root]['cfs_rq0']} se[0]={nodes[root]['se0']} (ROOT)")
    kids = children.get(root, [])
    for i, kid in enumerate(kids):
        is_last = (i == len(kids) - 1)
        connector = "└── " if is_last else "├── "
        print(f"{connector}", end="")
        child_prefix = "    " if is_last else "│   "
        print_tree_inline(kid, child_prefix)
else:
    print("No root found")
