#!/usr/bin/env bash

echo "swtich to sudo mode with sudo -i"
cd /sys/fs/cgroup/devices
mkdir -p cgroup_test_group # -p so we can run 
ls -l /dev/tty # just show that tty device number
cd cgroup_test_group
echo "c 5:0 w" > devices.deny
echo "make sure cgroup_test_script.sh is already running, press enter to continue"
read TMP_VAR_FOR_ME
echo $(pgrep -f  cgroup_test_script.sh) > /sys/fs/cgroup/devices/cgroup_test_group/tasks
