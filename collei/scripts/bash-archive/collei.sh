#!/usr/bin/env bash
set -E -e -u -o pipefail
replace_kernel=false
machine_uuid=""
arg_vmtest=""
arg_nixos=""
set -x

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
pushd "$SCRIPT_DIR" >/dev/null
# shellcheck source=collei/scripts/vfio.sh
source ./vfio.sh
# shellcheck source=collei/scripts/collei-net.sh
source ./collei-net.sh
# shellcheck source=collei/scripts/collei-lib.sh
source ./collei-lib.sh
popd >/dev/null

mkdir -p /tmp/martins3/

PWD=$(pwd)
PROGDIR=$(readlink -m "$SCRIPT_DIR/..")

# @todo 这个报错是什么意思
# qemu-system-x86_64: We need to set caching-mode=on for intel-iommu to enable device assignment with IOMMU protection.
# 有更好的配置方法吗？

vm_dir=""

function setup_2m_hugetlb() {
	local ram_gb=$1
	local hugetlb_path=/sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
	local target_pages=$((ram_gb * 512))
	local current_pages

	current_pages=$(cat "$hugetlb_path")
	if [[ $current_pages -ge $target_pages ]]; then
		return
	fi
	echo "$target_pages" | sudo tee "$hugetlb_path"
}

function setup_host_cpu_arch() {
	host_cpu_arch=none
	if grep "GenuineIntel" /proc/cpuinfo >/dev/null; then
		host_cpu_arch="intel"
	elif grep "AuthenticAMD" /proc/cpuinfo >/dev/null; then
		host_cpu_arch="amd"
	else
		host_cpu_arch="arm"
	fi
}

kernel_image=""
function get_kernel_image() {
	if [[ $ARCH == x86_64 ]]; then
		kernel_image=${kernel_dir}/arch/x86/boot/bzImage
	elif [[ $ARCH == aarch64 ]]; then
		kernel_image=${kernel_dir}/arch/arm64/boot/Image
		# kernel=/boot/vmlinuz-6.8.9-405.asahi.fc40.aarch64+16k
	fi
}

disk_format=""
function get_disk_format() {
	local disk=$1
	# 不用使用 qemu-img info ，如果 qemu 正在运行，会有 filelock 的问题
	if file "$disk" | grep "QEMU QCOW" &>/dev/null; then
		disk_format="qcow2"
	else
		disk_format="raw"
	fi
}

function setup_initramfs_arg() {
	# virtme 模式: 使用自定义 initramfs
	if is_virtme_mode; then
		generate_virtme_initramfs
		arg_initrd="-initrd $virtme_initramfs"
		return
	fi

	if check_option vmtest; then
		echo "vmtest mode don't need initramfs now"
		return
	fi

	# 如果配置了，那么就用默认的
	if check_option initrd; then
		arg_initrd="-initrd $option_result"
		# arg_initrd=""
	fi

	# 通过 kernel 的配置自动获取到 initrd
	check_option kernel
	local kernel=$option_result
	local kernel_name
	local kernel_path
	kernel_name=$(basename "$kernel")
	kernel_path=$(dirname "$kernel")

	# echo "$kernel_name"
	# echo "$kernel_path"

	readarray -d '' initramfs_array < <(find "$kernel_path" -maxdepth 1 -type f -name "*$kernel_name.raw.zst" -print0)
	if [[ ${#initramfs_array[@]} == 1 ]]; then
		initramfs=${initramfs_array[0]}
	else
		echo "need exactly one, found"
		exit 1
	fi
	# echo "$initramfs"
	arg_initrd="-initrd $initramfs"
}

function setup_kernel_initrd() {
	get_kernel_image
	arg_kernel="-kernel $kernel_image"
	setup_initramfs_arg
}

# 参数 name size format
# 返回值 @disk_path
disk_path=""
function create_disk_file() {
	local name=$1
	local size=${2:-10}
	local format=${3:-qcow2}

	local path="$vm_dir/img/${name}"
	if [[ ! -f $path ]]; then
		mkdir -p "${vm_dir}"/img/
		qemu-img create -f "$format" "$path" "${size}"G
	fi
	disk_path="${path}"
}

function create_disk_file_raw() {
	create_disk_file "$1" 10 raw
}

iommufd_counter=0
function passthrough_pci() {
	local dx=$1
	echo "unind $(lspci -s "$dx")"
	local use_iommufd=0
	pci_bind_to_vfio "$dx"

	if [[ $use_iommufd == 0 ]]; then
		# 我意识到通过这个 bootindex，可以测试物理网卡的 ipxe 功能
		# arg_vfio+="-device vfio-pci,host=$dx,bootindex=10"
		arg_vfio+="-device vfio-pci,host=$dx,rombar=0 "
	else
		# 使用 iommufd
		arg_vfio+=" -object iommufd,id=iommufd$iommufd_counter"
		arg_vfio+=" -device vfio-pci,host=$dx,iommufd=iommufd$iommufd_counter"
		iommufd_counter=$((iommufd_counter + 1))
	fi
}

# 看看 https://www.qemu.org/docs/master/system/devices/vfio-user.html 该如何模拟吧
function setup_vfio_user() {
	:
}

function setup_vfio() {
	arg_vfio=""

	if ! check_option vfio; then
		return
	fi

	readarray -t devices <<<"$option_result"
	for dx in "${devices[@]}"; do
		lspci -s "$dx"
	done

	for dx in "${devices[@]}"; do
		passthrough_pci "$dx"
	done
}

function setup_boot_nbd() {
	check_option disk_num
	boot_disk_num=$option_result
	if [[ $boot_disk_num -gt 1 ]]; then
		info "boot_disk_num=$boot_disk_num"
		error "No need to be so tricky now !"
	fi

	# 10 ms 之类不返回就是有问题
	fping "$nbd_ip" -t 10
	set -x
	python3 "$PROGDIR"/nbd/c.py --operation setup_nbd_disk \
		--host "$nbd_ip" \
		--dir "$vm_dir" \
		--disk 0
	set +x

	disk_idx=0
	get_tcp_port nbd $disk_idx
	local nbd_port=$tcp_port

	local loop_times=1000
	set -x
	for ((i = 1; i < loop_times; i++)); do
		local time
		time="$(echo "0.3 * $i" | bc)"
		sleep "$time"
		echo "nbd-client -l $nbd_ip $nbd_port"
		if nbd-client -l "$nbd_ip" $nbd_port; then
			break
		else
			[[ $i == $((loop_times - 1)) ]] && error "can't connect to nbd server"
		fi
	done
	echo "nbd server setup !"

	arg_boot_img=" -device virtio-scsi-pci,id=scsi4 "
	arg_boot_img+="-device scsi-hd,bus=scsi4.0,channel=0,scsi-id=0,lun=10,drive=root1,id=root1 "
	arg_boot_img+="-blockdev driver=nbd,server.type=inet,server.host=$nbd_ip,server.port=$nbd_port,export=martins3,node-name=root1 "
}

# TODO 似乎 virtio blk / virtio scsi 的 discard 是默认的选项
# 无论是 cat /sys/block/sdc/queue/discard_*

# 这里的 media=disk 的意思是什么?
# arg_boot_img+=" -drive file=$disk_path,format=$format,id=$id,if=none$aio,media=disk "

# 不知道为什么有的时候 swtpm 无法随着 qemu 结束而自动结束
# 如果发现了对应的虚拟机的 swtpm ，首先把他们杀掉
function workaround_tpswm() {
	readarray -t array < <(pgrep swtpm)
	for i in "${array[@]}"; do
		if grep "$vm_dir" /proc/"$i"/cmdline; then
			kill -9 "$i"
		fi
	done
}

arg_win11=""
function secure_boot() {
	if ! check_windows 11; then
		return
	fi
	workaround_tpswm
	# https://www.microsoft.com/en-us/software-download/windows10ISO
	#
	# win11 需要 secure boot
	mkdir -p "$vm_dir"/tpm
	arg_win11=" -chardev socket,id=chrtpm,path=$vm_dir/$which_qemu/swtpm-sock"
	arg_win11+=" -tpmdev emulator,id=tpm0,chardev=chrtpm"
	arg_win11+=" -device tpm-tis,tpmdev=tpm0"

	# https://undus.net/posts/qemu-install-windows11-guest/

	# https://github.com/infokiller/win10-vm

	# 关于 secure boot : https://wiki.debian.org/SecureBoot/VirtualMachine
	# TODO swtpm socket 也是可以无限开的，用起来有什么问题吗?

	pueue add -i -g qemu -- swtpm socket --tpmstate dir="$vm_dir"/tpm \
		--ctrl type=unixio,path="$vm_dir"/"$which_qemu"/swtpm-sock \
		--log level=20 --tpm2

	# TODO 有待理解的参数，是 windows 启动必须的?
	arg_win11+=" -global driver=cfi.pflash01,property=secure,value=on"

	# @todo 不知道为什么，windows 启动之后，所有的内存都会被踩一遍
	#
	# https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/archive-virtio/virtio-win-0.1.208-1/
	# https://leduccc.medium.com/improving-the-performance-of-a-windows-10-guest-on-qemu-a5b3f54d9cf5
}

arg_share_dir=""

setup_fs_share() {
	# virtme 模式: 共享 rootfs
	if is_virtme_mode; then
		setup_virtme_rootfs
		setup_virtme_vsock
		return
	fi

	local share_dir
	if check_option share_dir; then
		share_dir=$option_result
	else
		return 0
	fi

	if [[ ! -d $share_dir ]]; then
		"$share_dir is not a directory"
	fi

	virtfs_sock="$vm_dir/$which_qemu"/vfsd.sock
	# socket 不会自动消失的
	if [[ ! -e $virtfs_sock ]]; then
		echo "podman unshare -- virtiofsd --socket-path $virtfs_sock --shared-dir $share_dir --announce-submounts --sandbox chroot"
		echo "/usr/libexec/virtiofsd --socket-path $virtfs_sock --shared-dir $share_dir "
		echo "virtiofsd --socket-path $virtfs_sock --shared-dir $share_dir --cache=always"
	fi
	virtiofsd=virtiofsd
	if ! virtiofsd --help &>/dev/null; then
		virtiofsd=/usr/libexec/virtiofsd
	fi
	pueue add -i -g qemu -- $virtiofsd --socket-path "$virtfs_sock" --shared-dir "$share_dir" --allow-direct-io
	# --cache=always
	# 理论上说，参考这个就可以了:
	# https://github.com/virtio-win/kvm-guest-drivers-windows/wiki/Virtiofs:-Shared-file-system
	arg_share_dir=" -chardev socket,id=char0,path=$virtfs_sock"
	arg_share_dir+=" -device vhost-user-fs-pci,queue-size=1024,chardev=char0,tag=myfs"
}

function setup_network_user() {
	# arg_network="-netdev user,id=net1,hostfwd=tcp::$guest_port-:22 -device e1000e,netdev=net1"
	# arg_network="-netdev user,id=net1,$arg_fwd -device virtio-net-pci,netdev=net1,romfile=/home/martins3/core/zsh/README.md" # 替代 romfile
	net_device=$1
	get_tcp_port ssh
	local hostname
	local ssh_port=$tcp_port
	local fwd=",hostfwd=tcp::$ssh_port-:22"

	# 神奇，原来 hostname 可以通过网卡配置配置，真的好方便
	hostname=",hostname=$(basename "$vm_dir")"

	# TODO 这个 smb 如何理解，而且为什么配置到这个参数上
	# Could not find '/usr/sbin/smbd', please install it
	# qemu-system-x86_64: -netdev user,id=net1,hostfwd=tcp::50304-:22,hostname=fedora-42,smb=/home/martins3/: Could not find '/usr/sbin/smbd', please install it
	local smb=""
	if [[ -f /usr/sbin/smbd ]]; then
		smb=",smb=/home/martins3/"
	fi
	arg_network+=" -device $net_device,netdev=net1"
	arg_network+=" -netdev user,id=net1$fwd$hostname$smb"
}

function setup_network_tap() {
	local tap_dev=(
		"tap0-${guest_id}"
		# "tap1-${guest_id}"
	)

	for i in "${tap_dev[@]}"; do
		create_orphan_tap
		arg_network+=" -netdev tap,id=$tap_name,ifname=$tap_name,script=no,downscript=no,vhost=on"
		arg_network+=" -device virtio-net,netdev=$tap_name,mac=$mac_addr,iommu_platform=on,disable-legacy=on "
		# -net nic 已经明确标记为被弃用了
		# arg_network+=" -net nic,netdev=$i,model=virtio"
	done

}


function setup_network_sriov() {
	# sudo ip link set dev enp130s0f1np1 up
	if ! check_option sriov; then
		return
	fi
	local nic=$option_result

	split_vf "$nic"
	get_vf "$nic"
	if [[ -z $vf_target_pci ]]; then
		error "no vf available in $nic"
	fi
	echo "$vf_target_pci"
	# vf_target_pci="0000:05:01.3"
	passthrough_pci "$vf_target_pci"
}

function setup_network_emulate_sriov() {
	# TODO vhost 只能使用 sriov 吧
	#
	# https://kvm-forum.qemu.org/2024/Unleashing_SR-IOV_on_Virtual_Machines_qSX9OJ9.pdf
	#
	# arg_network+=" -netdev user,id=n -netdev user,id=o"
	# arg_network+=" -netdev user,id=p -netdev user,id=q"
	# arg_network+=" -device pcie-root-port,id=b"
	# arg_network+=" -device virtio-net-pci,bus=b,netdev=q,sriov-pf=f"
	# arg_network+=" -device virtio-net-pci,bus=b,netdev=p,sriov-pf=f"
	# arg_network+=" -device virtio-net-pci,bus=b,netdev=o,sriov-pf=f"
	# arg_network+=" -device virtio-net-pci,bus=b,netdev=n,id=f"

	# 参考 https://github.com/knuto/qemu/wiki/Knut's-QEMU-patchwork
	# echo 1 | sudo tee /sys/devices/pci0000:00/0000:00:07.0/0000:01:00.0/sriov_numvfs
	for ((i = 0; i < 2; i = i + 1)); do
		create_switch_tap
		local name=pcie_port.$i
		arg_network+=" -device pcie-root-port,slot=$((i + 1)),id=$name" # 使用 pcie-root-port
		arg_network+=" -device igb,bus=$name,netdev=igb$i,mac=$mac_addr"
		arg_network+=" -netdev tap,ifname=$tap_name,id=igb$i,script=no,downscript=no,vhost=off"
	done

	# 如果这样配置，有如下错误，必须配置 pcie-root-port
	# arg_network+=" -device igb"
	# [   27.382826] igb 0000:00:07.0: 1 VFs allocated
	# [   28.398573] pci 0000:00:17.0: [8086:10ca] type 7f class 0xffffff conventional PCI
	# [   28.399385] pci 0000:00:17.0: unknown header type 7f, ignoring device
	# [   28.765553] igb 0000:00:07.0 enp0s7: igb: enp0s7 NIC Link is Up 1000 Mbps Full Duplex, Flow Control: RX/TX

	# 切分之后，发现 device 都是 NO-CARRIER 的
	# 7: enp1s0v0: <NO-CARRIER,BROADCAST,MULTICAST,UP> mtu 1500 qdisc fq_codel state DOWN group default qlen 1000
	# link/ether fa:89:41:3b:06:3e brd ff:ff:ff:ff:ff:ff
	#
	# 如果使用 arg_network+=" -device igbvf " 会得到这个错误:
	#  Parameter 'driver' expects a pluggable device type
}

function setup_network_switch() {
	if check_global_option bridge; then
		if [[ $option_result == no ]]; then
			return
		fi
	fi
	init_switch

	net_device="$1"
	for ((i = 0; i < 1; i++)); do
		create_switch_tap
		local vhost_net=on
		# 可以达到的性能为: 13.8 GBytes ，而 tap 性能只有 10.7 GBytes
		# TODO ,multifunction=on 是什么东西
		arg_network+=" -device $net_device,netdev=$tap_name,mac=$mac_addr,iommu_platform=on,disable-legacy=on "
		arg_network+=" -netdev tap,ifname=$tap_name,id=$tap_name,script=no,downscript=no,vhost=$vhost_net"
	done
}

# 参考 man qemu(1) 中 -netdev bridge 中的描述，这里并不是没有创建 tap 设备，
# 而是因为 qemu 有代码自动创建了，他是用了脚本的，但是我不理解为什么需要在 qemu 中执行脚本
# arg_network+=" -device virtio-net-pci,netdev=hn0,id=nic1"
# arg_network+=" -netdev bridge,id=hn0,br=br0,helper=$PROGDIR/qemu-bridge-helper"

# https://wiki.qemu.org/Documentation/Networking
# sudo brctl addif virtbr0 enp3s0
arg_network=""
function setup_network() {
	setup_network_switch "virtio-net"
	# setup_network_tap
	setup_network_sriov
	# setup_network_emulate_sriov

	# 总是把用户态网络放到最后，这样在虚拟机中一眼就可以看到
	setup_network_user "virtio-net"
}

arg_rng=""
function setup_rng() {
	# win 11 一定需要 rng 的
	arg_rng+="-object rng-random,id=rng0,filename=/dev/urandom "
	arg_rng+="-device virtio-rng-pci,rng=rng0 "
}

arg_audio=""
function setup_audio() {
	# 虽然 QEMU 抛出了很多警告，但是也是在 pop os 的图形界面中的确都看到了东西
	# 默认关闭掉这个功能
	if ! check_option audio; then
		return
	fi
	# https://www.qemu.org/docs/master/system/devices/virtio/virtio-snd.html
	arg_audio+="-device virtio-sound-pci,audiodev=my_audiodev "
	arg_audio+="-audiodev alsa,id=my_audiodev "
}

setup_iommu() {
	hacking_iommu=false
	if ! check_option iommu; then
		return
	fi
	hacking_iommu=true
	local iommu_mode=$option_result

	# 实际上，看来 IOMMU 实际上是纯纯的模拟操作，你可以在 intel 的平台上使用 amd 的 IOMMU
	# iommu 的选项具体看: x86_iommu_properties 和 vtd_properties

	if [[ $host_cpu_arch == aarch64 && $iommu_mode == virtio ]]; then
		error "M2 aarch64 support virtio iommu"
	fi

	if [[ $iommu_mode != virtio && $arg_machine =~ "machine pc" ]]; then
		error "iommu need q35"
	fi

	case "$iommu_mode" in
		intel)
			# intel 需要使用 q35
			# device-iotlb=on 的作用是什么?
			arg_machine+=" -device intel-iommu,device-iotlb=on,intremap=on "
			arg_machine+=" ,caching-mode=on"
			arg_machine+=" ,x-pasid-mode=on,x-scalable-mode=on"
			;;
		amd)
			# amd 也是需要修改 machine, 不然报错为，但是这是不是最小约束不知道
			# qemu-system-x86_64: -device amd-iommu,intremap=on: Parameter 'driver' expects a dynamic sysbus device type for the machine
			arg_machine+=" -device amd-iommu,intremap=on"
			;;
		virtio)
			arg_machine+=" -device virtio-iommu-pci"
			;;
		*)
			echo "skip iommu :[$iommu_mode] 🐈"
			;;
	esac

	# TODO vfio + virtio 测试下
	# arg_machine=$intel_iommu
	echo " "
	# 使用 vfio + iommu 的场景下，使用 amd iommu 存在如下报错
	# qemu-system-x86_64: -device vfio-pci,host=03:00.0: vfio 0000:03:00.0: failed to setup container for group 16: memory listener initialization failed: Region amd_iommu: device 00.07.0 requires iommu notifier which is not currently supported

}

