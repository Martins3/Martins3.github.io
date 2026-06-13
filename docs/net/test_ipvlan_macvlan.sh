#!/usr/bin/env bash
# ipvlan/macvlan 功能测试脚本
# 测试各种模式和通信场景

set -E -e -u -o pipefail

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# 测试配置
TEST_NETNS_PREFIX="test"
PHY_DEV="${1:-eth0}"
IPVLAN_IP1="10.99.1.1"
IPVLAN_IP2="10.99.1.2"
MACVLAN_IP1="10.99.2.1"
MACVLAN_IP2="10.99.2.2"
NETMASK="24"

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 清理函数
cleanup() {
    log_info "清理测试环境..."
    
    # 删除 netns
    for ns in $(ip netns list | grep "^${TEST_NETNS_PREFIX}" | awk '{print $1}'); do
        sudo ip netns del "$ns" 2>/dev/null || true
    done
    
    # 删除可能残留的虚拟接口
    for dev in $(ip link show | grep -E "ipvl|macv" | awk -F: '{print $2}' | tr -d ' '); do
        sudo ip link del "$dev" 2>/dev/null || true
    done
    
    log_info "清理完成"
}

# 测试 ipvlan L2 模式
test_ipvlan_l2() {
    log_info "=== 测试 ipvlan L2 模式 ==="
    
    local ns1="${TEST_NETNS_PREFIX}-ipvl-l2-1"
    local ns2="${TEST_NETNS_PREFIX}-ipvl-l2-2"
    
    # 创建 netns
    sudo ip netns add "$ns1"
    sudo ip netns add "$ns2"
    
    # 创建 ipvlan L2 模式接口
    sudo ip link add link "$PHY_DEV" name ipvl-l2-1 type ipvlan mode l2
    sudo ip link add link "$PHY_DEV" name ipvl-l2-2 type ipvlan mode l2
    
    # 移动到 netns
    sudo ip link set ipvl-l2-1 netns "$ns1"
    sudo ip link set ipvl-l2-2 netns "$ns2"
    
    # 配置 IP
    sudo ip netns exec "$ns1" ip addr add "${IPVLAN_IP1}/${NETMASK}" dev ipvl-l2-1
    sudo ip netns exec "$ns1" ip link set ipvl-l2-1 up
    sudo ip netns exec "$ns1" ip link set lo up
    
    sudo ip netns exec "$ns2" ip addr add "${IPVLAN_IP2}/${NETMASK}" dev ipvl-l2-2
    sudo ip netns exec "$ns2" ip link set ipvl-l2-2 up
    sudo ip netns exec "$ns2" ip link set lo up
    
    # 显示配置
    log_info "ipvlan L2 接口配置:"
    sudo ip netns exec "$ns1" ip addr show ipvl-l2-1
    sudo ip netns exec "$ns2" ip addr show ipvl-l2-2
    
    # 测试连通性
    log_info "测试 ${ns1} -> ${ns2} 连通性..."
    if sudo ip netns exec "$ns1" ping -c 3 -W 2 "$IPVLAN_IP2" > /dev/null 2>&1; then
        log_info "✓ L2 模式互通测试通过"
    else
        log_warn "✗ L2 模式互通测试失败（可能需要外部交换机支持）"
    fi
    
    # 清理
    sudo ip netns del "$ns1"
    sudo ip netns del "$ns2"
}

# 测试 ipvlan L3 模式
test_ipvlan_l3() {
    log_info "=== 测试 ipvlan L3 模式 ==="
    
    local ns1="${TEST_NETNS_PREFIX}-ipvl-l3-1"
    local ns2="${TEST_NETNS_PREFIX}-ipvl-l3-2"
    
    sudo ip netns add "$ns1"
    sudo ip netns add "$ns2"
    
    sudo ip link add link "$PHY_DEV" name ipvl-l3-1 type ipvlan mode l3
    sudo ip link add link "$PHY_DEV" name ipvl-l3-2 type ipvlan mode l3
    
    sudo ip link set ipvl-l3-1 netns "$ns1"
    sudo ip link set ipvl-l3-2 netns "$ns2"
    
    sudo ip netns exec "$ns1" ip addr add "${IPVLAN_IP1}/${NETMASK}" dev ipvl-l3-1
    sudo ip netns exec "$ns1" ip link set ipvl-l3-1 up
    sudo ip netns exec "$ns1" ip link set lo up
    
    sudo ip netns exec "$ns2" ip addr add "${IPVLAN_IP2}/${NETMASK}" dev ipvl-l3-2
    sudo ip netns exec "$ns2" ip link set ipvl-l3-2 up
    sudo ip netns exec "$ns2" ip link set lo up
    
    log_info "ipvlan L3 接口配置 (注意: 共享 MAC 地址):"
    sudo ip netns exec "$ns1" ip addr show ipvl-l3-1
    sudo ip netns exec "$ns2" ip addr show ipvl-l3-2
    
    # L3 模式下，同子网通信可能有问题（ARP 不工作）
    log_info "测试 ${ns1} -> ${ns2} 连通性 (L3 模式 ARP 被禁用)..."
    if sudo ip netns exec "$ns1" ping -c 3 -W 2 "$IPVLAN_IP2" > /dev/null 2>&1; then
        log_info "✓ L3 模式互通测试通过"
    else
        log_warn "✗ L3 模式互通测试失败 (预期行为，需要路由配置)"
    fi
    
    sudo ip netns del "$ns1"
    sudo ip netns del "$ns2"
}

