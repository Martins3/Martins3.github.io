#!/usr/bin/env bash

set -E -e -u -o pipefail

function gscp() {

	file_name=$1
	if [ -z "$file_name" ]; then
		echo "$0" file
		return 1
	fi

	# 参考 : https://stackoverflow.com/a/26694162/10449460
	if ip -4 addr show port-mgt &>/dev/null; then
		# 艰苦的环境中，仅仅使用 port-mgt 就可以了
		ip_addr=$(ip -4 addr show port-mgt | grep -oP '(?<=inet\s)\d+(\.\d+){3}')
	else

		readarray -t addr_list < <(
			ip -4 -j a | jq -r '.[] | select(.ifname != "lo") | select(.operstate != "DOWN") | .addr_info[] | .local'
		)
		local ip_num=${#addr_list[@]}
		if [[ $ip_num -eq 0 ]]; then
			echo "no valid ip found!"
			return 1
		elif [[ $ip_num -eq 1 ]]; then
			ip_addr=${addr_list[0]}
		else
			ip_addr=$(printf "%s\n" "${addr_list[@]}" | fzf)
		fi
	fi

	file_path=$(readlink -f "$file_name")
	echo "scp -r $(whoami)@${ip_addr}:$file_path ."
}

if [[ $# -eq 0 ]]; then
	cat "$0"
	exit 0
fi
gscp "$1"
