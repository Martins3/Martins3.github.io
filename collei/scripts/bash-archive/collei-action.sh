#!/usr/bin/env bash
set -E -e -u -o pipefail

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
pushd "$SCRIPT_DIR" >/dev/null
# shellcheck source=collei/scripts/collei-net.sh
source ./collei-net.sh
# shellcheck source=collei/scripts/vfio.sh
source ./vfio.sh
popd >/dev/null

PROGDIR=$(readlink -m "$SCRIPT_DIR/..")

function cmd_edit() {
	cd "$vm_dir"
	open_nvim
}

function set_default() {
	local choose=$1
	rm -f "$vm_dir_symbol"
	ln -s "$choose" "$vm_dir_symbol"
}

function clone_vm() {
	set -x
	cd "$all_vm_dir"
	local origin=$1
	local name=$2
	local new_vm_dir="$all_vm_dir/$name"
	local real_vm_dir="$all_vm_dir/$origin"

	local clear_vm_info=false
	if is_vm_active "$vm_dir"; then
		gum confirm "vm is alive, be careful" || exit 0
		clear_vm_info=true
	fi

	echo "cp -r $real_vm_dir $new_vm_dir"
	cp -r "$real_vm_dir" "$new_vm_dir"

	get_new_guest_id "$new_vm_dir"
	uuidgen >"$new_vm_dir/opt/uuid"

	# 如果 clone 一个正在运行的虚拟机，需要清理掉 pid ，新虚拟机会被误判
	# 是正在运行的，也许还有其他的资源需要清理
	if [[ $clear_vm_info == true ]]; then
		rm "$new_vm_dir"/*/pid
	fi

	echo "default vm is $(basename "$new_vm_dir")"
	set_default "$new_vm_dir"
	exit 0
}

function cmd_clone_vm() {
	get_vm_name "$(basename "$vm_dir")"
	clone_vm "$(basename "$vm_dir")" "$new_vm_name"
}

# 非交互式克隆虚拟机
# 用法: collei-action.sh -a clone_vm_auto [新虚拟机名称]
# 必须提供名称
function clone_vm_auto() {
	local new_vm_name="${1}"
	clone_vm "$(basename "$vm_dir")" "$new_vm_name"
}

function hmp() {
	echo "$1" \
		| socat - unix-connect:"$vm_dir/$which_qemu"/hmp
}

function qmp_to_qemu() {
	local qemu=$1
	local cmd=$2
	qmp_result=$(
		echo " { \"execute\": \"qmp_capabilities\" } { $cmd }" \
			| socat - unix-connect:"$vm_dir/$qemu"/qmp | jq -s '.[2]'
	)
	echo "$qmp_result" | tee /tmp/qmp.json

}

function qmp_cmd() {
	local cmd=$1
	local qemu=$which_qemu
	echo "$cmd"

	qmp_to_qemu "$qemu" "$cmd"
}

function cmd_add_iso() {
	choose_iso
	echo "$iso_path"
	basename "$iso_path" >>"$vm_dir/opt/iso"
}

function cmd_add_boot_disk() {
	bash "$SCRIPT_DIR/collei-disk.sh" -c add "$vm_dir"
}

# 先这样了，就是的确可以这么操作而已
function cmd_trace() {
	local
	what=$(qemu-system-x86_64 -trace help | fzf)
	hmp "info trace-events $what"
	hmp "trace-event $what on"
	hmp "info trace-events $what"
}

function cmd_kvm_dmesg() {
	# https://github.com/rayylee/kvm-dmesg
	pushd "$WORKSTATION"
	if [[ ! -d kvm-dmesg ]]; then
		git clone https://github.com/rayylee/kvm-dmesg
		cd kvm-dmesg
		make -j32
	fi
	popd
	local kd=$WORKSTATION/kvm-dmesg/kvm-dmesg
	get_guest_system_map

	set -x
	$kd "$vm_dir"/"$which_qemu"/qmp-no-pretty "$guest_system_map"
	set +x
}

function cmd_hotplug_usb() {
	# https://qemu-project.gitlab.io/qemu/system/devices/usb.html
	echo "TODO"
	# 应该是不难的
}

function cmd_hotplug_nic() {
	init_switch
	create_switch_tap
	hmp "netdev_add tap,ifname=$tap_name,id=$tap_name,script=no,downscript=no,vhost=on"
	hmp "device_add virtio-net,netdev=$tap_name,mac=$mac_addr,bus=root_port_1"
}

function cmd_hotplug_cpu() {
	# 如果是 qmp shell 的话，可以这么操作
	# device_add id=cpu2 driver=host-x86_64-cpu socket-id=0 core-id=10 thread-id=0
	#
	# 1. 通过 query-hotpluggable-cpus 可以查那些 CPU 还可以 plug
	# - [ ] 需要调查下 qemu 是如何分配两个 CPU 到 socket 的规则是什么，我希望可以精细的控制
	# - [ ] 通过 query-hotpluggable-cpus 看，其实索引一个 CPU ，也就是通过 socket / core /thread 即可，但是 qemu 支持的可不仅仅这些
	#
	# 参考:
	# https://www.qemu.org/docs/master/system/cpu-hotplug.html

	# 如果一个位置还没有热插上 CPU ，是这个样子的:
	# {
	#   "props": {
	#     "core-id": 31,
	#     "thread-id": 0,
	#     "socket-id": 0
	#   },
	#   "vcpus-count": 1,
	#   "type": "host-x86_64-cpu"
	# }
	#
	# 如果已经插上了，那么是这个样子的:
	# {
	#     "props": {
	#         "core-id": 0,
	#         "thread-id": 0,
	#         "socket-id": 0
	#     },
	#     "vcpus-count": 1,
	#     "qom-path": "/machine/unattached/device[0]",
	#     "type": "host-x86_64-cpu"
	# }

	local hot="$vm_dir"/hotplug_cpu.json
	echo "
	{ 'execute': 'qmp_capabilities' }
	{ 'execute': 'query-hotpluggable-cpus' }
	" \
		| socat - unix-connect:"$vm_dir/$which_qemu"/qmp | jq -s '.[2]' >"$hot"

	# 有时候编译内核的时候需要，有时候是测试 cpu 的机制
	local add_one=true
	if gum confirm "hotplug all cpu?"; then
		add_one=false
	fi

	local cpu_num
	cpu_num=$(jq '.return | length // 0' "$hot")
	for ((i = 0; i < cpu_num; i = i + 1)); do
		local is_full
		is_full=$(jq ".return[$i]" "$hot" | jq 'has("qom-path")')
		if [[ $is_full == true ]]; then
			continue
		fi

		local thread_id
		local core_id
		local socket_id
		core_id=$(jq ".return[$i]" "$hot" | jq '.props."core-id"')
		thread_id=$(jq ".return[$i]" "$hot" | jq '.props."thread-id"')
		socket_id=$(jq ".return[$i]" "$hot" | jq '.props."socket-id"')

		echo "
	{ 'execute': 'qmp_capabilities' }
	{	'execute' : 'device_add',
		'arguments': {
			'driver': 'host-$ARCH-cpu',
			'id': 'CPU${socket_id}_${core_id}_${thread_id}',
			'socket-id' : $socket_id,
			'core-id': $core_id,
			'thread-id': $thread_id
		}
	}
	" \
			| socat - unix-connect:"$vm_dir/$which_qemu"/qmp | jq -s '.[2]'

		if [[ $add_one == true ]]; then
			break
		fi
	done
	# TODO 似乎有一个 systemd 服务来自动 online 添加的设备
	# echo "echo 1 | sudo tee /sys/devices/system/cpu/cpu31/online"
}

function cmd_hotplug_disk() {
	# 如果是 hmp 命令的话 :
	# TODO drive_add 调用 hmp_drive_add() 第一个参数啥意思
	#
	# qmp 参考 : https://gist.github.com/devimc/e9fd533e52b08387f1df65df8b19e038
	# 需要记住，blockdev 现在才是正确的配置方法

	local i
	local m="$vm_dir/$which_qemu/hp_disk_counter"
	i=$(cat "$m")
	echo $((i + 1)) >"$m"

	local hot_img="${vm_dir}/img/hotplug$i"
	if [[ ! -f $hot_img ]]; then
		qemu-img create -f qcow2 "${hot_img}" 400G
	fi

	# 参考 https://wiki.ubuntu.com/QemuDiskHotplug
	# 看这个文档，似乎 nvme 的 hotplug 没实现，但是实际上很容易
	# https://kvm-forum.qemu.org/2021/jensen-qemu-emulated-nvme.pdf
	#
	# 热插 nvme ，发现仅仅能热插一次，似乎是槽位是需要配置一下的
	local disk_id=hp_disk$i
	local drive_id=hp_drive$i
	hmp "drive_add 0 if=none,file=$hot_img,format=qcow2,id=$drive_id"
	sleep 1
	local disk_type="virtio_scsi"
	# disk_type="nvme"
	# disk_type="virtio_blk"
	case $disk_type in
		virtio_blk)
			hmp "device_add virtio-blk-pci,drive=$drive_id,id=$disk_id,bus=root_port_1 "
			;;
		nvme)
			hmp "device_add nvme,drive=$drive_id,id=$disk_id,serial=$(uuidgen),bus=root_port_1"
			;;
		virtio_scsi)
			# TODO 什么时候整理一下这个 scsi4.0 ，也就是 arg_hba 的
			# 地址空间可以划分一下，很容易出现重叠
			#
			# [0:0:0:0]    disk    Linux    scsi_debug       0191  /dev/sda
			# [1:0:0:0]    disk    QEMU     QEMU HARDDISK    2.5+  /dev/sdc # <- 热插
			# [1:0:0:10]   disk    QEMU     QEMU HARDDISK    2.5+  /dev/sdb # <- 配置的 boot
			# [1:0:1:0]    disk    QEMU     QEMU HARDDISK    2.5+  /dev/sdd # <- 热插
			hmp "device_add scsi-hd,bus=scsi4.0,channel=0,scsi-id=$i,lun=0,drive=$drive_id,id=$disk_id"
			;;
		*) ;;
	esac
}

function cmd_unplug_disk() {
	# hmp "drive_del hp_drive0"
	hmp "device_del hp_disk0"
	# TODO 当然，可以搞成先查询有那些 hp_drive ，然后再去 remove
	# 不过现在就这样吧，插上一个，然后拔掉一个
	local m="$vm_dir/$which_qemu/hp_disk_counter"
	echo 0 >"$m"
}

function unplug_nic() {
	# 有点烦，有点复杂
	:
}

function cmd_unplug_vfio() {
	# 为了测试 libvirt 直接拔掉 vfio 设备的操作
	# TODO 存在热插 vfio 的操作吗?
	if ! check_option vfio; then
		error "vfio is not configured"
	fi

	readarray -t devices <<<"$option_result"
	for dx in "${devices[@]}"; do
		lspci -s "$dx"
		unbind_from_vfio "$dx"
	done
}

function cmd_unplug_sriov() {
	if ! check_option sriov; then
		error "sriov is not configured"
	fi

	readarray -t devices <<<"$option_result"
	for dx in "${devices[@]}"; do
		ip link show "$dx"
		for virtfn in /sys/class/net/"$dx"/device/virtfn*; do
			[[ ! -L $virtfn ]] && continue
			vf_num="${virtfn##*virtfn}"
			target_pci=$(readlink -f "$virtfn" | sed 's|.*/||')
			iommu_group_link="/sys/bus/pci/devices/$target_pci/iommu_group"
			if [[ ! -L $iommu_group_link ]]; then
				continue
			fi
			echo "$target_pci"
			unbind_from_vfio "$target_pci"
		done

		echo "after unbind"
		# shellcheck disable=2010
		ls -la /sys/class/net | grep "${dx}v"
	done
}

