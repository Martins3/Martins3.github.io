#!/usr/bin/env bash
# Simple RDMA test runner

set -e

ITER=${1:-50}
IP=${2:-10.0.3.2}
LOCAL_DEV=${3:-rocep0s6}
REMOTE_DEV=${4:-mlx5_0}

# SSH 选项：禁用密码交互，使用当前用户密钥
SSH_OPTS="-o BatchMode=yes -o ConnectTimeout=3 -o StrictHostKeyChecking=no"

echo "=== RDMA Test ($ITER iterations) ==="

# 检查是否以 root 运行（会导致 SSH 密钥问题）
if [[ $EUID -eq 0 ]]; then
    echo "Warning: Running as root. SSH key authentication may fail."
    echo "Please run without sudo: ./run.sh"
fi

# 检查 SSH 免密登录
echo "[*] Checking SSH connection..."
if ! ssh $SSH_OPTS root@$IP "echo OK" 2>/dev/null | grep -q OK; then
    echo "Error: SSH key authentication failed for root@$IP"
    echo "Debug: testing SSH..."
    ssh -v $SSH_OPTS root@$IP "echo OK" 2>&1 | tail -5
    exit 1
fi

# 复制二进制文件
if ! ssh $SSH_OPTS root@$IP "test -x /tmp/rdma_pingpong_poll.out" 2>/dev/null; then
    echo "[*] Copying binaries to remote..."
    cat rdma_pingpong_poll.out | ssh $SSH_OPTS root@$IP "cat > /tmp/rdma_pingpong_poll.out; chmod +x /tmp/rdma_pingpong_poll.out" 2>/dev/null
    cat rdma_pingpong_event.out | ssh $SSH_OPTS root@$IP "cat > /tmp/rdma_pingpong_event.out; chmod +x /tmp/rdma_pingpong_event.out" 2>/dev/null
fi

# 清理函数
cleanup() {
    ssh $SSH_OPTS root@$IP "pkill -f 'rdma_pingpong' 2>/dev/null || true" 2>/dev/null || true
}
trap cleanup EXIT

cleanup
sleep 1

# Polling test
echo ""
echo "--- POLLING MODE ---"
ssh -f $SSH_OPTS root@$IP "timeout 60 /tmp/rdma_pingpong_poll.out -s -d $REMOTE_DEV -i $ITER" > /tmp/srv_poll.log 2>&1
sleep 2
./rdma_pingpong_poll.out -d $LOCAL_DEV -i $ITER $IP 2>&1

sleep 1
cleanup
sleep 1

# Event test  
echo ""
echo "--- INTERRUPT MODE ---"
ssh -f $SSH_OPTS root@$IP "timeout 60 /tmp/rdma_pingpong_event.out -s -d $REMOTE_DEV -i $ITER -e" > /tmp/srv_event.log 2>&1
sleep 2
./rdma_pingpong_event.out -d $LOCAL_DEV -i $ITER $IP 2>&1

echo ""
echo "=== Done ==="
