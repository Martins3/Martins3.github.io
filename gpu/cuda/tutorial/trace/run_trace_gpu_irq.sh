#!/usr/bin/env bash
set -E -e -u -o pipefail

TRACE_DIR="/home/martins3/data/vn/gpu_demo/cuda"
TRACE_SCRIPT="${TRACE_DIR}/trace_gpu_irq.bt"
TRACE_LOG="${TRACE_DIR}/trace_gpu_irq.log"
DEMO_LOG="${TRACE_DIR}/trace_gpu_irq.demo.log"
export BPFTRACE_PERF_RB_PAGES="${BPFTRACE_PERF_RB_PAGES:-512}"

function cleanup() {
	if [[ -n ${BPFTRACE_PID:-} ]]; then
		kill "${BPFTRACE_PID}" >/dev/null 2>&1 || true
		wait "${BPFTRACE_PID}" >/dev/null 2>&1 || true
	fi
}

trap cleanup EXIT

cd "${TRACE_DIR}"
# shellcheck disable=SC1091
source "${TRACE_DIR}/env.sh" >/dev/null

rm -f "${TRACE_LOG}" "${DEMO_LOG}"

stdbuf -oL -eL bpftrace -B full "${TRACE_SCRIPT}" >"${TRACE_LOG}" 2>&1 &
BPFTRACE_PID=$!

sleep 1
./memcpy_roundtrip.out >"${DEMO_LOG}" 2>&1
sleep 1

cleanup
unset BPFTRACE_PID

echo "[OK] demo log: ${DEMO_LOG}"
echo "[OK] trace log: ${TRACE_LOG}"