function hotplug_mem_qmp() {
	echo "
		{ 'execute': 'qmp_capabilities' }

		{	'execute' : 'object-add',
			'arguments': {
				'qom-type': 'memory-backend-ram',
				'id': 'mem1',
				'size' : 0x200000
			}
		}

		{	'execute' : 'device-add',
			'arguments': {
				'driver': 'pc-dimm',
				'id': 'dimm1',
				'memdev': 'mem1'
			}
		}

		" \
		| socat - unix-connect:"$vm_dir"/qmp
}

function hotplug_mem_hmp() {
	# 基本参考:
	# https://github.com/qemu/qemu/blob/master/docs/memory-hotplug.txt
	local counter
	local m="$vm_dir/$which_qemu/hp_mm_counter"
	counter=$(cat "$m")
	echo $((counter + 1)) >"$m"
	echo "object_add memory-backend-memfd,id=hp_mem$counter,size=10G" \
		| socat - unix-connect:"$vm_dir/$which_qemu"/hmp
	sleep 1
	echo "device_add pc-dimm,id=hp_dimm$counter,memdev=hp_mem$counter " \
		| socat - unix-connect:"$vm_dir/$which_qemu"/hmp
}

function cmd_hotplug_mem() {
	# 就 memory 热插 qmp 和 hmp 没有什么区别，不过 hotplug_mem_qmp 没有测试过
	hotplug_mem_hmp
	# hotplug_mem_qmp
}

function edit_options() {
	if env | grep neovim >/dev/null; then
		echo "Don't Run nvim in nvim 😸"
		exit 0
	fi
	cd "$vm_dir"
	nvim
	exit
}

# TODO 似乎 xfs 的 cow 性能很差，可以用 fio + perf 测试下到底在搞什么
function cmd_backup() {
	local bak=false

	if is_vm_active "$vm_dir"; then
		gum confirm "vm is alive, be carefull" || exit 0
	fi

	for f in "$vm_dir"/img/boot*.bak; do
		if [[ -f $f ]]; then
			bak=true
			break
		fi
	done
	if [[ $bak == true ]]; then
		gum confirm "overwrite existing snapshot ?" || exit 0
	fi
	for i in "$vm_dir"/img/boot*; do
		# 跳过 bak 文件
		if [[ $i == *.bak ]]; then
			continue
		fi
		echo "cp $i $i.bak"
		cp "$i" "$i".bak
	done

}
function cmd_restore() {
	for i in "$vm_dir"/img/boot*.bak; do
		cp "$i" "${i%.bak}"
		echo "cp $i ${i%.bak}"
	done
}

# 获取 vmlinux 或者 System.map
# 如果不支持内核替换，那么就使用 vm_dir 下的获取，如果支持就直接从 linux-build 中获取
function get_guest_debug_files() {
	local file=$1
	local result=""
	if [[ $replace_kernel != true ]]; then
		if [[ ! -e "$vm_dir/$file" ]]; then
			echo "$vm_dir/$file not found"
			return 1
		fi
		result=$(realpath "$vm_dir"/"$file")
	else
		result="${kernel_dir}/$file"
	fi

	case "$file" in
		vmlinux)
			guest_vmlinux=$result
			;;
		System.map)
			guest_system_map=$result
			;;
		*)
			error "unknown"
			;;
	esac
}

# @return guest_system_map
function get_guest_system_map() {
	get_guest_debug_files System.map
}

# @return guest_vmlinux
function get_guest_vmlinux() {
	get_guest_debug_files vmlinux
}

function cmd_vmlinux() {
	get_guest_vmlinux
	copy_to_guest "$guest_vmlinux"
	# echo "drgn ...."
	echo "sudo crash vmlinux"
	# 模块的
	echo "drgn -s HOME/vmlinux --debug-directory /lib/modules/6.14.2/"
}

function cmd_dump_and_crash() {
	# 此外 qemu 给 gdb 增加了命令 dump-guest-memory 的命令，可以调查下:
	# https://github.com/qemu/qemu/blob/master/scripts/dump-guest-memory.py
	local dump_guest_path=$vm_dir/$which_qemu/dump
	get_guest_vmlinux
	local vmlinux=$guest_vmlinux
	rm -rf "$dump_guest_path"
	# https://qemu-project.gitlab.io/qemu/interop/qemu-qmp-ref.html#qapidoc-1278
	echo "
	{ 'execute': 'qmp_capabilities' }


	{
		'execute': 'dump-guest-memory',
		'arguments': {
			'detach': true,
			'paging': false,
			'protocol': 'file:${dump_guest_path}',
			'format' : 'kdump-raw-zlib'
		}
	}
	" \
		| socat - unix-connect:"$vm_dir/$which_qemu"/qmp

	# 使用这种方法不可以，dump 出来的内存是被 truncate 的
	# echo "dump-guest-memory -z $dump_guest_path" | socat - unix-connect:"$vm_dir"/hmp
	#
	# 手动进入到 hmp 然后执行命令是可以的。目前更加科学的方法是，等待一段时间:
	while true; do
		result=$(echo ' { "execute": "qmp_capabilities" } { "execute": "query-dump" }' | socat - unix-connect:"$vm_dir/$which_qemu"/qmp)
		# 完成之后的状态是:
		# {
		#     "return": {
		#         "total": 2156003328,
		#         "status": "completed",
		#         "completed": 2156003328
		#     }
		# }
		echo "$result" | grep completed
		if echo "$result" | grep status | grep completed; then
			break
		fi
		sleep 1
	done

	if [[ -f $dump_guest_path ]]; then
		crash "$dump_guest_path" "$vmlinux"
	else
		error "dump failed ?"
	fi
}


