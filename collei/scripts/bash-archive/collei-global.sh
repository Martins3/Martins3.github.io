#!/usr/bin/env bash
set -E -e -u -o pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
PROGDIR=$(readlink -m "$SCRIPT_DIR/..")
pushd "$SCRIPT_DIR" >/dev/null
# shellcheck source=code/const
source ../../code/const
# shellcheck source=collei/scripts/collei-lib.sh
source ./collei-lib.sh
popd >/dev/null

function qemu_top() {
	if ! pgrep qemu &>/dev/null; then
		error "no qemu process found"
	fi
	mapfile -t pids < <(pgrep qemu)
	top "${pids[@]/#/-p }"
}

function config_edit() {
	cd "$HOME"/.config/collei
	open_nvim
}

function config_show() {
	if [[ ! -d $global_config_dir ]]; then
		error "global config not setup"
	fi

	readarray -t parent < <(find "$global_config_dir" -type f)
	# printf "%s\n" "${parent[@]}"
	{

		for file in "${parent[@]}"; do
			filename=$(basename "$file")
			# 读取文件内容（假设内容短小）
			content=$(tr '\n' ' ' <"$file" | head -c 50) # 限制长度为 50 字符，避免太长
			echo -e "$filename,$content"
		done

	} \
		| gum table -s "," -c "Option" -c "Content" --widths "20,50" --height 1 -p --border="rounded"

}

function dashboard() {
	"$PROGDIR"/dashboard.py
}

