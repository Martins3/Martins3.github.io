#!/bin/bash
# https://lore.kernel.org/linux-mm/fee37e75-e818-46b0-8494-684ef3eb5cd4@lucifer.local/T/#m21e331d32ce035b5d68d77421929f5b4eef527ca
#
# 按道理 6.18.5-100.fc42.x86_64 没有合并这个，我还是观察到的是
# grep memory /proc/cgroups
#	memory  0       109     1


# Create a temporary file 'temp' filled with zero bytes
dd if=/dev/zero of=temp bs=4096 count=1

# Display memory-cgroup info from /proc/cgroups
cat /proc/cgroups | grep memory

for i in {0..2000}
do
    mkdir /sys/fs/cgroup/memory/test$i
    echo $$ > /sys/fs/cgroup/memory/test$i/cgroup.procs

    # Append 'temp' file content to 'log'
    cat temp >> log

    echo $$ > /sys/fs/cgroup/memory/cgroup.procs

    # Potentially create a dying memory cgroup
    rmdir /sys/fs/cgroup/memory/test$i
done

# Display memory-cgroup info after test
cat /proc/cgroups | grep memory

rm -f temp log
