#!/usr/bin/env bash
set -E -e -u -o pipefail

# GENEVE 隧道实验脚本
# 创建两个命名空间，通过 GENEVE 隧道连接

NS1="host1"
NS2="host2"
VNI="100"

function cleanup() {
    echo "[*] 清理环境..."
    sudo ip netns del "$NS1" 2>/dev/null || true
    sudo ip netns del "$NS2" 2>/dev/null || true
    echo "[*] 清理完成"
}

function setup() {
    echo "[*] 创建命名空间..."
    sudo ip netns add "$NS1"
    sudo ip netns add "$NS2"

    echo "[*] 创建 veth 对模拟底层网络..."
    sudo ip link add veth0 type veth peer name veth1
    sudo ip link set veth0 netns "$NS1"
    sudo ip link set veth1 netns "$NS2"

    echo "[*] 配置底层网络 IP..."
    sudo ip -n "$NS1" addr add 192.168.100.1/24 dev veth0
    sudo ip -n "$NS2" addr add 192.168.100.2/24 dev veth1
    sudo ip -n "$NS1" link set veth0 up
    sudo ip -n "$NS2" link set veth1 up
    sudo ip -n "$NS1" link set lo up
    sudo ip -n "$NS2" link set lo up

    echo "[*] 创建 GENEVE 隧道..."
    # NS1 上的 GENEVE 隧道
    sudo ip -n "$NS1" link add geneve0 type geneve id "$VNI" remote 192.168.100.2
    sudo ip -n "$NS1" addr add 10.0.0.1/24 dev geneve0
    sudo ip -n "$NS1" link set geneve0 up

    # NS2 上的 GENEVE 隧道
    sudo ip -n "$NS2" link add geneve0 type geneve id "$VNI" remote 192.168.100.1
    sudo ip -n "$NS2" addr add 10.0.0.2/24 dev geneve0
    sudo ip -n "$NS2" link set geneve0 up

    echo "[*] 配置完成"
}

function show_status() {
    echo ""
    echo "=== $NS1 网络接口 ==="
    sudo ip -n "$NS1" addr show
    echo ""
    echo "=== $NS2 网络接口 ==="
    sudo ip -n "$NS2" addr show
    echo ""
    echo "=== $NS1 路由表 ==="
    sudo ip -n "$NS1" route show
    echo ""
    echo "=== $NS2 路由表 ==="
    sudo ip -n "$NS2" route show
}

function test_connectivity() {
    echo ""
    echo "[*] 测试底层网络连通性..."
    sudo ip netns exec "$NS1" ping -c 2 192.168.100.2 || true

    echo ""
    echo "[*] 测试 GENEVE 隧道连通性..."
    sudo ip netns exec "$NS1" ping -c 3 10.0.0.2 || true
}

function capture_geneve() {
    echo ""
    echo "[*] 在 $NS1 上抓包查看 GENEVE 封装 (持续 5 秒)..."
    sudo timeout 5 ip netns exec "$NS1" tcpdump -i veth0 -nn -e -l udp port 6081 || true
}

case "${1:-}" in
    setup)
        cleanup
        setup
        show_status
        test_connectivity
        ;;
    clean)
        cleanup
        ;;
    status)
        show_status
        ;;
    test)
        test_connectivity
        ;;
    capture)
        capture_geneve
        ;;
    *)
        echo "用法: $0 {setup|clean|status|test|capture}"
        echo "  setup   - 创建并配置 GENEVE 隧道"
        echo "  clean   - 清理环境"
        echo "  status  - 显示网络状态"
        echo "  test    - 测试连通性"
        echo "  capture - 抓包查看 GENEVE 封装"
        exit 1
        ;;
esac