# 测试 macvlan bridge 模式
test_macvlan_bridge() {
    log_info "=== 测试 macvlan Bridge 模式 ==="
    
    local ns1="${TEST_NETNS_PREFIX}-macv-br-1"
    local ns2="${TEST_NETNS_PREFIX}-macv-br-2"
    
    sudo ip netns add "$ns1"
    sudo ip netns add "$ns2"
    
    sudo ip link add link "$PHY_DEV" name macv-br-1 type macvlan mode bridge
    sudo ip link add link "$PHY_DEV" name macv-br-2 type macvlan mode bridge
    
    sudo ip link set macv-br-1 netns "$ns1"
    sudo ip link set macv-br-2 netns "$ns2"
    
    sudo ip netns exec "$ns1" ip addr add "${MACVLAN_IP1}/${NETMASK}" dev macv-br-1
    sudo ip netns exec "$ns1" ip link set macv-br-1 up
    sudo ip netns exec "$ns1" ip link set lo up
    
    sudo ip netns exec "$ns2" ip addr add "${MACVLAN_IP2}/${NETMASK}" dev macv-br-2
    sudo ip netns exec "$ns2" ip link set macv-br-2 up
    sudo ip netns exec "$ns2" ip link set lo up
    
    log_info "macvlan Bridge 接口配置 (独立 MAC 地址):"
    sudo ip netns exec "$ns1" ip addr show macv-br-1
    sudo ip netns exec "$ns2" ip addr show macv-br-2
    
    log_info "测试 ${ns1} -> ${ns2} 连通性..."
    if sudo ip netns exec "$ns1" ping -c 3 -W 2 "$MACVLAN_IP2" > /dev/null 2>&1; then
        log_info "✓ Bridge 模式互通测试通过"
    else
        log_warn "✗ Bridge 模式互通测试失败"
    fi
    
    sudo ip netns del "$ns1"
    sudo ip netns del "$ns2"
}

# 测试 macvlan private 模式
test_macvlan_private() {
    log_info "=== 测试 macvlan Private 模式 ==="
    
    local ns1="${TEST_NETNS_PREFIX}-macv-prv-1"
    local ns2="${TEST_NETNS_PREFIX}-macv-prv-2"
    
    sudo ip netns add "$ns1"
    sudo ip netns add "$ns2"
    
    sudo ip link add link "$PHY_DEV" name macv-prv-1 type macvlan mode private
    sudo ip link add link "$PHY_DEV" name macv-prv-2 type macvlan mode private
    
    sudo ip link set macv-prv-1 netns "$ns1"
    sudo ip link set macv-prv-2 netns "$ns2"
    
    sudo ip netns exec "$ns1" ip addr add "${MACVLAN_IP1}/${NETMASK}" dev macv-prv-1
    sudo ip netns exec "$ns1" ip link set macv-prv-1 up
    sudo ip netns exec "$ns1" ip link set lo up
    
    sudo ip netns exec "$ns2" ip addr add "${MACVLAN_IP2}/${NETMASK}" dev macv-prv-2
    sudo ip netns exec "$ns2" ip link set macv-prv-2 up
    sudo ip netns exec "$ns2" ip link set lo up
    
    log_info "macvlan Private 接口配置:"
    sudo ip netns exec "$ns1" ip addr show macv-prv-1
    sudo ip netns exec "$ns2" ip addr show macv-prv-2
    
    log_info "测试 ${ns1} -> ${ns2} 连通性 (Private 模式应该失败)..."
    if sudo ip netns exec "$ns1" ping -c 3 -W 2 "$MACVLAN_IP2" > /dev/null 2>&1; then
        log_warn "✗ Private 模式互通测试通过（意外，应该有隔离）"
    else
        log_info "✓ Private 模式隔离生效（预期行为）"
    fi
    
    sudo ip netns del "$ns1"
    sudo ip netns del "$ns2"
}

# 显示内核模块信息
show_module_info() {
    log_info "=== 内核模块信息 ==="
    
    if lsmod | grep -q ipvlan; then
        log_info "ipvlan 模块: 已加载"
    else
        log_info "ipvlan 模块: 尝试加载..."
        sudo modprobe ipvlan 2>/dev/null || log_warn "ipvlan 加载失败或 builtin"
    fi
    
    if lsmod | grep -q macvlan; then
        log_info "macvlan 模块: 已加载"
    else
        log_info "macvlan 模块: 尝试加载..."
        sudo modprobe macvlan 2>/dev/null || log_warn "macvlan 加载失败或 builtin"
    fi
    
    # 显示支持的模式
    log_info "支持的 ipvlan 模式: l2, l3, l3s"
    log_info "支持的 macvlan 模式: private, vepa, bridge, passthru, source"
}

# 主函数
main() {
    echo "=========================================="
    echo "ipvlan/macvlan 功能测试"
    echo "物理网卡: $PHY_DEV"
    echo "=========================================="
    
    # 检查 root 权限
    if [[ $EUID -ne 0 ]]; then
        log_error "需要 root 权限运行"
        exit 1
    fi
    
    # 检查物理网卡
    if ! ip link show "$PHY_DEV" > /dev/null 2>&1; then
        log_error "物理网卡 $PHY_DEV 不存在"
        log_info "可用网卡:"
        ip link show | grep -E "^[0-9]+:" | awk -F: '{print $2}'
        exit 1
    fi
    
    # 设置清理陷阱
    trap cleanup EXIT
    
    # 先清理可能残留的环境
    cleanup
    
    # 显示模块信息
    show_module_info
    
    echo ""
    
    # 运行测试
    test_ipvlan_l2
    echo ""
    
    test_ipvlan_l3
    echo ""
    
    test_macvlan_bridge
    echo ""
    
    test_macvlan_private
    echo ""
    
    log_info "所有测试完成"
}

# 处理命令行参数
case "${1:-}" in
    -h|--help)
        echo "用法: $0 [物理网卡名]"
        echo "默认使用 eth0"
        exit 0
        ;;
    *)
        main
        ;;
esac