# 不断的让虚拟机暂停恢复，来复现一些问题
function hmp_stop_cont() {
	VAR=10000
	echo "stop" | socat - unix-connect:"$vm_dir/$which_qemu/hmp"
	for ((i = 0; i < VAR; i = i + 1)); do
		echo "cont" | socat - unix-connect:"$vm_dir/$which_qemu/hmp"
		sleep 0.3
		echo "stop" | socat - unix-connect:"$vm_dir/$which_qemu/hmp"
		sleep 0.2
	done
}

function hmp_reset() {
	echo "system_reset" | socat - unix-connect:"$vm_dir/$which_qemu/hmp"
}

function cmd_force_reboot() {
	echo "system_reset" | socat - unix-connect:"$vm_dir/$which_qemu/hmp"
}

function hmp_qom() {
	# qom-get path property -- print QOM property
	# qom-list path -- list QOM properties
	# qom-set [-j] path property value -- set QOM property.
	#                         -j: the value is specified in json format.
	#
	# - qom-list /
	#	type (string)
	# 	backend (child<container>)
	# 	machine (child<pc-i440fx-10.1-machine>)
	# 	objects (child<container>)
	# 	chardevs (child<container>)
	hmp "qom-list /"

	# 如果是 -machine q35,memory-backend=mem 那么可以
	# hmp "qom-list /machine/memory-backend/pc.ram[0]"

	# 如果是: -numa node,nodeid=0,memdev=mem0 那么可以
	hmp "qom-list /objects/mem0"
	hmp "qom-set /objects/mem0 seal false"
}

# 也可以使用 qmp shell 来执行，但是不知道如何让输出好看点，全部都是一坨的
function qmp_qom() {
	# 支持的命令行
	qmp_cmd '"execute": "query-command-line-options"'

	# 查询 qmp 支持的命令
	qmp_cmd '"execute": "query-commands"'

	# 展示一个 Type 以及他的属性，这个也非常好
	qmp_cmd '"execute": "qom-list-properties", "arguments": { "typename": "kvm-accel" }'

	# 展示 QEMU 中定义的所有的 TypeInfo 以及他的 parent
	qmp_cmd '"execute": "qom-list-types"'

	qmp_cmd '"execute": "device-list-properties" , "arguments": { "typename": "virtio-blk-pci" }'
}

# 秉持基本的原则是，只要做过的事情，就记录下来才可以
function cmd_auto() {
	testcase=unplug
	case $testcase in
		unplug)
			hmp "device_del boot1"
			;;
		syscall)
			get_qemu_pid "$vm_dir"
			sudo syscount.bt -p "$qemu_pid"
			;;
		hva)
			get_qemu_pid "$vm_dir"
			grep memory-backend-memfd /proc/"$qemu_pid"/maps
			;;
		hotplug)
			hotplug_disk
			sleep 2
			unplug_disk
			;;
		screendump)
			hmp "screendump a.ppm"
			;;
		qmp)
			echo "" | socat - unix-connect:"$vm_dir/$which_qemu"/qmp
			;;
		page_size)
			qmp_cmd '"execute": "x-query-ramblock"'
			;;
		misc)
			# hmp_stop_cont
			# hmp_reset
			# qmp_qom
			# hmp_qom
			;;
		*)
			error "unmached case"
			;;
	esac

}

function cmd_monitor() {
	items=(
		"qmp" "shell" "qga" "main"
	)
	resource=$(printf "%s\n" "${items[@]}" | fzf)
	case "$resource" in
		qmp)
			socat -,echo=0,icanon=0 unix-connect:"$vm_dir/$which_qemu"/qmp
			;;
		shell)
			qmp_shell=${QEMU_DIR}/scripts/qmp/qmp-shell
			$qmp_shell "$vm_dir/$which_qemu"/qmp-shell
			;;
		qga)
			# 原来 qga 可以执行这么多命令啊
			# https://qemu-project.gitlab.io/qemu/interop/qemu-ga-ref.html
			socat -,echo=0,icanon=0 unix-connect:"$vm_dir/$which_qemu"/qga.sock
			;;
		main)
			# 当 gdb 调试的时候，使用 socket 来连接
			socat -,echo=0,icanon=0 unix-connect:"$vm_dir/$which_qemu"/main.sock
			;;
		*)
			printf "%s\n" "${items[@]}"
			;;
	esac

}

function cmd_perf_qemu() {
	get_qemu_pid "$vm_dir"
	# sudo perf record --call-graph dwarf -p "$qemu_pid" -- sleep 10
	# TODO 和这个区别是什么，用这个观测 firecracker 的时候，发现 perf report 特别慢
	# 2026-06-02 我发现，用户态程序用这个才可以观测到 stack
	sudo perf record -g -p "$qemu_pid" -- sleep 10
}

# TODO man 一下 perf-kvm ，里面还有很多东西可以挖掘的
#
# 很遗憾，对于 3.10 内核走不通，用 perf report 可以看到 host 中东西，但是用
# perf kvm .... 的 report 看不到 guest 中的东西
function cmd_perf_guest() {
	get_qemu_pid "$vm_dir"
	if ! get_guest_vmlinux; then
		local vmlinux=$guest_vmlinux
		set -x
		sudo perf kvm --guest --guestvmlinux="$vmlinux" record --pid "$qemu_pid" -- sleep 3
		sudo perf kvm --guest --guestvmlinux="$vmlinux" report
	else
		local kallsyms="$vm_dir"/kallsyms
		local modules="$vm_dir/modules"
		if [[ -f $kallsyms ]]; then
			set -x
			sudo perf kvm --guest --guestkallsyms="$kallsyms" --guestmodules="$modules" record --pid "$qemu_pid" -- sleep 3
			sudo perf kvm --guest --guestkallsyms="$kallsyms" --guestmodules="$modules" report
		else
			error "guest kallsyms not found"
		fi
	fi
}

function cmd_rename() {
	local new_vm_dir
	get_vm_name "$(basename "$vm_dir")"
	new_vm_dir="$all_vm_dir/$new_vm_name"
	mv "$vm_dir" "$new_vm_dir"

	rm "$vm_dir_symbol"
	ln -sf "$new_vm_dir" "$vm_dir_symbol"
	exit 0
}

function cmd_throttle() {
	# 效果不错
	# https://github.com/qemu/qemu/blob/master/docs/throttle.txt
	# 可惜，没有延迟注入的
	# Jobs: 1 (f=1): [w(1)][1.1%][w=8004KiB/s][w=2001 IOPS][eta 16m:29s]
	var='
"execute": "block_set_io_throttle",
"arguments": {
	"device": "virtio_blk_1",
	"iops": 100,
	"iops_rd": 0,
	"iops_wr": 0,
	"bps": 0,
	"bps_rd": 0,
	"bps_wr": 0,
	"iops_max": 2000,
	"iops_max_length": 60
}
'
	qmp_cmd "$var"
}

function cmd_addr() {
	addr="virtqueue_get_buf_ctx_split+0x63"
	# 参考 https://serverfault.com/questions/605946/kernel-stack-trace-to-source-code-lines
	sed -i '1i #!/usr/bin/env bash' "$kernel_dir"/scripts/faddr2line
	echo "$kernel_dir/scripts/faddr2line $kernel_dir/vmlinux $addr"
	echo "$addr"
	"$kernel_dir"/scripts/faddr2line "${kernel_dir}/vmlinux" "$addr"
}

function metrics() {
	# 刷新 metrics
	curl --unix-socket "$vm_dir/firecracker.socket" -i \
		-X PUT "http://localhost/actions" \
		-H "Accept: application/json" \
		-H "Content-Type: application/json" \
		-d '{
       "action_type": "FlushMetrics"
     }'
}

function cmd_kill() {
	# 不知道为什么，通过 hmp 的方式失效了
	# gum confirm "Kill $vm ?" && echo "quit" | socat - unix-connect:"$vm/hmp"
	get_qemu_pid "$vm_dir"

	# 暂时就这么写，也许抽象出来
	if [[ $auto_yes == false ]]; then
		gum confirm "Kill $vm_dir ?" && kill -9 "$qemu_pid"
	else
		kill -9 "$qemu_pid"
	fi
	info "Done"
}

