#!/usr/bin/env bash
set -E -e -u -o pipefail

function cstate() {
	# 仅仅统计 CPU0 的
	for state in /sys/devices/system/cpu/cpu0/cpuidle/state*; do
		echo "$(basename "$state"): $(cat "$state"/name)"
		echo "  usage=$(cat "$state"/usage)"
		echo "  time=$(cat "$state"/time)us"
	done

	# 可以动态修改 cstate
	#	for cpu in /sys/devices/system/cpu/cpu*/cpuidle/state3; do
	#		echo 1 | sudo tee $cpu/disable >/dev/null
	#	done
}


function supported_cstate() {
	grep . /sys/devices/system/cpu/cpu0/cpuidle/state*/name

	# /sys/devices/system/cpu/cpu0/cpuidle/state0/name:POLL
	# /sys/devices/system/cpu/cpu0/cpuidle/state1/name:C1
	# /sys/devices/system/cpu/cpu0/cpuidle/state2/name:C1E
	# /sys/devices/system/cpu/cpu0/cpuidle/state3/name:C6
}

# cstate
#

cd /sys/devices/system/cpu/cpufreq/

# 基本上，每一个 CPU 都是
# .r--r--r--  4.1k root 16 Jan 16:17   affected_cpus
# .r--r--r--  4.1k root 16 Jan 16:17   base_frequency
# .r--r--r--  4.1k root 16 Jan 16:17   cpuinfo_avg_freq
# .r--r--r--  4.1k root 16 Jan 16:17   cpuinfo_max_freq
# .r--r--r--  4.1k root 16 Jan 16:17   cpuinfo_min_freq
# .r--r--r--  4.1k root 16 Jan 16:17   cpuinfo_transition_latency
# .r--r--r--  4.1k root 16 Jan 16:17   energy_performance_available_preferences
# .rw-r--r--  4.1k root 16 Jan 16:17   energy_performance_preference
# .r--r--r--  4.1k root 16 Jan 16:17   related_cpus
# .r--r--r--  4.1k root 16 Jan 16:17   scaling_available_governors
# .r--r--r--  4.1k root 16 Jan 16:17   scaling_cur_freq
# .r--r--r--  4.1k root 16 Jan 16:17   scaling_driver
# .rw-r--r--  4.1k root 16 Jan 16:17   scaling_governor
# .rw-r--r--  4.1k root 16 Jan 16:17   scaling_max_freq
# .rw-r--r--  4.1k root 16 Jan 16:17   scaling_min_freq
# .rw-r--r--  4.1k root 16 Jan 16:17   scaling_setspeed
