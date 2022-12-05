#!/usr/bin/env bash
cgcreate -g cpuset:testset
cgset -r cpuset.cpus=3 testset
cgset -r cpuset.mems=0 testset
cgexec -g cpuset:testset ls
