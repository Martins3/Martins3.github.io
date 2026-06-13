#!/usr/bin/env bash
set -E -e -u -o pipefail

# 还是使用陪着文件是最方便的
#
# endpoint 是对端的 ip
function create_config() {
	address=$1
	address2=$2
	mkdir -p /etc/wireguard
	umask 077
	wg genkey >wg0.key
	wg pubkey <wg0.key >wg0.pub

	# Address 是 wg0 的 ip 地址，Endpoint 是对面的 ip 的
	cat <<_EOF_ >/etc/wireguard/wg0.conf
	[Interface]
	PrivateKey = $(cat wg0.key)
	ListenPort = 51000
	Address = 10.1.0.$address/24

	[Peer]
	PublicKey = xxxxxx
	Endpoint = 10.0.0.$address2:51000
	AllowedIPs = 10.1.0.0/24
_EOF_
	cat wg0.pub
}

# 最后
# sudo wg-quick up wg0
