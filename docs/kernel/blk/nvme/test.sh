#!/usr/bin/env bash
set -E -e -u -o pipefail

sudo fio --name=test --filename=/dev/nvme2n1p2 \
	--ioengine=io_uring --hipri --fixedbufs \
	--rw=randread --bs=4k --iodepth=300 \
	--direct=1 --time_based=1 --runtime=300 --numjobs=4 --thread --cpus_allowed=10,11,12,13

# sudo fio --name=test --filename=/dev/nvme2n1p2 \
# 	--ioengine=io_uring  --fixedbufs  \
# 	--rw=randread --bs=4k --iodepth=1 \
# 	--direct=1 --time_based=1 --runtime=300 --thread
