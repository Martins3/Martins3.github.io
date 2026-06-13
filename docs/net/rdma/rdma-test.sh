#!/usr/bin/env bash
# RDMA 测试脚本 - ConnectX-5 网卡

set -E -u -o pipefail

DEV_NAME="mlx5_0"
IFACE="enp0s6np0"
PEER_IP="10.0.3.2"

echo "=== RDMA 网卡状态检查 ==="
echo ""

# 检查设备是否存在
if [[ ! -d "/sys/class/infiniband/${DEV_NAME}" ]]; then
	echo "[FAIL] RDMA 设备 ${DEV_NAME} 不存在"
	exit 1
fi
echo "[OK] RDMA 设备 ${DEV_NAME} 存在"

# 检查端口状态
PORT_STATE_RAW=$(cat /sys/class/infiniband/${DEV_NAME}/ports/1/state 2>/dev/null)
if [[ ${PORT_STATE_RAW} == *"4: ACTIVE"* ]]; then
	echo "[OK] 端口状态: ACTIVE"
else
	echo "[WARN] 端口状态: ${PORT_STATE_RAW}"
fi

# 检查物理链路状态
PHY_STATE_RAW=$(cat /sys/class/infiniband/${DEV_NAME}/ports/1/phys_state 2>/dev/null)
if [[ ${PHY_STATE_RAW} == *"5: LinkUp"* ]]; then
	echo "[OK] 物理链路: LinkUp"
else
	echo "[WARN] 物理链路: ${PHY_STATE_RAW}"
fi

# 检查网卡 IP
IP_ADDR=$(ip -4 addr show ${IFACE} 2>/dev/null | grep -oP '(?<=inet\s)\d+(\.\d+){3}' | head -1)
if [[ -n ${IP_ADDR} ]]; then
	echo "[OK] IPv4 地址: ${IP_ADDR}"
else
	echo "[TODO] 需要配置 IPv4 地址"
	echo "       运行: sudo ip addr add 10.0.3.1/30 dev ${IFACE}"
fi

# 检查对端连通性
if ping -c 1 -W 2 ${PEER_IP} &>/dev/null; then
	echo "[OK] 对端 ${PEER_IP} 可达"
else
	echo "[INFO] 对端 ${PEER_IP} 未连通"
fi

echo ""
echo "=== 设备信息 ==="
echo "设备: ${DEV_NAME}"
echo "网卡: ${IFACE}"
echo "MAC: $(cat /sys/class/net/${IFACE}/address 2>/dev/null || echo 'N/A')"
echo "驱动: $(ethtool -i ${IFACE} 2>/dev/null | grep driver | awk '{print $2}' || echo 'N/A')"
echo "固件: $(cat /sys/class/infiniband/${DEV_NAME}/fw_ver 2>/dev/null || echo 'N/A')"

echo ""
echo "=== RDMA 详细信息 ==="
if command -v ibstat &>/dev/null; then
	ibstat
else
	echo "[WARN] ibstat 未安装"
fi

echo ""
echo "=== 可用测试工具 ==="
for tool in ib_write_bw ib_read_bw ib_send_bw ib_write_lat ib_read_lat ib_send_lat rdma; do
	if command -v ${tool} &>/dev/null; then
		echo "[OK] ${tool}"
	else
		echo "[MISSING] ${tool}"
	fi
done

echo ""
echo "=== 下一步 ==="
if [[ -z ${IP_ADDR} ]]; then
	echo "1. 配置 IP: sudo ip addr add 10.0.3.1/30 dev ${IFACE}"
	echo "2. 配置对端 IP: 10.0.3.2/30"
	echo "3. 运行测试: ib_write_bw -d ${DEV_NAME}"
else
	echo "RDMA 已就绪，可以连接对端进行测试"
	echo ""
	echo "服务端（本机）:"
	echo "  ib_write_bw -d ${DEV_NAME}"
	echo ""
	echo "客户端（对端 ${PEER_IP}）:"
	echo "  ib_write_bw -d mlx5_0 ${IP_ADDR}"
fi
