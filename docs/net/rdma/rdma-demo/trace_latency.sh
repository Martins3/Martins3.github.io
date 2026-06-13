#!/usr/bin/env bash
# =============================================================================
# RDMA Latency Measurement Script
# 
# 功能: 对比测量 polling 和 interrupt 模式的延迟
# 包括: 用户态延迟、内核态延迟、端到端延迟
# =============================================================================

set -E -e -u -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ITERATIONS="${1:-1000}"
DEVICE="${2:-rocep0s6}"
SERVER_IP="${3:-10.0.3.2}"

echo "=== RDMA Latency Comparison ==="
echo "Iterations: $ITERATIONS"
echo "Device: $DEVICE"
echo "Server: $SERVER_IP"
echo ""

cd "$SCRIPT_DIR"

# 检查程序是否存在
if [[ ! -f "rdma_pingpong_poll.out" ]] || [[ ! -f "rdma_pingpong_event.out" ]]; then
    echo "[*] Building programs..."
    make clean && make
fi

# 启动服务器（在对端）
start_server() {
    local mode="$1"
    local bin="./rdma_pingpong_${mode}.out"
    
    echo "[*] Starting server on $SERVER_IP (${mode} mode)..."
    ssh root@"$SERVER_IP" "cd $SCRIPT_DIR && pkill -f 'rdma_pingpong' 2>/dev/null; sleep 0.5; nohup $bin -s -d mlx5_0 -i $ITERATIONS > /tmp/rdma_server_${mode}.log 2>&1 & sleep 1 && echo 'Server started'"
}

stop_server() {
    echo "[*] Stopping server..."
    ssh root@"$SERVER_IP" "pkill -f 'rdma_pingpong' 2>/dev/null || true"
}

# 测试 polling 模式
run_poll_test() {
    echo ""
    echo "=========================================="
    echo "=== POLLING MODE TEST ==="
    echo "=========================================="
    
    start_server "poll"
    sleep 1
    
    echo "[*] Running client (polling)..."
    ./rdma_pingpong_poll.out -d "$DEVICE" -i "$ITERATIONS" "$SERVER_IP" 2>&1 | tee /tmp/rdma_poll_result.txt
    
    stop_server
}

# 测试 interrupt 模式
run_event_test() {
    echo ""
    echo "=========================================="
    echo "=== INTERRUPT MODE TEST ==="
    echo "=========================================="
    
    start_server "event"
    sleep 1
    
    echo "[*] Running client (interrupt)..."
    ./rdma_pingpong_event.out -d "$DEVICE" -i "$ITERATIONS" "$SERVER_IP" 2>&1 | tee /tmp/rdma_event_result.txt
    
    stop_server
}

# 运行测试
trap stop_server EXIT

run_poll_test
sleep 2
run_event_test

# 对比结果
echo ""
echo "=========================================="
echo "=== COMPARISON SUMMARY ==="
echo "=========================================="

echo ""
echo "--- POLLING MODE ---"
grep -E "Avg latency|Messages|Throughput" /tmp/rdma_poll_result.txt 2>/dev/null || echo "No results"

echo ""
echo "--- INTERRUPT MODE ---"
grep -E "Avg latency|Messages|Throughput" /tmp/rdma_event_result.txt 2>/dev/null || echo "No results"

echo ""
echo "=========================================="
echo "Notes:"
echo "  - POLLING: Lower latency, higher CPU usage"
echo "  - INTERRUPT: Higher latency, lower CPU usage"
echo "  - Difference shows interrupt overhead"
echo "=========================================="