function setup_mdev_mtty() {
	if [[ ! -d /sys/devices/virtual/mtty/mtty/ ]]; then
		sudo modprobe mtty
	fi

	local uuid=83b8f4f2-509f-382f-3c1e-e6bfe0fa1001
	if [[ ! -d /sys/bus/mdev/devices/"$uuid" ]]; then
		echo "$uuid" | sudo tee /sys/devices/virtual/mtty/mtty/mdev_supported_types/mtty-2/create
	fi

	mdev_bind_to_vfio $uuid
	arg_mdev+=" -device vfio-pci,sysfsdev=/sys/bus/mdev/devices/$uuid "
	# echo 1 | sudo tee /sys/bus/mdev/devices/83b8f4f2-509f-382f-3c1e-e6bfe0fa1001/remove
}

# 00:08.0 Display controller [0380]: Red Hat, Inc. Device [1b36:000f] (rev 01)
function setup_mdev_mdpy() {
	if [[ ! -d /sys/devices/virtual/mdpy/mdpy ]]; then
		sudo modprobe mdpy
	fi

	local array=(
		82a31eeb-094b-4464-9a0e-69c416f76cd4
		# bd52f358-1920-48ed-9f85-920b58859259
	)
	local counter=0
	for uuid in "${array[@]}"; do
		if [[ ! -d /sys/bus/mdev/devices/"$uuid" ]]; then
			# TODO mdev_supported_types 下还有其他的设备，这些都如何理解
			echo "$uuid" | sudo tee /sys/devices/virtual/mdpy/mdpy/mdev_supported_types/mdpy-vga/create
		fi

		mdev_bind_to_vfio "$uuid"
		# arg_vfio+=" -object iommufd,id=iommufd_mdpy$counter"
		# arg_mdev+=" -device vfio-pci,sysfsdev=/sys/bus/mdev/devices/$uuid,iommufd=iommufd_mdpy$counter"

		arg_mdev+=" -device vfio-pci,sysfsdev=/sys/bus/mdev/devices/$uuid"

		counter=$((counter + 1))
	done

}

function setup_mdev_hct() {
	if ! check_option hct; then
		return
	fi
	readarray -t array <<<"$option_result"
	local counter=0
	for uuid in "${array[@]}"; do
		if [[ ! -d /sys/bus/mdev/devices/"$uuid" ]]; then
			echo "$uuid" | sudo tee /sys/devices/virtual/hct/hct/mdev_supported_types/hct-1/create
			if [[ -f /sys/bus/mdev/devices/"${uuid}"/vendor/use ]]; then
				echo 1 | sudo tee /sys/bus/mdev/devices/"${uuid}"/vendor/use
			fi
		fi
		local iommu_group
		local sysfsdev=/sys/bus/mdev/devices/$uuid
		iommu_group=$(basename "$(realpath "$sysfsdev"/iommu_group)")
		set -x
		change_file_owner /dev/vfio/"$iommu_group"
		set +x

		# hct 有自己的启动方式，这种启动方式会有问题
		# Failed to set up TRIGGER eventfd signaling for interrupt INTX-0: VFIO_DEVICE_SET_IRQS failure: Invalid argument
		# arg_mdev+=" -device vfio-pci,sysfsdev=$sysfsdev"
		#
		# -device hct,id=$uuid,sysfsdev=/sys/bus/mdev/devices/uuid,bus=pci.0,addr=0xa
		arg_mdev+=" -device hct,id=hct,sysfsdev=/sys/bus/mdev/devices/$uuid"
	done
}

function setup_mdev() {
	arg_mdev=""
	# setup_mdev_mtty
	# setup_mdev_mdpy
	# setup_mdev_hct
	# setup_mdev_vgpu
}

function setup_cpu_model() {
	if [[ $ARCH != x86_64 ]]; then
		arg_cpu_model="-cpu host"
		return
	fi

	local model
	model="-cpu host"
	# model="-cpu Skylake-Client-IBRS,hle=off,rtm=off"
	# 如果 see=off 或者 see2=off ，系统直接无法启动
	#
	# model="-cpu Skylake-Client-IBRS,hle=off,rtm=off,sse4_2=off,sse4_1=off,ssse3=off,sep=off"
	# model="-cpu Skylake-Client-IBRS,vmx=on,hle=off,rtm=off"
	# model="-cpu Broadwell-noTSX-IBRS,vmx=on,hle=off,rtm=off"
	# model="-cpu Broadwell-noTSX-IBRS,spec_ctrl=on,ssbd=on,ibrs_all=on,arch_capabilities=on"
	# model="-cpu Icelake-Server,arch_capabilities=on,sbdr-ssdp-no=on,ibrs-all=on,ssb-no"
	# model="-cpu Icelake-Server,arch_capabilities=on,rdctl_no=on"
	# model="-cpu host,phys_bits=37,host-phys-bits=off"
	# model="-cpu host,phys-bits=42,host-phys-bits-limit=40"
	# model="-cpu core2duo"
	# model="-cpu SandyBridge"
	# model="-cpu Broadwell-IBRS"
	# model="-cpu Skylake-Client"
	# model="-cpu Skylake"
	# model="-cpu Denverton"
	# model="-cpu GraniteRapids"
	#
	# intel 平台上使用 model="-cpu Opteron_G1"  cat /proc/cpuinfo 之后可以看到:
	# vendor_id       : GenuineIntel
	# model name      : AMD EPYC Processor
	# 当使用 -cpu EPYC 的时候还是会存在警告的，但是 -cpu Opteron_G1 不会存在任何警告。
	#
	# model="-cpu host,hv_vapic,-kvm-asyncpf,-kvm-asyncpf-int"
	# model="-cpu host,tsc-frequency=1000000000,pmu=on"
	# model="-cpu host,tsc-frequency=1000000000"
	# model="-cpu max"
	# model="-cpu host"
	# model="-cpu host"
	if check_option kvmclock; then
		model="-cpu host"
	fi
	if check_windows; then
		model="-cpu host"
		# TODO 使用 model="-cpu host,hv-time,hv-relaxed,hv-spinlocks=0x1000" #windows 8 无法启动
		# model="-cpu host,hv-time,hv-relaxed,hv-spinlocks=0x1000"
		# model="-cpu host,hv_time,hv_relaxed,hv_vapic,hv_spinlocks=0x1000 "
		# model="-cpu host,hv_time,hv_relaxed,hv_spinlocks=0x1000 "
		model="-cpu host,hv_spinlocks=0x1fff,hv_vapic,hv_time,hv_reset,hv_vpindex,hv_runtime,hv_relaxed"
		:
	fi
	arg_cpu_model=$model
}
function setup_pci_bridge() {
	# TODO 有待分析的问题:
	# 1. 为什么 bridge 下的两个 nvme 都是在一个 iommu group 下的
	# 2. 这个搭配 intel 的 iommu ，结果必有
	# [    1.621462] DMAR: DRHD: handling fault status reg 2
	# [    1.622449] DMAR: [DMA Read NO_PASID] Request device [00:0d.0] fault addr 0x10c176000 [fault reason 0x02] Present bit in context entry is clear
	# [    1.623699] DMAR: Dump dmar0 table entries for IOVA 0x10c176000
	# [    1.624998] DMAR: root entry: 0x00000001032b5001
	# [    1.624999] DMAR: context entry: hi 0x0000000000000000, low 0x0000000000000000
	# [    1.626163] DMAR: pte level: 2, pte value: 0xf000ff53f000ff53
	# [    1.626915] Oops: general protection fault, probably for non-canonical address 0xf00087d3f000fbb0: 0 000 [#1] PREEMPT SMP NOPTI

	local bridge=""
	create_disk_file nvme2
	# 这些 device 都是可以设置 bus 的
	# TODO 不知道为什么，使用上 virtio iommu 的时候，pci.0 这个 bus 消失了
	# 需要去掉 bus=pci.0,addr=0xa
	bridge="-device pci-bridge,id=mybridge,chassis_nr=1"
	for ((i = 0; i < 2; i = i + 1)); do
		create_disk_file nvme1
		printf '%s\n' "$i"
		bridge+=" -device nvme,drive=nvme$i,serial=$(uuidgen),bus=mybridge,addr=0x1 "
		bridge+=" -drive file=$disk_path,format=qcow2,if=none,id=nvme$i"
	done

	# 到时候把 setup_nvme_basic 中 bridge 部分放到这里来:
	bridge+=" -device pci-bridge,id=bridge0,chassis_nr=1"
	bridge+=" -device pci-bridge,id=bridge1,chassis_nr=2"
	arg_pci_topo+=$bridge
}
function setup_pci_root_bus() {
	# i440fx 使用 pxb
	if [[ $arg_machine =~ .*pc.* ]]; then
		# 多出来的是这个:
		# 03:00.0 PCI bridge: Red Hat, Inc. QEMU PCI-PCI bridge
		# 08:00.0 PCI bridge: Red Hat, Inc. QEMU PCI-PCI bridge
		arg_pci_topo+=" -device pxb,id=bridge2,bus=pci.0,bus_nr=3"
		arg_pci_topo+=" -device pxb,id=bridge3,bus=pci.0,bus_nr=8"
		echo ""
	else
		# TODO 很奇怪，多出来的地方是这样的，和  oracle 的 blog 讲的不一样
		# 00:13.0 Host bridge: Red Hat, Inc. QEMU PCIe Expander bridge
		# 00:14.0 Host bridge: Red Hat, Inc. QEMU PCIe Expander bridge
		# 00:1f.0 ISA bridge: Intel Corporation 82801IB (ICH9) LPC Interface Controller (rev 02)
		# 00:1f.2 SATA controller: Intel Corporation 82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode] (rev 02)
		# 00:1f.3 SMBus: Intel Corporation 82801I (ICH9 Family) SMBus Controller (rev 02)
		arg_pci_topo+=" -device pxb-pcie,id=pcie.1,bus_nr=2,bus=pcie.0"
		arg_pci_topo+=" -device pxb-pcie,id=pcie.2,bus_nr=8,bus=pcie.0"
		echo " "
	fi
}

function setup_pci_root_complex() {
	# 如果配置使用了这个，
	# qemu-system-x86_64: ../hw/virtio/virtio-pci.c:620: virtio_address_space_lookup: Assertion `mrs.mr' failed.
	if [[ $arg_machine =~ .*pc.* ]]; then
		arg_pci_topo+=" -device ioh3420,id=root_port1,bus=pci.0"
	else
		arg_pci_topo+=" -device ioh3420,id=root_port1,bus=pcie.0"
	fi
}

function setup_pci_switch() {
	# 有问题
	arg_pci_topo+=" -device x3130-upstream,id=upstream1,bus=root_port1"
	arg_pci_topo+=" -device xio3130-downstream,id=downstream1,bus=upstream1,chassis=9"
	arg_pci_topo+=" -device virtio-scsi-pci,bus=downstream1"
	arg_pci_topo+=" -device xio3130-downstream,id=downstream2,bus=upstream1,chassis=10"
	arg_pci_topo+=" -device virtio-scsi-pci,bus=downstream2"
}

arg_pci_topo=""
function setup_pci_topo() {
	# 基本上是参考:
	# https://blogs.oracle.com/linux/post/a-study-of-the-linux-kernel-pci-subsystem-with-qemu
	# 除了 bridge 是正常的，其余的基本都不太能用
	if ! check_option pci; then
		return
	fi
	local pci_option=$option_result
	case $pci_option in
		bridge)
			setup_pci_bridge
			;;
		root_bus)
			setup_pci_root_bus
			;;
		root_complex)
			setup_pci_root_complex
			;;
		switch)
			setup_pci_switch
			;;
	esac

	# TODO 还有这种配置方法:
	# https://github.com/knuto/qemu/wiki/Knut's-QEMU-patchwork
	# -device pcie-root-port,slot=2,id=pcie_port.2
	# -device igb,bus=pcie_port.2
}

arg_edu=""
function setup_edu() {
	# 	./qemu-system-x86_64 -device edu,help
	# edu options:
	#   acpi-index=<uint32>    -  (default: 0)
	#   addr=<str>             - Slot and optional function number, example: 06.0 or 06 (default: -1)
	#   busnr=<busnr>
	#   dma_mask=<uint64>
	#   failover_pair_id=<str>
	#   multifunction=<bool>   - on/off (default: off)
	#   rombar=<int32>         -  (default: -1)
	#   romfile=<str>
	#   romsize=<uint32>       -  (default: 4294967295)
	#   sriov-pf=<str>
	#   x-max-bounce-buffer-size=<size> - Maximum buffer size allocated for bounce buffers used for mapped access to indirect DMA memory (default: 4096)
	#   x-pcie-ari-nextfn-1=<bool> - on/off (default: off)
	#   x-pcie-err-unc-mask=<bool> - on/off (default: on)
	#   x-pcie-ext-tag=<bool>  - on/off (default: on)
	#   x-pcie-extcap-init=<bool> - on/off (default: on)

	# 相关文档 : docs/specs/edu.rst
	# 虚拟机中可以观测到:
	# 00:07.0 Unclassified device [00ff]: Device [1234:11e8] (rev 10)
	arg_edu="-device edu"
}

# ============================================
# virtme 模式支持
# ============================================

# 检测是否启用 virtme 模式
function is_virtme_mode() {
	if check_option virtme; then
		return 0
	fi
	return 1
}

function get_virtme_mode() {
	if ! is_virtme_mode; then
		return 1
	fi

	local mode=""
	if check_option virtme_mode; then
		mode=$option_result
	elif check_option exec; then
		mode="exec"
	else
		mode="manual"
	fi

	case "$mode" in
		manual | exec)
			option_result=$mode
			;;
		*)
			error "unknown virtme_mode: $mode"
			;;
	esac
}

function is_virtme_manual_mode() {
	if ! get_virtme_mode; then
		return 1
	fi
	[[ $option_result == "manual" ]]
}

# 设置 virtme rootfs 共享 (virtio-fs)
function setup_virtme_rootfs() {
	log "setting up virtme rootfs..."

	# 1. 确定共享目录 (默认是 /)
	local share_root="/"
	if check_option share_root; then
		share_root="$option_result"
	fi

	# 2. 启动 virtiofsd (rootfs)
	local virtfs_sock="$vm_dir/$which_qemu/virtme.sock"

	# 使用 pueue 启动 virtiofsd
	local virtiofsd_bin=""
	if command -v virtiofsd &>/dev/null; then
		virtiofsd_bin=$(command -v virtiofsd)
	elif [[ -x "/usr/libexec/virtiofsd" ]]; then
		virtiofsd_bin="/usr/libexec/virtiofsd"
	fi

	if [[ -z $virtiofsd_bin ]]; then
		error "virtiofsd not found, please install virtiofsd"
	fi

	pueue add -i -- "$virtiofsd_bin" \
		--socket-path "$virtfs_sock" \
		--shared-dir "$share_root" \
		--sandbox none \
		--cache always \
		--no-announce-submounts

	# 3. 设置 QEMU 参数
	arg_share_dir=" -chardev socket,id=virtme_root,path=$virtfs_sock"
	arg_share_dir+=" -device vhost-user-fs-pci,chardev=virtme_root,tag=ROOTFS"
	log "virtme rootfs configured: $share_root -> ROOTFS"
}

# 生成 virtme initramfs
function generate_virtme_initramfs() {
	local out_file="$vm_dir/$which_qemu/virtme-initramfs.cpio.gz"
	local tmpdir
	tmpdir=$(mktemp -d)

	log "generating virtme initramfs..."

	# 1. 创建目录结构
	mkdir -p "$tmpdir"/{bin,dev,proc,sys,newroot,run,lib/modules,tmp}

	# 2. 查找 busybox (优先静态链接版本)
	local busybox_bin=""
	for bb in busybox-static busybox.static busybox; do
		if command -v "$bb" &>/dev/null; then
			busybox_bin=$(command -v "$bb")
			break
		fi
	done

	if [[ -z $busybox_bin ]]; then
		rm -rf "$tmpdir"
		error "busybox not found, please install busybox-static"
	fi

	# 检查是否静态链接
	if ! file "$busybox_bin" | grep -q "statically linked\|static-pie"; then
		log "warning: busybox may not be statically linked"
	fi

	cp "$busybox_bin" "$tmpdir/bin/busybox"
	chmod +x "$tmpdir/bin/busybox"

	# 创建常用命令链接
	for cmd in sh mount umount switch_root insmod modprobe mkdir mknod sleep uname cp cat chmod echo ln printf base64 setsid cttyhack; do
		ln -sf busybox "$tmpdir/bin/$cmd"
	done

	# 3. 创建设备节点
	# 如果 devtmpfs 不可用，需要这些基本设备
	[[ -e "$tmpdir/dev/null" ]] || mknod -m 666 "$tmpdir/dev/null" c 1 3 2>/dev/null || true
	[[ -e "$tmpdir/dev/zero" ]] || mknod -m 666 "$tmpdir/dev/zero" c 1 5 2>/dev/null || true
	[[ -e "$tmpdir/dev/random" ]] || mknod -m 666 "$tmpdir/dev/random" c 1 8 2>/dev/null || true
	[[ -e "$tmpdir/dev/urandom" ]] || mknod -m 666 "$tmpdir/dev/urandom" c 1 9 2>/dev/null || true
	[[ -e "$tmpdir/dev/console" ]] || mknod -m 622 "$tmpdir/dev/console" c 5 1 2>/dev/null || true
	[[ -e "$tmpdir/dev/kmsg" ]] || mknod -m 660 "$tmpdir/dev/kmsg" c 1 11 2>/dev/null || true

	# 4. 复制 init 脚本
	local init_script="$PROGDIR/virtme/virtme-init.sh"
	if [[ ! -f $init_script ]]; then
		rm -rf "$tmpdir"
		error "virtme-init.sh not found at $init_script"
	fi

	cp "$init_script" "$tmpdir/init"
	chmod +x "$tmpdir/init"

	# 5. 复制必要内核模块 (如果内核目录可用)
	if [[ -n ${kernel_dir:-} && -d $kernel_dir ]]; then
		log "collecting kernel modules..."

		# 查找并复制必要模块
		# 注意: virtiofs 模块文件名是 virtiofs.ko，但加载时可能用 virtio_fs
		# 注意: virtiofs 依赖 fuse，需要先加载 fuse
		local mod_mappings=()

		# 检查机器类型：microvm 使用 virtio-mmio，其他使用 virtio-pci
		local machine_type="pc"
		# 使用 vm_dir_symbol 获取机器类型（因为 vm_dir 此时可能还未设置）
		local vm_dir_for_machine="${vm_dir:-$(realpath "$vm_dir_symbol" 2>/dev/null)}"
		if [[ -f "$vm_dir_for_machine/opt/machine" ]]; then
			machine_type=$(cat "$vm_dir_for_machine/opt/machine")
		fi

		if [[ $machine_type == "microvm" ]]; then
			# microvm 使用 virtio-mmio 设备
			# 注意: virtio_mmio 通常是内建的 (CONFIG_VIRTIO_MMIO=y)，不需要加载模块
			log "machine type: microvm, using virtio-mmio (usually built-in)"
			mod_mappings=("fuse:fuse" "virtio_fs:virtiofs" "overlay:overlay")
		else
			# pc/q35 使用 virtio-pci 设备
			log "machine type: $machine_type, using virtio-pci modules"
			mod_mappings=("virtio_pci_modern_dev:virtio_pci_modern_dev" "virtio_pci_legacy_dev:virtio_pci_legacy_dev" "virtio_pci:virtio_pci" "fuse:fuse" "virtio_fs:virtiofs" "overlay:overlay")
		fi

		for mapping in "${mod_mappings[@]}"; do
			local modname="${mapping%%:*}"
			local modfile_name="${mapping##*:}"
			local modfile
			modfile=$(find "$kernel_dir" -name "${modfile_name}.ko*" -type f 2>/dev/null | head -1)
			if [[ -n $modfile ]]; then
				local ext="${modfile##*.}"
				if [[ $ext == "zst" ]]; then
					# 解压 zstd 压缩的模块
					zstd -d -c "$modfile" >"$tmpdir/lib/modules/${modname}.ko" 2>/dev/null || true
				elif [[ $ext == "ko" ]]; then
					# 复制并重命名为 init 脚本期望的名称
					cp "$modfile" "$tmpdir/lib/modules/${modname}.ko"
				fi
			fi
		done
	fi

	# 6. 打包为 cpio.gz
	(
		cd "$tmpdir"
		find . -print0 | cpio --null -o --format=newc 2>/dev/null | gzip >"$out_file"
	)

	local initramfs_size
	initramfs_size=$(stat -c%s "$out_file" 2>/dev/null || echo "0")
	log "virtme initramfs created: $out_file (${initramfs_size} bytes)"

	# 清理
	rm -rf "$tmpdir"

	virtme_initramfs="$out_file"
}

