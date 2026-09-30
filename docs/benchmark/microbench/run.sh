#!/usr/bin/env bash
set -E -e -u -o pipefail

cd -- "$(dirname -- "${BASH_SOURCE[0]}")"
make -B
mkdir -p results
result_dir=$(mktemp -d "results/$(date +%Y%m%d-%H%M%S).XXXXXX")

function record_environment() {
	date --iso-8601=seconds
	uname -a
	/usr/bin/g++ --version
	lscpu
	lscpu -e=CPU,CORE,SOCKET,NODE,ONLINE
	free -h
	cat /proc/loadavg
	cat /proc/cmdline
	cat /proc/self/status | rg 'Cpus_allowed_list|Mems_allowed_list'
	for path in /sys/devices/system/cpu/cpu0/cpufreq/scaling_governor \
		/sys/devices/system/cpu/smt/active /proc/sys/kernel/perf_event_paranoid \
		/sys/devices/system/cpu/cpu0/cache/index*/size \
		/sys/devices/system/cpu/cpu0/cache/index*/coherency_line_size \
		/sys/kernel/mm/transparent_hugepage/enabled; do
		echo "$path"
		if [[ -r $path ]]; then
			cat "$path"
		else
			echo unavailable
		fi
	done
	sha256sum Makefile run.sh microbench.cpp microbench.out
}

record_environment >"$result_dir/environment.txt"
./microbench.out >"$result_dir/samples.csv" 2>"$result_dir/run.log"
cat /proc/loadavg >"$result_dir/load-after.txt"
echo "$result_dir"
