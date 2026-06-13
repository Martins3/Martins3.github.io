#!/usr/bin/env bash
set -E -e -u -o pipefail

# https://github.com/torvalds/linux/tree/master/tools/bpf/bpftool/Documentation

bpftool prog show
bpftool prog dump xlated id 70 opcodes
bpftool prog dump xlated id 70
bpftool prog dump jited id 70

## 基本的功能检查
bpftool feature
sudo bpftrace --info

# 将构建的 bpf ping 加载 kernel 中
