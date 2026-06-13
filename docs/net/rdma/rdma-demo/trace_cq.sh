#!/usr/bin/env bash
# shellcheck disable=SC2009
# =============================================================================
# RDMA CQ Completion Tracing Script
#
# 功能: 使用 ftrace 追踪内核中的 CQ (Completion Queue) 处理
# 包括: polling vs interrupt 路径、NAPI、软中断等
# =============================================================================

set -E -e -u -o pipefail

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 内核源码路径
KERNEL_SRC="${KERNEL_SRC:-/home/martins3/data/kernel/linux-build}"
TRACE_DIR="/sys/kernel/debug/tracing"
DURATION="${1:-10}"

echo "=== RDMA CQ Kernel Tracing ==="
echo "Duration: ${DURATION} seconds"
echo "Kernel: $(uname -r)"
echo ""

# 检查 root 权限
if [[ $EUID -ne 0 ]]; then
    echo -e "${RED}Error: This script must be run as root${NC}"
    exit 1
fi

# 检查 debugfs 是否挂载
if [[ ! -d "$TRACE_DIR" ]]; then
    mount -t debugfs none /sys/kernel/debug 2>/dev/null || {
        echo -e "${RED}Failed to mount debugfs${NC}"
        exit 1
    }
fi

# 清理之前的 trace
echo 0 > "$TRACE_DIR/tracing_on" 2>/dev/null || true
echo > "$TRACE_DIR/trace" 2>/dev/null || true

echo "[*] Setting up trace filters..."

# 启用函数追踪
echo function > "$TRACE_DIR/current_tracer" 2>/dev/null || {
    echo -e "${YELLOW}Warning: function tracer not available, using nop${NC}"
    echo nop > "$TRACE_DIR/current_tracer"
}

# 设置要追踪的函数
# ib_uverbs: 用户态 verbs 接口

echo > "$TRACE_DIR/set_ftrace_filter"
echo "ib_uverbs_poll_cq" >> "$TRACE_DIR/set_ftrace_filter"
echo "ib_uverbs_comp_handler" >> "$TRACE_DIR/set_ftrace_filter"

# mlx5 驱动相关
echo "mlx5_ib_poll_cq" >> "$TRACE_DIR/set_ftrace_filter"
# echo "mlx5_ib_completion_event" >> "$TRACE_DIR/set_ftrace_filter"
# echo "mlx5_cq_event" >> "$TRACE_DIR/set_ftrace_filter"

# RDMA 核心
echo "rdma_user_mmap_io" >> "$TRACE_DIR/set_ftrace_filter"
echo "ib_process_cq_direct" >> "$TRACE_DIR/set_ftrace_filter"
echo "ib_cq_poll_work" >> "$TRACE_DIR/set_ftrace_filter"


# 启用 trace
echo 1 > "$TRACE_DIR/tracing_on"

echo -e "${GREEN}[*] Tracing started. Run your RDMA test now...${NC}"
echo -e "${YELLOW}    Press Ctrl+C to stop early${NC}"

# 等待指定时间或用户中断
cleanup() {
    echo 0 > "$TRACE_DIR/tracing_on" 2>/dev/null || true
    echo -e "\n${GREEN}[*] Tracing stopped${NC}"
}
trap cleanup EXIT

sleep "$DURATION"

cat "$TRACE_DIR/trace"
