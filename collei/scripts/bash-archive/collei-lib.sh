#!/usr/bin/env bash
set -E -e -u -o pipefail

PROGDIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." &>/dev/null && pwd)

# shellcheck source=code/const
source "$PROGDIR"/../code/const
# shellcheck source=code/lib.sh
source "$PROGDIR"/../code/lib.sh

ARCH=$(uname -m)
function get_ipv4_addr() {
	ip -4 addr show "$1" | grep -oP '(?<=inet\s)\d+(\.\d+){3}'
}

# https://stackoverflow.com/questions/8529181/which-terminal-command-to-get-just-ip-address-and-nothing-else
function get_master_ip() {
	if check_global_option vnc; then
		echo "$option_result"
		return
	fi
	local ovs_bridge=br-in
	local linux_bridge=br9527
	if ip link show dev $ovs_bridge &>/dev/null; then
		ip_addr=$(get_ipv4_addr $ovs_bridge)
	else
		ip_addr=$(get_ipv4_addr $linux_bridge)
	fi
	echo "$ip_addr"
}

function is_hypersivor() {
	if grep hypervisor /proc/cpuinfo &>/dev/null; then
		return 1
	fi
}

function is_container() {
	if env | grep container &>/dev/null; then
		return 0
	fi
	return 1
}

function setup_pueue() {
	# 如果无法使用 systemd 来启动 pueue
	# 还是使用 screen 这个方法最简单
	if ! pueue &>/dev/null; then
		screen -d -m pueued
		sleep 1
		pueue group add qemu &>/dev/null || true
	fi
}

function check_dep() {
	setup_pueue
	gum -h >/dev/null
}

global_config_dir=$HOME/.config/collei
function setup_global_config() {
	if [[ ! -d $global_config_dir ]]; then
		mkdir -p "$global_config_dir"
		cd "$HOME"/.config/collei
		echo "$VM_DIR"/iso >iso
		echo "$VM_DIR"/vm >vm
		echo "$WORKSTATION"/martins3 >last
		echo 10.0.0.2 >ip
		echo 10.0.0.2 >nbd
		echo 10.0.0.2 >vnc

		local level
		echo "VM level, level 0 means physical machine"
		level=$(gum choose 0 1 2 3)
		echo "$level" >level

		gum confirm "use default configuration?" && exit 0
		nvim
		exit 0
	fi
	check_global_option level
	host_level=$option_result
	check_global_option ip
	ovs_br_ip=$option_result
	check_global_option iso
	iso_repo=$option_result
	check_global_option vm
	all_vm_dir=$option_result
	check_global_option last
	vm_dir_symbol=$option_result
	if check_global_option nbd; then
		nbd_ip=$option_result
		log "$nbd_ip"
	fi
	if check_global_option bridge; then
		network_switch=$option_result
	else
		if which ovs-vsctl 2>/dev/null; then
			network_switch="ovs"
		else
			network_switch="bridge"
		fi
		# 一共三个选择
		{
			echo "# ovs"
			echo "# bridge"
			echo "# no"
			echo "$network_switch"
		} >"$global_config_dir"/bridge
	fi

	mkdir -p "$iso_repo"
	mkdir -p "$all_vm_dir"
	log "$ovs_br_ip"
	log "$host_level"
	log "$network_switch"
}

function show_help() {
	show_msg "$(awk "/\shelp begin/,/help end/" "$0")"
	exit 0
}

function show_msg() {
	gum style --foreground 212 \
		--border-foreground 212 \
		--border double \
		--margin "1 2" \
		--padding "2 4" \
		"$1"
}

qemu_pid=0
function get_qemu_pid() {
	vm=$1
	# 来特殊考虑 workaround_firecracker 的 pid 在根目录
	local pid_files=("$vm"/s "$vm"/t "$vm")
	for f in "${pid_files[@]}"; do
		if [[ -f "$f/pid" ]]; then
			local id
			id=$(cat "$f/pid")
			if [[ -f /proc/$id/status ]]; then
				qemu_pid=$id
				log "$qemu_pid"
				# 存在两个 pid 都是有效的情况，就是热迁移的时候
				# 只有 perf 才会考虑到这个情况，所以意义不大
				return 0
			fi
		fi
	done
	return 1
}

# If qemu killed out of sigkill, pidfile won't be removed automatically,
# check it once again
function is_vm_active() {
	vm=$1
	get_qemu_pid "$vm"
}

