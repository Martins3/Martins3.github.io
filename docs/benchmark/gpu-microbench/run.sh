#!/usr/bin/env bash
set -E -e -u -o pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
make -B
mkdir -p results
result_dir=$(mktemp -d "results/$(date +%Y%m%d-%H%M%S).XXXXXX")

function print_if_readable() {
	local path=$1
	echo "$path"
	if [[ -r $path ]]; then
		cat "$path"
	else
		echo unavailable
	fi
}

function record_environment() {
	date --iso-8601=seconds
	uname -a
	systemd-detect-virt
	/usr/local/cuda-13.1/bin/nvcc --version
	/usr/bin/g++ --version
	nvidia-smi
	nvidia-smi -q
	nvidia-smi topo -m
	lspci -nn
	cat /proc/loadavg
	cat /proc/cmdline
	print_if_readable /sys/kernel/mm/transparent_hugepage/enabled
	print_if_readable /proc/sys/kernel/perf_event_paranoid
	echo "iommu groups under /sys/kernel/iommu_groups"
	find /sys/kernel/iommu_groups -maxdepth 1 -mindepth 1
	dmesg | grep -iE 'IOMMU|vfio'
	# Guest and host see different PCIe link state for a passed-through GPU, so
	# both the sysfs view and the driver's own view are recorded.
	local path dir
	for path in /sys/bus/pci/devices/*/vendor; do
		dir=${path%/vendor}
		if [[ $(cat "$path") == "0x10de" ]]; then
			echo "$dir is an NVIDIA function"
			print_if_readable "$dir/current_link_speed"
			print_if_readable "$dir/current_link_width"
			print_if_readable "$dir/max_link_speed"
			print_if_readable "$dir/max_link_width"
			readlink -f "$dir/driver"
		fi
	done
	sha256sum Makefile run.sh bench.h bench.cu grp_*.cu gpubench.out
}

record_environment >"$result_dir/environment.txt"
./gpubench.out >"$result_dir/samples.csv" 2>"$result_dir/run.log"
cat /proc/loadavg >"$result_dir/load-after.txt"
echo "$result_dir"
