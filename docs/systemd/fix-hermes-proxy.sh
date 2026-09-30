#!/usr/bin/env bash

# 学到了，给 sytemd 继续打补丁的方法
set -E -e -u -o pipefail

readonly service="hermes-gateway.service"
readonly proxy_url="http://127.0.0.1:7890"
readonly drop_in_dir="${HOME}/.config/systemd/user/${service}.d"
readonly drop_in_file="${drop_in_dir}/proxy.conf"

function install_proxy_drop_in() {
	mkdir -p "${drop_in_dir}"

	tee "${drop_in_file}" <<EOF
[Service]
Environment="HTTP_PROXY=${proxy_url}"
Environment="HTTPS_PROXY=${proxy_url}"
Environment="http_proxy=${proxy_url}"
Environment="https_proxy=${proxy_url}"
Environment="NO_PROXY=127.0.0.1,localhost,::1"
Environment="no_proxy=127.0.0.1,localhost,::1"
EOF
}

function restart_gateway() {
	systemctl --user daemon-reload
	systemd-analyze --user verify "${service}"
	systemctl --user restart "${service}"
	systemctl --user is-active "${service}"
}

function verify_gateway_environment() {
	local main_pid

	main_pid="$(systemctl --user show --property=MainPID --value "${service}")"
	if [[ ${main_pid} == "0" ]]; then
		echo "${service} has no running process" >&2
		return 1
	fi

	echo "Gateway PID: ${main_pid}"
	tr '\0' '\n' <"/proc/${main_pid}/environ" \
		| grep -E '^(HTTP_PROXY|HTTPS_PROXY|http_proxy|https_proxy|NO_PROXY|no_proxy)='
}

install_proxy_drop_in
restart_gateway
verify_gateway_environment

echo "Installed: ${drop_in_file}"