function check_option_internal() {
	local option_file=$1
	option_result=""
	if [[ ! -f $option_file ]]; then
		return 1
	fi
	# 读入的内容会被双引号包围，这导致 tailing white space 不会被自动去掉
	sed -i 's/[ \t]*$//' "$option_file"
	local content
	# 将 "#" 开头的，或者空行给删掉
	content=$(sed -e '/^[[:space:]]*#/d' -e '/^[[:space:]]*$/d' "$option_file")
	if [[ -z $content ]]; then
		return 1
	fi
	option_result="$content"

}

# 如果文件存在且不为空，执行成功
# 支持 # 来作为注释
# return : option_result
function check_option() {
	local option_file="$vm_dir/opt/$1"
	check_option_internal "$option_file"
}

function check_global_option() {
	local option_file="$global_config_dir/$1"
	if ! check_option_internal "$option_file"; then
		echo "$option_file is missing or invalid"
		return 1
	fi
}

replace_kernel=false
function setup_kernel() {
	local kernel
	if ! check_option kernel; then
		return
	fi
	kernel=$option_result
	if [[ $kernel != /* ]]; then
		error "use absolute path in 'kernel' : $kernel"
	fi
	kernel_dir=$kernel
	replace_kernel=true
	echo "$kernel_dir"
}

function setup_qemu() {
	qemu="qemu-system-$ARCH"
	qemu=${QEMU_DIR}/build/qemu-system-$ARCH
	# qemu=/usr/libexec/qemu-kvm
}

function supress_warning() {
	# 如果不去引用这几个变量，会警告的
	echo "qemu=$qemu"
	echo "replace_kernel=$replace_kernel"
	echo "kernel_dir=$kernel_dir"
}

function open_nvim() {
	if env | grep neovim >/dev/null; then
		error "Don't Run nvim in nvim"
	fi
	nvim
}

function debug() {
	echo "$1" >/dev/null
}

function check_name_validity() {
	if [[ $1 =~ [a-zA-Z0-9_]*$ ]]; then
		echo "$1: valid name"
	else
		echo "$1: invalid name"
		return 1
	fi
}

guest_id=""
function setup_guest_id() {
	guest_id=$(cat "$vm_dir/opt/id")
}

# ---- which qemu -----

# 不想让 s 和 t 到处都是
# 此外，get_qemu_pid 也会使用 s t 目录
which_qemu=""
live_qemu_count=0
# 提供 live_qemu_count 和 which_qemu
function setup_which_qemu() {
	if [[ -z $vm_dir ]]; then
		error "vm is not initiated"
	fi

	if [[ -n $which_qemu ]]; then
		return
	fi

	mkdir -p "$vm_dir/s"
	mkdir -p "$vm_dir/t"

	which_qemu=s
	for p in s t; do
		if [[ -f $vm_dir/$p/pid ]]; then
			if [[ -f /proc/$(cat "$vm_dir"/$p/pid)/status ]]; then
				live_qemu_count=$((live_qemu_count + 1))
				which_qemu=$p
			fi
		fi
	done
}

another_qemu=""
function get_another_qemu() {
	which=$1
	another_qemu=s
	if [[ $which == s ]]; then
		another_qemu=t
	fi
	log $another_qemu
}

qemu_index=
function get_qemu_index() {
	if [[ -z $which_qemu ]]; then
		error "which qemu is not determined"
	fi
	qemu_index=0
	if [[ $which_qemu == t ]]; then
		qemu_index=1
	fi
	log $qemu_index
}
# ---- which qemu -----

VM_PORT_RANGE=20
PORT_ALLOCATE_START=50000
PORT_ALLOCATE_END=60000
# port 总是动态调整的，其实还好
tcp_port=""
function get_tcp_port() {
	if [[ -z $guest_id ]]; then
		error "guest_id is not setup"
	fi
	opt=$1
	get_qemu_index
	offset=$qemu_index
	local base=$((PORT_ALLOCATE_START + guest_id * VM_PORT_RANGE))
	local ssh_base=4
	local disk_base=6
	case $opt in
		vnc)
			# qemu vnc 和 novnc 各自一个
			tcp_port=$((base + offset * 2))
			;;
		ssh)
			tcp_port=$((base + ssh_base + offset))
			;;
		ndb)
			local disk=${2}
			tcp_port=$((base + disk_base + disk))
			if ((base + disk_base + disk > PORT_ALLOCATE_END)); then
				error "too many vm"
			fi
			;;
		*) ;;
	esac
	log "tcp port : $tcp_port"
}

function migrate_form_port_guest_id() {
	shopt -u nullglob
	if ls "$all_vm_dir"/*/opt/port 2>/dev/null; then
		for i in "$all_vm_dir"/*/opt/port; do
			local port
			port=$(cat "$i")
			local id=$((port - 4000))
			local new_name=${i%%port}/id
			echo $id >"$new_name"
			rm "$i"
		done
	fi

	shopt -s nullglob
	for i in "$all_vm_dir"/*/opt/id; do
		opt="${i%%/id}"
		if [[ -f $opt/uuid ]]; then
			continue
		fi
		uuidgen >"$opt"/uuid

		# 将 pid 都移除掉，现在用新的接口
		vm="${i%%/opt}"
		rm -f "$vm"/pid
	done
}

function migrate_global_config_ip() {
	local ip
	ip=$(cat "$global_config_dir"/ip)
	if [[ $ip == 10.0* ]]; then
		return
	fi
	echo "10.0.$ip" >"$global_config_dir"/ip
}

function workaround_firecracker() {
	# 显然，我们只能处理一个 firecracker 的场景，不过已经够了
	if ! pgrep firecracker &>/dev/null; then
		return
	fi
	local target=""
	for i in "$all_vm_dir"/*/; do
		if [[ -f $i/opt/fire ]]; then
			if [[ -n $target ]]; then
				error "two firecracker found, update the script"
			fi
			target=$i
		fi
	done
	if [[ -n $target ]]; then
		if [[ ! -f /proc/$(cat "$target"/pid)/pid ]]; then
			pgrep firecracker >"$target"/pid
		fi
	fi

}

function check_vm_name() {
	if ! check_name_validity "$vm_name"; then
		echo "$vm_name is invalid"
		return 1
	fi

	local dir=$all_vm_dir/$vm_name
	if [[ -d $dir ]]; then
		echo "$vm_name is duplicated to $dir"
		return 2
	fi
	return
}

# parameter：备选名称
# return : new_vm_name（虚拟机名）
function get_vm_name() {
	local candidate=$1
	while true; do
		mkdir -p /tmp/martins3
		echo "$candidate" >/tmp/martins3/vm_name
		nvim /tmp/martins3/vm_name
		local vm_name
		vm_name="$(cat /tmp/martins3/vm_name)"
		vm_name=$(echo "$vm_name" | head -n1 | cut -d " " -f1)
		if ! check_vm_name; then
			if gum confirm "continue ?"; then
				continue
			else
				exit 1
			fi
		fi
		new_vm_name=$vm_name
		echo "$new_vm_name"
		break
	done
}

collei_lib_init=false
if [[ $collei_lib_init == false ]]; then
	collei_lib_init=true
	# check_vm_dir 使用的 vm_dir_symbol 是依赖 setup_vm_config 做初始化的
	setup_global_config
	check_dep
	migrate_form_port_guest_id
	migrate_global_config_ip
	workaround_firecracker
fi

function check_vm_dir() {
	# 如果不存在符号链接 || 如果符号链接不合法
	vm_dir="$(realpath "$vm_dir_symbol")"
	if [[ ! -L ${vm_dir_symbol} ]] || [[ ! -e ${vm_dir_symbol} ]]; then
		error "is $vm_dir a valid vm dir ?"
	fi
}

function get_new_guest_id() {
	local dir=$1
	local max_id

	# 必须关闭 nullglob 才可以正确的探测
	shopt -u nullglob
	if ls "$all_vm_dir"/*/opt/id; then
		max_id=$(cat "$all_vm_dir"/*/opt/id | sort -n | tail -1)
	else
		# 为什么不是 0 而是 10, 参考 setup_vsock
		max_id=10
	fi
	max_id=$((max_id + 1))
	mkdir -p "$dir/opt"
	echo $max_id >"$dir/opt/id"
}

function show_current_vm() {
	local current_vm_dir=$1
	local vm
	vm=$(basename "$current_vm_dir")
	get_qemu_index
	# 212 橙红
	# 150 绿色
	local color=212
	if [[ $qemu_index == 1 ]]; then
		color=112
	fi
	gum style \
		--foreground "$color" \
		--border-foreground "$color" \
		--border double \
		--align center \
		"$vm"
}

function change_file_owner() {
	local file=$1
	if [[ ! -e $file ]]; then
		return
	fi
	if [[ $(stat -c "%U" "$file") != martins3 ]]; then
		sudo chown martins3 "$file"
	fi

}

iso_path=""
function choose_iso() {
	shopt -s nullglob
	pushd "$iso_repo"
	files=(./*.iso)
	if [ ${#files[@]} -eq 0 ]; then
		error "[$iso_repo] is empty"
	fi
	iso_path=$(printf "%s\n" "${files[@]}" | fzf)
	log "$iso_path"
}
