#!/usr/bin/env bash

dir=devices_test
echo "swtich to sudo mode with sudo -i"
cd /sys/fs/cgroup/devices || exit 1
mkdir -p $dir  # -p so we can run
ls -l /dev/tty # just show that tty device number
cd $dir || exit 1
echo "c 5:0 w" >devices.deny
echo "make sure device_test_script.sh is already running, press enter to continue"
read -r TMP_VAR_FOR_ME || exit 1
echo "$TMP_VAR_FOR_ME"
pgrep -f device_test_script.sh >/sys/fs/cgroup/devices/$dir/tasks
