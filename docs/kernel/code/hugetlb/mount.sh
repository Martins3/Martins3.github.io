#!/usr/bin/env bash

set -x

gcc -g mapfile.c
mkdir -p /mnt/huge
umount /mnt/huge
mount -t hugetlbfs -o nr_inodes=1000 none /mnt/huge
./a.out
