#!/usr/bin/env bash

# @todo 如何设置 page size
mount -t hugetlbfs -o min_size=100M,nr_inodes=100 none /mnt/huge