function cmd_debug_kernel() {
	if is_vm_active "$vm_dir"; then
		if gum confirm "Kill the machine?"; then
			kill_qemu "$vm"
		else
			echo "Give up"
			exit 0
		fi
	fi

	# 使用 screen -r 来进入到 detach 的脚本
	# screen -d -m "$SCRIPT_DIR"/collei.sh -s
	pueue add -i -g qemu -- "$SCRIPT_DIR"/collei.sh -s

	# 原则上来说，是可以使用任意的目录，因为只是使用了其中 gdb scripts 而已
	cd "${kernel_dir}"

	vmlinux="$vm_dir/vmlinux"
	if [[ ! -f $vmlinux ]]; then
		vmlinux=${kernel_dir}/vmlinux
	fi

	# 等待启动
	sleep 1
	if check_option fire; then
		workaround_firecracker
	fi

	cd "${kernel_dir}"
	gdb "$vmlinux" -ex "target remote $vm_dir/gdb.socket" \
		-ex "hbreak start_kernel" \
		-ex "hbreak __crash_kexec" \
		-ex "continue"

	# gdb 会有如下错误，但是似乎关系不大
	# warning: Section .debug_names in /home/martins3/data/linux-build/vmlinux length 139056 does not match section length 390560, ignoring .debug_names.
	#
	# 执行 lx-symbols 需要等待系统已经启动之后才可以
	# -ex "lx-symbols "
	#
	# 而且虚拟机会 crash
	#
	#    - asm_exc_page_fault
	#	- exc_page_fault
	# 	  - handle_page_fault
	# 	    - do_kern_addr_fault
	# 	      - bad_area_nosemaphore
	# 	        - page_fault_oops
	# 	          - oops_end
	# 	            - crash_kexec
	# 	              - __crash_kexec
	kill_qemu "$vm"
}

# 提供 @ssh_ip @ssh_user @ssh_port
function get_ssh_info() {
	if ! check_option user; then
		ssh_user=root
	else
		ssh_user=$option_result
	fi

	if ! check_option ip; then
		ssh_ip=localhost
		get_tcp_port ssh
		ssh_port="$tcp_port"
	else
		ssh_ip=$option_result
		ssh_port=""
	fi
}

function cmd_rsync() {
	get_ssh_info

	if [[ $ssh_user == root ]]; then
		location="/root"
	else
		location="/home/$ssh_user"
	fi

	mkdir -p .nvim
	if [[ -n $ssh_port ]]; then
		# 不知道为什么， rsync 不可以使用 127.0.0.1
		cat <<_EOF_ >.nvim/deployment.lua
return {
	port = "$ssh_port",
	ip = "localhost",
	user = "$ssh_user",
	location = "$location",
	ignore_git = false,
}
_EOF_
	else
		cat <<_EOF_ >.nvim/deployment.lua
return {
	ip = "$ssh_ip",
	user = "$ssh_user",
	location = "$location",
	ignore_git = false,
}
_EOF_
	fi

}

function cmd_ssh() {
	local do_copy_ssh=${1:-}
	get_ssh_info
	ssh_port_opt=""
	if [[ -n $ssh_port ]]; then
		ssh_port_opt="-p$ssh_port"
	fi
	# @todo 似乎我的 tmux 配置有问题导致 ssh 前需要设置一下环境变量
	if [[ $do_copy_ssh ]]; then
		cmd="TERM=xterm-256color ssh-copy-id $ssh_port_opt $ssh_user@$ssh_ip"
	else
		cmd="TERM=xterm-256color ssh $ssh_port_opt $ssh_user@$ssh_ip"
	fi
	# 修改目录，从而让 tmux 的名称可以修改
	cd "$vm_dir"
	# 为了把 $ssh_port 没有定义的时候，命令会变为
	# ssh "" martins3@10.0.101.0
	# 为了解决掉这个双引号，所以使用这个
	local ssh_log=$vm_dir/.tmp_ssh_log
	if ! eval "$cmd" 2>"$ssh_log"; then
		# 如果一个虚拟机被删掉了，然后创建了新的虚拟机复用之前的 id
		# 可以自动修改一下 ~/.ssh/known_hosts
		if grep "has changed and you have" "$ssh_log"; then
			if [[ -n $ssh_port ]]; then
				ssh-keygen -R "[localhost]:$ssh_port"
			else
				ssh-keygen -R "$ssh_ip"
			fi
			error "try again"
		fi
		error "ssh failed"
	fi
}

# 仅输出 SSH 命令，不执行（用于 AI 助手等非交互式场景）
function cmd_ssh_auto() {
	get_ssh_info
	ssh_port_opt=""
	if [[ -n $ssh_port ]]; then
		ssh_port_opt="-p $ssh_port"
	fi
	echo "ssh $ssh_port_opt $ssh_user@$ssh_ip"
}

function cmd_vnc() {
	get_tcp_port vnc
	novnc_port=$((tcp_port + 1))
	echo "http://$(get_master_ip):$novnc_port/vnc.html"
}

# 将 vcpu thread 过滤出来, kvm 将 vcpu 用 thread 的方式暴露出来，也许有一些操作这些 thread 的方法
# 仅仅暂停一个 vcpu
function bind_cpu() {
	local skip=false
	# echo "pidstat -t -p $qemu_pid"

	mapfile -t threads < <(echo "info cpus" | socat - unix-connect:"$vm_dir"/hmp | grep -E -o "=[0-9][0-9]+" | grep -E -o "[0-9][0-9]+")
	for i in "${threads[@]}"; do
		echo "--> $i"
	done
	# 使用 perf 只是去观察一个 vcpu 的退出状态:
	# sudo perf trace -e kvm:kvm_exit -t 3023637

	if [[ $skip == true ]]; then
		exit 0
	fi
	local cpu=0
	for i in "${threads[@]}"; do
		echo "bind $i"
		taskset -cp $cpu "$i"
		cpu=$((cpu + 1))
	done
	exit 0
}

get_pswp() {
	local in out
	in=$(awk '/^pswpin / {print $2}' memory.stat)
	out=$(awk '/^pswpout / {print $2}' memory.stat)
	printf '%s %s\n' "$in" "$out"
}

function check_swap_speed() {
	cd "$1"
	local pswpin1 pswpout1 pswpin2 pswpout2

	read -r pswpin1 pswpout1 < <(get_pswp)
	echo "初始: pswpin=$pswpin1 pswpout=$pswpout1"

	while true; do
		sleep 1

		read -r pswpin2 pswpout2 < <(get_pswp)
		echo "采样: pswpin=$pswpin2 pswpout=$pswpout2"

		delta_in=$((pswpin2 - pswpin1))
		delta_out=$((pswpout2 - pswpout1))
		speed_in_mb=$((delta_in * 4 / 1024))
		speed_out_mb=$((delta_out * 4 / 1024))

		echo ""
		echo "swap in : ${speed_in_mb} MB/s"
		echo "swap out: ${speed_out_mb} MB/s"
		echo "swap in : ${delta_in}"
		echo "swap out: ${delta_out}"

		pswpin1=$pswpin2
		pswpout1=$pswpout2
	done
}

function cmd_cgroup() {
	local fs
	get_qemu_pid "$vm_dir"
	fs=/sys/fs/cgroup$(awk -F: '{print $3}' /proc/"$qemu_pid"/cgroup)

	echo 1G >"$fs"/memory.high
	# TODO 顶层没有 cpuset ，暂时没有和等价的内容，我理解是有解决办法的
	# cgset -r cpuset.cpus=15 "$qemu_cgroup"
	echo "$fs"
	# viddy "cat $fs/memory.stat"
	# check_swap_speed "$fs"
	exit 0
}

function send_special_key() {
	key=$1
	echo "=== sending: $key"
	echo "sendkey $key" | socat -t 3 - "$vm_dir"/hmp
}

# @cmd
# @seconds to sleep
function sendkeys() {
	echo "send keys === $1"
	echo "$1" | awk -f "$PROGDIR"/sendkeys.awk | socat -t 3 - "$vm_dir/$which_qemu"/hmp
	if [[ -n ${2:-} ]]; then
		sleep "$2"
	fi
}

function install_text_mode_coreos() {
	sendkeys "sudo coreos-installer install /dev/sda --ignition-file /sys/firmware/qemu_fw_cfg/by_key/46/raw" 30
	send_special_key "ret" 1
	sendkeys "sudo shutdown now" 1
	send_special_key "ret" 1
	echo >"$vm_dir/opt/install"
	echo martins3 >"$vm_dir/opt/user"
	echo "finished"
}

