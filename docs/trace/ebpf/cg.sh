#!/usr/bin/env bash
set -E -e -u -o pipefail

make
# 更加复杂的测试参考 : samples/bpf/test_cgrp2_sock2.sh
./cg.out /sys/fs/cgroup/user.slice .output/cg.bpf.o
# 的确有一个
ls /sys/fs/bpf/test_cgrp2_sock2
ping 10.0.2.2
