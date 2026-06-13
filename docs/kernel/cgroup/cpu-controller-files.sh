#!/usr/bin/env bash
set -E -e -u -o pipefail

readonly SCOPE_ENV=CGROUP_CPU_FILES_IN_SCOPE

function rerun_in_delegated_scope() {
	if [[ ${!SCOPE_ENV:-0} == "1" ]]; then
		return
	fi

	exec systemd-run --user --scope -p Delegate=yes --quiet \
		env "${SCOPE_ENV}=1" bash "$0"
}

function current_cgroup_dir() {
	local cgroup_path

	cgroup_path=$(sed -n 's|0::||p' /proc/self/cgroup)
	printf '/sys/fs/cgroup%s\n' "${cgroup_path}"
}

function pick_cpu() {
	sed -n \
		's/^Cpus_allowed_list:[[:space:]]*\([0-9][0-9]*\).*/\1/p' \
		/proc/self/status
}

function read_stat_field() {
	local file="$1"
	local key="$2"

	awk -v key="${key}" '$1 == key { print $2 }' "${file}"
}

function read_pressure_total() {
	local file="$1"

	awk '
		$1 == "some" {
			for (i = 1; i <= NF; i++) {
				if ($i ~ /^total=/) {
					split($i, a, "=")
					print a[2]
				}
			}
		}
	' "${file}"
}

function new_cgroup() {
	local path="$1"

	mkdir "${path}"
	printf '%s\n' "${path}"
}

function enable_cpu_controller() {
	local path="$1"

	printf '+cpu\n' >"${path}/cgroup.subtree_control"
}

function move_pid() {
	local path="$1"
	local pid="$2"

	printf '%s\n' "${pid}" >"${path}/cgroup.procs"
}

function spawn_busy_loop() {
	local cpu="$1"
	local path="$2"

	# 创建 subshell
	# subshell 先把自己的 $BASHPID 写入目标 cgroup.procs，完成自我迁移
	# 然后 exec 替换为 stress-ng，worker 进程自然继承正确的 cgroup
	(
		printf '%s\n' "$BASHPID" >"${path}/cgroup.procs"
		exec taskset -c "${cpu}" stress-ng --cpu 1 --quiet
	) &
	SPAWNED_PID=$!
}

function kill_and_wait() {
	local pid

	for pid in "$@"; do
		kill "${pid}"
	done

	for pid in "$@"; do
		wait "${pid}" || true
	done
}

function print_cpu_files() {
	local path="$1"

	find "${path}" -maxdepth 1 -mindepth 1 -printf '%f\n' | rg '^cpu(\.|$)' || true
}

function setup_tree() {
	local parent
	local anchor

	parent=$(current_cgroup_dir)
	anchor="${parent}/anchor"
	CGROUP_ROOT="${parent}"

	new_cgroup "${anchor}"
	move_pid "${anchor}" "$$"

	enable_cpu_controller "${parent}"
}

function case_path() {
	local name="$1"

	printf '%s/cpu-file-demo-%s\n' "${CGROUP_ROOT}" "${name}"
}

function demo_files() {
	local sample

	sample=$(case_path sample)
	new_cgroup "${sample}"

	echo '[files]'
	echo "scope=${CGROUP_ROOT}"
	echo "展示一共存在那些文件"
	print_cpu_files "${sample}"
	echo
}

function demo_weight_and_nice() {
	local low
	local high
	local low_pid
	local high_pid
	local low_usage
	local high_usage

	low=$(case_path weight-low)
	high=$(case_path weight-high)
	new_cgroup "${low}"
	new_cgroup "${high}"

	printf '100\n' >"${low}/cpu.weight"
	printf '%s\n' '-5' >"${high}/cpu.weight.nice"

	spawn_busy_loop "${CPU_ID}" "${low}"
	low_pid="${SPAWNED_PID}"
	spawn_busy_loop "${CPU_ID}" "${high}"
	high_pid="${SPAWNED_PID}"

	sleep 3

	kill_and_wait "${low_pid}" "${high_pid}"

	low_usage=$(read_stat_field "${low}/cpu.stat" usage_usec)
	high_usage=$(read_stat_field "${high}/cpu.stat" usage_usec)

	echo '[weight-vs-nice]'
	echo "cpu.weight 和 cpu.weight.nice 是一个含义的两个接口:"
	echo "low.weight=$(cat "${low}/cpu.weight")"
	echo "low.weight.nice=$(cat "${low}/cpu.weight.nice")"
	echo "high.weight=$(cat "${high}/cpu.weight")"
	echo "high.weight.nice=$(cat "${high}/cpu.weight.nice")"
	echo "对比运行时间:"
	echo "low.usage_usec=${low_usage}"
	echo "high.usage_usec=${high_usage}"
	echo
}

function demo_idle() {
	local normal
	local idle
	local normal_pid
	local idle_pid

	normal=$(case_path idle-normal)
	idle=$(case_path idle-idle)
	new_cgroup "${normal}"
	new_cgroup "${idle}"

	printf '1\n' >"${idle}/cpu.idle"

	spawn_busy_loop "${CPU_ID}" "${normal}"
	normal_pid="${SPAWNED_PID}"
	spawn_busy_loop "${CPU_ID}" "${idle}"
	idle_pid="${SPAWNED_PID}"

	sleep 3

	kill_and_wait "${normal_pid}" "${idle_pid}"

	echo '[idle]'
	echo "展示如果 cpu.idle=1 后的结果"
	echo "idle/cpu.flag=$(cat "${idle}/cpu.idle")"
	echo "idle/cpu.weight=$(cat "${idle}/cpu.weight")"
	echo "idle/cpuweight.nice=$(cat "${idle}/cpu.weight.nice")"
	echo "idle/cpu.usage_usec=$(read_stat_field "${idle}/cpu.stat" usage_usec)"
	echo "normal/usage_usec=$(read_stat_field "${normal}/cpu.stat" usage_usec)"
	echo
}