function cmd_setup_nmcli() {
	local mac_level mac_guest
	mac_level=$(printf "%02x" "$host_level")
	mac_guest=$(printf "%02x" "$guest_id")

	mac="52:54:00:${mac_level}:${mac_guest}:00"
	ip="10.0.${guest_id}.0/16"

	# sendkeys "systemctl start NetworkManager" 1
	# sendkeys 'sudo nmcli connection add type ethernet con-name user ifname "*" mac 52:54:00:12:34:56 ipv4.method auto' 1
	sendkeys "sudo nmcli connection add type ethernet con-name vhost ifname \"*\" mac ${mac} ip4 ${ip}"
	send_special_key "ret" 1
}

function cmd_auto_install() {
	resource=$(gum choose "net" "coreos" "6" "5")
	case $resource in
		coreos)
			install_text_mode_coreos
			;;
		auto)
			# 如果想要部署，那么就需要这些
			echo 128 >opt/ram
			echo 48 >opt/smp
			echo basic >nvme
			qemu-img create -f qcow2 1.qcow2 512G
			qemu-img create -f qcow2 2.qcow2 512G
			qemu-img create -f qcow2 3.qcow2 512G
			;;
	esac
}

# 似乎 fedora 上，配置文件没用，必须使用 NetworkManager
# 所以 systemd 中的 NetworkManager.service 和 network.service 是什么关系
function network_nmcli() {
	local eth=enp0s20f0u1c2
	local ip=10.0.0.6
	sudo nmcli con add type bridge ifname br9527
	sudo nmcli con add type bridge-slave ifname $eth master br9527
	sudo nmcli con mod bridge-br9527 ipv4.method manual ipv4.addresses $ip/16
	sudo nmcli con mod bridge-br9527 connection.autoconnect yes
	sudo nmcli con mod bridge-slave-$eth connection.autoconnect yes
}

# return $guest_level
guest_level=0
function choose_guest_level() {
	echo "VM level, level 0 means physical machine"
	local level
	level=$(gum choose 1 2 3)
	guest_level=$level
}

function cmd_bash_prompt() {
	local hostname
	hostname="$(basename "$vm_dir")"
	local ps1_content="PS1=\"\\[\\033[01;32m\\]\\u@$hostname\\[\\033[00m\\]:\\[\\033[01;34m\\]\\w\\[\\033[00m\\]\\$ \""
	echo "echo '$ps1_content' >> ~/.bashrc"

	get_ssh_info
	if [[ -n $ssh_port ]]; then
		ssh_port="-p$ssh_port"
		# shellcheck disable=2029
		ssh $ssh_port "$ssh_user@$ssh_ip" "echo '$ps1_content' >> ~/.bashrc"
	else
		# shellcheck disable=2029
		ssh "$ssh_user@$ssh_ip" "echo '$ps1_content' >> ~/.bashrc"
	fi

}

function cmd_tmp_ip() {
	choose_guest_level
	local level=$((guest_level - 1))
	local mac_guest
	mac_level=$(printf "%02x\n" "$level")
	mac_guest=$(printf "%02x\n" "$guest_id")

	local mac_address0="52:54:00:$mac_level:$mac_guest:00"
	local mac_address1="52:54:00:$mac_level:$mac_guest:01"

	# 使用网卡名称来配置，当网卡配置变化的时候(iommu 打开、关闭，添加新网卡)，容易有问题
	# sudo nmcli c add type ethernet ifname ens4 con-name Y ipv4.method auto
	# sudo nmcli c add type ethernet ifname ens5 con-name X ip4 10.0.${guest_id}.${level}/16
	# sudo nmcli c add type ethernet ifname ens6 con-name X ip4 172.213.0.2/16
	cat <<_EOF_
	nmcli c show
	sudo systemctl start NetworkManager

	# 给 vfio 配置 ip 地址
	sudo nmcli connection add type ethernet ifname ens1f0np0 ipv4.method manual ipv4.addresses 172.22.129.26/17

	sudo nmcli connection add type ethernet con-name vhost ifname "*" mac $mac_address0 ip4 10.0.${guest_id}.${level}/16
	sudo nmcli connection add type ethernet con-name  user ifname "*" mac 52:54:00:12:34:56 ip4 10.0.2.2/16
	sudo nmcli c up vhost
	sudo nmcli c up user

	# 似乎直接配置 nmcli 就是最好的
	# sudo nmcli connection add type ethernet con-name user ifname "*" mac 52:54:00:12:34:56 ipv4.method auto
	# 可能还需要配置上
	# ip route add default via 10.0.2.2 dev ens5

	sudo nmcli connection add type ethernet con-name tap ifname "*" mac $mac_address1 ip4 172.213.0.2/24

	sudo nmcli connection modify X ipv4.gateway 10.0.0.2

	ip addr add dev ens4 10.0.${guest_id}.${level}/16

	sudo nmcli connection add type bridge ifname br9527
	sudo nmcli connection add type bridge-slave ifname enp125s0f0 master br9527
	sudo nmcli connection modify br9527 \
	    ipv4.addresses 10.0.${guest_id}.${level}/16 \
	    ipv4.gateway 10.0.0.2 \
	    ipv4.method manual

	sudo nmcli connection modify br9527 \
	    ipv4.addresses 192.168.19.60/20 \
	    ipv4.gateway 192.168.16.3 \
	    ipv4.method manual

	# 这个不可以工作
	sudo ovs-vsctl add-br br-in
	sudo nmcli connection add type ovs-bridge conn.interface br-in con-name br-in
	sudo nmcli connection add type ovs-port conn.interface br-in-port master br-in con-name br-in-port
	sudo nmcli connection add type ovs-interface slave-type ovs-port conn.interface br-in master br-in-port con-name br-in-intf \
	    ipv4.method manual \
	    ipv4.addresses 10.0.${guest_id}.0/16 \
	    ipv4.gateway "" \
	    ethernet.cloned-mac-address 6A:D6:5E:4E:00:44
	sudo nmcli connection up br-in-intf
	# 这一步遇到问题:
	# Error: Connection activation failed: Open vSwitch database connection failed
	#
	# 不知道为什么需要这一步，但是这一步的确是需要的
	sudo ip route add default via 192.168.16.3 dev br-in

_EOF_
}

function cmd_log() {
	pueue log -f "$(cat "$vm_dir"/pueue)"
}

function cmd_follow_log() {
	pueue follow "$(cat "$vm_dir"/pueue)"
}

function cmd_network() {
	# 到时候在处理自动 ens5 和 10.0.10.0
	#
	# cd /etc/sysconfig/network-scripts/
	# 顺便说下连 wifi 的方法
	# nmcli device wifi connect <AP name> password <password>

	choose_guest_level
	local level=$((guest_level - 1))
	vhost_mac=$(printf "%02x\n" "$guest_id")
	cat <<_EOF_
== ifcfg-slirp ==
NAME=qemu
TYPE=Ethernet
PROXY_METHOD=none
BROWSER_ONLY=no
BOOTPROTO=dhcp
DEFROUTE=yes
IPV4_FAILURE_FATAL=no
IPV6INIT=yes
IPV6_AUTOCONF=yes
IPV6_DEFROUTE=yes
IPV6_FAILURE_FATAL=no
IPV6_ADDR_GEN_MODE=eui64
UUID=$(uuidgen)
DEVICE=qemu-slirp
ONBOOT=yes
HWADDR=52:54:00:12:34:56

# 不用 ovs 的时候
== ifcfg-vhost ==
TYPE=Ethernet
PROXY_METHOD=none
BROWSER_ONLY=no
BOOTPROTO=static
DEFROUTE=yes
IPV4_FAILURE_FATAL=no
IPV6INIT=yes
IPV6_AUTOCONF=yes
IPV6_DEFROUTE=yes
IPV6_FAILURE_FATAL=no
IPV6_ADDR_GEN_MODE=eui64
NAME=qemu
UUID=$(uuidgen)
DEVICE=vhost
IPADDR=10.0.${guest_id}.${level}
NETMASK=255.255.0.0
ONBOOT=yes
HWADDR=52:54:00:00:02:${vhost_mac}

# 配置 ovs 的时候，无需配置网卡，只用配置 ovs 就可以了
# 但是如果内核配置变化(打开 CONFIG_HOTPLUG_PCI)，网卡名称变化了，ovs 关联的网络就断了
== ifcfg-br-in ==
DEVICE=br-in
BOOTPROTO=static
ONBOOT=yes
DEVICETYPE=ovs
TYPE=OVSIntPort
IPADDR=10.0.${guest_id}.0
NETMASK=255.255.0.0
OVS_BRIDGE=br-in
HOTPLUG=no
MACADDR=6A:D6:5E:4E:00:44


# 配置 bridge 的配置
== ifcfg-br9527 ==
DEVICE=br9527
TYPE=Bridge
ONBOOT=yes
BOOTPROTO=static
NM_CONTROLLED=no
DELAY=0
IPADDR=10.0.${guest_id}.${level}
NETMASK=255.255.0.0
# GATEWAY=192.168.16.1
ONBOOT=yes
# 不用带 HWADDR ，br9527 没有 mac

# 添加 bridge 关联的网卡
== ifcfg-br-eth ==
NAME=ifcfg-br-eth
TYPE=Ethernet
PROXY_METHOD=none
BROWSER_ONLY=no
BOOTPROTO=static
DEFROUTE=yes
IPV4_FAILURE_FATAL=no
IPV6INIT=yes
IPV6_AUTOCONF=yes
IPV6_DEFROUTE=yes
IPV6_FAILURE_FATAL=no
IPV6_ADDR_GEN_MODE=eui64
NM_CONTROLLED=no
UUID=$(uuidgen)
# 控制 device 的名称是可以不用的 ?
# DEVICE=br-eth
BRIDGE=br9527
ONBOOT=yes
HWADDR=52:54:00:00:02:${vhost_mac}

# 物理机参考
TYPE=Ethernet
PROXY_METHOD=none
BROWSER_ONLY=no
BOOTPROTO=static # hdcp -> static
DEFROUTE=yes
IPV4_FAILURE_FATAL=no
IPV6INIT=yes
IPV6_AUTOCONF=yes
IPV6_DEFROUTE=yes
IPV6_FAILURE_FATAL=no
IPV6_ADDR_GEN_MODE=eui64
NAME=enp125s0f0
UUID=8816c4ff-1ecb-47a9-81c1-d1628f21082d
DEVICE=enp125s0f0
ONBOOT=yes
# 添加如下内容
IPADDR=192.168.19.60
NETMASK=255.255.240.0
HWADDR=F0:33:E5:D1:A4:FD
GATEWAY=192.168.16.3

_EOF_

}

