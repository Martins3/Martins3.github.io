#!/usr/bin/env bash
set -E -e -u -o pipefail
# TODO 这里有一个问题，有这么多的 ccp 设备，当 create 的时候，
# 如何知道在哪一个环境中

function get_all_cpp_devices() {
	# 05:00.2 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. PSPCCP Command DMA Processor
	# 06:00.1 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. NTBCCP
	# 24:00.2 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. PSPCCP Command DMA Processor
	# 25:00.1 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. NTBCCP
	# 43:00.2 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. PSPCCP Command DMA Processor
	# 44:00.1 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. NTBCCP
	# 63:00.2 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. PSPCCP Command DMA Processor
	# 64:00.1 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. NTBCCP
	# 85:00.2 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. PSPCCP Command DMA Processor
	# 86:00.1 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. NTBCCP
	# a4:00.2 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. PSPCCP Command DMA Processor
	# a5:00.1 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. NTBCCP
	# c1:00.2 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. PSPCCP Command DMA Processor
	# c2:00.1 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. NTBCCP
	# e1:00.2 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. PSPCCP Command DMA Processor
	# e2:00.1 Encryption controller: Chengdu Haiguang IC Design Co., Ltd. NTBCCP
	lspci | grep "Encryption controller: Chengdu Haiguang" | awk '{print $1}'
}

function bind_to_hct() {
	local devices
	mapfile -t devices < <(get_all_cpp_devices)
	local device_count=${#devices[@]}

	if [[ ! -d /sys/devices/virtual/hct ]]; then
		sudo modprobe hct
	fi

	if [ "$device_count" -le 1 ]; then
		echo "Only $device_count device(s) found, no binding needed"
		return
	fi

	echo "Found $device_count Encryption controller devices"

	# Keep the first device unbound, bind all others to hct
	for ((i = 1; i < ${#devices[@]}; i++)); do
		local device=${devices[$i]}
		local bdf="0000:${device}"
		local device_path="/sys/bus/pci/devices/$bdf"
		local driver_path="/sys/bus/pci/devices/$bdf/driver"
		local module

		echo "$device_path"
		echo "$driver_path"
		if [[ -f $driver_path/unbind ]]; then
			module=$(basename "$(realpath "$driver_path/module")")
			if [[ $module != hct ]]; then
				echo "$bdf" | sudo tee "$driver_path"/unbind
			fi
		fi

		set -x
		# 告诉设备可以绑定到 driver 上
		echo hct | sudo tee "$device_path/driver_override"
		# 然后才可以进行绑定
		echo "$bdf" | sudo tee /sys/bus/pci/drivers/hct/bind
		set +x
	done

	echo "Binding complete - left ${devices[0]} unbound"
}

bind_to_hct