# TODO 这几个依赖有点奇怪，iommu 和 pci 的设置其实都是依赖
# 到底是 q35 还是 pc ，所以 setup_iommu 中继续使用 arg_machine
# 所以，这里的依赖反过来了，如果发现了 iommu 或者 pci topo ，
function setup_machine() {
	# arg_machine=" -machine microvm,pcie=on,rtc=on"
	# TODO 确认一下这个两个，低优先级
	# microvm 不支持 hpet=off 参数
	# microvm 不支持 seabios，使用默认的 bios

	if [[ $ARCH == x86_64 ]]; then
		arg_machine=" -machine pc"
		# arg_machine=" -machine q35"
		# arg_machine=" -machine microvm"

		# arg_machine+=",usb=off"
		# arg_machine+=",pit=off"
		# arg_machine+=",acpi=off"
		arg_machine+=",hpet=off"
		arg_machine+=",smm=off"
		# arg_machine+=",cxl=on"
	elif [[ $ARCH == aarch64 ]]; then
		# TODO 思考一个问题，如果 arm 环境中不可以带 kernel-irqchip=split
		# 那么 arm 是如何来实现驱动的模拟的
		arg_machine=" -machine virt"
		arg_machine+=" -cpu host "
	fi
	# cpr 需要的，和上面的东西没有耦合
	arg_machine+=" -machine aux-ram-share=on "

	# 给机器增加 iommu 需要特殊的配置吗?
	setup_iommu
	# 这个 setup pci topo 应该是在 arm 测试一下，看看什么是正常的东西
	setup_pci_topo
}

function setup_accel() {
	arg_accel+=" -accel kvm"
	# arg_accel+=",dirty-ring-size=4096 "
	if check_option accel; then
		local accel=$option_result
		if [[ $accel == tcg ]]; then
			# arg_accel="--accel tcg,thread=single"
			arg_accel="--accel tcg"
			arg_cpu_model="" # cpu model 不能支持 host 了
		else
			error "unknown accel"
		fi
	fi
}

arg_mem_balloon=""

# Apple 上会报告 host address bit limit 只有 32 无法配置 256G 的
# qemu-system-aarch64: Invalid CPU topology: maxcpus must be equal to or greater than smp:
# sockets (1) * clusters (1) * cores (32) * threads (1) == maxcpus (32) < smp_cpus (96)
# TODO Apple 上 CPU 有限制吗?
# maxcpus 是 32 ，那么 node0_cpus 也许调整
function is_asahi() {
	grep "CPU implementer" /proc/cpuinfo | grep 0x61 >/dev/null
}

function get_mem_size() {
	ramsize=16
	# 1. max_ramsize 需要 phys_bits 足够大，但是如何自动检测 phys_bits 呢?
	# 2. max_ramsize 需要比 ram 大，不可以等于
	#
	# 如果配置 max_ramsize=2048G ，就容易触发这个问题
	# qemu-system-x86_64: Address space limit 0xffffffffff < 0x202bfffffff phys-bits too low (40)
	max_ramsize=256

	if check_option ram; then
		ramsize=$option_result
	fi

	if [[ $ramsize -ge $max_ramsize ]]; then
		error "max_ramsize=$max_ramsize ramsize=$ramsize"
	fi

	if [[ $ARCH == aarch64 ]]; then
		if is_asahi; then
			max_ramsize=32
			max_cpu_num=32
		fi
	fi
	# TODO 需要给 slot 指定 numa 吗? slots 的含义是什么?
	arg_mem_cpu+=" -m ${ramsize}G,slots=7,maxmem=${max_ramsize}G"
}

function get_cpu_num() {
	cpu_num=8
	max_cpu_num=$(($(getconf _NPROCESSORS_ONLN) + 0))
	# pc machine 最多启动 255
	# max_cpu_num=254
	max_cpu_num=8
	if check_option smp; then
		cpu_num=$option_result
	fi

	# 如果开启了 numa ，那么就不支持热插
	# 原因见 docs/qemu/cpu-topo.md
	if check_option numa_num; then
		max_cpu_num=$cpu_num
	fi

	# 将 max_cpu_num 设置超过物理机的 CPU 数量，qemu 总是会警告
	# 无论是 CPU 热插还是什么，其实都用不了这么多的 CPU 的
	if [[ $cpu_num -gt $max_cpu_num ]]; then
		error "max_cpu_num=$max_cpu_num cpu_num=$cpu_num"
	fi
	local maxcpus_per_socket=$((max_cpu_num / 2))
	echo $maxcpus_per_socket
	# 这里假设是两个 socket ，其实还好
	# TODO clusters=1 是 qemu 6.2 之后的功能，这个功能就感觉很奇怪
	arg_mem_cpu=" -smp $cpu_num,maxcpus=${max_cpu_num},sockets=$SOCKET_NUM,dies=1,cores=$maxcpus_per_socket,threads=1"
}

# arg_mem_cpu="$arg_mem_cpu -smp sockets=2,cores=2"
# arg_mem_cpu=" -smp sockets=2,cores=2"
# arg_mem_cpu="-m 12G,maxmem=12G"
function get_mem_obj() {
	numa_num=$1
	local node_ramsize=$((ramsize / numa_num))
	mode="private"
	mode="memfd"
	for ((i = 0; i < numa_num; i = i + 1)); do

		case $mode in
			private)
				# TODO 如果 -object memory-backend-ram 的话，virtio-fs 是无法使用的，即便是已经 share=on
				arg_mem_cpu+=" -object memory-backend-ram,id=mem$i,size=${node_ramsize}G,prealloc=off,share=off "
				;;
			share)
				# 如果用 virtiofs ，那么需要使用这种方法来共享内存给其他的 thread ，但是不确定这是不是唯一方法
				arg_mem_cpu+=" -object memory-backend-file,id=mem$i,size=${node_ramsize}G,mem-path=/dev/shm/qemu.$guest_id.$i,share=on,discard-data=on"
				;;
			async)
				# 只有使用这种方式才会启动 async page fault
				# 如果真的如此，memory-backend-file 如何区分自己的内存到底是在
				arg_mem_cpu+=" -object memory-backend-file,id=mem$i,size=${node_ramsize}G,prealloc=off,mem-path=$vm_dir/qemu.ram.$i,share=on"
				;;
			hugepage)
				# TODO 实现一个自动分配的代码
				# echo 6144 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
				arg_mem_cpu+=" -object memory-backend-file,id=mem$i,size=${node_ramsize}G,prealloc=off,mem-path=/dev/hugepages/$guest_id.$i "
				;;
			memfd)
				# 这才是标准的共享给 vhost 的方法
				arg_mem_cpu+=" -object memory-backend-memfd,id=mem$i,size=${node_ramsize}G,prealloc=off,share=on "
				;;
			*) ;;
		esac
	done

	# TODO 把这里的内容都清理一下
	if [[ -z $arg_mem_cpu ]]; then
		arg_mem_cpu+=" -object memory-backend-ram,id=mem0,size=2G"
		arg_mem_cpu+=" -object memory-backend-ram,id=mem1,size=2G"
		arg_mem_cpu+=" -object memory-backend-ram,id=mem2,size=2G"
		arg_mem_cpu+=" -object memory-backend-ram,id=mem3,size=2G"
		arg_mem_cpu+=" -device virtio-mem-pci,id=vm0,memdev=mem2,node=0,requested-size=1G"
		arg_mem_cpu+=" -device virtio-mem-pci,id=vm1,memdev=mem3,node=1,requested-size=1G"
		arg_mem_cpu+=" -numa node,nodeid=0,cpus=0-1,nodeid=0,memdev=mem0"
		arg_mem_cpu+=" -numa node,nodeid=1,cpus=2-3,nodeid=1,memdev=mem1"

		# 1. QEMU 将 pmem 的大小也是计算到 maxmem 中的
		# 2. 如果加上 maxmem 的设置，那么存在如下的报错
		# memory devices (e.g. for memory hotplug) are not enabled, please specify the maxmem option
		local pmem_img=${vm_dir}/virtio_pmem.img
		arg_mem_cpu+=" -object memory-backend-file,id=nvmem1,share=on,mem-path=${pmem_img},size=4G"
		arg_mem_cpu+=" -device virtio-pmem-pci,memdev=nvmem1,id=nv1"
	fi

}

# 当 2 socket 16 numa 的时候，其 latency 规则为:
# 1. l3 内 10
# 2. numa 内 11 ，一个 l3 中
# 3. socket 内 12
# 4. soccket 外 32
latency=0
function get_numa_latency_2socket_16numa() {
	src=$1
	dest=$2

	if [[ $src == "$dest" ]]; then
		latency=10
		return
	fi

	local src_l3=$((src / 2))
	local dest_l3=$((dest / 2))

	if [[ $src_l3 == "$dest_l3" ]]; then
		latency=11
		return
	fi

	local src_socket=$((src / 8))
	local dest_socket=$((dest / 8))

	if [[ $src_socket == "$dest_socket" ]]; then
		latency=12
		return
	fi

	latency=32
}

# TODO 现在的配置让 cmd.sh 中的 qemu 启动参数太长了
function setup_memory_explict() {
	SOCKET_NUM=2
	numa_num=2
	if check_option numa_num; then
		numa_num=$option_result
	fi
	arg_mem_cpu=""
	get_cpu_num

	get_mem_size
	get_mem_obj "$numa_num"

	# echo 1 | sudo tee /proc/sys/vm/overcommit_memory
	# 从 qemu 的设计来讲，但是现在的情况是，
	# numa 是内存的分布，socket 是 cpu 的分布
	# 两个不用耦合，但是实际上是，操作系统中认为 cpu 在 numa 中的
	#
	# 例如这个警告
	# qemu-system-aarch64: warning: CPU-0 and CPU-1 in socket-0-cluster-0 have been associated with node -0 and node-1 respectively.
	# It can cause OSes like Linux to misbehave
	for ((i = 0; i < numa_num; i = i + 1)); do
		arg_mem_cpu+=" -numa node,nodeid=$i,memdev=mem$i"
	done

	for ((i = 0; i < max_cpu_num; i = i + 1)); do
		local socket_id=$((i % SOCKET_NUM))
		local numa_id=$((i % numa_num))
		# 原来如此，这里可以指定一个 CPU 分配到哪一个 nuam 和哪一个 socket 上
		arg_mem_cpu+=" -numa cpu,node-id=$numa_id,socket-id=$socket_id,core-id=$((i / 2))"
	done

	if [[ $numa_num == 16 ]]; then
		for ((i = 0; i < numa_num; i = i + 1)); do
			for ((j = i + 1; j < numa_num; j = j + 1)); do
				get_numa_latency_2socket_16numa "$i" "$j"
				arg_mem_cpu+=" -numa dist,src=$i,dst=$j,val=$latency"
			done
		done
	fi
}

function setup_memory_implict() {
	local cpu_num
	local max_cpu_num
	cpu_num=$(($(getconf _NPROCESSORS_ONLN) + 0))
	max_cpu_num=$(($(getconf _NPROCESSORS_ONLN) + 0))
	# max_cpu_num=4

	ramsize=8
	if check_option smp; then
		cpu_num=$option_result
	fi

	if [[ $cpu_num -gt $max_cpu_num ]]; then
		# cpu_num=$max_cpu_num
		max_cpu_num=$cpu_num
	fi
	if check_option ram; then
		ramsize=$option_result
	fi
	# 是的，没错，为了实现最基本的功能，热插，vhost 共享，这就是最简单的配置
	arg_mem_cpu+=" -smp ${cpu_num},maxcpus=$max_cpu_num"
	if is_asahi; then
		arg_mem_cpu+="  -m ${ramsize}G,slots=8,maxmem=32G"
	else
		arg_mem_cpu+="  -m ${ramsize}G,slots=8,maxmem=256G"
	fi

	local hugetlb=false
	if check_option hugetlb; then
		hugetlb=true
		setup_2m_hugetlb "$ramsize"
	fi
	arg_mem_cpu+=" -object memory-backend-memfd,id=mem0,size=${ramsize}G,prealloc=off,share=on,hugetlb=$hugetlb "

	# 2026-04-21 这段当时想要表达什么来着?
	# 如果使用 hugetlb 的话
	# arg_mem_cpu+=" -object memory-backend-ram,id=mem0,size=${ramsize}G,prealloc=off,share=off "
	# arg_mem_cpu+=" -object memory-backend-ram,id=mem0,size=${ramsize}G,prealloc=off,share=on "

	# arg_mem_cpu+=" -object memory-backend-file,id=mem0,size=${ramsize}G,mem-path=/dev/shm/qemu_$guest_id,share=on,discard-data=on"

	arg_mem_cpu+=" -numa node,nodeid=0,memdev=mem0"
	# 这个 mem0 必须被这个 numa 配置或者 -machine q35,memory-backend=mem
	# 不然，就是让 qemu 多分配了一个内存而已，这个会让 qemu-storage-daemon 或者 virtiofsd 报错
	# 但是他们都不会退出，但是 qemu 在访问这些设备的时候会出现错误
	#
	# 也就是我们可以观察到两个如下两个 backtrace 的:
	#
	# - main
	#   - qemu_init
	#     - qemu_create_late_backends
	#       - object_option_foreach_add
	#         - user_creatable_add_qapi
	#           - user_creatable_add_type
	#             - user_creatable_complete
	#               - host_memory_backend_memory_complete
	#
	# - main
	#   - qemu_init
	#     - qmp_x_exit_preconfig
	#       - qemu_init_board
	#         - machine_run_board_init
	#           - create_default_memdev
	#             - user_creatable_complete
	# 	        - host_memory_backend_memory_complete
	#
	# 其中第二个 backtrace 的结果如果有 numa 之后，就是不会有了
}

function setup_mem_cpu() {
	if check_option hack_memory_cpu; then
		# 这个不仅复杂，而且颠覆对于计算机的理解，具体讨论在: docs/qemu/cpu-topo.md
		setup_memory_explict
	else
		setup_memory_implict
	fi
}

function setup_kernel_cmdline() {
	kernel_args=" "

	# virtme 模式: 特殊处理
	if is_virtme_mode; then
		setup_virtme_kernel_cmdline
		return
	fi

	# kernel_args+=" default_hugepagesz=2M hugepagesz=1G hugepages=1 hugepagesz=2M hugepages=512"
	# kernel_args+=" default_hugepagesz=2M"
	# kernel_args+=" hugepages=100 hugepagesz=2M  hugepages=200 hugepagesz=2M"
	# kernel_args+=" hugepagesz=2M hugepages=100 hugepagesz=2M hugepages=200"
	# kernel_args+=" hugepagesz=1G hugepages=2 hugepages=200"
	# kernel_args+=" scsi_mod.scsi_logging_level=0x3fffffff"
	# 配置 panic 后的等待时间
	kernel_args+=" oops=panic panic=0"

	kernel_args+=" nokaslr apparmor=0 selinux=0"
	kernel_args+=" preempt=full"
	# kernel_args+=" apic=verbose"
	# kernel_args+=" threadirqs"

	# kernel_args+=" ftrace=function_graph ftrace_filter=arch_freq_get_on_cpu "
	# kernel_args+=" ftrace=function ftrace_filter=arch_freq_get_on_cpu "
	# kernel_args+=" ftrace=function ftrace_filter=dmar_set_interrupt "

	# 进入之后 cat /sys/kernel/debug/tracing/trace
	# kernel_args+="memblock=debug"
	kernel_args+=" systemd.unified_cgroup_hierarchy=1 "
	kernel_args+=" mitigations=off "
	# kernel_args+=" nohz=off "
	# kernel_args+=" nohz_full=0-7"
	# kernel_args+=" rcu_nocbs=0,1 nohz_full=0,1 "
	# kernel_args+=" intremap=off "
	# kernel_args+=" iommu=pt "
	# kernel_args+=" nvme_core.multipath=0 "
	# kernel_args+="iommu=off "
	# kernel_args+="emergency " # 加上这个参数，直接进入到 emergency mode 中
	# 通过这个参数可以直接 disable avx2
	# kernel_args+=" clearcpuid=156"
	# kernel_args+=" transparent_hugepage=always "
	kernel_args+=" rcutree.sysrq_rcu=1 "
	kernel_args+=" crashkernel=512M "

	# TODO loglevel 和 quiet 真是含义需要等待一下
	kernel_args+=" loglevel=8 "
	# kernel_args+=" systemd.mask=network.target " # 以防网络阻塞
	# kernel_args+="cpuidle_haltpoll.force=1 "
	# kernel_args+="dyndbg= "
	# 一般情况，调试需要使用这个问题:
	# kernel_args+="ignore_loglevel "
	# kernel_args+="loglevel=4 "

	# kernel_args+="pci=nomsi "
	if [[ $ARCH == x86_64 ]]; then
		# kernel_args+="apic=debug show_lapic=all "
		# kernel_args+="noapic "
		echo ""
	fi
	# kernel_args+="clocksource=tsc "
	# kernel_args+="no-kvmclock "
	# kernel_args+="vdso=0 vsyscall=none"
	# kernel_args+="cma=100m " # TODO 没搞懂咋回事
	# kernel_args+="zswap.enabled=1 zswap.compressor=lz4 zswap.max_pool_percent=20 zswap.zpool=z3fold "
	kernel_args+="zswap.enabled=0 "
	# kernel_args+="kernelcore=50%"
	# kernel_args+="memblock=debug "
	# kernel_args+="idle=poll "
	# pcpu_list="1-3"
	# kernel_args+="isolcpus=$pcpu_list rcu_nocbs=$pcpu_list nohz_full=$pcpu_list"

	if ! check_option cmdline; then
		if [[ -s ${vm_dir}/opt/partuuid ]]; then
			partuuid=$(cat "$vm_dir/opt"/partuuid)
			kernel_args+=" root=PARTUUID=$partuuid "
		else
			echo "${vm_dir}/opt/partuuid missed"
			error "boot vm with --kernel, kernel cmdline need partuuid cmdline"
		fi
		# 似乎 init=/bin/bash 变成了 systemd 的参数了
		# arg_kernel_args="root=$root nokaslr console=ttyS0,9600 earlyprintk=serial init=/bin/bash"
	else
		# 现在看来，当用来实现内核替换的时候 lvm 反而更加简单
		kernel_args+=" $option_result"
	fi
	arg_kernel_args="-append '$kernel_args '"
}