function run_quota_case() {
	local path="$1"
	local burst="$2"
	local pid

	printf '20000 100000\n' >"${path}/cpu.max"
	printf '%s\n' "${burst}" >"${path}/cpu.max.burst"

	spawn_busy_loop "${CPU_ID}" "${path}"
	pid="${SPAWNED_PID}"

	sleep 3

	kill_and_wait "${pid}"
}

function demo_max_and_burst() {
	local no_burst
	local burst

	no_burst=$(case_path quota-no-burst)
	burst=$(case_path quota-burst)
	new_cgroup "${no_burst}"
	new_cgroup "${burst}"

	run_quota_case "${no_burst}" 0
	run_quota_case "${burst}" 20000

	echo '[max-vs-burst]'
	echo "no-burst.cpu.max=$(cat "${no_burst}/cpu.max")"
	echo "no-burst.cpu.max.burst=$(cat "${no_burst}/cpu.max.burst")"
	echo "no-burst.usage_usec=$(read_stat_field "${no_burst}/cpu.stat" usage_usec)"
	echo "no-burst.nr_throttled=$(read_stat_field "${no_burst}/cpu.stat" nr_throttled)"
	echo "no-burst.nr_bursts=$(read_stat_field "${no_burst}/cpu.stat" nr_bursts)"
	echo "no-burst.burst_usec=$(read_stat_field "${no_burst}/cpu.stat" burst_usec)"

	echo "burst.cpu.max=$(cat "${burst}/cpu.max")"
	echo "burst.cpu.max.burst=$(cat "${burst}/cpu.max.burst")"
	echo "burst.usage_usec=$(read_stat_field "${burst}/cpu.stat" usage_usec)"
	echo "burst.nr_throttled=$(read_stat_field "${burst}/cpu.stat" nr_throttled)"
	echo "burst.nr_bursts=$(read_stat_field "${burst}/cpu.stat" nr_bursts)"
	echo "burst.burst_usec=$(read_stat_field "${burst}/cpu.stat" burst_usec)"
	echo
}

function demo_stat_and_stat_local() {
	local stat_case
	local pid

	stat_case=$(case_path stat)
	new_cgroup "${stat_case}"

	printf '20000 100000\n' >"${stat_case}/cpu.max"

	spawn_busy_loop "${CPU_ID}" "${stat_case}"
	pid="${SPAWNED_PID}"

	sleep 3

	kill_and_wait "${pid}"

	echo '[stat-vs-stat-local]'
	echo "usage_usec=$(read_stat_field "${stat_case}/cpu.stat" usage_usec)"
	echo "nr_periods=$(read_stat_field "${stat_case}/cpu.stat" nr_periods)"
	echo "nr_throttled=$(read_stat_field "${stat_case}/cpu.stat" nr_throttled)"
	echo "throttled_usec=$(read_stat_field "${stat_case}/cpu.stat" throttled_usec)"
	echo "local.throttled_usec=$(read_stat_field "${stat_case}/cpu.stat.local" throttled_usec)"
	echo
}

function demo_pressure() {
	local pressure_case
	local pid1
	local pid2
	local before_total
	local after_total

	pressure_case=$(case_path pressure)
	new_cgroup "${pressure_case}"

	before_total=$(read_pressure_total "${pressure_case}/cpu.pressure")

	spawn_busy_loop "${CPU_ID}" "${pressure_case}"
	pid1="${SPAWNED_PID}"
	spawn_busy_loop "${CPU_ID}" "${pressure_case}"
	pid2="${SPAWNED_PID}"

	sleep 3

	kill_and_wait "${pid1}" "${pid2}"

	after_total=$(read_pressure_total "${pressure_case}/cpu.pressure")

	echo '[pressure]'
	echo "cpu.pressure.some.total.before=${before_total}"
	echo "cpu.pressure.some.total.after=${after_total}"
	echo "cpu.pressure.some.total.delta=$((after_total - before_total))"
	echo
	cat "${pressure_case}/cpu.pressure"
	echo
}

function demo_uclamp() {
	local clamp_case

	clamp_case=$(case_path uclamp)
	new_cgroup "${clamp_case}"

	printf '25.50\n' >"${clamp_case}/cpu.uclamp.min"
	printf '60.00\n' >"${clamp_case}/cpu.uclamp.max"

	echo '[uclamp]'
	echo "cpu.uclamp.min=$(cat "${clamp_case}/cpu.uclamp.min")"
	echo "cpu.uclamp.max=$(cat "${clamp_case}/cpu.uclamp.max")"
	echo
}

function main() {
	rerun_in_delegated_scope

	CPU_ID=$(pick_cpu)
	setup_tree

	echo "[env] kernel=$(uname -r)"
	echo "[env] cpu=${CPU_ID}"
	echo

	# demo_files
	# demo_weight_and_nice
	# demo_idle
	demo_max_and_burst
	# demo_stat_and_stat_local
	# demo_pressure
	# demo_uclamp
}

main "$@"