# 参考 https://docs.kernel.org/filesystems/ramfs-rootfs-initramfs.html
function initrd_hello() {
	dir=/tmp/martins3/initrd/
	mkdir -p $dir
	pushd "$PROGDIR"
	command="gcc -static hello.c -o hello.out"
	if [[ -d /nix ]]; then
		nix-shell -p glibc.static --command "$command"
	else
		eval "$command"
	fi
	echo hello.out | cpio -o -H newc | gzip >$dir/initrd.hello

	echo "配套的参数，放到 cmdline 中"
	echo "root=/dev/ram rdinit=/hello.out"
	echo "输出在 vnc 中"
}

function run() {
	if [[ -d /nix ]]; then
		nix-shell --command "$1"
	else
		eval "$1"
	fi
}

function initrd_busybox() {
	if [[ ! -d ~/core/busybox ]]; then
		pushd ~/core/
		git clone https://git.busybox.net/busybox/
	fi
	pushd ~/core/busybox
	if [[ ! -f .config ]]; then
		make defconfig
		# TODO 靠，并没有用
		sed -i "s/# CONFIG_STATIC is not set/CONFIG_STATIC=y/" .config
	fi

	ln -sf ~/.dotfiles/scripts/nix/env/busybox.nix default.nix
	# TODO nix 环境必须使用 busybox
	# run "make -j32"
	# make install -j32
	pushd _install
	mkdir -p bin sbin etc proc sys usr/bin usr/sbin
	cat <<'EOF' >init
#!/bin/sh
set -x
mount -t proc none /proc
mount -t sysfs none /sys
mknod /dev/ttyS0 c 4 64
mknod /dev/tty c 5 0
mknod /dev/tty1 c 4 1
mknod /dev/tty2 c 4 2
mknod /dev/tty3 c 4 3
mknod /dev/tty4 c 4 4
cat <<!
Boot took $(cut -d' ' -f1 /proc/uptime) seconds
        _       _     __ _
  /\/\ (_)_ __ (_)   / /(_)_ __  _   ___  __
 /    \| | '_ \| |  / / | | '_ \| | | \ \/ /
/ /\/\ \ | | | | | / /__| | | | | |_| |>  <
\/    \/_|_| |_|_| \____/_|_| |_|\__,_/_/\_\
Welcome to mini_linux
!
ifup eth0
setsid cttyhack /bin/sh
exec /bin/sh
EOF

	chmod +x init
	mkdir -p etc/network/
	# https://unix.stackexchange.com/questions/128439/good-detailed-explanation-of-etc-network-interfaces-syntax
	# 1. 既然没有 systemd ，那么 udhcpc 是如何给自动加载这个的?
	# 2. 这两个配置都不生效
	#	1. eth0 没有 dhcp
	#	2. eth1 无法获取 ip
	#
	#	可以通过这两个命令来获取到
	#	~ # ip addr add dev eth2 10.0.111.111/16
	#	~ # ip link set dev eth2 up
	#	但是手动调用也没用 udhcpc -i eth0
	cat <<_EOF_ >etc/network/interfaces
auto eth0
iface eth0 inet dhcp

auto eth1
iface eth1 inet static
    address 10.0.111.111/16
    gateway 10.0.0.2
_EOF_
	# 需要配置 cmdline 为 init=/init console=ttyS0 nokaslr earlyprink=serial
	which=/home/martins3/vm/base/busybox.cpio.gz
	find . -print0 | cpio --null -ov --format=newc | gzip -9 >$which
	# $QEMU -enable-kvm -kernel "$KERNEL" -initrd $initrd -nographic -append "console=ttyS0"
}

# https://docs.yoctoproject.org/brief-yoctoprojectqs/index.html
# http://downloads.yoctoproject.org/releases/yocto/yocto-3.1/machines/qemu/qemu${ARCH}/core-image-minimal-qemu${ARCH}.ext4
# cmd="${QEMU} -kernel ${KERNEL} -enable-kvm -drive file=${yocto_img},if=virtio,format=raw --append 'root=/dev/vda console=ttyS0' -nographic"
#
# TODO yocto 中各种脚步需要重点关注:
function initrd_yocto() {
	if [[ ! -d ~/core/poky ]]; then
		git clone https://github.com/yoctoproject/poky
	fi
}

function balloon_get_stat() {
	echo "
	{ 'execute': 'qmp_capabilities' }
	{ 'execute': 'qom-get',
		     'arguments': { 'path': '/machine/peripheral/balloon0',
		     'property': 'guest-stats' } }
	" \
		| socat - unix-connect:"$vm_dir"/"$which_qemu"/qmp | grep "$1" \
		| grep -Eo '[0-9]+'
}

function balloon_get_available() {
	balloon_get_stat "stat-available-memory"
}

function balloon_get_total_memory() {
	balloon_get_stat "stat-total-memory"
}

function balloon_get_actual_size() {
	echo "info balloon" | socat - unix-connect:"$vm_dir"/"$which_qemu"/hmp \
		| grep "actual=" \
		| grep -Eo '[0-9]+'
}

function balloon_set() {
	size=$1
	echo "balloon $size" | socat - unix-connect:"$vm_dir"/"$which_qemu"/hmp
	balloon_get_actual_size
}

# TODO 会有一种场景，available memory 大于 0 ，即便是总是被 swap 出去了很多内存
function cmd_balloon() {
	local available_memory
	local actual_size
	local target_size
	available_memory=$(($(balloon_get_available) / 1024 / 1024))
	total_memory=$(($(balloon_get_total_memory) / 1024 / 1024))
	echo "available memory $available_memory"
	actual_size=$(balloon_get_actual_size)
	target_size=$((actual_size - available_memory + 500))
	echo "${actual_size}M -> ${target_size}M"
	gum confirm "do it ?"
	balloon_set "$target_size"
	sleep 10
	balloon_set "$total_memory"
}


function cmd_cpr_exec() {
	# 的确没想到，也是可以用 bash 的
	#
	# TODO
	# qemu-system-x86_64: -netdev tap,ifname=vif_s_29_0,id=vif_s_29_0,script=no,downscript=no,vhost=on: could not configure /dev/net/tun (vif_s_29_0): Device or resource busy
	cp "$vm_dir/cmd.sh" "$vm_dir/cmd-cpr.sh"
	sed -i "$ s/$/ \\\\/; $ a\	-incoming file:$vm_dir/cpr_vmstate.img" "$vm_dir/cmd-cpr.sh"
	local cmds=(
		"info status"
		"migrate_set_parameter mode cpr-exec"
		"migrate_set_parameter cpr-exec-command $vm_dir/cmd-cpr.sh"
		"migrate -d file:$vm_dir/cpr_vmstate.img"
		"info status"
	)
	for cmd in "${cmds[@]}"; do
		echo "$cmd" \
			| socat - unix-connect:"$vm_dir/$which_qemu/hmp"
	done

}