# virtme 模式的 kernel cmdline 设置
function setup_virtme_kernel_cmdline() {
	log "setting up virtme kernel cmdline..."

	# 基础参数
	local kernel_args=" rootfstype=virtiofs root=ROOTFS"

	# 主机名
	local hostname
	hostname=$(basename "$vm_dir")
	kernel_args+=" virtme_hostname=$hostname"

	# 控制台
	if [[ $ARCH == x86_64 ]]; then
		kernel_args+=" virtme_console=ttyS0 console=ttyS0,115200n8"
	else
		kernel_args+=" virtme_console=ttyAMA0 console=ttyAMA0,115200n8"
	fi

	# 用户设置
	local virtme_user
	if check_option user; then
		virtme_user=$option_result
	elif [[ -n ${SUDO_USER:-} ]]; then
		virtme_user=$SUDO_USER
	else
		virtme_user=$(id -un)
	fi
	kernel_args+=" virtme_user=$virtme_user"

	# root 用户标记 (如果以 root 运行)
	if [[ $(id -u) -eq 0 ]]; then
		kernel_args+=" virtme_root_user=1"
	fi

	# 工作目录 (可选)
	if check_option cwd; then
		kernel_args+=" virtme_chdir=$option_result"
	fi

	# overlay 可写目录配置
	if check_option virtme_rw; then
		# 默认的可写目录
		local rw_dirs=(/etc /home /var /tmp)
		local idx=0
		for dir in "${rw_dirs[@]}"; do
			# 检查目录是否存在于共享的 rootfs 中
			kernel_args+=" virtme_rw_overlay${idx}=$dir"
			idx=$((idx + 1))
		done
	fi

	# 额外的可写目录 (逗号分隔)
	if check_option virtme_rw_overlay; then
		IFS=',' read -ra extra_dirs <<<"$option_result"
		local idx=0
		# 先找到已有的最大索引
		for i in 0 1 2 3 4 5 6 7 8 9; do
			if [[ $kernel_args == *"virtme_rw_overlay${i}="* ]]; then
				idx=$((i + 1))
			fi
		done
		for dir in "${extra_dirs[@]}"; do
			kernel_args+=" virtme_rw_overlay${idx}=$dir"
			idx=$((idx + 1))
		done
	fi

	# 模块链接 (如果 kernel_dir 设置)
	if [[ -n ${kernel_dir:-} && -d $kernel_dir ]]; then
		# 检查是否有 virtme_mods 目录
		if [[ -d "$kernel_dir/.virtme_mods/lib/modules" ]]; then
			local mod_link
			mod_link=$(find "$kernel_dir/.virtme_mods/lib/modules" -mindepth 1 -maxdepth 1 -printf '%f\n' | head -1)
			if [[ -n $mod_link ]]; then
				kernel_args+=" virtme_link_mods=$kernel_dir/.virtme_mods/lib/modules/$mod_link"
			fi
		fi
	fi

	# 其他常用参数
	kernel_args+=" nokaslr"
	kernel_args+=" mitigations=off"
	kernel_args+=" loglevel=8"

	# 网络配置 (如果启用)
	if check_option network; then
		kernel_args+=" virtme.dhcp"
		kernel_args+=" net.ifnames=0"
		kernel_args+=" biosdevname=0"
	fi

	# 脚本执行 (如果配置了 exec)
	if check_option exec; then
		local exec_script
		exec_script="$option_result"

		# 如果是指向文件的路径，读取文件内容
		if [[ -f $exec_script ]]; then
			exec_script=$(cat "$exec_script")
		fi

		# base64 编码脚本内容
		local encoded_script
		encoded_script=$(echo -n "$exec_script" | base64 -w0)
		kernel_args+=" virtme.exec=\`${encoded_script}\`"

		log "exec script encoded (length: ${#encoded_script})"
	fi

	# 用户自定义参数
	if check_option cmdline; then
		kernel_args+=" $option_result"
	fi

	# VSOCK SSH 支持
	if check_option virtme_vsock; then
		local vsock_cid=$((guest_id + 1000))
		kernel_args+=" virtme.vsock_cid=$vsock_cid"
		setup_virtme_ssh_config
	fi

	log "virtme kernel args: $kernel_args"
	arg_kernel_args="-append '$kernel_args '"
}

# 设置 VSOCK 用于 SSH 连接
function setup_virtme_vsock() {
	if ! check_option virtme_vsock; then
		return
	fi

	log "setting up virtme vsock for SSH..."

	# 获取 vsock CID (基于 guest_id)
	local vsock_cid
	vsock_cid=$((guest_id + 1000))

	# 添加 vsock 设备
	arg_network+=" -device vhost-vsock-pci,id=vhost-vsock-pci0,guest-cid=$vsock_cid"

	# 在 kernel cmdline 中标记启用 SSH
	virtme_vsock_cid="$vsock_cid"

	log "vsock configured with CID: $vsock_cid"
}

# 生成 SSH 配置
function setup_virtme_ssh_config() {
	if ! check_option virtme_vsock; then
		return
	fi

	mkdir -p "$VIRTME_SSH_DIR"

	local vm_name
	vm_name=$(basename "$vm_dir")

	# 生成 SSH 配置文件
	cat >"$VIRTME_SSH_DIR/$vm_name.config" <<SSHCONFIG
Host virtme-$vm_name
    HostName localhost
    Port 2222
    User root
    StrictHostKeyChecking no
    UserKnownHostsFile /dev/null
    ProxyCommand socat VSOCK-CONNECT:\${VIRTME_CID:-$virtme_vsock_cid}:22 -
SSHCONFIG

	log "SSH config created: $VIRTME_SSH_DIR/$vm_name.config"
	log "Connect with: ssh -F $VIRTME_SSH_DIR/$vm_name.config virtme-$vm_name"
}

function nvme_sriov_guest_setup() {
	pci=0000:01:00.0
	# 必须在虚拟机中使用文档中说的方法来使用
	sudo nvme virt-mgmt /dev/nvme0 -c 0 -r 1 -a 1 -n 0
	sudo nvme virt-mgmt /dev/nvme0 -c 0 -r 0 -a 1 -n 0
	echo 1 | sudo tee /sys/bus/pci/devices/$pci/reset
	echo 1 | sudo tee /sys/bus/pci/devices/$pci/sriov_numvfs
	sudo nvme virt-mgmt /dev/nvme0 -c 1 -r 1 -a 8 -n 1
	sudo nvme virt-mgmt /dev/nvme0 -c 1 -r 0 -a 8 -n 2
	sudo nvme virt-mgmt /dev/nvme0 -c 1 -r 0 -a 9 -n 0
	echo $pci | sudo tee /sys/bus/pci/drivers/nvme/bind
	# TODO
	# 发现构建的新盘默认展示的是一个盘
	# 但是如果强制把 multipath 禁用掉，会发现结果为:
	# [   11.869866] nvme nvme1: Found shared namespace 1, but multipathing not supported.
	# [   11.870715] nvme nvme1: Support for shared namespaces without CONFIG_NVME_MULTIPATH is deprecated and will be removed in Linux 6.0.
	# 这个时候发现有两个盘
	# rwxrwxrwx - root 18 Dec 14:29  nvme0n1 -> ../devices/pci0000:00/0000:00:0b.0/0000:02:00.0/nvme/nvme0/nvme0n1
	# lrwxrwxrwx - root 18 Dec 14:29  nvme1n1 -> ../devices/pci0000:00/0000:00:0b.0/0000:02:00.1/nvme/nvme1/nvme1n1
	# 也许我们对于 nvme 的理解再深入一点之后在
}

function setup_nvme_sriov() {
	local arg_nvme=""
	create_disk_file nvme1
	# https://qemu-project.gitlab.io/qemu/system/devices/nvme.html
	arg_nvme+="-device pcie-root-port,slot=3,id=pcie_port.3 "
	arg_nvme+="-device nvme-subsys,id=subsys0 "
	arg_nvme+="-device nvme,serial=deadbeef,subsys=subsys0,sriov_max_vfs=1,sriov_vq_flexible=2,sriov_vi_flexible=1,bus=pcie_port.3 "
	arg_nvme+="-device nvme-ns,drive=nvme3,nsid=1 "
	arg_nvme+="-drive file=$disk_path,format=qcow2,if=none,id=nvme3 "
	arg_storage+="$arg_nvme "
	# 使用 nvme_sriov_guest_setup 中方法配置
}

function setup_nvme_multipath() {
	local arg_nvme=""
	for ((i = 0; i < 3; i = i + 1)); do
		printf '%s\n' "$i"
		create_disk_file nvme1
		arg_nvme+=" -drive file=${disk_path},if=none,id=nvm-$i"
	done

	arg_nvme+=" -device nvme-subsys,id=nvme-subsys-0,nqn=subsys0"
	arg_nvme+=" -device nvme-subsys,id=nvme-subsys-1,nqn=subsys1"

	arg_nvme+=" -device nvme-ns,drive=nvm-1,nsid=1"
	arg_nvme+=" -device nvme-ns,drive=nvm-2,nsid=3,shared=off,detached=on"
	arg_nvme+=" -device nvme-ns,drive=nvm-3,nsid=1"

	arg_nvme+=" -device nvme,serial=deadbeef,subsys=nvme-subsys-1,id=nc3"
	arg_nvme+=" -device nvme,serial=deadbeef,subsys=nvme-subsys-0,id=nc1"
	arg_nvme+=" -device nvme,serial=deadbeef,subsys=nvme-subsys-0,id=nc2"

	arg_storage+="$arg_nvme "
}

function setup_nvme_basic() {
	# kimi 说 : serial 对应的是 NVMe 规范中的 Serial Number（SN，序列号），它是 NVMe 控制器的身份标识字段之一。
	# 如果两个 nvme 都填写 serial=foo，那么就会在 guest 中得到如下的报错
	# [    0.686202] nvme nvme1: Duplicate cntlid 0 with nvme0, subsys nqn.2019-08.org.qemu:foo, rejecting
	#
	# 这个 serial 在虚拟机可以通过这个方法检查:
	# 🧀  nvme list
	# Node                  Generic               SN                   Model                                    Namespace  Usage                      Format           FW Rev
	# --------------------- --------------------- -------------------- ---------------------------------------- ---------- -------------------------- ---------------- --------
	# /dev/nvme0n1          /dev/ng0n1            foo2                 QEMU NVMe Ctrl                           0x1          1.61  TB /   1.61  TB    512   B +  0 B   9.2.50
	# /dev/nvme1n1          /dev/ng1n1            foo                  QEMU NVMe Ctrl                           0x1          1.61  TB /   1.61  TB    512   B +  0 B   9.2.50
	#
	# 硬编码 serial + nvme 设备直通，可能会导致错误，所以，先这样吧
	# 00:12.0 Non-Volatile memory controller [0108]: Red Hat, Inc. QEMU NVM Express Controller [1b36:0010] (rev 02)

	# max_ioqpairs 是设置队列数量
	create_disk_file nvme_basic_1
	arg_nvme+=" -device nvme,drive=nvme_basic1,max_ioqpairs=14,serial=$(uuidgen),id=nvme_b1 "
	arg_nvme+=" -drive file=${disk_path},format=qcow2,if=none,id=nvme_basic1,aio=native,cache.direct=on "

	# 可以给 nvme 配置了 pci 地址
	# -device nvme,drive=nvme_basic2,max_ioqpairs=14,serial=$(uuidgen),id=nvme_b2,bus=pci.0,addr=0x12
	# 但是发现在 arm 中这个会有报错 Bus 'pci.0' not found

	# qemu 启动之后为什么这个日志:
	# [ 3.284536] virtio_blk virtio8: [vdb] 20971520 512-byte logical blocks (10.7 GB/10.0 GiB)
	# [ 3.288452] nvme nvme1: Ignoring bogus Namespace Identifiers
	# [ 3.292774] nvme nvme0: Ignoring bogus Namespace Identifiers
	#
	# 原因是在 drivers/nvme/host/pci.c 中，定义包含了NVME_QUIRK_BOGUS_NID
	# 	{ PCI_VDEVICE(REDHAT, 0x0010),	/* Qemu emulated controller */
	#	.driver_data = NVME_QUIRK_BOGUS_NID, },

	create_disk_file nvme_basic_2
	arg_nvme+=" -device nvme,drive=nvme_basic2,max_ioqpairs=14,serial=$(uuidgen),id=nvme_b2 "
	arg_nvme+=" -drive file=${disk_path},format=qcow2,if=none,id=nvme_basic2,aio=native,cache.direct=on "
	arg_storage+="$arg_nvme "
}

function setup_many_nvme() {
	for ((i = 0; i < 10; i++)); do
		create_disk_file many_nvme
		# TODO 为什么 nvme 不支持 iothread 啊
		arg_nvme=" -device nvme,drive=many_nvme$i,max_ioqpairs=96,serial=$(uuidgen) "
		arg_nvme+=" -drive file=${disk_path},format=qcow2,if=none,id=many_nvme$i"
		# arg_nvme+=" -object iothread,id=nvme_io$i"
		arg_storage+="$arg_nvme "
	done
}

function setup_nvme_host() {
	if ! check_option nvme_host; then
		return
	fi
	# 0000:80:0a.0
	local dx="$option_result"
	echo "unind $(lspci -s "$dx")"
	pci_bind_to_vfio "$dx"

	local nvme_host
	nvme_host+="-blockdev driver=nvme,node-name=disk_backend,device=$dx,namespace=1 "
	nvme_host+="-device virtio-blk-pci,drive=disk_backend "
	arg_storage+="$nvme_host"
	# 注意在 qemu 中这个两个位置的区别:
	# - hw/nvme/ : 模拟 nvme
	# - block/nvme.c : 使用 nvme 作为后端
	#
	# 然后在虚拟机中就可以观察到，原来的 /dev/nvme 被转化为 virtio 了
	# vdb                251:16   0   1.5T  0 disk
	# ├─vdb1             251:17   0 745.2G  0 part
	# └─vdb2             251:18   0 745.2G  0 part
}

function setup_nvme() {
	if ! check_option nvme; then
		return 0
	fi
	local nvme_hacking=$option_result
	# 看来 device 的 id 空间和 driver 的 id 空间是不重合的
	# device 和 driver 可以使用相同的变量
	case $nvme_hacking in
		basic)
			setup_nvme_basic
			;;
		multipath)
			setup_nvme_multipath
			;;
		sriov)
			setup_nvme_sriov
			;;
		many)
			setup_many_nvme
			;;
		*)
			echo "what do you want to hack with nvme ? 🐶"
			exit 0
			;;
	esac
}

function setup_many_vda() {
	# TODO 不知道为什么 qemu 会报告这个错误
	# qemu-system-aarch64: -device virtio-blk-pci,drive=vd01,bus=vd_bridge0,iothread=many_1:
	# Unsupported PCI slot 0 for standard hotplug controller.
	# Valid slots are between 1 and 31.
	if [[ $ARCH == aarch64 ]]; then
		return
	fi
	local disk

	for ((i = 0; i < 1; i = i + 1)); do
		disk+="-device pci-bridge,id=vd_bridge$i,chassis_nr=$((10 + i)) "
		for d in $(seq 1 10); do
			local f
			create_disk_file many_vda
			local id=vd$i$d
			disk+=" -device virtio-blk-pci,drive=$id,bus=vd_bridge$i,iothread=many_$d  "
			disk+=" -drive file=${disk_path},format=qcow2,if=none,id=$id "
			disk+=" -object iothread,id=many_$d"
		done
	done

	arg_storage+="$disk "
}

function setup_ssh_backend() {
	local ssh_backend
	# TODO 自动化执行这个?
	# ssh-copy-id martins3@localhost
	# no host key was found in known_hosts
	create_disk_file ssh_img

	ssh_backend+=" -object iothread,id=ssh_blk_io0 "
	ssh_backend+=" -device virtio-blk,drive=ssh_1"
	# ssh_backend+=" -device virtio-blk,drive=ssh_1,iothread=ssh_blk_io0"
	# TODO if=none 的作用是什么来着，一直都没有注意，结果发现是必须添加的
	# -device virtio-blk,drive=ssh_1,iothread=ssh_blk_io0: Drive 'ssh_1' is already in use because it has been automatically connected to another device (did you need 'if=none' in the drive options?)
	ssh_backend+=" -drive file=ssh://martins3@localhost:${disk_path},id=ssh_1,if=none"
	# 第二种配置方法:
	# ssh_backend="-drive file.driver=ssh,file.user=user,file.host=host,file.port=22,file.path=/path/to/disk.img"
	arg_storage+=" $ssh_backend"
}

function setup_nfs_backend() {
	# 之前实现嵌套存储的方法，把 qcow2 文件放到 l1 虚拟机的 nfs 的位置上
	# flock 在 nfs 上的时候，需要添加一个这个参数才可以勉强 workaround
	# kernel_args+=" nfs.recover_lost_locks=1 "

	# 相关实现的代码 block/nfs.c
	# 最后依赖的是一个 libnfs 的库
	#
	# TODO 在物理机中无法观察到 nfs 的 mount ，所以这个相当于把内核中的 nfs client 放到用户态了吗?
	# 可以用 libnfs 写写程序，应该很容易理解的
	create_disk_file nfs_img

	nfs_backend=""
	nfs_backend+=" -device virtio-blk,drive=nfs_1"
	nfs_backend+=" -drive file=nfs://10.0.0.2:${disk_path},if=none,id=nfs_1"
	arg_storage+="$nfs_backend"
}

