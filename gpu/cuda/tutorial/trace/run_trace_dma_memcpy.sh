#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"

sudo bpftrace ./trace_dma_memcpy.bt &

sleep 1
../memcpy_roundtrip.out
sleep 1
