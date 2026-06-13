#!/usr/bin/env bash
set -E -e -u -o pipefail

# https://www.wireguard.com/quickstart/
function setup_one() {
	addresss=$1
	umask 077
	wg genkey >private
	wg pubkey <private
	ip link add wg0 type wireguard
	ip addr add 10.1.0."$addresss"/24 dev wg0
	wg set wg0 private-key ./private
	ip link set wg0 up
	wg set wg0 listen-port 10000
	wg # 检查结果
	# interface: wg0
	#   public key: zjOkhwrJgHbYOCvcnlv8RmlnysZeu05ZPfqYf0KDbGI=
	#   private key: (hidden)
	#   listening port: 10000
	#
	# interface: wg0
	#   public key: OFrAkEPTCn/U+QYU9s8E/K3GgNLmfu8H6h58g91cZSE=
	#   private key: (hidden)
	#   listening port: 10000
	#
	# wg set wg0 peer yX6nVKs8Az5o0aaIVu/6GBtuzerlqrmJjeBbBDkJZyU= allowed-ips 0.0.0.0/0 endpoint 10.0.0.109:10000
}

function add_peer() {
	peer_pub_key=$(gum input --placeholder "peer pub key")
	endpoint_ip=$(gum input --placeholder "endpoint ip")
	set -x
	wg set wg0 peer "$peer_pub_key" allowed-ips 10.1.0.0/24 endpoint "$endpoint_ip":10000
}

# 检查端口是否生效
# nc -ul 10000
# nc -u 192.168.1.2 10000