function cmd_top() {
	get_qemu_pid "$vm_dir"
	set -x
	cat /proc/"$qemu_pid"/sched
	echo ""
	cat /proc/"$qemu_pid"/schedstat
	echo ""
	echo "/sys/fs/cgroup/$(cat /proc/"$qemu_pid"/cgroup)"
}

# 保存之后，干掉 QEMU ，然后使用 rk -L 启动
function cmd_save_vm_file() {
	if [[ $live_qemu_count -ne 1 ]]; then
		error "need exactly one qemu"
	fi

	# 类似的命令:
	# (qemu) migrate "exec:cat > mig"
	# (qemu) migrate "exec:gzip > mig.gz"

	# info vcpu_dirty_limit
	# info migrate_capabilities
	# info migrate_parameters
	local use_fd=false
	local hmp_migrate
	local img=$vm_dir/vmstate.img
	local img_json=$vm_dir/vmstate.json
	if [[ $use_fd == true ]]; then
		# 这个方法居然真的可以，有趣的
		# 这个方法甚至不需要 qemu 中 --add-fd 来支持
		# 也可以用这个方法:
		# socat UNIX-LISTEN:/tmp/qemu-socket,fd=100 &
		exec 100<>/tmp/qemu_img
		hmp_migrate="migrate -d fd:100"
	else
		hmp_migrate="migrate -d file:$img"
	fi

	local option="mapped-ram"
	option="basic"
	option="background-snapshot"
	do_migration "$hmp_migrate" "$option"

	local extract=false
	if [[ $extract == true ]]; then
		"$QEMU_DIR"/scripts/analyze-migration.py -f "$img" >"$img_json"
		jq 'keys_unsorted' "$img_json"
	else
		:
		# socat -,echo=0,icanon=0 unix-connect:"$vm_dir/$which_qemu/hmp"
	fi
	# 原来这种状态下，qemu 直接就暂停了啊，这也太弱了
}

# 和 save_vm_file 不同，save_vm_cpr 需要配合 load_vm_cpr 来使用
function cmd_save_vm_cpr() {
	if [[ $live_qemu_count -ne 1 ]]; then
		error "need exactly one qemu"
	fi

	local cmds=(
		"info status"
		"migrate_set_parameter mode cpr-reboot"
		"migrate_set_capability x-ignore-shared on"
		"migrate -d file:$vm_dir/cpr_vmstate.img"
		"info status"
	)
	for cmd in "${cmds[@]}"; do
		echo "$cmd" \
			| socat - unix-connect:"$vm_dir/$which_qemu/hmp"
	done
	while true; do
		result=$(echo ' { "execute": "qmp_capabilities" } { "execute": "query-status" }' \
			| socat - unix-connect:"$vm_dir/$which_qemu"/qmp | jq -s '.[2]' \
			| jq -r '.return.status')
		# hmp "info migrate -a"
		if [[ $result == postmigrate ]]; then
			break
		fi
		sleep 1
	done
	#  感觉和 save_vm_file 一样
}

function cmd_load_vm_cpr() {
	local cmds=(
		"info status"
		"migrate_set_parameter mode cpr-reboot"
		"migrate_set_capability x-ignore-shared on"
		"migrate_incoming file:$vm_dir/cpr_vmstate.img"
		"info status"
	)
	for cmd in "${cmds[@]}"; do
		echo "$cmd" \
			| socat - unix-connect:"$vm_dir/$which_qemu/hmp"
	done

}

# 参考 https://blog.davidv.dev/posts/learning-pcie/
function initrd_busybox_binary() {
	set -x
	echo "$vm_dir"
	pushd "$vm_dir"
	mkdir -p tmp.initramfs
	cd tmp.initramfs

	cat <<_EOF_ >init.sh
#!/sh
/busybox mkdir /sys
/busybox mkdir /proc
/busybox mount -t proc null /proc
/busybox mount -t sysfs null /sys
# /busybox mknod /dev/mem c 1 1
/busybox lspci
exec /busybox sh
_EOF_
	# 这种更加简单
	if [[ ! -f busybox ]]; then
		wget https://www.busybox.net/downloads/binaries/1.35.0-x86_64-linux-musl/busybox
	fi
	find . -print0 | cpio --null -H newc -o | gzip -9 >../initramfs-busybox.gz
	cd ..
	ln -sf initramfs-busybox.gz initrd
	popd
}

function cmd_pty() {
	set -x
	local pts
	pts=$(echo "info chardev" | socat - unix-connect:"$vm_dir/$which_qemu/hmp" | grep -E --line-buffered -o "/dev/pts/[0-9]*")
	# 两种方法都可以，使用 minicom 或者 screen
	# minicom -D "$pts"
	screen "$pts" 115200

	# TODO 想不到还有更好的方法
	# docs/system/mux-chardev.rst.inc
}

# 在图形/TUI 安装界面和 TTY shell 之间切换
# Anaconda 安装介质中，TTY3 通常提供 root shell
function cmd_tty3() {
	log "Sending Ctrl+Alt+F3 to $(basename "$vm_dir") ..."
	echo "sendkey ctrl-alt-f3" | socat -t 3 - "$vm_dir/$which_qemu/hmp"
}

function cmd_gdb() {
	# 为什么需要 sudo 才可以 attach，因为打开了 sudo 才能打开资源？
	# 但是 qemu 运行的时候没有使用 sudo 啊
	#
	# TODO 为什么 hmp 会触发这个东西
	# Thread 35 "qemu-system-x86" received signal SIGUSR1, User defined signal 1.
	#
	cd "$QEMU_DIR"

	local qemu_pid
	local my_uid
	local proc_uid
	set -x
	qemu_pid="$(cat "$vm_dir/$which_qemu/pid")"
	my_uid=$(id -u)
	proc_uid=$(grep "^Uid:" /proc/"$qemu_pid"/status | awk '{print $2}')

	if [[ $my_uid -eq $proc_uid ]]; then
		gdb -ex "handle SIGUSR1 nostop noprint" -p "$(cat "$vm_dir/$which_qemu/pid")"
	else
		# 如果用 sudo ，会导致 qemu 仓库中 gdb scripts 无法吗?
		sudo gdb -ex "handle SIGUSR1 nostop noprint" -p "$(cat "$vm_dir/$which_qemu/pid")"
	fi
	# amdvi_mem_ir_write
}

# 当在 qemu 中安装 vmware 的时候，需要做这些调整
function cmd_setup_vmware() {
	echo "ovmf_binary" >"$vm_dir/opt/bios"
	echo "vmxnet3" >"$vm_dir/opt/netdev"
	echo "ide" >"$vm_dir/opt/boot"
	# 需要更多的存储容量
	echo "1" >"$vm_dir/opt/sata"
}

function copy_to_guest() {
	local what=$1
	local scp_port=""
	get_ssh_info
	if [[ -n $ssh_port ]]; then
		# TODO 注意，这里 P 是大写的
		scp_port="-P $ssh_port"
	fi
	cmd="scp $scp_port $what $ssh_user@$ssh_ip:"
	echo "$cmd"
	eval "$cmd"
}

function execute_in_guest() {
	local cmd=$1
	get_ssh_info
	if [[ -n $ssh_port ]]; then
		ssh_port="-p$ssh_port"
	fi
	cmd="ssh $ssh_port $ssh_user@$ssh_ip '$cmd'"
	echo "$cmd"
	eval "$cmd"
}

# 测试 kexec 直接切换 kernel
function cmd_kexec() {
	if [[ ! -f "$vm_dir"/initrd || ! -f "$vm_dir"/kernel ]]; then
		error "kexec need initrd and kernel"
	fi

	copy_to_guest "$vm_dir/kernel"
	copy_to_guest "$vm_dir/initrd"
	echo " sudo kexec -l kernel --initrd=initrd --reuse-cmdline"
	echo " sudo kexec e"
}

function cmd_bg() {
	local original
	local now
	if check_option bg; then
		original=$option_result
		if [[ $original == 0 ]]; then
			now=1
		else
			now=0
		fi
	else
		original=1
		now=0
	fi
	echo $now >"$vm_dir/opt/bg"
	info "option is : $original ==> $now"
}

