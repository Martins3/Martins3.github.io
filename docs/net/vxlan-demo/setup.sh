#!/usr/bin/env bash
set -E -e -u -o pipefail

# VXLAN 实验环境搭建脚本
# 使用 network namespace 模拟两台主机

# sudo 封装，使用密码 'a'
SUDO() {
    echo 'a' | sudo -S "$@"
}

log_info() {
    echo "[INFO] $1"
}

log_warn() {
    echo "[WARN] $1"
}

log_error() {
    echo "[ERROR] $1"
}

# 清理已有环境
cleanup_existing() {
    log_info "检查并清理已有环境..."
    SUDO ip netns del ns-host1 2>/dev/null || true
    SUDO ip netns del ns-host2 2>/dev/null || true
    SUDO ip netns del ns-container1 2>/dev/null || true
    SUDO ip netns del ns-container2 2>/dev/null || true
    sleep 1
}

# 创建基础网络命名空间
create_namespaces() {
    log_info "创建 network namespace..."
    SUDO ip netns add ns-host1
    SUDO ip netns add ns-host2
    SUDO ip netns add ns-container1
    SUDO ip netns add ns-container2
}

# 创建 underlay 网络连接
create_underlay() {
    log_info "创建 underlay 网络 (veth pair)..."

    # 创建 veth pair
    SUDO ip link add veth1 type veth peer name veth2
    SUDO ip link add veth3 type veth peer name veth4
    SUDO ip link add veth5 type veth peer name veth6

    # 将 veth 移入 namespace
    SUDO ip link set veth1 netns ns-host1
    SUDO ip link set veth4 netns ns-host1

    SUDO ip link set veth3 netns ns-container1
    SUDO ip link set veth5 netns ns-container2
    SUDO ip link set veth2 netns ns-host2
    SUDO ip link set veth6 netns ns-host2

    # 配置 underlay IP (VTEP 地址)
    SUDO ip netns exec ns-host1 ip addr add 172.16.0.1/24 dev veth1
    SUDO ip netns exec ns-host2 ip addr add 172.16.0.2/24 dev veth2

    # 配置容器 IP
    SUDO ip netns exec ns-container1 ip addr add 10.200.1.10/24 dev veth3
    SUDO ip netns exec ns-container2 ip addr add 10.200.1.20/24 dev veth5

    # 启动接口
    SUDO ip netns exec ns-host1 ip link set veth1 up
    SUDO ip netns exec ns-host2 ip link set veth2 up
    SUDO ip netns exec ns-host1 ip link set veth4 up
    SUDO ip netns exec ns-host2 ip link set veth6 up
    SUDO ip netns exec ns-container1 ip link set veth3 up
    SUDO ip netns exec ns-container2 ip link set veth5 up
    SUDO ip netns exec ns-container1 ip link set lo up
    SUDO ip netns exec ns-container2 ip link set lo up

    log_info "underlay 网络配置完成"
    log_info "  ns-host1 VTEP: 172.16.0.1"
    log_info "  ns-host2 VTEP: 172.16.0.2"
}

# 创建 VXLAN 隧道
create_vxlan() {
    log_info "创建 VXLAN 隧道..."

    # 在 ns-host1 创建 VXLAN 设备
    SUDO ip netns exec ns-host1 ip link add vxlan0 type vxlan \
        id 100 \
        local 172.16.0.1 \
        remote 172.16.0.2 \
        dstport 4789 \
        dev veth1

    # 在 ns-host2 创建 VXLAN 设备
    SUDO ip netns exec ns-host2 ip link add vxlan0 type vxlan \
        id 100 \
        local 172.16.0.2 \
        remote 172.16.0.1 \
        dstport 4789 \
        dev veth2

    # 启动 VXLAN 接口
    SUDO ip netns exec ns-host1 ip link set vxlan0 up
    SUDO ip netns exec ns-host2 ip link set vxlan0 up

    log_info "VXLAN 隧道创建完成 (VNI: 100)"
}

# 创建 bridge 连接容器
create_bridges() {
    log_info "创建 bridge 连接容器..."

    # ns-host1: bridge + vxlan + container veth
    SUDO ip netns exec ns-host1 ip link add br0 type bridge
    SUDO ip netns exec ns-host1 ip link set vxlan0 master br0
    SUDO ip netns exec ns-host1 ip link set veth4 master br0
    SUDO ip netns exec ns-host1 ip link set br0 up

    # ns-host2: bridge + vxlan + container veth
    SUDO ip netns exec ns-host2 ip link add br0 type bridge
    SUDO ip netns exec ns-host2 ip link set vxlan0 master br0
    SUDO ip netns exec ns-host2 ip link set veth6 master br0
    SUDO ip netns exec ns-host2 ip link set br0 up

    log_info "bridge 配置完成"
}

# 显示网络配置信息
show_config() {
    echo ""
    echo "=============================================="
    echo "         VXLAN 实验环境配置完成"
    echo "=============================================="
    echo ""
    echo "网络拓扑:"
    echo ""
    echo "  ns-container1 (10.200.1.10)"
    echo "        |"
    echo "  ns-host1 --- VXLAN --- ns-host2"
    echo "  (172.16.0.1)  VNI 100  (172.16.0.2)"
    echo "        |                  |"
    echo "  ns-container2 (10.200.1.20)"
    echo ""
    echo "测试命令:"
    echo "  bash test.sh"
    echo ""
    echo "抓包分析:"
    echo "  SUDO tcpdump -i veth1 -n udp port 4789"
    echo ""
    echo "=============================================="
}

main() {
    cleanup_existing
    create_namespaces
    create_underlay
    create_vxlan
    create_bridges
    show_config

    log_info "环境搭建完成! 运行 bash test.sh 进行测试"
}

main "$@"
