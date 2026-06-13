#!/usr/bin/env bash
set -E -e -u -o pipefail

PID=$1
pushd /sys/kernel/mm/damon/admin/kdamonds/
function on() {
	# echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds'
	# echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/nr_contexts'
	# echo '1' > '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions'
	# echo 'on' >'/sys/kernel/mm/damon/admin/kdamonds/0/state'

	echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds'
	echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/nr_contexts'
	echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets'
	echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions'

	tree

}

function setup() {
	echo 'vaddr' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/operations'
	echo '5000' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/sample_us'
	echo '1000000' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/aggr_us'
	echo '10000000' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us'
	echo '10' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/min'
	echo '1000' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/max'

	# kernel 6.6
	# echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/access_bp'
	# echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/aggrs'
	# echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/min_sample_us'
	# echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/monitoring_attrs/intervals/intervals_goal/max_sample_us'


	cat '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets'
	echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets'
	# 跟踪 pid
	echo "$PID" >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/pid_target'

	cat '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions'
	# 默认监控整个虚拟地址空间
	echo 0 >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions'
	# echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/start'
	# echo '0xfffffffff' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/0/end'

	cat '/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes'

	# 必须设置 access_pattern ，不然就不会显式 tried_regions 了
	echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes'
	echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/sz/min'
	echo '18446744073709551615' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/sz/max'
	echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/nr_accesses/min'
	echo '3689348814741910528' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/nr_accesses/max'
	echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/age/min'
	echo '184467440737095' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/age/max'

	echo 'stat' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/action'
	# kernel 6.6
	# echo '5000' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/apply_interval_us'

	echo 'on' >'/sys/kernel/mm/damon/admin/kdamonds/0/state'
	echo 'commit' >'/sys/kernel/mm/damon/admin/kdamonds/0/state'
	echo 'update_schemes_stats' >'/sys/kernel/mm/damon/admin/kdamonds/0/state'
	# kernel 6.6
	# echo 'update_tuned_intervals' >'/sys/kernel/mm/damon/admin/kdamonds/0/state'
	echo 'update_schemes_tried_regions' >'/sys/kernel/mm/damon/admin/kdamonds/0/state'
}

function show() {
	pushd /sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/tried_regions/
	for d in ./*/; do
		start=$(printf "%x\n" "$(cat "$d"/start)")
		end=$(printf "%x\n" "$(cat "$d"/end)")
		[ -L "${d%/}" ] && continue
		echo "$d $(cat "$d"/nr_accesses) $(cat "$d"/age) $start $end"
	done

	echo 'update_schemes_tried_regions' >'/sys/kernel/mm/damon/admin/kdamonds/0/state'
	popd
}

function clear() {
	# tree
	cat '/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds'
	if [[ $(cat '/sys/kernel/mm/damon/admin/kdamonds/0/state') != off ]]; then
		echo 'off' >'/sys/kernel/mm/damon/admin/kdamonds/0/state'
	fi
	echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds'
}

trap clear EXIT

on
setup
VAR=1000
for ((i = 0; i < VAR; i = i + 1)); do
	printf '%s\n' "$i"
	show
	sleep 1
done
clear