function checkpoint_restart() {
	:
	# CheckPoint and Restart
	# docs/devel/migration/CPR.rst
}

function cmd_vlan() {
	set -x

	# TODO 继续通过这里搞清楚如何实现 vlan 的 tag 和 trunk 的功能

	# 配置
	# sudo ovs-vsctl set port vif_s_"${guest_id}"_1 tag=100
	# sudo ovs-vsctl set port vif_s_"${guest_id}"_2 trunks=100,200

	# 清理
	sudo ovs-vsctl set port vif_s_"${guest_id}"_1 tag="[]"
	sudo ovs-vsctl set port vif_s_"${guest_id}"_2 tag="[]"
	sudo ovs-vsctl set port vif_s_"${guest_id}"_2 trunks="[]"

	# 配置的效果:
	# Port vif_s_19_2
	#     tag: 200
	#     trunks: [100, 200]
	#     Interface vif_s_19_2

	# TODO 不知道为什么，直接给 br-in 配置不可以:
	# sudo ovs-vsctl set port br-in trunks=100,200
	#
	# deepseek 说是需要配置新的 port 才可以，但是也不容易解决:
	#
	# sudo ip link add link br-in br-in.100 type vlan id 100
	# sudo ip addr add 10.0.255.100/16 dev br-in.100
	# sudo ip link set br-in.100 up
	#
	# sudo ip link add link br-in br-in.200 type vlan id 200
	# sudo ip addr add 10.0.255.200/16 dev br-in.200
	# sudo ip link set br-in.200 up
	#
	# sudo ovs-vsctl show
	#
	#     Bridge br-in
	#         Port vif_s_19_1
	#             tag: 100
	#             Interface vif_s_19_1
	#         Port vif_s_30_1
	#             tag: 100
	#             Interface vif_s_30_1
	set +x
}

function choose_vm() {
	option=$1
	readarray -d '' dirs_array < <(find "$all_vm_dir" -maxdepth 1 -type d -print0)
	live_vms=()
	for i in "${dirs_array[@]}"; do
		# 过滤一些无关的内容
		if [[ ! -f $i/cmd.sh ]]; then
			continue
		fi

		if [[ $option == "active" ]]; then
			if is_vm_active "$i"; then
				live_vms+=("$i")
			fi
		elif [[ $option == "inactive" ]]; then
			if ! is_vm_active "$i"; then
				live_vms+=("$i")
			fi
		else
			live_vms+=("$i")
		fi

	done

	if [[ ${#live_vms[@]} == 0 ]]; then
		echo "nothing get 😀"
		exit 0
	fi

	# 如果只有一个，那么无需 fzf 的选择
	if [[ ${#live_vms[@]} == 1 ]]; then
		choice="${live_vms[0]}"
		return
	fi

	choice=$(printf "%s\n" "${live_vms[@]}" | fzf)
	log "$choice"
}

cmd_action="none"
auto_yes=false
need_choose=false
specify_vm=""
while getopts "a:n:syh" opt; do
	case $opt in
		a) cmd_action=$OPTARG ;;
		n) specify_vm=$OPTARG ;;
		s) need_choose=true ;;
		h) show_help ;;
		y) auto_yes=true ;;
		*) show_help ;;
	esac
done
shift $((OPTIND - 1))

# ========== action -> filter 映射表（唯一需要维护的地方）==========
# filter: active | inactive | debug_kernel | (空=不限制)
declare -A action_filters=(
	# --- active (需要 VM 运行) ---
	["hotplug_cpu"]="active"
	["hotplug_mem"]="active"
	["hotplug_disk"]="active"
	["hotplug_nic"]="active"
	["hotplug_usb"]="active"
	["unplug_disk"]="active"
	["unplug_nic"]="active"
	["unplug_sriov"]="active"
	["dump_and_crash"]="active"
	["pty"]="active"
	["perf_qemu"]="active"
	["perf_guest"]="active"
	["kexec"]="active"
	["vmlinux"]="active"
	["trace"]="active"
	["kvm_dmesg"]="active"
	["kill"]="active"
	["cgroup"]="active"
	["hmp"]="active"
	["throttle"]="active"
	["top"]="active"
	["migrate"]="active"
	["gdb"]="active"
	["ssh"]="active"
	["ssh_auto"]="active"
	["ssh_copy_id"]="active"
	["auto_install"]="active"
	["balloon"]="active"
	["vnc"]="active"
	["monitor"]="active"
	["auto"]="active"
	["save_vm_cpr"]="active"
	["load_vm_cpr"]="active"
	["save_vm_file"]="active"
	["cpr_exec"]="active"
	["force_reboot"]="active"
	["migrate_cpr"]="active"
	["setup_nmcli"]="active"
	["tty3"]="active"

	# --- inactive (需要 VM 停止) ---
	["rename"]="inactive"
	["run"]="inactive"
	["cold_migrate"]="inactive"

	# --- debug_kernel ---
	["debug_kernel"]="debug_kernel"

	# --- 无限制 (空字符串) ---
	["unplug_vfio"]=""
	["edit"]=""
	["tmp_ip"]=""
	["bash_prompt"]=""
	["path"]=""
	["clone_vm"]=""
	["backup"]=""
	["restore"]=""
	["rsync"]=""
	["vlan"]=""
	["add_iso"]=""
	["add_boot_disk"]=""
	["setup_vmware"]=""
	["network"]=""
	["default"]=""
	["addr"]=""
	["bg"]=""
	["follow_log"]=""
	["log"]=""
)

# ========== 动态生成（无需维护）==========
all_actions=("${!action_filters[@]}")
# step 1 : 如果在命令行没有选择 action ，那么首先选择 action
if [[ $cmd_action == "none" ]]; then
	cmd_action=$(printf "%s\n" "${all_actions[@]}" | fzf)
fi

get_action_filter() {
	local action=$1
	if [[ ${action_filters[$action]+_} ]]; then
		echo "${action_filters[$action]}"
	elif [[ $action == "none" ]]; then
		echo "action is specified"
		exit 0
	else
		echo "unsupported action: $action"
		show_help
	fi
}

# step 2 : 需要选择虚拟机，还是直接使用 default 虚拟机，或者使用指定的虚拟机
action_filter=$(get_action_filter "$cmd_action")
if [[ -n $specify_vm ]]; then
	# 使用指定的虚拟机
	vm="${all_vm_dir}/${specify_vm}"
	if [[ ! -d $vm ]]; then
		error "VM not found: $specify_vm (expected at $vm)"
	fi
	set_default "$vm"
	if is_vm_active "$vm"; then
		if [[ $action_filter == "inactive" ]]; then
			error "$cmd_action need vm is inactive"
		fi
	else
		if [[ $action_filter == "active" ]]; then
			error "$cmd_action need vm is active"
		fi
	fi
elif [[ $need_choose == true ]]; then
	# 使用 fzf 手动选择虚拟机
	choose_vm "$action_filter"
	set_default "$choice"
	vm_dir="$choice"
else
	# 使用默认虚拟机，也就是 vm_dir_symbol 指向的虚拟机
	vm_dir="$(realpath "$vm_dir_symbol")"
	action_filter=$(get_action_filter "$cmd_action")
	case "$action_filter" in
		inactive)
			if is_vm_active "$vm_dir"; then
				error "vm is running"
			fi
			;;
		active)
			# 因为我们习惯于使用这个东西来 ge 来看那些机器是开机的状态
			# ge 首先默认使用 default ，如果关机了，那么使用当前还 active 的
			if ! is_vm_active "$vm_dir"; then
				if [[ $cmd_action == ssh ]]; then
					choose_vm "$action_filter"
					set_default "$choice"
					vm_dir="$choice"
				else
					error "$(basename "$vm_dir") is not running"
				fi
			fi
			;;
	esac
fi

check_vm_dir
setup_kernel
setup_qemu
setup_guest_id
setup_which_qemu
show_current_vm "$vm_dir"

# ========== action 执行器（通过约定 cmd_$action 自动调用）==========
execute_action() {
	local func="cmd_$1"
	if declare -f "$func" >/dev/null; then
		shift
		$func "$@"
	else
		error "function $func not found for action: $1"
	fi
}

# step 3 : 执行命令
# 特殊处理的 action（带参数或执行外部命令）
case "$cmd_action" in
	ssh_copy_id) cmd_ssh copy ;;
	run) "$SCRIPT_DIR"/collei.sh "$*" ;;
	default) echo "default is : $vm_dir" ;;
	clone_vm_auto) clone_vm_auto "$@" ;;
	# 其他 action 通过命名约定自动调用
	*) execute_action "$cmd_action" ;;
esac
