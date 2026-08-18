#!/usr/bin/env bash
set -E -e -u -o pipefail


cd "$(dirname "$0")"

# 这个是没有任何输出的
# sudo bpftrace ./trace_gpu_irq.bt &
sudo bpftrace ./trace_nvidia_isr.bt &

sleep 1
../vector_add.out
sleep 1