function setup_iscsi_backend() {
	# 按照 docs/kernel/blk/scsi/storage-scsi-iscsi.md 中配置 target 端吧，其实还挺容易的
	# 注意这里都是硬编码 10.0.0.2 和 lun id
	# 此外，这�� index 也是需要被特意配置的，默认为 0 ，当前配置中，0 和 1 都是被占用的
	iscsi_backend=""
	iscsi_backend+=" -device virtio-blk,drive=iscsi_1"
	iscsi_backend+=" -drive file=iscsi://10.0.0.2:3260/iqn.2003-01.org.linux-iscsi.localhost.aarch64:sn.980b9a90bf36/0,format=raw,index=8,if=none,id=iscsi_1"
	arg_storage+="$iscsi_backend"
}

function setup_curl_backend() {
	# sudo rpm -ql qemu-block-curl
	# /usr/lib64/qemu/block-curl.so

	create_disk_file curl_img
	dir=$(dirname "$disk_path")
	file=$(basename "$disk_path")
	echo "cd $dir and python3 -m http.server"
	# 然后会有这个问题
	# Error opening file: Server does not support 'range' (byte ranges).
	#
	# 应该使用 nginx 就没问题了，配置 nginx 也不难，不过后面再说吧

	local url=http://0.0.0.0:8000/"$file"
	curl_backend+=" -device virtio-blk,drive=curl_1"
	curl_backend+=" -drive file=${url},driver=http,if=none,id=curl_1"
	arg_storage+="$curl_backend"
}

function setup_vvfat_backend() {
	# block/vvfat.c 对应的代码
	:
}

function setup_null_backend() {
	local null_backend
	null_backend+=" -object iothread,id=null_blk_io0 "
	null_backend+=" -device virtio-blk,drive=null_1,iothread=null_blk_io0"
	null_backend+=" -drive file=null-aio://,id=null_1,format=raw,if=none"
	# 可以观察到这个设备
	# vdb                252:16   0     1G  0 disk
	# 4k 的随机读写只有这个性能
	# bs: 1 (f=1): [r(1)][0.5%][r=111MiB/s][r=28.4k IOPS][eta 16m:35s]

	null_backend+=" -object iothread,id=null_blk_io2 "
	null_backend+=" -device virtio-blk,drive=null_2,iothread=null_blk_io2"
	null_backend+=" -drive file=null-co://,id=null_2,format=raw,if=none"
	# 看上去通过 block/null.c:runtime_opts 可以调整参数，但是实际上并不行
	# 似乎是 qemu 的 bug

	arg_storage+=" $null_backend"
}

# virtio_blk 设备很简单，我们用这个来测试 qemu 的 io 模型
# 显然，之后，需要让这个东西可以自动的配置到所有的
function setup_virtio_blk() {
	if ! check_option virtio_blk; then
		return
	fi
	local arg_virtio_blk
	local ioengine=aio
	# ioengine=io_uring
	local fmt=qcow2
	# local fmt=raw
	local use_iothread="mapping"
	use_iothread=no
	use_iothread=single
	case "$use_iothread" in
		single)
			arg_virtio_blk+=" -object iothread,id=virtio_blk_io0 "
			arg_virtio_blk+=" -device virtio-blk,drive=virtio_blk_1,id=virtio_blk_1,iothread=virtio_blk_io0,num-queues=2"
			;;
		mapping)
			arg_virtio_blk+=" -object iothread,id=virtio_blk_io0 "
			arg_virtio_blk+=" -object iothread,id=virtio_blk_io1 "
			arg_virtio_blk+=" -device '{\"driver\": \"virtio-blk\", \"drive\": \"virtio_blk_1\", \"iothread-vq-mapping\": [{\"iothread\": \"virtio_blk_io0\"}, {\"iothread\": \"virtio_blk_io1\"}]}'"
			;;
		no)
			# TODO virtio-blk 也是不可以添加,iommu_platform=on 的
			arg_virtio_blk+=" -device virtio-blk,drive=virtio_blk_1,id=virtio_blk_1 "
			;;
		*)
			error ""
			;;
	esac

	# arg_virtio_blk+=",disable-legacy=on "

	# TODO 写一个自动判断，format 不相等的时候就重构一下
	# 而且 qcow2 可以自动变为 raw
	# rm -f "$vm_dir/img/virtio_blk_1"
	if [[ $fmt == qcow2 ]]; then
		create_disk_file virtio_blk_1
	else
		create_disk_file_raw virtio_blk_1
	fi
	arg_virtio_blk+=" -drive file=${disk_path},format=$fmt,if=none,id=virtio_blk_1"
	case $ioengine in
		aio)
			arg_virtio_blk+=",aio=native,cache.direct=on"
			;;
		io_uring)
			arg_virtio_blk+=",aio=io_uring"
			;;
		thread)
			# 默认 thread
			# TODO 这种模式也是使用的 epoll 吗?
			;;
		*)
			error "unknown"
			;;
	esac
	arg_storage+=" $arg_virtio_blk "
}

function setup_usb_net() {
	# qemu 会有这个报错，但是不影响
	# usbnet: failed control transaction: request 0x8006 value 0x600 index 0x0 length 0xa
	create_switch_tap
	arg_usb+=" -device usb-net,netdev=$tap_name,mac=$mac_addr "
	arg_usb+=" -netdev tap,ifname=$tap_name,id=$tap_name,script=no,downscript=no,vhost=off "
}

function setup_usb_storage() {
	create_disk_file usb1
	arg_usb+="-drive if=none,id=usbstick,format=raw,file=${disk_path} "
	arg_usb+="-device usb-storage,bus=usb.0,drive=usbstick "

	create_disk_file usb2
	arg_usb+="-drive if=none,id=usbstick2,format=raw,file=${disk_path} "
	arg_usb+="-device usb-storage,bus=usb.0,drive=usbstick2 "
}

function setup_usb_misc() {
	# TODO 联和 virtio-keyboard 分析下，usb-kbd 和 usb-mouse 到底啥关系?
	# 1. 这个会拖慢内核启动的速度，但是如果没有，aarch64 没有键盘鼠标
	# 2. 2025-12-25 在 x86 下，如果存在，而且用 uefi 启动，那么会发现整个安装的图形界面中发现鼠标点了
	# 就像是没有反应一样
	#
	# TODO usb-mouse 和 usb-tablet 有什么区别?
	# 参考了一下 libvirt 的配置，发现 usb-mouse 是不能配置的，在 aarch64 上，鼠标也是没法操作的
	arg_usb+="-device usb-kbd,id=input0,bus=usb.0,port=2 "
	arg_usb+="-device usb-tablet,id=input1,bus=usb.0,port=3 "
	# arg_usb+="-device usb-mouse,id=input2,bus=usb.0,port=4 "
}

arg_usb=""
function setup_usb() {
	if is_virtme_manual_mode; then
		return
	fi

	# 参考 https://qemu-project.gitlab.io/qemu/system/devices/usb.html
	#
	# https://gist.github.com/ichisadashioko/cfc6446764516bf7eccaffdb3799f041
	# arg_usb="-usb -device usb-host,bus=usb-bus.0,hostbus=1,hostport=1"
	# -device qemu-xhci,id=xhci
	# 似乎只能使用 xhci 了
	arg_usb+="-usb "
	# arg_usb+="-device qemu-xhci,id=xhci "
	arg_usb+="-device qemu-xhci,p2=8,p3=8,id=usb "

	setup_usb_misc
	# setup_usb_net
	# setup_usb_storage

	# 不管怎么说，虚拟机中看到的就是
	# 1   2  0x0627 0x0001 QEMU USB Tablet               28754-0000:00:01.2-1 usb     12.0 Mb/s
	# 2   2  0x0525 0xa4a2 RNDIS/QEMU USB Network Device 1-0000:00:0e.0-2     usb     12.0 Mb/s
	# 3   2  0x46f4 0x0001 QEMU USB HARDDRIVE            1-0000:00:0e.0-1     usb      5.0 Gb/s
	#
	# lspci -s 0000:00:0e.0 -v
	#
	# 00:0e.0 USB controller: Red Hat, Inc. QEMU XHCI Host Controller (rev 01) (prog-if 30 [XHCI])
	# Subsystem: Red Hat, Inc. Device 1100
	# Physical Slot: 14
	# Flags: bus master, fast devsel, latency 0, IRQ 10
	# Memory at fc2d0000 (64-bit, non-prefetchable) [size=16K]
	# Capabilities: <access denied>
	# Kernel driver in use: xhci_hcd
	#
	# sysfs 的观察:
	# sdb -> ../devices/pci0000:00/0000:00:0e.0/usb3/3-1/3-1:1.0/host3/target3:0:0/3:0:0:0/block/sdb
	# ens14u2i1 -> ../../devices/pci0000:00/0000:00:0e.0/usb2/2-2/2-2:1.1/net/ens14u2i1

	# TODO 直通物理机中 usb
}

# 可以看到多出来设备为:
# lrwxrwxrwx  1 root root 0 5月   7 22:37 sdb -> ../devices/pci0000:00/0000:00:01.1/ata1/host1/target1:0:0/1:0:0:0/block/sdb
# lrwxrwxrwx  1 root root 0 5月   7 22:37 sdc -> ../devices/pci0000:00/0000:00:01.1/ata1/host1/target1:0:1/1:0:1:0/block/sdc
# [root@localhost ~]# lspci -s 0000:00:01.1
# 00:01.1 IDE interface: Intel Corporation 82371SB PIIX3 IDE [Natoma/Triton II]
function setup_ide_disk() {
	# TODO 这里没有 -device ，这里合理吗?
	local arg_ide
	create_disk_file ide_1
	arg_ide+=" -drive file=${disk_path},media=disk,format=qcow2 "
	create_disk_file ide_2
	arg_ide+=" -drive file=${disk_path},media=disk,format=qcow2 "
	arg_storage+="$arg_ide "
}

function setup_sata() {
	if ! check_option sata; then
		return
	fi

	# https://stackoverflow.com/questions/48351096/how-to-emulate-a-sata-disk-drive-in-qemu
	#
	# 真的有 AHCI MODE 啊，其驱动为 ahci
	# 00:07.0 SATA controller: Intel Corporation 82801IR/IO/IH (ICH9R/DO/DH) 6 port SATA Controller [AHCI mode] (rev 02)
	arg_sata="-device ahci,id=ahci "
	create_disk_file sata1
	arg_sata+="-drive id=sata1,file=${disk_path},if=none "
	arg_sata+="-device ide-hd,drive=sata1,bus=ahci.0 "

	create_disk_file sata2
	arg_sata+="-drive id=sata2,file=${disk_path},if=none "
	arg_sata+="-device ide-hd,drive=sata2,bus=ahci.1 "
	arg_storage+="$arg_sata "

	# 如果 bus=ide.0 的时候，
	# -drive id=sata1,file=${d1},if=none
	# -device ide-hd,drive=sata1,bus=ide.0
	#
	# 在虚拟机可以观察到，除了 setup_ide_disk 产生的两个 disk ，
	# 构建的新 disk 中其实还是挂到 ide 下的。
	# lrwxrwxrwx  1 root root 0 5月   7 22:59 sdb -> ../devices/pci0000:00/0000:00:01.1/ata1/host1/target1:0:0/1:0:0:0/block/sdb
	# lrwxrwxrwx  1 root root 0 5月   7 22:59 sdc -> ../devices/pci0000:00/0000:00:01.1/ata2/host2/target2:0:0/2:0:0:0/block/sdc
	# lrwxrwxrwx  1 root root 0 5月   7 22:59 sdd -> ../devices/pci0000:00/0000:00:01.1/ata1/host1/target1:0:1/1:0:1:0/block/sdd
}

function setup_floppy() {
	create_disk_file floppy
	local arg_floppy=" -blockdev driver=file,node-name=f0,filename=${disk_path}"
	arg_floppy+=" -device floppy,drive=f0 "
	if [[ $hacking_iommu == true ]]; then
		# 似乎是 q35 上没有 floppy 总线
		arg_floppy=""
	fi
	arg_storage+="$arg_floppy "
}

function setup_scsi_hba() {
	local arg_scsi=""

	# device 只能设置为 scsi-cd 和 scsi-hd ，其他的两种 scsi-generic 和 scsi-block 是用设备直通的
	# https://pve-devel.pve.proxmox.narkive.com/qZdhk8h5/integration-of-scsi-hd-scsi-block-scsi-generic
	# With scsi-block and scsi-generic you can bypass qemu scsi emulation and
	# use trim / discard support as the guest can talk directly to the underlying storage.
	local scsi_hosts=(
		# 必须将 virtio-scsi 放到第一个位置的，这个是后面的 scsi-hd 配置参数的依据
		virtio-scsi

		# 这里可以配置为 megasas-gen2 或者 megasas ，有两个设备
		# 2025-11-18 最新内核和 qemu ，都是可以正常工作的
		# root@localhost:~# lspci -s 0000:00:08.0 -v
		# Kernel driver in use: megaraid_sas

		# megasas-gen2
		# megasas

		# 这个内核无法识别的，切换了各种版本的内核都不支持
		# 2025-11-18 发现启动之后，会导致虚拟机启动的时候 zstd compressed data corrupted
		# 这其实非常不科学，按道理，这个设备只会不识别，但是现在让 initramfs 中出现了问题
		# mptsas1068

		# 这个可以用，但是被 https://patchwork.kernel.org/project/qemu-devel/patch/1485444454-30749-4-git-send-email-armbru@redhat.com/ 中说这个该被 deprecated
		# lspci -s 0000:00:09.0 -vv
		# Kernel driver in use: sym53c8xx
		# lsi53c895a

		# 暂时不支持如何打开，具体参考 386e20a5-ed1c-482b-b151-57dcfbc32be1
		# ncr53c710

	)

	local counter=1
	for host in "${scsi_hosts[@]}"; do
		echo "$host"
		# 1. virtqueue_size 看似可以配置，但是如果设置为 8000 或者 257 ，要么无法识别，要么 QEMU crash FIXME
		# 2. -device virtio-scsi-pci,id=scsi0 是必须的，用他来提供总线给 scsi-hd
		# 3. 如果 -device virtio-scsi-pci,id=scsi1 ，那么就会有错误  Bus 'scsi0.0' not found ，说对于名称有依赖
		# 具体原因未知
		arg_scsi+="  -device $host,id=scsi$counter "

		VAR=2
		for ((i = 1; i <= VAR; i = i + 1)); do
			printf '%s\n' "$i"
			local id="${host}_$i"
			create_disk_file "$id"
			arg_scsi+=" -device scsi-hd,drive=$id,bus=scsi$counter.0,channel=0,scsi-id=$i,lun=0,id=$id "
			arg_scsi+=" -drive file=$disk_path,if=none,id=$id "
		done
		counter=$((counter + 1))
	done
	arg_storage+="$arg_scsi "
}

function setup_many_sda() {
	local arg_many=""

	# 极限在 bash 脚本，实际上支持几百万块盘是没问题的
	arg_many="  -device virtio-scsi-pci,id=scsi1"
	for lun in $(seq 1 20); do
		for target in $(seq 1 20); do
			local d1
			create_disk_file d1
			id=sd_${target}_$lun
			# 根据 qemu 中 VIRTIO_SCSI_MAX_CHANNEL ，channel 总是指定为 0
			# lun 的上限 :  16,383
			# target 的上限 : 255
			arg_many+=" -device scsi-hd,bus=scsi1.0,channel=0,scsi-id=$target,lun=$lun,drive=$id"
			arg_many+=" -drive file=${d1},format=qcow2,id=$id,if=none "
		done
	done

	arg_storage+="$arg_many "

}

function setup_spdk() {
	# Jobs: 1 (f=1): [w(1)][0.6%][w=2634MiB/s][w=674k IOPS][eta 16m:34s]
	local spdk_uds=$HOME/data/spdk/vhost.0
	if [[ ! -e $spdk_uds ]]; then
		return
	fi
	arg_storage+=" -chardev socket,id=spdk,path=$spdk_uds "
	arg_storage+=" -device vhost-user-blk-pci,id=blk1,chardev=spdk,num-queues=2 "
}

function setup_vduse() {
	echo "vduse"
	# 继续 vduse.md 中的操作吧，这里似乎有好几种模式:
	# sudo modprobe vduse
	sudo qemu-storage-daemon \
		--blockdev file,filename=/home/martins3/hack/vm/cn_windows_10/img/1,node-name=file \
		--blockdev qcow2,file=file,node-name=qcow2 \
		--export type=vduse-blk,id=vduse0,name=vduse0,node-name=qcow2,writable=on

}

function setup_pipe_serial() {
	if [[ $ARCH == aarch64 ]]; then
		return
	fi
	# 用这个可以做一些有趣的实验
	# cat pipe1 和 pipe2 分别对应 ttyS0 和 ttyS1 如果 cat pipe1 的时候，
	# 加上 kernel cmdline 为 console=ttyS0 ，那么 dmesg 直接从 cat 那里输出出来
	# TODO ttyS0 是存在什么特殊地位来着
	#
	# 对于 pipe.in 来输出，使用 pipe.out 来输出从而进行交互
	# 不知道是靠什么实现的自动识别后面的 .in 和 .out 的
	# -virtio-serial 是 bus ，必须存在后面才可以有，有趣的

	# TODO 未解之谜，aarch64 环境中使用这个启动，直接无法���动
	if [[ ! -e $vm_dir/pipe1.in ]]; then
		mkfifo "$vm_dir/pipe1.in" "$vm_dir/pipe1.out"
	fi
	if [[ ! -e $vm_dir/pipe2.in ]]; then
		mkfifo "$vm_dir/pipe2.in" "$vm_dir/pipe2.out"
	fi
	arg_serial+=" -serial pipe:$vm_dir/pipe1 "
	arg_serial+=" -serial pipe:$vm_dir/pipe2 "
}

arg_ipmi=""
function setup_ipmi() {
	if ! check_option ipmi; then
		return
	fi

	# 配置这个可以在虚拟机中看到 /dev/ipmi0
	# 然后就可以执行 ipmitool shell 了
	# TODO 我感觉可以把这个配置的很完整才对
	if [[ $ARCH == aarch64 ]]; then
		return
	fi
	arg_ipmi="-device ipmi-bmc-sim,id=virt-bmc -device pci-ipmi-kcs,bmc=virt-bmc,id=virt-bmc-pci"

	# 参考一下这个: https://mslacken.github.io/2023/08/15/virtual-ipmi.html
	#
	# https://www.linux-kvm.org/images/7/76/03x08-Juniper-Corey_Minyard-UsingIPMIinQEMU.ods.pdf
	# arg_ipmi=" -chardev socket,id=ipmi0,host=localhost,port=9002,reconnect-ms=100"
	# arg_ipmi+=" -device ipmi-bmc-extern,id=virt-bmc,chardev=ipmi0"
	# arg_ipmi+=" -device pci-ipmi-kcs,bmc=virt-bmc,id=virt-bmc-pci"

	# 似乎需要有点什么配套的东西
	# qemu-system-x86_64: Unable to connect character device ipmi0: Failed to connect to 'localhost:9002': Connection refused
}

