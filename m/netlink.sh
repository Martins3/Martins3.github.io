#!/usr/bin/env bash
set -E -e -u -o pipefail

# netlink 测试脚本
# 用法: ./netlink.sh [action]
#   action 0 - 基本回显测试 (默认)
#   action 1 - 循环测试

ACTION=${1:-0}

echo "=== Netlink Test ==="
echo "action = $ACTION"

cd "$(dirname "$0")"

# 编译用户空间测试程序
gcc netlink-user.c -o netlink-user.out

# 确保 netlink 已初始化
echo "0" | sudo tee /sys/kernel/hacking/netlink > /dev/null

# 运行用户空间测试程序
./netlink-user.out "$ACTION"

echo "=== Test completed ==="
