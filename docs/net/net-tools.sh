#!/usr/bin/env bash
set -E -e -u -o pipefail

# TODO 用 nc 测试各种基本的网络内容
function netcat() {
	# 配合 code/module/c/udp_mini.c 使用，nc 可以将发送的 unicode 回显
	nc -u -l 11346

	# 发送消息到 uds 中
	nc -U /var/run/socket
}

function tcpdump() {
	echo ""
}