function kill_all_qemu() {
	live_vms=()
	readarray -d '' dirs_array < <(find "$all_vm_dir" -maxdepth 1 -type d -print0)

	for i in "${dirs_array[@]}"; do
		if is_vm_active "$i"; then
			live_vms+=("$i")
		fi

	done
	if [ ${#live_vms[@]} -eq 0 ]; then
		echo "🐖"
		exit 0
	fi
	for i in "${live_vms[@]}"; do
		echo "$i"
	done
	if gum confirm "Kill these machine?"; then
		for i in "${live_vms[@]}"; do
			set -x
			get_qemu_pid "$i"
			kill -9 "$qemu_pid"
			set +x
		done
	fi
}

# 这个捕获完全随缘啊
function trace_qemu() {
	setup_qemu
	local which
	which=$(objdump -t "$qemu" | fzf)
	# which="00000000009fc200 g     F .text  00000000000000ce              qemu_poll_ns"
	which=$(echo "$which" | awk '{print $NF}')
	# echo "$which"
	set -x
	sudo bpftrace -e "uprobe:$qemu:$which { @[ustack] = count(); }"
}

function clear_tun() {
	# 其实一般也没有必要
	# sudo ovs-vsctl del-br br-in
	for d in /sys/class/net/*; do
		# echo "$d"
		vb=$(basename -- "$d")
		if [[ $vb =~ vif[[:digit:]]+.[[:digit:]] ]]; then
			echo "$vb"
			echo "sudo ip link delete $vb"
		fi
	done
}

# 将 ovs 上不存在的设备全部都去掉
function clear_ovs_config() {
	# 这种设备被永远的数据库中，时间长了，里面的东西很多
	# sudo ovs-vsctl show
	#
	# 4899c619-b0fc-41cc-b085-657f2ef88a25
	#     Bridge br-in
	#         Port vif93.2
	#             Interface vif93.2
	#                 error: "could not open network device vif93.2 (No such device)"
	readarray -t array < <(sudo ovs-vsctl list-ports br-in)
	for i in "${array[@]}"; do
		echo "$i"
		if sudo ovs-vsctl show | grep "$i" | grep "No such device"; then
			sudo ovs-vsctl del-port "$i"
		fi
	done
}

function clear_hugetlb() {
	# 那么如果有的 hugetlb 没有 fault ，直接清零所有的 hugetlb ，会导致虚拟机 fault 新的 page 的时候出现 sigbus 吗?
	for d in /sys/kernel/mm/hugepages/hugepages-*/nr_hugepages; do
		echo 0 | sudo tee "$d"
	done
}

function clean() {
	# 将不用的 tun 设备去掉
	# 将 pueue 中不用的去掉
	clear_ovs_config
	# clear_tun
	# pueue kill -a
	pueue clean

	echo "clean"

}

# 在回头看看这个吧:
# https://github.com/yoctoproject/poky/blob/master/scripts/runqemu-ifup
# 参考一下 firecracker 中的 docs/getting-started.md
#
# 之前参考这个，失败了:
# https://www.spad.uk/posts/really-simple-network-bridging-with-qemu/
function setup_nat() {
	# net/netfilter.md
	# 错误的
	# sudo ip route add to 10.0.2.15 dev br-in
	# sudo iptables -A POSTROUTING -t nat -j MASQUERADE -s 10.0.2.15/32
	# sudo iptables -A POSTROUTING -t nat -j MASQUERADE -s 10.0.101.0/32
	# echo 1 | sudo tee /proc/sys/net/ipv4/ip_forward
	# echo 1 | sudo tee /proc/sys/net/ipv4/conf/br-in/proxy_arp
	# sudo iptables -P FORWARD ACCEPT

	# 正确的，测试通过的
	#
	# 物理机中配置:
	# 这两个设备似乎可以是机器上的任何两个网卡，例如两个 bridge
	wifi=wlan0
	vb=br-in # virtual bridge
	# sudo iptables -t nat -F
	# 添加 NAT 规则，将 10.0.0.0/16 的流量通过 wlo1 伪装
	sudo iptables -t nat -A POSTROUTING -s 10.0.0.0/16 -o $wifi -j MASQUERADE
	# 添加转发规则
	sudo iptables -A FORWARD -i $vb -o $wifi -j ACCEPT
	sudo iptables -A FORWARD -i $wifi -o $vb -m state --state RELATED,ESTABLISHED -j ACCEPT

	# 虚拟机中配置:
	# sudo ip route delete default
	# sudo ip route add default via 10.0.0.2
	# ping 8.8.8.8
	# 然后处理 DNS ，如果需要
}

# 找到那些连接了网线的，可以切分的网卡
function sriov() {
	for d in /sys/class/net/*/device; do
		t=${d##*net/}
		nic=${t%%/*}
		driver=$(basename "$(realpath "$d"/driver/module)")
		# vf 下面没有这些设备
		if [[ ! -f "$d/sriov_totalvfs" ]]; then
			continue
		fi

		speed=$(ethtool "$nic" | grep Speed)
		if [[ $speed == *Unknown* ]]; then
			continue
		fi
		echo "$nic"
		echo "$driver"
		echo -e "$speed" | sed 's/\t//'

		# 输出 sysfs 的各种接口
		cd "$d"
		metric=$(grep . sriov_*)
		echo -e "$metric" | sed 's/^/\t/'
		echo " "

		# 输出 vf 的对应关系
		have_vf=false
		for v in "$d"/virtfn*/; do
			id=$(basename "$(realpath "$v"/iommu_group)")
			if lsof /dev/vfio/"$id" &>/dev/null; then
				echo "	$(basename "$v") $(basename "$(realpath "$v")") **"
			else
				echo "	$(basename "$v") $(basename "$(realpath "$v")")"
			fi
			have_vf=true
		done

		# 展示各种 vf 相关内容
		if [[ $have_vf == true ]]; then
			ip link show "$nic"
		fi
		echo ""
	done

}

# ai 写的，专门处理 xe 驱动的 intel 显卡
function intel_gpu() {
	for d in /sys/class/drm/card[0-9]/device; do
		card=${d##*drm/}
		card=${card%%/*}
		# 跳过 VF，只展示 PF
		if [[ -L $d/physfn ]]; then
			continue
		fi
		vendor=$(cat "$d"/vendor 2>/dev/null || echo "")
		if [[ $vendor != "0x8086" ]]; then
			continue
		fi

		driver=$(basename "$(realpath "$d"/driver/module 2>/dev/null)" 2>/dev/null || echo "unknown")
		device=$(cat "$d"/device 2>/dev/null || echo "")
		revision=$(cat "$d"/revision 2>/dev/null || echo "")
		echo "$card"
		echo "$driver"
		echo "vendor=$vendor device=$device revision=$revision"

		if [[ ! -f $d/sriov_totalvfs ]]; then
			echo ""
			continue
		fi

		cd "$d"
		echo "$d/sriov_numvfs"
		metric=$(grep . sriov_* 2>/dev/null || true)
		echo -e "$metric" | sed 's/^/\t/'
		echo " "

		have_vf=false
		for v in "$d"/virtfn*/; do
			[[ ! -d $v ]] && continue
			vf_pci=$(basename "$(realpath "$v")")
			iommu_group=$(basename "$(realpath "$v"/iommu_group 2>/dev/null)" 2>/dev/null || echo "")
			if [[ -n $iommu_group ]] && lsof /dev/vfio/"$iommu_group" &>/dev/null; then
				echo "	$(basename "$v") $vf_pci **"
			else
				echo "	$(basename "$v") $vf_pci"
			fi
			have_vf=true
		done

		if [[ $have_vf == true ]] && [[ -d $d/sriov_admin ]]; then
			echo "	sriov_admin:"
			find "$d"/sriov_admin -mindepth 1 -maxdepth 1 | sort | sed 's|.*/||' | sed 's/^/\t\t/'
		fi
		echo ""
	done
}

function check_env() {
	cd "$all_vm_dir"

	echo "当前一共存在如下虚拟机:"
	for d in "$(pwd)"/*; do
		[[ ! -d $d ]] && continue
		# 仅仅统计运行过的机器
		# 正在安装的机器没有 cmd.sh ，跳过
		[[ ! -f $d/cmd.sh ]] && continue
		basename "$d"
	done
}

declare -A all_actions
all_actions=(
	# 第一个是 action，第二个是函数
	["top"]="qemu_top"
	["dashboard"]="dashboard"
	["check_env"]="check_env"
	["config_show"]="config_show"
	["config_edit"]="config_edit"
	["kill_all_qemu"]="kill_all_qemu"
	["clean"]="clean"
	["nat"]="setup_nat"
	["trace"]="trace_qemu"
	["sriov"]="sriov"
	["intel_gpu"]="intel_gpu"
	["clear_hugetlb"]="clear_hugetlb"
)

cmd_action="none"
while getopts "ha:" opt; do
	case $opt in
		# help begin
		h) show_help ;;
		a) cmd_action=$OPTARG ;;
		*) show_help ;;
			# help end
	esac
done
shift $((OPTIND - 1))

if [[ $cmd_action == "none" ]]; then
	cmd_action=$(printf "%s\n" "${!all_actions[@]}" | fzf)
fi

"${all_actions[$cmd_action]}"
