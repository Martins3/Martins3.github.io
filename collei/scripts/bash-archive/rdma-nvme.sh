#!/usr/bin/env bash
set -E -e -u -o pipefail

target=nvme0n1

function setup_tcp_source() {
	# 创建 namespaces
	cd /sys/kernel/config/nvmet/subsystems
	sudo mkdir nvme-test-target
	cd nvme-test-target/
	echo 1 | sudo tee -a attr_allow_any_host >/dev/null
	sudo mkdir namespaces/1
	cd namespaces/1

	# 创建 target
	echo -n /dev/$target | sudo tee -a device_path >/dev/null
	echo 1 | sudo tee -a enable >/dev/null

	# 创建 target port 并且配置 ip
	sudo mkdir /sys/kernel/config/nvmet/ports/1
	cd /sys/kernel/config/nvmet/ports/1

	echo 10.0.0.2 | sudo tee -a addr_traddr >/dev/null
	echo tcp | sudo tee -a addr_trtype >/dev/null
	echo 4420 | sudo tee -a addr_trsvcid >/dev/null
	echo ipv4 | sudo tee -a addr_adrfam >/dev/null

	# 这个有作用吗?
	sudo ln -s /sys/kernel/config/nvmet/subsystems/nvme-test-target/ /sys/kernel/config/nvmet/ports/1/subsystems/nvme-test-target

	# 检查结果
	dmesg | grep "nvmet_tcp"
}

function setup_tcp_target() {
	sudo modprobe nvme
	sudo modprobe nvmet
	sudo modprobe nvmet-tcp
	sudo nvme discover -t tcp -a 10.0.0.2 -s 4420
	nvme connect -t tcp -n nvme-test-target -a 10.0.0.2 -s 4420
}

# https://docs.redhat.com/en/documentation/red_hat_enterprise_linux/8/html/managing_storage_devices/configuring-nvme-over-fabrics-using-nvme-rdma_managing-storage-devices
function setup_rdma_source() {
	sudo modprobe nvmet-rdma
	sudo mkdir /sys/kernel/config/nvmet/subsystems/testnqn

	cd /sys/kernel/config/nvmet/subsystems/testnqn
	# Allow any host to connect to this controller:
	echo 1 | sudo tee attr_allow_any_host
	# Configure a namespace
	sudo mkdir namespaces/10
	cd namespaces/10
	# 将 nvme0n1 配置到
	echo -n /dev/nvme0n1 | sudo tee device_path
	echo 1 | sudo tee enable
	# 打开端口
	sudo mkdir /sys/kernel/config/nvmet/ports/1
	cd /sys/kernel/config/nvmet/ports/1

	# 配置 rxe
	sudo rdma link add rxe_0 type rxe netdev ens5
	rdma link

	echo -n 10.10.101.0 | sudo tee addr_traddr
	echo rdma | sudo tee addr_trtype
	echo 4420 | sudo tee addr_trsvcid
	echo ipv4 | sudo tee addr_adrfam

	# 这个有用吗?
	sudo ln -s /sys/kernel/config/nvmet/subsystems/testnqn /sys/kernel/config/nvmet/ports/1/subsystems/testnqn

	# 验证结果:
	# dmesg | grep "enabling port"
	# [  338.636222] nvmet_rdma: enabling port 1 (10.10.101.0:4420)
}

function setup_rdma_with_nmcli() {
	# nvmetcli
	# 但是这个 rdma.json 从哪里获取呢？
	nvmetcli restore rdma.json
}

function setup_rdma_host() {
	sudo modprobe nvme-rdma
	sudo nvme discover -t rdma -a 10.10.101.0 -s 4420

	# Discovery Log Number of Records 1, Generation counter 1
	# =====Discovery Log Entry 0======
	# trtype:  rdma
	# adrfam:  ipv4
	# subtype: current discovery subsystem
	# treq:    not specified, sq flow control disable supported
	# portid:  1
	# trsvcid: 4420
	# subnqn:  nqn.2014-08.org.nvmexpress.discovery
	# traddr:  10.10.101.0
	# eflags:  none
	# rdma_prtype: not specified
	# rdma_qptype: connected
	# rdma_cms:    rdma-cm
	# rdma_pkey: 0000

	sudo nvme connect -t rdma -n testnqn -a 10.10.101.0 -s 4420
	# lsblk
	# cat /sys/class/nvme/nvme0/transport
	# nvme list
	# nvme disconnect -n testnqn
}
