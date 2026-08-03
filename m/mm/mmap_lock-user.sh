#!/usr/bin/env bash
set -E -e -u -o pipefail

../mod.sh -r
gcc mmap_lock-user.c -o mmap_lock-user.out
sudo ./mmap_lock-user.out