arg_input=""
function setup_input() {
	if is_virtme_manual_mode; then
		return
	fi

	# TODO 这个可以研究下
	# 00:0f.0 Keyboard controller: Red Hat, Inc. Virtio 1.0 input (rev 01)
	arg_input="-device virtio-keyboard"
}

function check_if_samba_mount() {
	local dir="$1"
	fs_type=$(findmnt -n -o FSTYPE --target "$dir" 2>/dev/null)

	if [[ $fs_type == "cifs" || $fs_type == "smb3" ]]; then
		return 0
	fi
	return 1
}

arg_monitor=""
arg_serial=""

# 将这个函数参数化，任何场景都是调用对应的
function setup_monitor() {
	# 如果后端是 windows 提供的 samba ，那么没办法创建 uds
	if check_if_samba_mount "$vm_dir"; then
		return
	fi

	# qmp-no-pretty 才可以被 kvm-dmesg 识别，所以单独给建立一个
	arg_monitor+=" -chardev socket,id=mon4,path=$vm_dir/$which_qemu/qmp-no-pretty,server=on,wait=off"
	arg_monitor+=" -mon chardev=mon4,mode=control"

	arg_monitor+=" -chardev socket,id=mon3,path=$vm_dir/$which_qemu/qmp-shell,server=on,wait=off"
	arg_monitor+=" -mon chardev=mon3,mode=control"

	arg_monitor+=" -chardev socket,id=mon2,path=$vm_dir/$which_qemu/hmp,server=on,wait=off"
	arg_monitor+=" -mon chardev=mon2"

	arg_monitor+=" -chardev socket,id=mon1,path=$vm_dir/$which_qemu/qmp,server=on,wait=off"
	arg_monitor+=" -mon chardev=mon1,mode=control,pretty=on"

}

function setup_chardev() {
	local hide=false
	local main_char_dev="stdio"

	# 为了让 qemu 界面和 guest console 的输出不要重合
	# 这个时候 stdio 指向到另外的一个位置
	if check_option hide; then
		hide=true
	fi
	if [[ $gdb_debug || $hide == true ]]; then
		main_char_dev=" socket,path=$vm_dir/$which_qemu/main.sock"
	fi

	# virtio-serial 是 virtconsole 和 virtserialport 的基础设备:
	# qemu-system-x86_64: -device virtconsole,chardev=virtiocon0: No 'virtio-serial-bus' bus found for device 'virtconsole'
	# qemu-system-x86_64: -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0: No 'virtio-serial-bus' bus found for device ' virtserialport'
	arg_serial+=" -device virtio-serial"

	if is_virtme_manual_mode; then
		if [[ $main_char_dev == "stdio" ]]; then
			arg_serial+=" -chardev stdio,id=main_char,signal=off"
		else
			arg_serial+=" -chardev $main_char_dev,id=main_char,server=on,wait=off "
		fi
		arg_serial+=" -serial chardev:main_char"

		# 手工 virtme 模式只保留一个前台串口，virtconsole 退化成辅助 PTY。
		arg_serial+=" -chardev pty,id=char_pty"
		arg_serial+=" -device virtconsole,chardev=char_pty"

		# vmtest 必须有一个 qga，不然 init.sh 中的 qga 报错
		arg_serial+=" -chardev socket,path=$vm_dir/$which_qemu/qga.sock,server=on,wait=off,id=qga0"
		arg_serial+=" -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0"

		# 不过 /dev/vport6p3 是没法做 console 的
		arg_serial+=" -chardev socket,path=$vm_dir/$which_qemu/vport.sock,server=on,wait=off,id=vport"
		arg_serial+=" -device virtserialport,chardev=vport,name=org.qemu.vport.0"

		setup_monitor
		return
	fi

	# 这个配置必须放到最前面，让这个串口是 ttyS0
	arg_serial+=" -chardev $main_char_dev,id=main_char,server=on,wait=off,id=main_char,mux=on "
	# 甚至可以同时指向两个 serial port
	# arg_serial+=" -serial chardev:stdio_char "
	arg_serial+=" -serial chardev:main_char"
	arg_serial+=" -device virtconsole,chardev=main_char"
	arg_serial+=" -mon chardev=main_char,mode=readline "

	# 配合 collei-action.sh:connect_to_pty 使用
	# screen 的退出方法是 ctrl a + d ，之后可以通过 screen -r 来连
	# 虚拟机中对应的设备是 : /dev/hvc0
	#
	# 配置了 mux=on ，所以虚拟机中的多个设备可以复用一个后端
	arg_serial+=" -chardev pty,mux=on,id=char_pty"
	arg_serial+=" -device virtconsole,chardev=char_pty"
	if [[ $ARCH == x86_64 ]]; then
		# 不知道为什么，arm 环境默认使用这个串口，导致 edk2 没有输出，找了半天才发现
		arg_serial+=" -serial chardev:char_pty"
	fi

	# vmtest 必须有一个 qga，不然 init.sh 中的 qga 报错
	arg_serial+=" -chardev socket,path=$vm_dir/$which_qemu/qga.sock,server=on,wait=off,id=qga0"
	arg_serial+=" -device virtserialport,chardev=qga0,name=org.qemu.guest_agent.0"

	# 不过 /dev/vport6p3 是没法做 console 的
	arg_serial+=" -chardev socket,path=$vm_dir/$which_qemu/vport.sock,server=on,wait=off,id=vport"
	arg_serial+=" -device virtserialport,chardev=vport,name=org.qemu.vport.0"

	setup_monitor
	# setup_pipe_serial
}

arg_vnc=""
function setup_vnc() {
	if is_virtme_manual_mode; then
		return
	fi

	# 将所有 vnc 都放到浏览器中，性能都非常差，还不如整齐一点
	local ip_addr
	vnc_exe=novnc_server
	if which novnc >/dev/null; then
		vnc_exe=novnc
	fi
	get_tcp_port vnc
	qemu_vnc_port=$tcp_port
	novnc_port=$((tcp_port + 1))

	pueue clean &>/dev/null
	pueue add -i -- $vnc_exe --vnc localhost:$qemu_vnc_port --listen $novnc_port

	ip_addr=$(get_master_ip)
	echo "http://$ip_addr:$novnc_port/vnc.html"
	# pueue add -i -- microsoft-edge http://127.0.0.1:$((vnc + 6000))/vnc.html
	arg_vnc+=" -vnc :$((qemu_vnc_port - 5900)),password=off "
}

arg_display=""
function setup_display() {
	# 配置放到 gpu/qemu.md 中具体分析，我们在这里搞了好多 workaround
	if check_option fire; then
		return
	fi

	if is_virtme_manual_mode; then
		arg_display="-display none"
		return
	fi

	if [[ $ARCH == aarch64 ]]; then
		arg_display="-device virtio-gpu-pci "
		return
	fi

	if check_windows; then
		# windows 就简单点了吧，配置驱动太痛苦了
		arg_display="-vga std "
		return
	fi
	if check_option display; then
		case "$option_result" in
			virtio-gpu)
				arg_display=" -device virtio-gpu-pci "
				;;
			none)
				# 如果直通了显卡进去，可以临时去掉这个
				arg_display="  "
				;;
			*)
				error "Later"
				;;
		esac
		return
	fi

	# 需要测试一下，到底 seabios / uefi / grub 都是支持什么
	arg_display="-vga std"
	# gpu="-vga qxl"
	# gpu="-vga virtio "
	# 选择这个，fedora 中的 grub 不识别
	arg_display=" -device virtio-gpu-pci "
	# 选择这个 fedora 43 显示不正常
	arg_display="-device cirrus-vga "
	if ! check_option bios; then
		return
	fi

	if [[ $option_result == ovmf* ]]; then
		arg_display=" -device virtio-gpu-pci "
		return
	fi

	# 这两个可以同时配置的
	# -vga none \
	# -device virtio-vga-gl,xres=2048,yres=1152 \
	# -display sdl,gl=on \
}

