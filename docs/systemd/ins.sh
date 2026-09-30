#!/usr/bin/env bash
set -E -e -u -o pipefail
# 1. 有办法自动配置 oneshot 或者 simple 吗? 无论程序是长期运行或者一下运行的
# 2. 真的需要 sh -c 来执行吗?

function install_service() {
	local path_to_cmd=$1
	if [[ ! -e $path_to_cmd ]]; then
		echo "$path_to_cmd is not a executable"
		exit 1
	fi

	if [[ $path_to_cmd != *.sh ]]; then
		echo "$path_to_cmd is not a shell script"
		exit 1
	fi

	local cmd
	cmd=$(realpath "$path_to_cmd")

	# 获取 shell 文件的名称作为 service 的名称
	local service
	service=$(basename -- "$cmd")
	service="${service%.*}"
	service+="_martins3"

	cat <<EOF >/tmp/"$service".service
[Unit]
Description=$service

[Service]
Type=simple
ExecStart=sh -c "$cmd"

[Install]
WantedBy=multi-user.target
EOF
	sudo cp /tmp/"$service".service /etc/systemd/system/"$service".service
	sudo systemctl enable "$service"
	sudo systemctl start "$service"
}

# ExecStart=mount -t virtiofs myfs $share_path
# Type=simple
# ExecStart=/bin/bash -c "/home/martins3/.nix-profile/bin/qemu-system-x86_64 --nographic -M pc,accel=kvm"
# ExecStart=/bin/bash -c "/usr/libexec/qemu-kvm --nographic -M pc,accel=kvm"

function list() {
	for service in $(systemctl list-unit-files '*_martins3*' --no-legend | awk '{print $1}'); do
		echo "=== $service ==="
		systemctl status "$service" --no-pager
		echo
	done
}

function remove() {
	for service in $(systemctl list-unit-files '*_martins3*' --no-legend | awk '{print $1}'); do
		echo "=== $service ==="
		sudo systemctl disable "$service" --no-pager
		sudo systemctl stop "$service" --no-pager
		echo
	done
}

cmd=""
while getopts "hi:r" opt; do
	case $opt in
		i)
			cmd=${OPTARG}
			install_service "$cmd"
			;;
		r)
			remove
			;;
		h) echo "-i instal" ;;
		*)
			exit 1
			;;
	esac
done
shift $((OPTIND - 1))

list
