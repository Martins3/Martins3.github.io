#!/usr/bin/env bash
set -E -e -u -o pipefail

echo 1 | sudo tee /sys/kernel/hacking/epoll
gcc epoll-user.c -o epoll.out
echo "在另外一个终端里面写入 /dev/amsg"
sudo chown martins3 /dev/amsg 
./epoll.out
