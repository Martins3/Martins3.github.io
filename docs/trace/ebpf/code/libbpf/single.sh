#!/usr/bin/env bash
set -E -e -u -o pipefail

make
bpftool prog load .output/single.bpf.o /sys/fs/bpf/adfsa
# TODO 不知道为什么， bpftool prog show  可以检查到的确 load 了，但是 kernel 没有 trace 信息。
