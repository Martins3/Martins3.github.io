#!/usr/bin/env bash
set -E -e -u -o pipefail

trace_dir=/sys/kernel/tracing/instances/suspend-study
function restore_settings() {
	echo 0 >"$trace_dir/tracing_on"
	echo none >/sys/power/pm_test
	echo 0 >/sys/power/freeze_filesystems
}
trap restore_settings EXIT
for stage in freezer devices platform processors core; do
	echo "$stage" >/sys/power/pm_test

	echo >"$trace_dir/trace"
	echo 1 >"$trace_dir/tracing_on"
	echo mem >/sys/power/state
	echo 0 >"$trace_dir/tracing_on"
	cat "$trace_dir/trace" >"/tmp/suspend-results/$stage.trace"

	sha256sum -c /tmp/suspend-results/payload.sha256
	dd if=/dev/zero of=/tmp/suspend-data/post-resume.bin bs=1M count=1 conv=fsync status=none
	echo "PASS pm_test=$stage"
done

echo 1 >/sys/power/freeze_filesystems
echo freezer >/sys/power/pm_test
echo >"$trace_dir/trace"
echo 1 >"$trace_dir/tracing_on"
echo mem >/sys/power/state
echo 0 >"$trace_dir/tracing_on"
cat "$trace_dir/trace" >/tmp/suspend-results/freezer-filesystems.trace
echo 'PASS pm_test=freezer freeze_filesystems=1'
dmesg >/tmp/suspend-results/stages.dmesg