function setup_trace() {
	arg_trace=""
	local tracepoint=(
		# kvm_set_user_memory
		# memory_region_ops_read
		# amdvi_ir_intctl
		# savevm_state_setup
		# kvm_dirty_ring_reaper
		# kvm_interrupt_exit_request
		# kvm_dirty_ring_reaper
		# kvm_dirty_ring_reaper
		# uffd_create_fd_nosys
		# uffd_create_fd_api_failed
		# uffd_create_fd_api_noioctl
		# uffd_detect_open_mode
		# qdev_update_parent_bus
		# qemu_coroutine_yield
		# kvm_irqchip_commit_routes
	)
	local trace_files=(
		# collei.mig.trace
		# collei.vfio.trace
	)
	for f in "${trace_files[@]}"; do
		echo "$f"
		local trace
		readarray -t trace < <(grep -v '^[[:space:]]*#' "$PROGDIR"/"$f" | grep -v '^[[:space:]]*$')
		tracepoint+=("${trace[@]}")
	done

	arg_trace=""
	if [[ ${#tracepoint[@]} -gt 0 ]]; then
		for i in "${tracepoint[@]}"; do
			arg_trace+=" --trace $i "
		done
	fi
}

arg_virtio_dummy=""
function setup_virtio_dummy() {
	if $qemu -device help | grep virtio-dummy &>/dev/null; then
		# arg_virtio_dummy="-device virtio-dummy "
		#
		# 真的很奇怪，必须添加 disable-legacy 才可以的
		arg_virtio_dummy=" -device virtio-dummy-pci,disable-legacy=on"
		# arg_virtio_dummy="-device virtio-dummy-pci-non-transitional "
		arg_virtio_dummy+=" -device gpu "
		:
	fi
}

function setup_smbios() {
	# 在 guset 中使用 sudo dmidecode -t 1 来检查
	arg_smbios="-smbios type=0,vendor=martins3,version=12,date=2012-3-4, -smbios type=1,manufacturer='Martins3 Inc',product='Hacking Collei',version=$guest_id,serial=,uuid=$(uuidgen)"
	arg_smbios=""
}

function firecracker_set_para() {
	jq --arg value "$2" \
		"$1 = \$value" "$FIRECRACKER_TEMPLATE" \
		| sponge "$FIRECRACKER_TEMPLATE"
}

function firecracker_set_num() {
	jq --argjson value "$2" \
		"$1 = \$value" "$FIRECRACKER_TEMPLATE" \
		| sponge "$FIRECRACKER_TEMPLATE"
}

function firecracker_disk() {
	local disk="$vm_dir"/img/firecracker-vhost-user-blk.img
	if [[ ! -f $disk ]]; then
		fallocate -l 10G "$disk"
		# qemu-img create -f qcow2 $disk 4000G
	fi
	if ! pgrep vhost-user-blk; then
		# TODO 可以同时运行的意义是什么，socket 和 fd 都不会互相冲突，就很奇怪
		# 通过其他的方法来提供吧
		VHOST_USER_BLK=${QEMU_DIR}/build/contrib/vhost-user-blk/vhost-user-blk
		pueue add -i -- "$VHOST_USER_BLK" --socket-path="${vm_dir}/disk.socket" --blk-file="$disk"
	fi

	# 这个 json 参考 tests/framework/vm_config.json
	firecracker_set_para '.drives[1].socket' "$vm_dir/disk.socket"
}

# https://docs.fedoraproject.org/en-US/fedora-coreos/tutorial-autologin/
#
# 默认不使用任何参数 sudo coreos-installer install /dev/sda
# 结果就是直接进入系统
arg_coreos=""
function setup_coreos() {
	local test="$vm_dir"/test.ign
	# opt/install 功能已经被移除掉了，如果需要继续使用 coreos ，到时候再来看吧
	if check_option install; then
		set -x
		butane --pretty --strict "$MAIN_REPO"/code/qemu/coreos/init.yaml >"$test"
		set +x
		# 虚拟机中的配合命令
		# sudo coreos-installer install /dev/sda --ignition-file /sys/firmware/qemu_fw_cfg/by_key/46/raw
		# 总而言之，很不好用
		arg_coreos=" -fw_cfg name=opt/com.coreos/config,file=$test"
	fi
}

function setup_firecracker() {
	if ! check_option fire; then
		return
	fi

	if pgrep firecracker &>/dev/null; then
		error "firecracker double run"
	fi

	if [[ $replace_kernel == false ]]; then
		error "firecracker can't boot without bzImage"
	fi

	FIRECRACKER_TEMPLATE="$vm_dir"/fire.json
	cp "$PROGDIR/firecracker/fire.json" "$FIRECRACKER_TEMPLATE"
	API_SOCKET="$vm_dir/firecracker.socket"
	rm -rf "$API_SOCKET"

	# 测试 firecracker 的 vhost blk
	firecracker_disk

	create_switch_tap
	firecracker_set_para '."network-interfaces"[0].host_dev_name' "$tap_name"
	firecracker_set_para '."network-interfaces"[0].guest_mac' "$mac_addr"

	# 到底改如何理解 drives[0].is_root_device 的作用，
	# 为什么配置了之后，直接无视参数中的 uuid 的配置，
	# 他会直接把 /dev/vda 一整个盘当做启动盘的。
	#
	#
	# 此外，现在的 disk 配置进一步验证了我的基本想法，
	# 1. 提供的 disk 只是需要只是需要有一个 ext4 即可，不在乎到底
	# 是 squashfs 的，ext4 的，qcow2 的。
	# 2. initramfs 可有可无，但是没有就直接使用 rootfs 中内容，所以最好还是有的

	get_kernel_image
	firecracker_set_para \
		'.["boot-source"].kernel_image_path' "$kernel_image"

	firecracker_set_para \
		'.["boot-source"].initrd_path' "$initramfs"

	firecracker_set_para '.drives[0].path_on_host' \
		"${vm_dir}/img/boot1"

	# 注意，这里创建了两个 fifo ，但是没有使用，fifo 需要提前有一个 reader ，配置比较麻烦
	# Error: RunWithApi(BuildFromJson(ParseFromJson(Metrics(InitializationFailure("No such device or address (os error 6)")))))
	if [[ ! -e $vm_dir/logs.fifo ]]; then
		mkfifo "$vm_dir"/logs.fifo
	fi
	if [[ ! -e $vm_dir/metrics.fifo ]]; then
		mkfifo "$vm_dir"/metrics.fifo
	fi

	touch "$vm_dir/logs"
	touch "$vm_dir/metrics"
	firecracker_set_para '.["logger"].log_path' "$vm_dir/logs"
	firecracker_set_para '.["metrics"].metrics_path' "$vm_dir/metrics"

	# 必须先去掉才可以，不然会有这个报错
	# Error: RunWithApi(BuildFromJson(ParseFromJson(VsockDevice(CreateVsockBackend(UnixBind(Os { code: 98, kind: AddrInUse, message: "Address already in use" }))))))
	rm -f "$vm_dir/vsock.socket"
	firecracker_set_para '.["vsock"].uds_path' "$vm_dir/vsock.socket"

	if [[ $debug_kernel == true ]]; then
		rm -f "$vm_dir"/gdb.socket
		firecracker_set_para '.["machine-config"].gdb_socket_path' "$vm_dir/gdb.socket"
	fi

	if [[ $ARCH == aarch64 ]]; then
		firecracker_set_num '.["machine-config"].smt' false
	fi

	cpu_num=$(($(getconf _NPROCESSORS_ONLN)))
	ramsize=8
	if check_option smp; then
		cpu_num=$option_result
	fi
	if check_option ram; then
		ramsize=$option_result
	fi

	firecracker_set_num '.["machine-config"].vcpu_count' "$cpu_num"
	firecracker_set_num '.["machine-config"].mem_size_mib' "$((ramsize * 1024))"

	firecracker_set_para \
		'.["boot-source"].boot_args' "$kernel_args"

	cmd="$FIRECRACKER --api-sock ${API_SOCKET} --config-file $FIRECRACKER_TEMPLATE"
}

function setup_blkio() {
	local arg_blkio
	local blkio_iouring
	create_disk_file blkio_iouring false 2 raw
	arg_blkio=" -blockdev driver=io_uring,node-name=blkio_iouring,filename=${blkio_iouring} "
	arg_blkio+=" -device virtio-blk-pci,drive=blkio_iouring "
	arg_storage+=" $arg_blkio "
}

function dump_cmd() {
	local cmd_file_path=$1
	local command=$2
	echo "$command" >"$cmd_file_path"
	# 将 tab 替换为 space ，方便后续的替换
	sed -i 's/\t/ /g' "$cmd_file_path"
	sed -i 's/ -/ \\\n-/g' "$cmd_file_path"
	sed -i '1i #!/usr/bin/env bash' "$cmd_file_path"
	sed -i '2i set -E -e -u -o pipefail ' "$cmd_file_path"
	chmod +x "$cmd_file_path"
	shfmt -w "$cmd_file_path"
}

# 1. 只有不存在的时候，才需要启动 qsd，qemu 可以反复连接同一个 qsd
# 2. 如果运行多个 qsd ，当 qsd 结束的时候，会删掉 socket
function run_qsd() {
	local img_qsd
	local dry_run=${1:-}
	# TODO 似乎默认不提供 qcow2 了?
	# vdb                252:16   0 192.5K  0 disk
	create_disk_file img_qsd false 10 qcow2
	local qsd="$QEMU_DIR"/build/storage-daemon/qemu-storage-daemon
	cmd="$qsd --blockdev driver=file,node-name=file,filename=${img_qsd}"
	cmd+=" --export type=vhost-user-blk,id=export,node-name=file,addr.type=unix,addr.path=$vm_dir/${which_qemu}/qsd.sock,num-queues=1,writable=on"
	cmd+=" --pidfile $vm_dir/$which_qemu/qsd.pid"

	dump_cmd "$vm_dir/qsd.sh" "$cmd"
	if [[ $dry_run != "daemon" ]]; then
		info "$vm_dir/qsd.sh"
		return 0
	fi
	local qsd_pid
	if [[ -e "$vm_dir/$which_qemu"/qsd.pid ]]; then
		qsd_pid=$(cat "$vm_dir/$which_qemu"/qsd.pid)
		if [[ -e /proc/$qsd_pid ]]; then
			return
		fi
	fi

	set -x
	pueue add -i -- eval "$cmd"
	set +x
}

function setup_qemu_storage_daemon() {
	# 如果内存不是共享，那么会有这个问题:
	# qemu-system-x86_64: Failed to read msg header. Read -1 instead of 12. Original request 0.
	# qemu-system-x86_64: Failed to write msg. Wrote -1 instead of 20.
	# qemu-system-x86_64: vhost VQ 0 ring restore failed: -22: Invalid argument (22)
	# qemu-system-x86_64: Failed to set msg fds.
	# qemu-system-x86_64: vhost_set_vring_call failed 22
	# qemu-system-x86_64: vhost-user-blk: vhost start failed: Error starting vhost: Input/output error
	#
	# qemu-storage-daemon: vu_panic: Invalid vring_addr message
	#
	# 最简单的方法就是启动使用 memfd 作为 backend 就可以了
	if ! check_option qsd; then
		return
	fi
	local qsd_mode=$option_result
	run_qsd "$qsd_mode"

	arg_qsd+=" -chardev socket,id=qsd,path=$vm_dir/$which_qemu/qsd.sock,reconnect=10  "
	# TODO 为什么必须设置 num-queues=1 ，似乎和 q35 和 pc 有关
	arg_qsd+=" -device vhost-user-blk-pci,id=blk1,num-queues=1,chardev=qsd"
	arg_storage+=" $arg_qsd "
}

arg_pcie_port=""
function setup_pcie_port() {
	# 给热插拔使用
	# -device pcie-root-port,port=0x10,chassis=1,id=root_port_1,bus=pcie.0,multifunction=on,addr=0x2
	arg_pcie_port="-device pcie-root-port,id=root_port_1"
}

function load_boot_index() {
	:
}

function setup_basic_storage() {
	if ! check_option disk; then
		error "opt/disk not found"
	fi
	# 自动解析，默认不配置 bootindex ，然后
	readarray -t attrs <<<"$option_result"
	# for t in "${attrs[@]}"; do
	# 	echo "-> $t"
	# done

	local disks=()
	for d in "$vm_dir"/img/boot[1-9]; do
		disks+=("$d")
	done

	if [[ ${#disks[@]} != "${#attrs[@]}" ]]; then
		printf '%s\n' "${disks[@]}"
		printf '%s\n' "${attrs[@]}"
		error "disks and attrs doesn't match"
	fi

	for ((i = 0; i < ${#disks[@]}; i++)); do
		local disk_name
		local attr_name
		local attr_drive
		local attr_index
		local format

		local disk_path=${disks[i]}
		disk_name=$(basename "${disks[i]}")
		attr_name=$(echo "${attrs[i]}" | cut -d' ' -f1 -s)
		attr_drive=$(echo "${attrs[i]}" | cut -d' ' -f2 -s)
		attr_index=$(echo "${attrs[i]}" | cut -d' ' -f3 -s)
		if [[ $attr_name != "$disk_name" ]]; then
			printf '%s\n' "${disks[i]} ${attrs[i]}"
			echo "-> $disk_name"
			echo "-> $attr_name"
			echo "-> $attr_drive"
			echo "-> $attr_index"
			error "mismatch"
		fi

		get_disk_format "$disk_path"
		format=$disk_format

		arg_boot_img+=" -drive file=$disk_path,format=$format,id=$disk_name,if=none,discard=on,aio=native,cache.direct=on,media=disk"
		case "$attr_drive" in
			virtio-blk)
				arg_boot_img+=" -device virtio-blk-pci,drive=$disk_name,id=$disk_name"
				;;
			virtio-scsi)
				arg_boot_img+=" -device scsi-hd,bus=scsi1.0,channel=0,scsi-id=0,lun=$i,drive=$disk_name,id=$disk_name"
				;;
			nvme)
				arg_boot_img+=" -device nvme,drive=$disk_name,serial=$(uuidgen),id=$disk_name"
				;;
			ide)
				arg_boot_img+=" -device ide-hd,drive=$disk_name,id=$disk_name"
				;;
			*)
				error "unknown disk type"
				;;
		esac
		if [[ -n $attr_index ]]; then
			arg_boot_img+=",bootindex=$attr_index"
		fi
	done

}

arg_cxl=""
function setup_cxl() {
	# https://stevescargall.com/blog/2022/01/how-to-emulate-cxl-devices-using-kvm-and-qemu/
	#
	# https://www.qemu.org/docs/master/system/devices/cxl.html
	# https://www.opencis.io/docs/run/memory-expander/run-qemu-host/
	# TODO 没那么容易，似乎很多东西都是需要调整的

	arg_cxl+="-object memory-backend-file,id=cxl-mem1,share=on,mem-path=/tmp/cxltest.raw,size=256M "
	arg_cxl+="-object memory-backend-file,id=cxl-lsa1,share=on,mem-path=/tmp/lsa.raw,size=256M "
	arg_cxl+="-device pxb-cxl,bus_nr=12,bus=pcie.0,id=cxl.1  "
	arg_cxl+="-device cxl-rp,port=0,bus=cxl.1,id=root_port13,chassis=0,slot=2 "
	arg_cxl+="-device cxl-type3,bus=root_port13,persistent-memdev=cxl-mem1,lsa=cxl-lsa1,id=cxl-pmem0,sn=0x1 "
	arg_cxl+="-M cxl-fmw.0.targets.0=cxl.1,cxl-fmw.0.size=4G "
}

function setup_storage() {
	setup_scsi_hba

	setup_basic_storage
	setup_iso

	setup_nvme_host
	setup_nvme
	setup_sata
	setup_virtio_blk
	# setup_many_sda
	# setup_qemu_storage_daemon
	# setup_curl_backend
	# setup_blkio
	# setup_floppy
	# setup_many_vda
	# setup_ssh_backend
	# setup_null_backend
	# setup_nfs_backend
	# setup_iscsi_backend
	setup_spdk
	# setup_vduse
	# setup_ide_disk
}

function init_git_repo() {
	pushd "$vm_dir"
	echo "init git repo at $(pwd)"
	git init
	cat <<_EOF_ >.gitignore
		*.qcow2
		*.qcow2.bak
		img/
		dump
		.bash_history
_EOF_
	git add -A
	git commit -m "init"
}

function setup_uuid() {
	check_option uuid
	machine_uuid=$option_result
	arg_uuid="-uuid $machine_uuid"
}

function check_windows() {
	local version=${1:-}
	if ! check_option win; then
		return 1
	fi
	if [[ -z $version ]]; then
		return 0
	fi

	if [[ $version == "$option_result" ]]; then
		return 0
	fi
	return 1
}

function setup_seabios() {
	local seabios=$BIOS_SOURCE_WORKDIR/seabios/out/bios.bin
	arg_bios=" -bios ${seabios}"

}

function setup_qboot() {
	local qboot=$BIOS_SOURCE_WORKDIR/qboot/build/bios.bin
	arg_bios="-bios $qboot"
}

function ovmf_generic() {
	local local_var=$vm_dir/OVMF_VARS.fd

	local output=$3
	local code=$output/$1
	local var=$output/$2

	if [[ ! -f $code ]]; then
		echo "$code not found"
		error "$SCRIPT_DIR/bios_build.sh"
	fi
	if [[ ! -f $local_var ]]; then
		cp "$var" "$local_var"
	fi

	# blockdev 的写法:
	# -blockdev {"driver":"file","filename":"","node-name":"pflash0-storage","auto-read-only":true,"discard":"unmap"}
	# -blockdev {"driver":"file","filename":"","node-name":"pflash1-storage","auto-read-only":true,"discard":"unmap"}
	# -blockdev {"node-name":"pflash0-format","read-only":true,"driver":"raw","file":"pflash0-storage"}
	# -blockdev {"node-name":"pflash1-format","read-only":false,"driver":"raw","file":"pflash1-storage"}

	# TODO 真的没有搞明白这里都是在搞什么，参数配置的莫名奇妙
	# 其实，就是有时候 var 和 code 是分开，有时候是合并的
	if [[ $ARCH == x86_64 ]]; then
		#   OVMF.fd   OVMF_CODE.fd   OVMF_VARS.fd
		# x86 + ovmf_binary 必须用这个方法才可以
		arg_bios=" -drive file=$code,if=pflash,format=raw,unit=0,readonly=on"
		arg_bios+=" -drive file=$local_var,if=pflash,format=raw,unit=1 "
	else
		# TODO 为什么自己构建的 ovmf 只有这个方法才可以启动啊
		# TODO shit arm 也是需要用这种方法
		# 这种启动办法是没有 var 的啊
		arg_bios="--bios $code"
	fi

	# 似乎 -L 没有想要的效果 ?
	# https://github.com/tianocore/tianocore.github.io/wiki/How-to-run-OVMF
	# arg_bios="-L /tmp/x2"

}

EFI_APPLICATIONS=(
	# 对应的目录的 README 告知如何编译
	"$HOME"/data/vn/code/module/gnuefi/hello.efi
	"$HOME"/data/edk2/Build/Bootloader/DEBUG_GCC/X64/Bootloader.efi
)

if [[ -z ${EFI_APPLICATIONS+x} ]]; then
	if find "$vm_dir" -name "*.efi"; then
		readarray -t EFI_APPLICATIONS < <(find "$vm_dir" -name "*.efi")
		printf "install efi : %s\n" "${EFI_APPLICATIONS[@]}"
	else
		exit 0
	fi
fi

function add_efi_application_legacy_way() {
	local efi_disk=$vm_dir/efi_disk.img
	local part_disk=${vm_dir}/part.img
	if [[ ! -f ${efi_disk} ]]; then
		dd if=/dev/zero of="${efi_disk}" bs=512 count=93750
		parted "${efi_disk}" -s -a minimal mklabel gpt
		parted "${efi_disk}" -s -a minimal mkpart EFI FAT16 2048s 93716s
		parted "${efi_disk}" -s -a minimal toggle 1 boot
	fi

	dd if=/dev/zero of="${part_disk}" bs=512 count=91669
	mformat -i "${part_disk}" -h 32 -t 32 -n 64 -c 1

	for VAR in "${EFI_APPLICATIONS[@]}"; do
		mcopy -i "${part_disk}" "${VAR}" ::
	done
	dd if="${part_disk}" of="${efi_disk}" bs=512 count=91669 seek=2048 conv=notrunc

	# 测试下这个 -net none
	arg_bios+=" -drive file=${efi_disk},format=raw "
}

function add_efi_application_fashion_way() {
	# https://github.com/tianocore/tianocore.github.io/wiki/How-to-run-OVMF
	# 可以将 application 直接拷贝到目录中
	for VAR in "${EFI_APPLICATIONS[@]}"; do
		cp "$VAR" "$VirtualDrive"
	done
}

function add_efi_application() {
	VirtualDrive=$PROGDIR/VirtualDrive
	# TODO 很奇怪，居然是 vnc 和 terminal 居然是互相同步的，难道不是两个设备吗?
	# 应该是 UEFI 故意设置的吧，而且 vnc 还是需要 GPU 的吧
	local add_efi_method="fashion"
	if [[ $add_efi_method == fashion ]]; then
		add_efi_application_fashion_way
	else
		add_efi_application_legacy_way
	fi
	# 两者等价的，将目录自动的转换为一个 fat 格式的 disk
	arg_bios+=" -drive file=fat:rw:${VirtualDrive},format=raw,media=disk "
	# legacy 版
	# arg_bios+=" -hda fat:rw:${VirtualDrive}"

	# 通过配置 VirtualDrive 来实现自动运行 uefi 程序
	#
	# 为了运行刚刚编译出来的 Main.efi，操作步骤是:
	# - 启动 QEMU 之后
	# - fs0:
	# - Mian.efi
	#
	# 1. 我们知道 UEFI 启动之后会自动执行 startup.nsh
	# 2. 在 edk2 中搜索 startup.nsh 可以找到 OvmfPkg/PlatformCI/PlatformBuild.py, 了解到 QEMU 可以通过下面的参数实现
	# -drive file=fat:rw:${VirtualDrive},format=raw,media=disk
	#
	# - https://stackoverflow.com/questions/22641605/running-an-efi-application-automatically-on-boot
	# - https://stackoverflow.com/questions/50011728/how-is-an-efi-application-being-set-as-the-bootloader-through-code
	#
	# 在 shell 会等待 5s 来等待程序的执行, 在 ShellPkg/Application/Shell/Shell.c 中修改为等待时间 0s
}

function setup_ovmf_binary2() {
	local ovmf=$BIOS_SOURCE_WORKDIR/ovmf_binary_secure/usr/share/edk2/ovmf
	ovmf_generic OVMF_CODE.fd OVMF_VARS.fd "$ovmf"
}

function setup_ovmf_binary() {
	if [[ $ARCH == aarch64 ]]; then
		local ovmf=$BIOS_SOURCE_WORKDIR/ovmf_binary/usr/share/edk2/aarch64
		# 内部的 edk2 是什么原因啊
		# http://192.168.17.20/repo/pub/openeuler/oe2003/common/aarch64/Packages/edk2.git-aarch64-0-20200515.1441.g5ffcbc4690.noarch.rpm
		# ovmf=/home/martins3/data/bios/ovmf_binary.2003/usr/share/edk2.git/aarch64/
		ovmf_generic QEMU_EFI.fd QEMU_VARS.fd "$ovmf"
	else
		local ovmf=$BIOS_SOURCE_WORKDIR/ovmf_binary/usr/share/edk2/ovmf
		ovmf_generic OVMF_CODE.fd OVMF_VARS.fd "$ovmf"
	fi
}

function setup_ovmf_binary_secure() {
	local ovmf=$BIOS_SOURCE_WORKDIR/ovmf_binary_secure/usr/share/edk2/ovmf
	ovmf_generic OVMF_CODE.secboot.fd OVMF_VARS.secboot.fd "$ovmf"
}

function setup_uboot() {
	local uboot=$BIOS_SOURCE_WORKDIR/u-boot/u-boot.rom
	arg_bios="-bios $uboot"
	# 如果是 x86 下运行必须用 tcg 模式，即便是调整了，也无法正常启动
	# uboot 的文档已经提到过，但是显然是测试不充分的
	# https://docs.u-boot.org/en/latest/board/emulation/qemu-x86.html
}

function setup_ovmf() {
	if [[ $ARCH == x86_64 ]]; then
		local ovmf=$BIOS_SOURCE_WORKDIR/edk2/Build/OvmfX64/DEBUG_GCC/FV/
		ovmf_generic OVMF_CODE.fd OVMF_VARS.fd "$ovmf"
	elif [[ $ARCH == aarch64 ]]; then
		local ovmf=$BIOS_SOURCE_WORKDIR/edk2/Build/ArmVirtQemu-AArch64/DEBUG_GCC/FV/
		ovmf_generic QEMU_EFI.fd QEMU_VARS.fd "$ovmf"
	fi

	if [[ $opt_efi_application_test == true ]]; then
		add_efi_application
	fi
}

arg_bios=""
function setup_bios() {
	bios_mode="none"

	if ! check_option bios; then
		if [[ $ARCH == x86_64 ]]; then
			if check_windows 11; then
				option_result=ovmf_binary_secure
			elif check_windows; then
				option_result=ovmf_binary
			else
				# 默认 seabios
				option_result=seabios
				# option_result=ovmf_binary
			fi
		else
			option_result=ovmf_binary
		fi

	fi

	bios_mode=$option_result
	case "$option_result" in
		uboot)
			setup_uboot
			;;
		qboot)
			setup_qboot
			;;
		ovmf_binary)
			setup_ovmf_binary
			;;
		ovmf_binary_secure)
			setup_ovmf_binary_secure
			;;
		seabios)
			setup_seabios
			;;
		ovmf)
			setup_ovmf
			;;
		*)
			error "unknown bios setup"
			;;
	esac
	if [[ $ARCH == x86_64 ]]; then
		# 这两个参数应该是一样的吧
		arg_bios+=" -chardev file,path=$vm_dir/$which_qemu/debugcon.log,id=seabios -device isa-debugcon,iobase=0x402,chardev=seabios "
		# 和下面这个是等价的吗?
		# arg_bios+="-debugcon $vm_dir/debugcon.log -global isa-debugcon.iobase=0x402"
	else
		:
		# arm 中没有单独的 debugcon 机制，直接复用 ttyAMA0 的
		# 而 x86 不会
	fi

	# 如果需要安装的话
	# "$SCRIPT_DIR"/bios_build.sh seabios

}

function setup_pidfile() {
	arg_pdifile="-pidfile $vm_dir/$which_qemu/pid"
}

arg_pstore=""
function setup_pstore() {
	local pstore_bin=$vm_dir/pstore.bin
	if [[ ! -f $pstore_bin ]]; then
		dd if=/dev/zero of="$pstore_bin" bs=1M count=1
	fi
	# 这个是不行的
	arg_pstore=" -drive if=pflash,format=raw,file=$pstore_bin,id=pstore-flash"
	arg_pstore+=" -device mtd,drive=pstore-flash"
}

function setup_misc() {
	# 参考 qemu-options.hx
	# -no-user-config 的含义，看 qemu_read_default_config_file ，就可以通过一个文件来加载
	# -nodefaults : 不要添加一些默认的设备
	arg_misc="-no-user-config"
	arg_misc+=" -nodefaults"
	# debug-threads=on 在 gdb 中就可以看到每一个 thread 的名称了，不然全部都是 qemu
	arg_misc+=" -name guest=martins3,debug-threads=on"
	# arg_misc+=" -overcommit cpu-pm=on"
	# arg_misc+=" -object event-loop-base"
	# arg_misc+=" -object throttle-group,id=group1"
	# 如果添加这个参数，shutdown 之后，qemu 进程不会消失
	# arg_misc+=" -no-shutdown"
	# arg_misc+=" -no-hpet"
}

# 也许这是最佳的办法了
# 1. 让 virtio-scsi 作为 scsi 的 scsi1.0
# 2. 让所有的channel=0 ，然后 lun 按照顺序分配
#
# 暂时不用考虑热插 virtio-scsi ，所以就不用写入到文件中了，考虑热插也不难
scsi10_lun_index=0
function get_scsi10_lun() {
	scsi10_lun_index=$((scsi10_lun_index + 1))
}

function add_cdrom2() {
	local file=$1
	local index=$2
	if [[ ! -f $file ]]; then
		error "$file is not valid"
	fi
	# virtio-scsi 就是版本答案
	get_scsi10_lun
	local id=cd$scsi10_lun_index
	arg_cdrom+=" -drive file=$file,format=raw,if=none,id=$id,readonly=on "
	# TODO 这里的 scsi-id 参数是什么意思?
	arg_cdrom+=" -device scsi-cd,bus=scsi1.0,channel=0,scsi-id=20,lun=$scsi10_lun_index,drive=$id"
	if [[ -n $index ]]; then
		arg_cdrom+=",bootindex=$index "
	fi
}

arg_cdrom=""
function setup_iso() {
	if ! check_option iso; then
		return
	fi

	readarray -t all_iso <<<"$option_result"

	for line in "${all_iso[@]}"; do
		echo "--> $line"
		iso=$(echo "${line}" | cut -d' ' -f1)
		bootindex=$(echo "${line}" | cut -d' ' -f2 -s)
		add_cdrom2 "$iso_repo/$iso" "$bootindex"
	done

}

