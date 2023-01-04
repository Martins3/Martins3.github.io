#!/usr/bin/env bash
sudo cgcreate -g cpuset:testset
sudo cgset -r cpuset.cpus=3 testset
sudo cgset -r cpuset.mems=0 testset
sudo cgexec -g cpuset:testset stress-ng --vm-bytes 6500M --vm-keep --vm 3
