#!/usr/bin/env bash
set -E -e -u -o pipefail


if [ $# -ne 1 ]; then
	echo "用法: $0 <目标进程 PID>"
	exit 1
fi

PID=$1
DAMON_SYSFS="/sys/kernel/mm/damon/admin"
MONITOR_DURATION=3

if [ ! -d "$DAMON_SYSFS" ]; then
	echo "错误: DAMON sysfs 接口不可用。..."
	exit 1
fi

if ! ps -p "$PID" >/dev/null; then
	echo "错误: PID $PID 不存在。"
	exit 1
fi

# 初始化 DAMON 配置
setup_damon() {
	echo "正在配置 DAMON 监控..."

	echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/nr_kdamonds'
	echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/nr_contexts'
	echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/nr_targets'
	echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/targets/0/regions/nr_regions'

	echo 'vaddr' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/operations'
	echo 5000 >"$DAMON_SYSFS/kdamonds/0/contexts/0/monitoring_attrs/intervals/sample_us"
	echo 100000 >"$DAMON_SYSFS/kdamonds/0/contexts/0/monitoring_attrs/intervals/aggr_us"
	echo 1000000 >"$DAMON_SYSFS/kdamonds/0/contexts/0/monitoring_attrs/intervals/update_us"

	echo 10 >"$DAMON_SYSFS/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/min"
	echo 1000 >"$DAMON_SYSFS/kdamonds/0/contexts/0/monitoring_attrs/nr_regions/max"

	# 设置目标进程
	echo 1 >"$DAMON_SYSFS/kdamonds/0/contexts/0/targets/nr_targets"
	echo "$PID" >"$DAMON_SYSFS/kdamonds/0/contexts/0/targets/0/pid_target"

	# 设置监控区域（默认监控整个虚拟地址空间）
	echo 0 >"$DAMON_SYSFS/kdamonds/0/contexts/0/targets/0/regions/nr_regions"
}

# 启动 DAMON 监控
start_monitoring() {
	echo '1' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/nr_schemes'

	# magic
	echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/sz/min'
	echo '18446744073709551615' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/sz/max'
	echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/nr_accesses/min'
	echo '3689348814741910528' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/nr_accesses/max'
	echo '0' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/age/min'
	echo '184467440737095' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/access_pattern/age/max'

	echo 'stat' >'/sys/kernel/mm/damon/admin/kdamonds/0/contexts/0/schemes/0/action'

	echo "启动 DAMON 监控 PID $PID..."
	echo on >"$DAMON_SYSFS/kdamonds/0/state" || {
		echo "错误: 无法启动 DAMON 监控。"
		exit 1
	}

	echo commit >"$DAMON_SYSFS/kdamonds/0/state" || {
		echo "错误: 无法启动 DAMON 监控。"
		exit 1
	}
	# TODO 这两个东西到底是做什么的
	# echo 'update_tuned_intervals' > '/sys/kernel/mm/damon/admin/kdamonds/0/state'
	if [ "$(cat "$DAMON_SYSFS/kdamonds/0/state")" != "on" ]; then
		echo "错误: DAMON 监控未能启动。"
		exit 1
	fi
	echo 'update_schemes_stats' >'/sys/kernel/mm/damon/admin/kdamonds/0/state'
	echo update_schemes_tried_regions >$DAMON_SYSFS/kdamonds/0/state
}

# 获取并显示热页和冷页
report_access() {
	echo "更新监控结果..."
	echo update_schemes_tried_regions >"$DAMON_SYSFS/kdamonds/0/state" || {
		echo "错误: 无法更新 tried_regions 数据。"
		return 1
	}
	sleep 1 # 等待数据更新

	echo "内存访问报告（热页和冷页）："
	TOTAL_BYTES=$(cat "$DAMON_SYSFS/kdamonds/0/contexts/0/schemes/0/tried_regions/total_bytes" 2>/dev/null || echo 0)
	echo "总监控区域大小：$TOTAL_BYTES 字节"
	echo "-----------------------------------------"

	HOT_THRESHOLD=5  # 热页访问次数阈值
	COLD_THRESHOLD=1 # 冷页访问次数阈值
	COLD_AGE=20      # 冷页年龄阈值

	HOT_PAGES=()
	COLD_PAGES=()

	for dir in "$DAMON_SYSFS/kdamonds/0/contexts/0/schemes/0/tried_regions"/*; do
		if [[ -d $dir && $dir != *"total_bytes"* ]]; then
			REGION=$(basename "$dir")
			START=$(cat "$dir/start" 2>/dev/null || echo 0)
			END=$(cat "$dir/end" 2>/dev/null || echo 0)
			NR_ACCESSES=$(cat "$dir/nr_accesses" 2>/dev/null || echo 0)
			AGE=$(cat "$dir/age" 2>/dev/null || echo 0)
			SIZE=$((END - START))

			START_HEX=$(printf "0x%x" "$START")
			END_HEX=$(printf "0x%x" "$END")

			if [ "$NR_ACCESSES" -ge "$HOT_THRESHOLD" ]; then
				HOT_PAGES+=("Region $REGION: Start=$START_HEX, End=$END_HEX, Size=$SIZE bytes, Accesses=$NR_ACCESSES, Age=$AGE")
			elif [ "$NR_ACCESSES" -le "$COLD_THRESHOLD" ] || [ "$AGE" -ge "$COLD_AGE" ]; then
				COLD_PAGES+=("Region $REGION: Start=$START_HEX, End=$END_HEX, Size=$SIZE bytes, Accesses=$NR_ACCESSES, Age=$AGE")
			fi
		fi
	done

	echo "热页（访问次数 >= $HOT_THRESHOLD）："
	if [ ${#HOT_PAGES[@]} -eq 0 ]; then
		echo "  无热页"
	else
		for page in "${HOT_PAGES[@]}"; do
			echo "  $page"
		done
	fi
	echo "-----------------------------------------"

	echo "冷页（访问次数 <= $COLD_THRESHOLD 或年龄 >= $COLD_AGE）："
	if [ ${#COLD_PAGES[@]} -eq 0 ]; then
		echo "  无冷页"
	else
		for page in "${COLD_PAGES[@]}"; do
			echo "  $page"
		done
	fi
}

# 清理 DAMON 配置
cleanup() {
	echo "停止 DAMON 监控并清理..."
	if [ -f "$DAMON_SYSFS/kdamonds/0/state" ]; then
		echo off >"$DAMON_SYSFS/kdamonds/0/state" 2>/dev/null
		# TODO 这个如何理解?
		echo clear_schemes_tried_regions >"$DAMON_SYSFS/kdamonds/0/state" 2>/dev/null || echo "警告: 无法清除 tried_regions，可能已无数据。"
	fi
	if [ -f "$DAMON_SYSFS/kdamonds/nr_kdamonds" ]; then
		echo 0 >"$DAMON_SYSFS/kdamonds/nr_kdamonds" 2>/dev/null || echo "警告: 无法销毁 kdamond。"
	fi
}

# 捕获中断信号以确保清理
trap cleanup EXIT

# 主逻辑
main() {
	cleanup
	setup_damon
	start_monitoring
	echo "监控运行中，持续 $MONITOR_DURATION 秒..."
	sleep "$MONITOR_DURATION"
	while true; do
		report_access || echo "警告: 获取监控结果失败，可能是进程无内存活动或配置错误。"
	done
}

main