# 如何让虚拟机使用容器的镜像来着，忽然想到，如果有 virtio-fs
# 那么就可以让虚拟机直接使用 host 里面容器的文件系统。
#
# TODO 如果不去借助 virtio-fs ，那还有什么办法呢？
function choose_vm_dir_for_vmtest() {
	get_vm_name "vmtest"
	vm_dir=$all_vm_dir/$new_vm_name
}

function choose_vm_dir_for_virtme() {
	get_vm_name "virtme"
	vm_dir=$all_vm_dir/$new_vm_name
}

function choose_vm_dir_for_iso() {
	local iso_name
	choose_iso
	get_vm_name "$(basename "${iso_path%.*}")"
	vm_dir=$all_vm_dir/$new_vm_name
	mkdir -p "$vm_dir/opt"
	iso_name="$(basename "$iso_path")"
	echo "$iso_name 0" >>"$vm_dir/opt/iso"

	if echo "$iso_name" | grep -i win >/dev/null; then
		echo "unknown" >"$vm_dir/opt/win"
	fi
	if echo "$iso_name" | grep -i win11 >/dev/null; then
		echo "11" >"$vm_dir/opt/win"
	fi
}

# TODO 可以尝试一下 initramfs 中的 init 来作为启动项
vmtest_init_sh=/tmp/martins3/init.sh
function setup_opt_dir_vmtest() {
	pushd "$vm_dir/opt"

	local vmtest_extra_cmdline=" rootfstype=9p rootflags=trans=virtio,cache=mmap,msize=1048576 rw"
	vmtest_extra_cmdline+=" init=$vmtest_init_sh "

	# vmtest_extra_cmdline+=" init=/init.sh"
	vmtest_extra_cmdline+=" loglevel=7 raid=noautodetect "
	vmtest_extra_cmdline+=" printk.devkmsg=on"

	echo "$vmtest_extra_cmdline" >cmdline

	echo 1 >vmtest
	echo "$ALL_KERNEL_DIR/linux-vmtest" >kernel
	popd
}

# virtme-ng 配置目录
VIRTME_SSH_DIR="$HOME/.config/virtme-ssh"

function setup_opt_dir_virtme() {
	pushd "$vm_dir/opt"

	# virtme-ng 使用 virtio-fs 共享 rootfs
	# 基础配置
	echo 1 >virtme
	echo manual >virtme_mode
	realpath "$HOME"/data/kernel/linux-build >kernel
	# 启用可写 overlay ，不然很多命令执行都会报错
	echo 1 >virtme_rw

	# 可选配置
	# virtme_exec: 启动时执行的脚本

	popd
}
function is_vmtest() {
	if ! check_option vmtest; then
		return
	fi
	cp "$SCRIPT_DIR"/vmtest-init.sh $vmtest_init_sh

	local bash_binary
	bash_binary=$(which bash)
	# TODO 高级的 sed 语法 !
	sed -i "1s/bin\/bash/${bash_binary//\//\\/}/" $vmtest_init_sh
	sed -i "s/XXXXX/${PATH//\//\\/}/" $vmtest_init_sh

	# TODO 太难了，PATH 中有 / ，sed 命令好难啊
	# sed -i "s/REPLACE_THIS_WITH_PATH/$PATH/" $vmtest_init_sh
	setup_opt_dir_vmtest

	arg_vmtest="-virtfs local,id=root,path=/,mount_tag=/dev/root,security_model=none,multidevs=remap "
	arg_vmtest+=" -no-reboot"
	# 看看共享 root 和普通的文件的区别
	# -virtfs local,id=shared,path=/home/martins3/core/vmtest,mount_tag=vmtest-shared,security_model=none,multidevs=remap \
}

function install_vm_dir() {
	echo "install to ${vm_dir}"
	rm -f "$vm_dir_symbol"
	mkdir -p "$vm_dir"
	ln -s "$vm_dir" "$vm_dir_symbol"
	setup_opt_dir
	uuidgen >"$vm_dir/opt/uuid"
	# 在这里是一个好思路，不断的:
	# 默认新安装的是 ovmf
	echo "ovmf_binary" >"$vm_dir/opt/bios"
	get_new_guest_id "$vm_dir"
	init_git_repo
}

function create_nixos_rootfs() {
	local name=$1
	local size=$2
	local temp
	temp=$(mktemp)
	qemu-img create -f raw "$temp" "$size"
	mkfs.ext4 -L nixos "$temp"
	qemu-img convert -f raw -O qcow2 "$temp" "$name"
	rm "$temp"

}

function choose_vm_dir_for_nixos() {
	get_vm_name nixos
	vm_dir=$all_vm_dir/$new_vm_name

	pushd "$vm_dir"
	nixos-rebuild build-vm -I nixos-config="$PROGDIR"/configuration.nix
	mkdir shared
	mkdir xchg

	echo 1 >"$vm_dir/opt/disk_num"
	create_nixos_rootfs "$(create_disk_file 1)" 500G
	popd
	# 对于 /etc/nixos/configuration.nix 调整
	# 1. nv 驱动去掉，否则图形无法正确显示
	# 2. 图形不能去掉，否则无法进入系统
	# 3. 代理修改为使用 host 的代理
	# 4. imports 去掉 hardware.nix
}

# 两个问题:
# 1. 似乎无法通过 configuration.nix 修改密码，但是的确时候两个配置文件啊
# 2. 测试 9p 的性能 ?
function is_nixos() {
	arg_nixos=""
	if [[ ! -L $vm_dir/result ]]; then
		return
	fi
	pushd "$vm_dir"
	QEMU_KERNEL_PARAMS=""
	echo "$QEMU_KERNEL_PARAMS"
	arg_kernel=$(grep "\-kernel" result/bin/run-nixos-vm)
	arg_kernel=${arg_kernel% *}

	arg_initrd=$(grep "\-initrd" result/bin/run-nixos-vm)
	arg_initrd=${arg_initrd% *}

	arg_kernel_args=$(grep "\-append" result/bin/run-nixos-vm)
	arg_kernel_args=${arg_kernel_args% *}

	arg_nixos=$(grep "\-virtfs" result/bin/run-nixos-vm | grep store)
	arg_nixos=${arg_nixos% *}

	arg_nixos+=" -drive cache=writeback,file="$vm_dir/2.qcow2",id=drive1,if=none,werror=report"
	arg_nixos+=" -device virtio-blk-pci,drive=drive1,serial=root"

	arg_nixos+=" -virtfs local,path=${vm_dir}/shared,security_model=none,mount_tag=shared"
	arg_nixos+=" -virtfs local,path=${vm_dir}/xchg,security_model=none,mount_tag=xchg"

	popd
}

function arg_check() {
	if [[ $opt_efi_application_test == true ]]; then
		if [[ $bios_mode != ovmf ]]; then
			# uboot 无法运行 efi 吧
			error "也许用 ovmf 代码测试才有意义吧"
		fi
		if check_option kernel; then
			error "-kernel 直接跳过了 ovmf 了"
		fi
		# 也许有更好的办法，让系统启动了直奔 efi application
		# 防止 storage 阻塞
		# 我理解如果不去配置 bootindex ，那么就不会有这个问题
		# arg_storage=""
		# arg_boot_img=""
		# ipxe 启动会阻塞的
		arg_network="-net none"
	fi

	if check_windows 11; then
		if [[ $ARCH == x86_64 ]]; then
			arg_machine="-machine q35,smm=on"
		fi
	fi
}

function gdb_debug_qemu() {
	gdb_debug='gdb -ex "handle SIGUSR1 nostop noprint" "handle SIGPIPE nostop noprint" --args'
	# gdb 的时候，让 serial 输出从 unix domain socket 输出
	# https://unix.stackexchange.com/questions/426652/connect-to-running-qemu-instance-with-qemu-monitor
	# qemu 提供了一些 gdb 辅助脚本，例如:
	# bt               handlers         tcg-lock-status
	# coroutine        mtree            timers
	cd "${QEMU_DIR}"
}

function gdb_debug_firecracker() {
	# 很奇怪，firecracker 启动的时候也会暂停一下
	gdb_debug='gdb -ex "handle SIG34 nostop noprint" --args'
}

function setup_opt_dir() {
	mkdir -p "$vm_dir/opt"
}

opt_efi_application_test=false
opt_gdb_debug_qemu=false
opt_launch_migration_target=false
opt_load_vm_file=false
opt_load_vm_cpr=false
opt_load_vm_cpr_transfer=false
debug_kernel=""

install_mode=""
while getopts "alTLdEhixvstwV" opt; do
	case $opt in
		# help begin
		# collei.sh 中的热迁移测试
		# <!-- da410b8c-2c8f-4135-a5e3-373f000d7a8e -->
		# qemu 热迁移的几种方法
		#
		# 1. 普通的热迁移 : 通过 unix domain 等来连接
		# 2. 保存到文件中，原来的进程结束
		#
		# cpr 的三种模式:
		# 1. cpr-transfer : 类似热迁移，但是都是传输
		# 2. cpr-exec : 基于 exec ，也就是仅仅更新 binary 的目标
		# 3. cpr-reboot : 类似保存文件
		a) opt_launch_migration_target=true ;;
		l)
			# 如果 save_vm_file 保存
			opt_load_vm_file=true
			;;
		L)
			# 如果使用 save_vm_cpr 保存
			# 此外，还需要运行 load_vm_cpr
			opt_load_vm_cpr=true
			;;
		T)
			opt_load_vm_cpr_transfer=true
			# 需要启动一个 qemu ，执行 action 为 migrate_cpr
			# 这个方法是最好用的
			;;
			# 最后 cpr 的第三个方法为 exec
			# 另外需要说明的 cpr_exec 无需其他操作，
			# 不知道为什么是网卡有点问题
		d) opt_gdb_debug_qemu=true ;;
		E) opt_efi_application_test=true ;;
		h) show_help ;;
		i) install_mode=iso ;;
		x) install_mode=nixos ;;
		v) install_mode=vmtest ;;
		s) debug_kernel=true ;;
		V) install_mode=virtme ;;
		*) show_help ;;
			# help end
	esac
done

if [[ -n $install_mode ]]; then
	# 1. 确定 vm_dir
	case $install_mode in
		iso)
			choose_vm_dir_for_iso
			;;
		vmtest)
			choose_vm_dir_for_vmtest
			;;
		virtme)
			choose_vm_dir_for_virtme
			;;
		nixos)
			choose_vm_dir_for_nixos
			;;
	esac
	# 2. 安装基本文件
	install_vm_dir
	setup_guest_id
	setup_uuid
	bash "$SCRIPT_DIR"/collei-disk.sh -c create "$vm_dir"

	# vmtest 不需要 iso ，没有 ISO 就不用配置了
	case $install_mode in
		iso) ;;
		vmtest)
			setup_opt_dir_vmtest
			;;
		virtme)
			setup_opt_dir_virtme
			;;
		nixos)
			# 显然 nixos 的 boot image 是自己构建的
			choose_vm_dir_for_nixos
			;;
	esac
else
	check_vm_dir
	setup_guest_id
	setup_uuid
fi

setup_kernel
setup_qemu

# 通过 setup_which_qemu 和 opt_launch_migration_target 共同确定，当前到底启动的
# 是哪一个 qemu ，setup_which_qemu 会返回 which_qemu 和 live_qemu_count
setup_which_qemu
arg_migration_target=

if [[ $opt_launch_migration_target == true ]]; then
	arg_migration_target=" -incoming defer"
	# -only-migratable : 仅仅运行 migratable 的设备启动，提前把 nvme 检查出来
	arg_migration_target+=" -only-migratable"
	if [[ $live_qemu_count -eq 0 ]]; then
		error "launch source qemu firstly"
	fi
	if [[ $live_qemu_count -eq 2 ]]; then
		error "two qemu is running"
	fi
	# 给 collei-action.sh 中 migrate 使用，
	# 因为支持不断的热迁移，所以不能简单的认为源是 s 目的是 t
	# 需要区分谁是 source ，谁是 target
	echo $which_qemu >"$vm_dir"/migrate_source
	get_another_qemu $which_qemu
	which_qemu=$another_qemu
	echo $which_qemu >"$vm_dir"/migrate_target
elif [[ $opt_load_vm_file == true ]]; then
	arg_migration_target=" -incoming file:$vm_dir/vmstate.img"

	arg_migration_target+=" -only-migratable"
	if [[ $live_qemu_count -gt 0 ]]; then
		error "qemu is already running"
	fi
elif [[ $opt_load_vm_cpr == true ]]; then
	arg_migration_target=" -incoming defer"
	arg_migration_target+=" -only-migratable"
	if [[ $live_qemu_count -gt 0 ]]; then
		error "qemu is already running"
	fi
elif [[ $opt_load_vm_cpr_transfer == true ]]; then
	arg_migration_target=" -incoming tcp:0:44444"
	# arg_migration_target+=" -only-migratable"
	# 就用 /tmp/cpr.sock ，collei-action.sh 中替换 vm_dir 不容易
	arg_migration_target+=" -incoming '{\"channel-type\": \"cpr\", \"addr\": { \"transport\": \"socket\", \"type\": \"unix\", \"path\": \"/tmp/cpr.sock\"}}'"

	if [[ $live_qemu_count -eq 0 ]]; then
		error "launch source qemu firstly"
	fi
	if [[ $live_qemu_count -eq 2 ]]; then
		error "two qemu is running"
	fi

	echo $which_qemu >"$vm_dir"/migrate_source
	get_another_qemu $which_qemu
	which_qemu=$another_qemu
	echo $which_qemu >"$vm_dir"/migrate_target
else
	if [[ $live_qemu_count -ne 0 ]]; then
		error "qemu already launched"
	fi
fi

# 确定 qemu 之后，立刻配置
echo 0 >"$vm_dir/$which_qemu"/hp_mm_counter
echo 0 >"$vm_dir/$which_qemu"/hp_disk_counter
echo 0 >"$vm_dir/$which_qemu"/vif_counter

# 根据参数调整执行参数
if [[ $opt_gdb_debug_qemu == true ]]; then
	if check_option fire; then
		gdb_debug_firecracker
	else
		gdb_debug_qemu
	fi
else
	gdb_debug=''
fi

arg_kernel=""
arg_initrd=""
arg_kernel_args=""
arg_storage=""
arg_hacking=""

if [[ $replace_kernel == true ]]; then
	setup_kernel_initrd
	setup_kernel_cmdline
fi

setup_host_cpu_arch
setup_uuid
setup_bios
setup_vfio
setup_vfio_user

setup_edu
setup_machine
setup_mdev
setup_cpu_model
setup_mem_cpu
setup_fs_share
setup_rng
setup_audio
setup_network
setup_storage
setup_usb
setup_pcie_port
setup_chardev
setup_input
setup_ipmi
setup_vnc
setup_display
setup_trace
setup_virtio_dummy
setup_smbios
# setup_cxl
secure_boot
# setup_coreos
setup_firecracker
setup_accel
setup_pidfile
# setup_pstore

# 互相需要检查的参数
arg_check
setup_misc

# Rocky 安装必须指定 -cpu host ，默认的 cpu model 会让 kernel crash
# 参考 : https://community.nethserver.org/t/installation-of-rocky-9-x-display-kernel-panic/21722/2

is_nixos
is_vmtest

if check_option fire; then
	cmd="$gdb_debug $cmd"
else
	if [[ $debug_kernel == true ]]; then
		debug_kernel=" -chardev socket,path=$vm_dir/gdb.socket,server=on,wait=off,id=gdb "
		debug_kernel+=" -S -gdb chardev:gdb  "
	fi
	cmd="${gdb_debug} ${qemu} ${arg_trace} ${debug_kernel} ${arg_storage} \
	${arg_mem_cpu} ${arg_boot_img}  ${arg_kernel} ${arg_kernel_args} \
	${arg_network} ${arg_mdev} ${arg_machine} ${arg_monitor} ${arg_cxl} \
	${arg_initrd} ${arg_mem_balloon} ${arg_hacking} ${arg_bios}  \
	${arg_vfio} ${arg_smbios} ${arg_migration_target} ${arg_share_dir}  \
	${arg_cdrom} ${arg_ipmi} ${arg_accel} ${arg_coreos} ${arg_edu} \
	${arg_pdifile}  ${arg_cpu_model} ${arg_display} ${arg_vnc} ${arg_audio} \
	${arg_serial} ${arg_virtio_dummy} ${arg_pcie_port} ${arg_rng} \
	${arg_nixos} ${arg_vmtest} ${arg_pci_topo} ${arg_misc} \
	${arg_uuid} ${arg_win11} ${arg_pstore} ${arg_input} ${arg_usb}"
fi

dump_cmd "$vm_dir/cmd.sh" "$cmd"

# 备用吧
function run_command_in_bg() {
	local command=$1
	ok=$(pueue add -i -- "$command")
	id=$(echo "$ok" | grep -o -E "id [0-9]+" | sed 's/id //')
	pueue follow "$id"
}

function run_in_pueue() {
	# 为什么不使用 -daemonize 参数
	# qemu-system-x86_64: -serial mon:stdio: cannot use stdio with -daemonize
	# qemu-system-x86_64: -serial mon:stdio: could not connect serial device to character backend 'mon:stdio'
	#
	# 有一点小问题，只能看，如果想要输入就有点麻烦了
	# 需要通过 pueue send 来实现
	local log
	local id
	local command
	local vm_name
	vm_name=$(basename "$vm_dir")
	command="systemd-run --user --scope --collect --unit=$vm_name bash $vm_dir/cmd.sh"
	command="bash $vm_dir/cmd.sh"
	log=$(pueue add -i -g qemu -- "$command")
	id=$(echo "$log" | grep -o -E "id [0-9]+" | sed 's/id //')
	echo "$id" >"$vm_dir"/pueue
	if check_option follow; then
		if [[ $option_result == 0 ]]; then
			return
		fi
	fi
	pueue follow "$id"
}

function should_run_in_pueue() {
	mod="bg"
	# gdb 需要运行在前台的
	if [[ -n ${gdb_debug} ]]; then
		mod="fg"
		return
	fi

	# vmtest 没有网络，必须前台来交互
	if check_option vmtest; then
		mod="fg"
		return
	fi

	if check_option bg; then
		mod=$option_result
		if [[ $mod == "1" ]]; then
			mod="bg"
		elif [[ $mod == "0" ]]; then
			mod="fg"
		else
			error "bg"
		fi
	fi
}

show_current_vm "$vm_dir"
should_run_in_pueue
if [[ $mod == "bg" ]]; then
	run_in_pueue
else
	# -device 的后面参数如果是 json 格式的，会很麻烦，所以先写入到 cmd.sh 中
	# 然后使用 bash 来执行
	vm_name=$(basename "$vm_dir")
	# systemd-run --user --scope --collect --unit="$vm_name" bash "$vm_dir/cmd.sh"
	bash "$vm_dir/cmd.sh"
fi

# TODO virtme SSH 配置目录，继续优化吧，只是达到了基本的效果
VIRTME_SSH_DIR="$HOME/.config/virtme-ssh"
