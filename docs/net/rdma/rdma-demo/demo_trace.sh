#!/usr/bin/env bash
# =============================================================================
# RDMA Kernel Tracing Demo
# 
# 演示如何使用 ftrace 观察 RDMA CQ 处理
# =============================================================================

set -E -e -u -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DEVICE_LOCAL="${DEVICE_LOCAL:-rocep0s6}"
DEVICE_REMOTE="${DEVICE_REMOTE:-mlx5_0}"
SERVER_IP="${SERVER_IP:-10.0.3.2}"
ITERATIONS=100

echo "=== RDMA Kernel Tracing Demo ==="
echo ""
echo "This demo will:"
echo "1. Setup ftrace to capture RDMA related functions"
echo "2. Run RDMA ping-pong test (polling mode)"
echo "3. Show the kernel function call statistics"
echo ""

# 检查 root 权限
if [[ $EUID -ne 0 ]]; then
    echo "Error: This script must be run as root for ftrace"
    echo "Usage: sudo ./demo_trace.sh"
    exit 1
fi

# 挂载 debugfs 如果需要
TRACE_DIR="/sys/kernel/debug/tracing"
if [[ ! -d "$TRACE_DIR" ]]; then
    mount -t debugfs none /sys/kernel/debug
fi

# 清理之前的 trace
echo 0 > "$TRACE_DIR/tracing_on" 2>/dev/null || true
echo > "$TRACE_DIR/trace" 2>/dev/null || true
echo > "$TRACE_DIR/set_ftrace_filter" 2>/dev/null || true

echo "[*] Setting up ftrace..."

# 启用 function_graph tracer
echo function > "$TRACE_DIR/current_tracer" 2>/dev/null || {
    echo "Warning: function tracer not available, using nop"
    echo nop > "$TRACE_DIR/current_tracer"
}

# 添加感兴趣的函数
cat >> "$TRACE_DIR/set_ftrace_filter" << 'EOF'
ib_uverbs_poll_cq
ib_uverbs_comp_handler
mlx5_ib_poll_cq
mlx5_ib_completion_event
ib_process_cq_direct
napi_poll
net_rx_action
EOF

echo "[*] Trace filters set:"
cat "$TRACE_DIR/set_ftrace_filter" | head -10 || echo "  (using default)"

# 启动 trace
echo 1 > "$TRACE_DIR/tracing_on"
echo "[*] Tracing started"

# 准备服务端
echo "[*] Preparing remote server..."
if ! ssh root@"$SERVER_IP" "test -x /tmp/rdma_pingpong_poll.out" 2>/dev/null; then
    cat "$SCRIPT_DIR/rdma_pingpong_poll.out" | ssh root@"$SERVER_IP" "cat > /tmp/rdma_pingpong_poll.out && chmod +x /tmp/rdma_pingpong_poll.out"
fi

ssh root@"$SERVER_IP" "pkill -f 'rdma_pingpong' 2>/dev/null || true"
sleep 0.5

# 启动服务端
ssh root@"$SERVER_IP" "cd /tmp && nohup /tmp/rdma_pingpong_poll.out -s -d $DEVICE_REMOTE -i $ITERATIONS > /tmp/server_trace.log 2>&1 &"
sleep 1

# 运行客户端
echo "[*] Running RDMA test..."
"$SCRIPT_DIR/rdma_pingpong_poll.out" -d "$DEVICE_LOCAL" -i "$ITERATIONS" "$SERVER_IP" 2>&1 | grep -E "time:|latency:|Throughput" || true

# 停止 trace
echo 0 > "$TRACE_DIR/tracing_on"
echo "[*] Tracing stopped"

# 显示结果
echo ""
echo "=========================================="
echo "=== Kernel Trace Results ==="
echo "=========================================="

echo ""
echo "--- Function Call Statistics ---"
echo "Function                          Count"
echo "-------------------------------- -----"
grep -E "ib_uverbs|mlx5_ib|ib_process|napi_poll|net_rx_action" "$TRACE_DIR/trace" 2>/dev/null | \
    awk '{fn=$1; count[fn]++} END {for (f in count) printf "%-32s %5d\n", f, count[f]}' | sort -rn -k2 | head -20 || echo "  (no data)"

echo ""
echo "--- Sample Trace (last 30 lines) ---"
tail -30 "$TRACE_DIR/trace" 2>/dev/null || echo "  (trace buffer empty)"

# 清理
ssh root@"$SERVER_IP" "pkill -f 'rdma_pingpong' 2>/dev/null || true" 2>/dev/null || true

echo ""
echo "[*] Demo complete"
echo "[*] Full trace available at: $TRACE_DIR/trace"
