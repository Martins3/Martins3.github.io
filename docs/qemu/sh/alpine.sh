#!/usr/bin/env bash
set -E -e -u -o pipefail

replace_kernel=
hacking_kcov=false
# hacking_kcov=true
hacking_migration=false
arg_network=""

# @todo 这个报错是什么意思
# qemu-system-x86_64: We need to set caching-mode=on for intel-iommu to enable device assignment with IOMMU protection.

bios_option=
workstation=/home/martins3/hack/
host_cpu_arch=none
vm_dir_symbol=/home/martins3/hack/martins3
kernel_dir=/home/martins3/core/linux-build
qemu_dir=/home/martins3//core/qemu

if grep "GenuineIntel" /proc/cpuinfo >/dev/null; then
	host_cpu_arch="intel"
elif grep "AuthenticAMD" /proc/cpuinfo >/dev/null; then
	host_cpu_arch="amd"
else
	host_cpu_arch="arm"
fi

check_name_validity() {
	if [[ $1 =~ [a-zA-Z0-9_]*$ ]]; then
		echo "$1: valid name"
	else
		echo "$1: invalid name"
		exit 1
	fi
}

function error_arch() {
	echo "unsupported arch"
	exit 1
}

function shutdown_qemu() {
	if [[ -f $pid_file_path ]]; then
		qemu=$(cat "$pid_file_path")
		if ps -p "$qemu" >/dev/null; then
			gum confirm "Kill the machine?" && kill -9 "$qemu"
		fi
	else
		echo "No qemu process found, that's great !"
	fi
}

function setup_kernel_initrd() {
	# kernel_dir=/home/martins3/kernel/centos-upstream
	# kernel_dir=/home/martins3/core/nixos-kernel/v6.5
	# kernel_dir=/home/martins3/core/nixos-kernel/t
	if [[ $hacking_kcov == true ]]; then
		kernel=${kernel_dir}/kcov/arch/x86/boot/bzImage
	else
		kernel=${kernel_dir}/arch/x86/boot/bzImage
	fi
	# kernel="/nix/store/g4zdxdxj8sfbv08grmpahzajrm1gm4s8-linux-5.15.97/bzImage"

	readarray -d '' initramfs_array < <(find "${vm_dir}" -maxdepth 1 -type f -name "initramfs*" -print0)
	readarray -d '' vmlinuz_array < <(find "${vm_dir}" -maxdepth 1 -type f -name "vmlinuz*" -print0)
	if [[ ${#initramfs_array[@]} == 0 ]]; then
		echo "try to replace kernel, but vmlinux or initramfs hasn't been setup"
		exit 0
	fi
	if [[ ${#vmlinuz_array[@]} == 0 ]]; then
		echo "use default kernel=$kernel"
	fi

	if [[ ${#initramfs_array[@]} == 1 ]]; then
		initramfs=${initramfs_array[0]}
	fi

	if [[ ${#vmlinuz_array[@]} == 1 ]]; then
		kernel=${vmlinuz_array[0]}
	fi

	# @todo 原来这个选项不打开，内核无法启动啊
	# @todo 才意识到，这个打开之后，在 kernel cmdline 中的 init=/bin/bash 是无效的
	# @todo 为什么配合 3.10 内核无法正常使用
	arg_initrd="-initrd $initramfs"
	arg_kernel="-kernel $kernel"
	# arg_initrd="-initrd /nix/store/kfaz0nv43qwyvj4s7c5ak4lgdyzdf51s-initrd/initrd" # nixos 的 initrd
	# arg_initrd=""
}

function setup_limit() {
	cgroup_limit=""
	cpu_limit=""
}

function disk_img() {
	echo "$vm_dir"/"$1.qcow2"
}

setup_vfio_13900K_nvme() {
	# TODO /dev/vfio/16 的这个 16 这个数值是随便切换的吗?
	# 是的，当 GPU 拔掉之后 /dev/vfio/14 以及 0000:02:00.0 都是发生了变化
	if [[ ! -c /dev/vfio/16 ]]; then
		# lspci -nn
		# 03:00.0 Non-Volatile memory controller [0108]: Yangtze Memory Technologies Co.,Ltd Device [1e49:0071] (rev 01)
		## 恢复方法
		# 好吧，实际上没有什么好方法恢复!
		# echo 0000:03:00.0 | sudo tee /sys/bus/pci/devices/0000:03:00.0/driver/unbind
		# echo 1e49 0071 | sudo tee /sys/bus/pci/drivers/nvme/new_id
		#
		# 并没有办法恢复成功
		echo 0000:03:00.0 | sudo tee /sys/bus/pci/devices/0000:03:00.0/driver/unbind
		echo 1e49 0071 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
		sudo chown martins3 /dev/vfio/16
	fi
	arg_vfio="-device vfio-pci,host=03:00.0"
}

setup_vfio_13900K_nic() {
	# lspci -nn | grep Eth
	# 06:00.0 Ethernet controller [0200]: Intel Corporation Ethernet Controller I225-V [8086:15f3] (rev 03)
	if [[ ! -c /dev/vfio/19 ]]; then
		echo 0000:06:00.0 | sudo tee /sys/bus/pci/devices/0000:06:00.0/driver/unbind
		echo 8086 15f3 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
		sudo chown martins3 /dev/vfio/19
	fi
	arg_vfio="-device vfio-pci,host=06:00.0"
}

setup_vfio_13900K() {
	# setup_vfio_13900K_nvme
	setup_vfio_13900K_nic
}

setup_vfio_7945hx() {
	local pt_nvme=false

	if [[ $pt_nvme == false ]]; then
		if [[ ! -c /dev/vfio/2 ]]; then
			# 07:00.0 Ethernet controller [0200]: Realtek Semiconductor Co., Ltd. RTL8111/8168/8411 PCI Express Gigabit Ethernet Controller [10ec:8168] (rev 15)
			echo 0000:07:00.0 | sudo tee /sys/bus/pci/devices/0000:07:00.0/driver/unbind
			echo 10ec 8168 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
			sudo chown martins3 /dev/vfio/2
		fi
		arg_vfio="-device vfio-pci,host=07:00.0"
	else
		if [[ ! -c /dev/vfio/1 ]]; then
			# 03:00.0 Non-Volatile memory controller [0108]: MAXIO Technology (Hangzhou) Ltd. Device [1e4b:1602] (rev 01)
			# 04:00.0 Network controller [0280]: MEDIATEK Corp. MT7922 802.11ax PCI Express Wireless Network Adapter [14c3:0616]
			echo 0000:03:00.0 | sudo tee /sys/bus/pci/devices/0000:03:00.0/driver/unbind
			echo 0000:04:00.0 | sudo tee /sys/bus/pci/devices/0000:04:00.0/driver/unbind
			echo 1e4b 1602 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
			echo 14c3 0616 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id

			sudo chown martins3 /dev/vfio/1
		fi
		arg_vfio=" -device vfio-pci,host=03:00.0"
		arg_vfio+=" -device vfio-pci,host=04:00.0"
	fi

}

function setup_vfio() {
	hacking_vfio=false
	# hacking_vfio=true
	arg_vfio=""

	if [[ $hacking_vfio == false ]]; then
		return
	fi

	if [[ $host_cpu_arch == intel ]]; then
		setup_vfio_13900K
	elif [[ $host_cpu_arch == amd ]]; then
		setup_vfio_7945hx
	else
		echo "not supported yet"
	fi

}

function setup_boot_img() {
	arg_img="-drive aio=io_uring,file=$(disk_img 1),format=qcow2,if=virtio"
	# @todo 这个 cache.direct=on 是什么意思来着，为什么必须和 io_uring 放到一起，是因为 aio 不能用于 page cache 吗?
	arg_img="-drive aio=native,cache.direct=on,file=$(disk_img 1),format=qcow2,if=virtio"
	# arg_img="-device nvme,drive=boot_img,serial=foo,bootindex=1 -drive if=none,file=${disk_img},format=qcow2,id=boot_img,aio=native,cache.direct=on"
	# 如果不替换内核，那么就需要使用 bootindex=1 来指定，bootindex 是 -device 的参数，所以需要显示的指出 -device 的类型
	# 这里的 virtio-blk-pci 也可以修改 scsi-hd，总之 qemu-system-x86_64 -device help  中的代码是可以看看的

	# 如果一个系统装好，然后切换介质，通常来说，这不应该
	arg_img="-device virtio-blk-pci,drive=root1,bootindex=1,id=root1 -drive if=none,file=$(disk_img 1),format=qcow2,id=root1,aio=io_uring"

	if [[ $boot_disk_num == 3 ]]; then
		arg_img+=" -device virtio-scsi-pci,id=scsi4"
		arg_img+=" -device scsi-hd,bus=scsi4.0,channel=0,scsi-id=0,lun=11,drive=root2,id=root2"
		arg_img+=" -drive file=$(disk_img 2),format=qcow2,id=root2,if=none "
		arg_img+=" -device scsi-hd,bus=scsi4.0,channel=0,scsi-id=2,lun=12,drive=root3,id=root3"
		arg_img+=" -drive file=$(disk_img 3),format=qcow2,id=root3,if=none "

		# arg_img+=" -device nvme,drive=nvme4,serial=foo4,id=nvme4 -drive file=${disk_img2},format=qcow2,if=none,id=nvme4"
		# arg_img+=" -device nvme,drive=nvme5,serial=foo5,id=nvme5 -drive file=${disk_img3},format=qcow2,if=none,id=nvme5"
	fi
}

setup_fs_share() {
	arg_share_dir=""
	share_memory_option="9p"
	# share_memory_option="virtiofs"
	case $share_memory_option in
		# TODO 了解下 qemu-options.hx 中，-device virtio-9p-type,fsdev=id,mount_tag=mount_ta
		#
		# 建议重新检查下 openEuler-23 ，确认下，share_dir 应该是可以满足的才可以呀
		"9p")
			arg_share_dir="-virtfs local,path=$(pwd),mount_tag=host0,security_model=mapped,id=host0"
			;;
		"virtiofs")
			virtfs_sock=/tmp/martins3/qemu-vfsd.sock
			zellij run -- podman unshare -- virtiofsd --socket-path="$virtfs_sock" --shared-dir "$(pwd)" \
				--announce-submounts --sandbox chroot
			# guest 报错:
			# [   47.949744] SELinux: (dev virtiofs, type virtiofs) getxattr errno 4
			# virtiofsd 报错:
			# [2023-06-10T08:39:30Z ERROR virtiofsd] Waiting for daemon failed: HandleRequest(InvalidParam)

			# TODO 感觉这个工具成熟度还需要时间, 最后用来理解 vhost-user, Rust 和 fuse 的
			arg_share_dir="-device vhost-user-fs-pci,queue-size=1024,chardev=char0,tag=myfs"
			arg_share_dir+=" -chardev socket,id=char0,path=$virtfs_sock"
			# arg_share_dir+=" -m 4G -object memory-backend-file,id=mem,size=4G,mem-path=/dev/shm,share=on -numa node,memdev=mem"
			;;
	esac

	if [[ $hacking_migration == true ]]; then
		# @todo 遇到了这个报错，但是似乎之前没有遇到过
		# Error: Migration is disabled when VirtFS export path '/home/martins3/core/vn' is mounted in the guest using mount_tag 'host0'
		arg_share_dir=""
	fi
}

setup_iommu() {
	hacking_iommu=false
	# hacking_iommu=true

	if [[ $hacking_iommu == false ]]; then
		return
	fi

	use_viriot_iommu=false
	# use_viriot_iommu=true
	if [[ $use_viriot_iommu == true ]]; then
		# 这种模式应该是有 bug，如果不加上内核参数 iommu=pt，那么会卡半天，加上之后可以进入
		arg_machine+=" -device virtio-iommu-pci"
		return
	fi

	# 实际上，看来 IOMMU 实际上是纯纯的模拟操作，你可以在 intel 的平台上使用 amd 的 IOMMU
	# iommu 的选项具体看: x86_iommu_properties 和 vtd_properties

	# intel 需要使用 q35
	intel_iommu="-machine q35,accel=kvm,kernel-irqchip=split -device intel-iommu,intremap=on,caching-mode=on"

	# amd 也是需要修改 machine, 不然报错为，但是这是不是最小约束不知道
	# qemu-system-x86_64: -device amd-iommu,intremap=on: Parameter 'driver' expects a dynamic sysbus device type for the machine
	amd_iommu="-machine q35,accel=kvm,kernel-irqchip=split -device amd-iommu,intremap=on"
	arg_machine=$intel_iommu

	if [[ $host_cpu_arch == amd || $host_cpu_arch == intel ]]; then
		arg_machine=$amd_iommu
		arg_machine=$intel_iommu
	else
		echo "vIOMMU Not support yet for $host_cpu_arch"
		exit 0
	fi

	if [[ $hacking_vfio == true ]]; then
		arg_machine=$intel_iommu
		# 使用 vfio + iommu 的场景下，使用 amd iommu 存在如下报错
		# qemu-system-x86_64: -device vfio-pci,host=03:00.0: vfio 0000:03:00.0: failed to setup container for group 16: memory listener initialization failed: Region amd_iommu: device 00.07.0 requires iommu notifier which is not currently supported
	fi

}

function setup_cpumodel() {
	if [[ $(uname -m) != x86_64 ]]; then
		arg_cpu_model="-cpu host"
		return
	fi
	# arg_cpu_model="-cpu Skylake-Client-IBRS,hle=off,rtm=off"
	# 如果 see=off 或者 see2=off ，系统直接无法启动
	# arg_cpu_model="-cpu Skylake-Client-IBRS,hle=off,rtm=off,sse4_2=off,sse4_1=off,ssse3=off,sep=off"
	# arg_cpu_model="-cpu Skylake-Client-IBRS,vmx=on,hle=off,rtm=off"
	# arg_cpu_model="-cpu Broadwell-noTSX-IBRS,vmx=on,hle=off,rtm=off"
	# arg_cpu_model="-cpu Denverton"
	# arg_cpu_model="-cpu host,hv_relaxed,hv_vpindex,hv_time,"
	# arg_cpu_model="-cpu Broadwell-noTSX-IBRS,spec_ctrl=on,ssbd=on,ibrs_all=on,arch_capabilities=on"
	# arg_cpu_model="-cpu Icelake-Server,arch_capabilities=on,sbdr-ssdp-no=on,ibrs-all=on,ssb-no"
	# arg_cpu_model="-cpu Icelake-Server,arch_capabilities=on,rdctl_no=on"
	# arg_cpu_model="-cpu GraniteRapids"
	# arg_cpu_model="-cpu host,phys_bits=37,host-phys-bits=off"
	# arg_cpu_model="-cpu host,phys-bits=42,host-phys-bits-limit=40"
	# arg_cpu_model="-cpu Broadwell-IBRS"
	# arg_cpu_model="-cpu Skylake-Client"
	# TODO intel 平台上使用这个参数好奇怪哦，cat /proc/cpuinfo 之后可以看到:
	# vendor_id       : GenuineIntel
	# model name      : AMD EPYC Processor
	# 使用 -cpu EPYC 的时候，intel cpu 中显示还是存在 bug ，但是
	# arg_cpu_model="-cpu Opteron_G1"
	arg_cpu_model="-cpu host"
}

function setup_machine() {
	if [[ $(uname -m) == x86_64 ]]; then
		# 对于 smm 是否打开，热迁移没有检查，似乎这是一个 bug 吧
		arg_machine="-machine pc,accel=kvm,kernel-irqchip=on,smm=off"
	elif [[ $(uname -m) == aarch64 ]]; then
		arg_machine="-machine virt,accel=kvm,kernel-irqchip=on -cpu host"
	fi
}

function setup_memory() {
	hacking_memory="virtio-pmem"
	hacking_memory="virtio-mem"
	hacking_memory="prealloc"
	hacking_memory="sockets"
	hacking_memory="file"
	hacking_memory="huge"
	hacking_memory="numa"
	hacking_memory="hotplug"
	hacking_memory="none"

	arg_mem_balloon="-device virtio-balloon,id=balloon0,deflate-on-oom=true,page-poison=true,free-page-reporting=false,free-page-hint=true,iothread=io1 -object iothread,id=io1"
	arg_mem_balloon="-device virtio-balloon,id=balloon0,deflate-on-oom=true"
	# arg_mem_balloon=""

	arg_mem_cpu="-m 4G -smp 4"
	ramsize=8G
	if [[ -s $opt_dir/ram ]]; then
		ramsize=$(cat "$opt_dir"/ram)G
	fi

	cpu_num=$(($(getconf _NPROCESSORS_ONLN) + 0))
	if [[ -s $opt_dir/smp ]]; then
		cpu_num=$(cat "$opt_dir"/smp)
	fi

	case $hacking_memory in
		"none")
			arg_mem_cpu="-smp $cpu_num"
			# echo 1 | sudo  tee /proc/sys/vm/overcommit_memory
			# arg_mem_cpu+=" -object memory-backend-ram,id=pc.ram,size=$ramsize,prealloc=off,share=on -machine memory-backend=pc.ram -m $ramsize"
			# TODO 这里的 -machine memory-backend=mem0 不太科学吧
			arg_mem_cpu+=" -object memory-backend-ram,id=mem0,size=$ramsize,prealloc=off,policy=bind,host-nodes=0 -machine memory-backend=mem0 -m $ramsize"
			;;
		"file")
			# 只有使用这种方式才会启动 async page fault
			ramsize=12G
			arg_mem_cpu="-smp $(($(getconf _NPROCESSORS_ONLN) - 1))"
			arg_mem_cpu+=" -object memory-backend-file,id=id0,size=$ramsize,prealloc=off,mem-path=$workstation/qemu.ram -machine memory-backend=id0 -m $ramsize"
			;;
		"huge")
			ramsize=12G
			# echo 6144 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
			arg_mem_cpu="-smp $(($(getconf _NPROCESSORS_ONLN) - 1))"
			arg_mem_cpu+=" -object memory-backend-file,id=id0,size=$ramsize,prealloc=off,mem-path=/dev/hugepages/martins3-qemu -machine memory-backend=id0 -m $ramsize"
			;;
		"numa")
			# 通过 reserve = false 让 mmap 携带参数 MAP_NORESERVE，从而可以模拟超级大内存的 Guest
			arg_mem_cpu=" -m 32G -smp cpus=32"
			arg_mem_cpu+=" -object memory-backend-ram,size=16G,id=m0,prealloc=false -numa node,memdev=m0,cpus=0-15,nodeid=0"
			arg_mem_cpu+=" -object memory-backend-ram,size=16G,id=m1,reserve=false -numa node,memdev=m1,cpus=16-31,nodeid=1"
			# arg_mem_cpu+=" -object memory-backend-ram,size=4G,id=m2,reserve=false -numa node,memdev=m2,cpus=4,nodeid=2"
			# arg_mem_cpu+=" -numa node,cpus=5,nodeid=3" # 只有 CPU ，但是没有内存
			;;
		"prealloc")
			arg_mem_cpu=" -m 1G -smp cpus=1"
			arg_mem_cpu+=" -object memory-backend-file,size=1G,prealloc=on,share=on,id=m2,mem-path=/dev/hugepages -numa node,memdev=m2,cpus=0,nodeid=0"
			if [[ $(cat /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages) != 1000 ]]; then
				echo 1000 | sudo tee /sys/kernel/mm/hugepages/hugepages-2048kB/nr_hugepages
			fi
			;;
		"virtio-mem")
			# arg_mem_cpu="-m 12G,maxmem=12G"
			# arg_mem_cpu="$arg_mem_cpu -smp sockets=2,cores=2"
			# arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,id=mem0,size=6G"
			# arg_mem_cpu="$arg_mem_cpu -device virtio-mem-pci,memdev=mem0,node=0,size=4G"
			# arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,id=mem1,size=6G"
			# arg_mem_cpu="$arg_mem_cpu -device virtio-mem-pci,memdev=mem1,node=1,size=3G"
			arg_mem_cpu="-m 4G,maxmem=20G -smp sockets=2,cores=2"
			arg_mem_cpu+=" -numa node,nodeid=0,cpus=0-1,nodeid=0,memdev=mem0 -numa node,nodeid=1,cpus=2-3,nodeid=1,memdev=mem1"
			arg_mem_cpu+=" -object memory-backend-ram,id=mem0,size=2G"
			arg_mem_cpu+=" -object memory-backend-ram,id=mem1,size=2G"
			arg_mem_cpu+=" -object memory-backend-ram,id=mem2,size=2G"
			arg_mem_cpu+=" -object memory-backend-ram,id=mem3,size=2G"
			arg_mem_cpu+=" -device virtio-mem-pci,id=vm0,memdev=mem2,node=0,requested-size=1G"
			arg_mem_cpu+=" -device virtio-mem-pci,id=vm1,memdev=mem3,node=1,requested-size=1G"
			;;

		"hotplug")
			arg_mem_cpu="-m 2G,slots=7,maxmem=80G"
			;;
		"virtio-pmem")
			# 1. QEMU 将 pmem 的大小也是计算到 maxmem 中的
			# 2. 如果加上 maxmem 的设置，那么存在如下的报错
			# memory devices (e.g. for memory hotplug) are not enabled, please specify the maxmem option
			arg_mem_cpu="-m 4G,slots=7,maxmem=128G -smp 32"
			pmem_img=${vm_dir}/virtio_pmem.img
			arg_hacking+=" -object memory-backend-file,id=nvmem1,share=on,mem-path=${pmem_img},size=4G"
			arg_hacking+=" -device virtio-pmem-pci,memdev=nvmem1,id=nv1"
			;;
		"sockets")
			arg_mem_cpu="-m 8G -smp 8,sockets=2,cores=2,threads=2,maxcpus=8"
			;;
	esac
}

function setup_ovmf() {
	# @todo 不知道为什么现在使用 ovmf 界面没有办法正常刷新了
	# 将 ovmf 的环境一起集成过来，让 gdb 也可以调试 ovmf
	# 使用 ovmf 的时候，windows 也是无法启动
	#
	# 但是还可以正常使用的机器
	# 当然，如果是 ubuntu ，问题更加严重，直接卡在哪里了
	#
	# sudo cp /run/libvirt/nix-ovmf/OVMF_VARS.fd /tmp/OVMF_VARS.fd
	# sudo chmod 666 /tmp/OVMF_VARS.fd
	local ovmf_code=/run/libvirt/nix-ovmf/OVMF_CODE.fd
	ovmf_code=$workstation/OVMF.fd
	ovmf_var=/tmp/OVMF_VARS.fd
	arg_bios="-drive file=$ovmf_code,if=pflash,format=qcow2,unit=0,readonly=on"
	arg_bios="$arg_bios -drive file=$ovmf_var,if=pflash,format=qcow2,unit=1"

	# arg_bios="-bios $workstation/OVMF.fd"
}

function setup_seabios() {
	seabios=/home/martins3/core/seabios/out/bios.bin
	# 不知道为什么，之前用普通用户态创建了 /tmp/martins3/seabios.log ，如果用 sudo 运行 qemu ，这个文件会制造权限问题
	arg_bios="-chardev file,path=/tmp/martins3/seabios.log,id=seabios -device isa-debugcon,iobase=0x402,chardev=seabios -bios ${seabios}"
	if [[ ! -f $seabios ]]; then
		cd ~/core
		if [[ ! -d seabios ]]; then
			git clone https://github.com/coreboot/seabios
		fi
		cd seabios
		make -j
	fi

}

function setup_qboot() {
	qboot=/home/martins3/core/qboot/build/bios.bin
	if [[ ! -f $qboot ]]; then
		cd ~/core
		if [[ ! -d qboot ]]; then
			git clone https://github.com/bonzini/qboot.git
		fi
		cd qboot
		meson setup --reconfigure build
		cd build
		ninja
	fi
	arg_bios="-bios $qboot"
}

function x86_setup_bios() {
	bios_option=seabios
	case "$bios_option" in
		qboot) setup_qboot ;;
		ovmf) setup_ovmf ;;
		seabios) setup_seabios ;;
		*) echo "use built-in bios default" ;;
	esac

}

function aarch64_setup_bios() {
	if [[ ! -f $vm_dir/flash0.img ]]; then
		dd if=/dev/zero of="$vm_dir"/flash0.img bs=1M count=64
		dd if=/dev/zero of="$vm_dir"/flash1.img bs=1M count=64
		pflash="$workstation"/vm/usr/share/edk2/aarch64/QEMU_EFI-pflash.raw
		# 猜测 pflash 的 raw 和 fd 只是使用不同的参数而已
		# ./usr/share/edk2/aarch64/QEMU_EFI-pflash.raw
		# ./usr/share/edk2/aarch64/QEMU_EFI.fd
		# ./usr/share/edk2/aarch64/QEMU_VARS.fd
		# ./usr/share/edk2/aarch64/vars-template-pflash.raw
		if [[ ! -f $pflash ]]; then
			cd $workstation/vm
			wget https://mirrors.aliyun.com/openeuler/openEuler-22.03-LTS/everything/aarch64/Packages/edk2-aarch64-202011-3.oe2203.noarch.rpm
			rpm2cpio edk2-aarch64-202011-3.oe2203.noarch.rpm | cpio -idmv
		fi
		dd if=$pflash of="$vm_dir"/flash0.img conv=notrunc
	fi
	arg_bios=" -drive file=$vm_dir/flash0.img,if=pflash,format=raw,unit=0,readonly=on"
	arg_bios+=" -drive file=$vm_dir/flash1.img,if=pflash,format=raw "
}

function setup_bios() {
	case "$(uname -m)" in
		x86_64) x86_setup_bios ;;
		aarch64) aarch64_setup_bios ;;
		*) error_arch ;;
	esac

	# 在 guset 中使用 sudo dmidecode -t 1
	arg_smbios="-smbios type=0,vendor=martins3,version=12,date=2022-2-2, -smbios type=1,manufacturer='Martins3 Inc',product='Hacking Alpine',version=$guest_id,serial=,uuid=$(uuidgen)"
}

function setup_kernel_cmdline() {
	local kernel_args=" "
	# kernel_args+=" default_hugepagesz=2M hugepagesz=1G hugepages=1 hugepagesz=2M hugepages=512"
	# kernel_args+=" default_hugepagesz=2M"
	# kernel_args+=" hugepages=100 hugepagesz=2M  hugepages=200 hugepagesz=2M"
	# kernel_args+=" hugepagesz=2M hugepages=100 hugepagesz=2M hugepages=200"
	# kernel_args+=" hugepagesz=1G hugepages=2 hugepages=200"
	# kernel_args+="scsi_mod.scsi_logging_level=0x3fffffff"
	kernel_args+=" nokaslr console=ttyS0,9600 earlyprintk=serial "
	# 使用 function_graph 是分析从该函数开始的结果
	# kernel_args+="ftrace=function_graph ftrace_filter=arch_freq_get_on_cpu "
	# kernel_args+="ftrace=function ftrace_filter=arch_freq_get_on_cpu"
	kernel_args+=" ftrace=function ftrace_filter=dmar_set_interrupt "

	# 进入之后 cat /sys/kernel/debug/tracing/trace
	# kernel_args+="memblock=debug"
	kernel_args+=" systemd.unified_cgroup_hierarchy=1 "
	kernel_args+=" mitigations=off "
	kernel_args+=" nohz_full=0-7"
	# kernel_args+=" rcu_nocbs=0,1 nohz_full=0,1 "
	# kernel_args+=" intremap=off "
	# kernel_args+=" iommu=pt "
	# kernel_args+="iommu=off "
	# kernel_args+="emergency " # 加上这个参数，直接进入到 emergency mode 中
	# 通过这个参数可以直接 disable avx2
	# kernel_args+=" clearcpuid=156"
	# kernel_args+="transparent_hugepage=always "
	# kernel_args+="cma=100m " # TODO 没搞懂咋回事
	# kernel_args+="zswap.enabled=1 zswap.compressor=lz4 zswap.max_pool_percent=20 zswap.zpool=z3fold "
	# kernel_args+="kernelcore=50%"
	# kernel_args+="memblock=debug "
	# kernel_args+="idle=poll "
	# kernel_args+="isolcpus=1,3,5,7"

	if [[ -f "${vm_dir}"/cmdline ]]; then
		arg_kernel_args="-append \"$(cat "${vm_dir}"/cmdline)\""
		return
	fi

	# 现在看来，当用来实现内核替换的时候 lvm 反而更加简单
	if [[ -s ${opt_dir}/partuuid ]]; then
		partuuid=$(cat "$opt_dir"/partuuid)
		kernel_args+=" root=PARTUUID=$partuuid "
	elif [[ -s ${opt_dir}/lvm ]]; then
		# 不替换内核的时候，cat /proc/cmdline
		# 其内容大致为:
		# root=/dev/mapper/fedora_10-root ro rd.lvm.lv=fedora_10/root
		kernel_args+=" $(cat "${opt_dir}"/lvm) "
	else
		echo "${vm_dir}/partuuid missed"
		echo "boot vm with --kernel, kernel cmdline need partuuid cmdline"
		exit 0
	fi
	# @todo 可以看到，先会启动 initramfs 才会开始执行 /bin/bash 的
	# 似乎 init=/bin/bash 变成了 systemd 的参数了
	# arg_kernel_args="root=$root nokaslr console=ttyS0,9600 earlyprink=serial init=/bin/bash"
	arg_kernel_args="-append \"$kernel_args\""
}

function setup_nvme() {
	if [[ $hacking_migration == true ]]; then
		return
	fi
	local arg_nvme="-device pci-bridge,id=mybridge,chassis_nr=1"
	local nvme_multipath=false
	# 注意，nvme1 和 nvme2 可以重复给 device 和 driver 赋值，
	# 看来 device 的 id 空间和 driver 的 id 空间是不重合的
	arg_nvme+=" -device nvme,drive=nvme1,serial=foo,bus=mybridge,addr=0x1,id=nvme1"
	arg_nvme+=" -drive file=${vm_dir}/img/$1,format=qcow2,if=none,id=nvme1"
	# @todo serial 的含义是什么，如果两个 nvme 都填写 serial=foo，那么就会在 guest 中得到如下的报错
	# [    0.686202] nvme nvme1: Duplicate cntlid 0 with nvme0, subsys nqn.2019-08.org.qemu:foo, rejecting

	# max_ioqpairs 是设置队列数量
	arg_nvme+=" -device nvme,drive=nvme2,max_ioqpairs=1,serial=foo2,id=nvme2 "
	arg_nvme+=" -drive file=${vm_dir}/img/$2,format=qcow2,if=none,id=nvme2"

	if [[ $nvme_multipath == true ]]; then
		arg_nvme+=" -device nvme-subsys,id=nvme-subsys-0,nqn=subsys0"
		arg_nvme+=" -device nvme,serial=deadbeef,subsys=nvme-subsys-0,id=nc1"
		arg_nvme+=" -device nvme,serial=deadbeef,subsys=nvme-subsys-0,id=nc2"
		arg_nvme+=" -drive file=${vm_dir}/img/$3,if=none,id=nvm-1"
		arg_nvme+=" -device nvme-ns,drive=nvm-1,nsid=1"
		arg_nvme+=" -drive file=${vm_dir}/img/$4,if=none,id=nvm-2"
		arg_nvme+=" -device nvme-ns,drive=nvm-2,nsid=3,shared=off,detached=on"

		arg_nvme+=" -device nvme-subsys,id=nvme-subsys-1,nqn=subsys1"
		arg_nvme+=" -device nvme,serial=deadbeef,subsys=nvme-subsys-1,id=nc3"
		arg_nvme+=" -drive file=${vm_dir}/img/$5,if=none,id=nvm-3"
		arg_nvme+=" -device nvme-ns,drive=nvm-3,nsid=1"
	fi

	arg_storage+="$arg_nvme "

}

function setup_virtio_blk() {
	# @todo virtio-blk-pci vs virtio-blk-device ?
	# 显示的是 vda，所以 virtio-blk-pci 应该和 -drive 中 if=virtio 等价吧 @todo 代码中确认一下
	local arg_virtio_blk="-device virtio-blk-pci,drive=virtio-blk1,id=virtio-blk1,iothread=io0 -drive file=${vm_dir}/img/$1,format=qcow2,if=none,id=virtio-blk1 -object iothread,id=io0"
	# @todo 为什么 aio=native 后面必须跟上 cache.direct=on，如果不跟上，就会报错，这样的话，有意义吗?
	arg_virtio_blk+=" -device virtio-blk-pci,drive=virtio-blk2,id=virtio-blk2 -drive file=${vm_dir}/img/$2,format=qcow2,if=none,id=virtio-blk2,aio=native,cache.direct=on"
	arg_storage+="$arg_virtio_blk "
}

function setup_sata() {
	local arg_sata=" -drive file=${vm_dir}/img/$1,media=disk,format=qcow2 "
	# 性能太差，按需打开
	local hacking_sata=false
	if [[ $hacking_sata == true ]]; then
		arg_sata+=" -drive file=${vm_dir}/img/$2,media=disk,format=qcow2 "
		arg_storage+="$arg_sata "
	fi
}

function setup_floppy() {
	local arg_floppy=" -blockdev driver=file,node-name=f0,filename=${vm_dir}/img/$1 -device floppy,drive=f0 "
	if [[ $hacking_iommu == true ]]; then
		# 似乎是 q35 上没有 floppy 总线
		arg_floppy=""
	fi
	arg_storage+="$arg_floppy "
}

function setup_scsi() {
	local arg_scsi=""
	local arg_megasas="  -device megasas,id=scsi1 "
	arg_megasas+=" -device scsi-hd,drive=drive0,bus=scsi1.0,channel=0,scsi-id=0,lun=0 "
	arg_megasas+=" -drive file=${vm_dir}/img/$1,if=none,id=drive0 "
	arg_megasas+=" -device scsi-hd,drive=drive1,bus=scsi1.0,channel=0,scsi-id=1,lun=0 "
	arg_megasas+=" -drive file=${vm_dir}/img/$2,if=none,id=drive1 "
	# arg_scsi+=$arg_megasas # 这个启动内核有 bug

	local arg_mptsas="  -device mptsas1068,id=scsi2 "
	arg_mptsas+=" -device scsi-hd,drive=drive2,bus=scsi2.0,channel=0,scsi-id=0,lun=0 "
	arg_mptsas+=" -drive file=${vm_dir}/img/$3,if=none,id=drive2 "
	arg_mptsas+=" -device scsi-hd,drive=drive3,bus=scsi2.0,channel=0,scsi-id=1,lun=0 "
	arg_mptsas+=" -drive file=${vm_dir}/img/$4,if=none,id=drive3 "
	# arg_scsi+=$arg_mptsas # 这个内核无法识别的，切换了各种版本的内核都不支持

	local arg_lsi53c895a="  -device lsi53c895a,id=scsi3 "
	arg_lsi53c895a+=" -device scsi-hd,drive=drive4,bus=scsi3.0,channel=0,scsi-id=0,lun=0 "
	arg_lsi53c895a+=" -drive file=${vm_dir}/img/$5,if=none,id=drive4 "
	arg_lsi53c895a+=" -device scsi-hd,drive=drive5,bus=scsi3.0,channel=0,scsi-id=1,lun=0 "
	arg_lsi53c895a+=" -drive file=${vm_dir}/img/$6,if=none,id=drive5 "
	# arg_scsi+=$arg_lsi53c895a
	# 这个可以用，但是被 https://patchwork.kernel.org/project/qemu-devel/patch/1485444454-30749-4-git-send-email-armbru@redhat.com/ 中说，这个被
	# 看来真的没人对于这个有兴趣，工业上都是在用 virtio-scsi

	arg_storage+="$arg_scsi "
}

function setup_virtio_scsi() {
	# 看源码可以知道，channel 没的选，必须是 0
	# TODO : 不知道为什么 bus=scsi0.0 是必须的. 并不是必须的吧
	# 无论如何，理解下 bus=pci.0,addr=0xa 的含义吧
	#
	# 编程抽象上的痛苦，如何让 id=scsi0 的分配是自动的，至少是管理起来的
	local arg_virtio_scsi=""
	# TODO virtqueue_size 看似可以配置，但是如果设置为 8000 或者 257 ，要么无法识别，要么 QEMU crash
	# 本来想要用这个来测试下 can_queue 的，现在 barbecue 了
	arg_virtio_scsi="  -device virtio-scsi-pci,id=scsi0,addr=0xf,virtqueue_size=256"
	arg_virtio_scsi+=" -device scsi-hd,bus=scsi0.0,channel=0,scsi-id=0,lun=0,drive=scsi-drive1 "
	arg_virtio_scsi+=" -drive file=${vm_dir}/img/$1,format=qcow2,id=scsi-drive1,if=none "
	arg_virtio_scsi+=" -device scsi-hd,bus=scsi0.0,channel=0,scsi-id=2,lun=3,drive=scsi-drive2 "
	arg_virtio_scsi+=" -drive file=${vm_dir}/img/$2,format=qcow2,id=scsi-drive2,if=none "
	arg_virtio_scsi+=" -device scsi-hd,bus=scsi0.0,channel=0,scsi-id=3,lun=4,drive=scsi-drive3 "
	arg_virtio_scsi+=" -drive file=${vm_dir}/img/$3,format=qcow2,id=scsi-drive3,if=none "

	# arg_virtio_scsi+=" -device scsi-hd,bus=scsi0.0,channel=0,scsi-id=4,lun=5,drive=scsi-drive4 "
	# arg_virtio_scsi+=" -drive file=${vm_dir}/img/$4,format=qcow2,id=scsi-drive4,if=none "

	# arg_virtio_scsi+=" -device virtio-scsi-pci,id=scsi5,bus=pci.0,addr=0xb "
	# arg_virtio_scsi+=" -device scsi-hd,bus=scsi5.0,channel=0,scsi-id=0,lun=0,drive=scsi-drive11 "
	# arg_virtio_scsi+=" -drive file=${vm_dir}/img/$3,format=qcow2,id=scsi-drive11,if=none "
	# arg_virtio_scsi+=" -device scsi-hd,bus=scsi5.0,channel=0,scsi-id=2,lun=3,drive=scsi-drive12 "
	# arg_virtio_scsi+=" -drive file=${vm_dir}/img/$4,format=qcow2,id=scsi-drive12,if=none "

	# device 只能设置为 scsi-cd 和 scsi-hd ，其他的两种 scsi-generic 和 scsi-block 是用于设备直通的
	#
	# https://pve-devel.pve.proxmox.narkive.com/qZdhk8h5/integration-of-scsi-hd-scsi-block-scsi-generic
	# With scsi-block and scsi-generic you can bypass qemu scsi emulation and
	# use trim / discard support as the guest can talk directly to the underlying storage.
	#
	# 想个办法，这个 hacking_iommu 的调整放到哪里
	if [[ $hacking_iommu == true ]]; then
		# TODO 不知道为什么，使用上 virtio iommu 的时候，pci.0 这个 bus 消失了
		# 所以去掉 bus=pci.0,addr=0xa
		#
		arg_virtio_scsi="-device virtio-scsi-pci,id=scsi0"
		arg_virtio_scsi+=" -device scsi-hd,bus=scsi0.0,channel=0,scsi-id=0,lun=0,drive=scsi-drive"
		arg_virtio_scsi+=" -drive file=${vm_dir}/img/$1,format=qcow2,id=scsi-drive,if=none"
	fi
	arg_storage+="$arg_virtio_scsi "
	# 这个命令是不行的，scsi-hd 需要挂到一个 scsi bus 上，而创建 scsi bus 依赖于上述 megasas 或者 virtio scsi
	# "-device scsi-hd,bus=scsi4.0,drive=jj -drive if=none,file=${vm_dir}/img5,format=qcow2,id=jj"
}

# echo 0000:03:00.0 | sudo tee /sys/bus/pci/devices/0000:03:00.0/driver/unbind
# HUGEMEM=2000 PCI_ALLOWED=0000:03:00.0 sudo ./scripts/setup.sh
# build/bin/vhost -S /var/tmp -s 2000 -m 0x3
# ./scripts/rpc.py bdev_nvme_attach_controller -b Nvme0 -t PCIE -a 0000:03:00.0
# ./scripts/rpc.py bdev_nvme_attach_controller -b Nvme0 -t PCIE -a ${pci_id}
# ./scripts/rpc.py bdev_malloc_create 16 512 -b Nvme0
# ./scripts/rpc.py vhost_create_blk_controller --cpumask 0x3 spdk.sock Nvme0

# 存在一个小问题，这是所有的操作都是需要 sudo 的，有点难受啊
#
# 启动之后，存在如下错误
# qemu-system-x86_64: Failed initializing vhost-user memory map, consider using -object memory-backend-file share=on
# qemu-system-x86_64: vhost_set_mem_table failed: Invalid argument (22)
# qemu-system-x86_64: vhost-user-blk: vhost start failed: Error starting vhost: Invalid argument

function setup_spdk() {
	# 125s0f0
	arg_storage+=" -chardev socket,id=spdk_vhost_blk0,path=/var/tmp/spdk.sock "
	arg_storage+=" -device vhost-user-blk-pci,id=blk1,chardev=spdk_vhost_blk0,num-queues=2 "
}

function setup_network_bridge() {
	# TODO 这个写成脚本
	#
	# host setup
	#
	# sudo ip link add br0 type bridge
	# sudo ip link set enp7s0 master br0
	# sudo ip address add 10.0.0.3/24 dev br0
	# sudo ip link set dev br0 up
	#
	# guest setup
	#
	# sudo ip address add 10.0.0.4/24 dev ens5
	#
	# 参考 man qemu(1) 中 -netdev bridge 中的描述，这里并不是没有创建 tap 设备，
	# 而是因为 qemu 有脚本自动创建了
	arg_network+=" -netdev bridge,id=hn0,br=br0  -device virtio-net-pci,netdev=hn0,id=nic1"
}

# 有办法自动将 ovs 上的 No such device 都删除吗?
# 4899c619-b0fc-41cc-b085-657f2ef88a25
#     Bridge br-in
#         Port br-in
#             Interface br-in
#                 type: internal
#         Port vif5.2
#             Interface vif5.2
#                 error: "could not open network device vif5.2 (No such device)"
#         Port vif5.0
#             Interface vif5.0
#                 error: "could not open network device vif5.0 (No such device)"
#         Port vif3.1
#             Interface vif3.1
#         Port vif20.0
#             Interface vif20.0
#                 error: "could not open network device vif20.0 (No such device)"
#         Port vif3.0
#             Interface vif3.0
#         Port vif20.1
#             Interface vif20.1
#                 error: "could not open network device vif20.1 (No such device)"
#         Port vif5.1
#             Interface vif5.1
#                 error: "could not open network device vif5.1 (No such device)"
#
function ovs_add_device() {
	device=$1

	if ! ip link show dev br-in >/dev/null 2>&1; then
		sudo ovs-vsctl add-br br-in
		# sudo ifconfig br-in 10.0.0.2/16 up
		sudo ip address add 10.0.0.2/16 dev br-in
		sudo ifconfig br-in up
	fi

	# If device doesn't exist add device.
	if ! ip link show dev "$device" >/dev/null 2>&1; then
		sudo ip tuntap add mode tap user "$USER" dev "$device" || true
		sudo ovs-vsctl add-port br-in "$device" || true
		sudo ip link set "$device" up
	fi
	# 重建的方法
	# sudo ip link delete vif14.0
	#
	# sudo ovs-vsctl del-br br-in
	# sudo ovs-vsctl add-br br-in
	# sudo ip address add 10.0.0.2/16 dev br-in
	# sudo ifconfig br-in up
}

function setup_network_ovs() {
	# sudo ovs-vsctl add-port br-in enp7s0
	#
	# 如果想要 host 上联通 guest ，设置 br-in 的 ip 之后是一种方法
	# sudo ip address add 10.0.0.2/8 dev br-in
	# sudo ip link set br-in up
	# 但是该方法修改了 route -n ，应该会导致这个机器无法和其他的机器 ping 通
	# 这种方法必然有问题，如果真的没问题，那么就不需要 vxlan 直接所有的机器都可以链接到一起
	#
	# 可以 ping 通，iperf 可以，但是 http server 3434 就是无法联通，真的服了
	# 而且 guest 可以联通 host ，但是 host 无法联通 guest

	if0=vif$guest_id.0
	if1=vif$guest_id.1
	ovs_add_device "$if0"
	ovs_add_device "$if1"

	mac=$(printf "%02x\n" "$guest_id")
	arg_network+=" -device virtio-net-pci,netdev=hostnet0,mac=52:54:00:00:00:$mac "
	arg_network+=" -netdev tap,ifname=$if0,id=hostnet0,script=no,downscript=no"

	arg_network+=" -device virtio-net-pci,netdev=hostnet1,mac=52:54:00:00:01:$mac "
	arg_network+=" -netdev tap,ifname=$if1,id=hostnet1,script=no,downscript=no"
}

function setup_network_user() {
	# arg_network="-netdev user,id=net1,hostfwd=tcp::$guest_port-:22 -device e1000e,netdev=net1"
	# arg_network="-netdev user,id=net1,$arg_fwd -device virtio-net-pci,netdev=net1,romfile=/home/martins3/core/zsh/README.md" # 替代 romfile
	local arg_fwd="hostfwd=tcp::$guest_port-:22"
	arg_network+=" -netdev user,id=net1,$arg_fwd -device virtio-net-pci,netdev=net1 "
	# local arg_fwd2="hostfwd=tcp::$((guest_port+1))-:22"
	# arg_network+=" -netdev user,id=net2,$arg_fwd2 -device virtio-net-pci,netdev=net2 "
}

function setup_vhost_net() {
	# 设置方法比想象的还要简单，设置 vhost=on ，qemu 会自动配置 vhost
	# 可以达到的性能为: 13.8 GBytes ，而 tap 性能只有 10.7 GBytes
	if2=vif$guest_id.2
	ovs_add_device "$if2"
	mac=$(printf "%02x\n" "$guest_id")
	arg_network+=" -device virtio-net-pci,netdev=tap_vhost_net,mac=52:54:00:00:02:$mac "
	arg_network+=" -netdev tap,id=tap_vhost_net,script=no,downscript=no,ifname=$if2,vhost=on"
	# 勉强可以搭建起来，但是只能 ping ，无法 iperf3
}

function add_tap_device() {
	device=$1
	case "$device" in
		"lkt-tap0")
			subnet=172.213.0
			;;
		"lkt-tap1")
			subnet=172.30.0
			;;
		"lkt-tap-smbd")
			subnet=10.0.2
			;;
		*)
			echo "Unknown device" 1>&2
			exit 1
			;;
	esac

	# If device doesn't exist add device.
	if ! ip link show dev "$device" >/dev/null 2>&1; then
		sudo ip tuntap add mode tap user "$USER" dev "$device"
	fi

	# Reconfigure just to be sure (even if device exists).
	sudo ip address flush dev "$device"
	sudo ip link set dev "$device" down
	sudo ip address add $subnet.1/24 dev "$device"
	sudo ip link set dev "$device" up

	# 这个 dns 根本没有生效过
	# sudo dnsmasq --port=0 --no-resolv --no-hosts --bind-interfaces \
	# 	--interface "$device" -F $subnet.2,$subnet.20 --listen-address $subnet.1 \
	# 	-x /tmp/dnsmasq-"$device".pid -l /tmp/dnsmasq-"$device".leases || true
}

function setup_network_tap() {
	add_tap_device "lkt-tap0"
	add_tap_device "lkt-tap1"
	# 有点神奇，虽然 dnsmasq 无法正常工作，但是可以直接在 guest 中赋 : sudo ip address add 172.213.0.2/24 dev ens3
	# 然后就可以正常沟通了
	#
	# model 可以切换，有趣的
	# arg_network+=" -netdev tap,id=lkt-tap0,ifname=lkt-tap0,script=no,downscript=no -net nic,netdev=lkt-tap0,model=virtio"
	arg_network+=" -netdev tap,id=lkt-tap1,ifname=lkt-tap1,script=no,downscript=no -net nic,netdev=lkt-tap1,model=e1000e "

}

function setup_vsock() {
	arg_network+=" -device vhost-vsock-pci,id=vhost-vsock-pci0,guest-cid=${guest_id} "
}

function setup_network() {
	setup_network_user
	# setup_network_tap
	# setup_network_bridge
	setup_network_ovs
	setup_vhost_net
	setup_vsock
}

function setup_pipe_serial() {
	# TODO 未解之谜，aarch64 环境中使用这个启动，直接无法启动
	if [[ ! -e $vm_dir/pipe1.in ]]; then
		mkfifo "$vm_dir/pipe1.in" "$vm_dir/pipe1.out"
	fi
	if [[ ! -e $vm_dir/pipe2.in ]]; then
		mkfifo "$vm_dir/pipe2.in" "$vm_dir/pipe2.out"
	fi
	arg_serial+=" -serial pipe:$vm_dir/pipe1 "
	arg_serial+=" -serial pipe:$vm_dir/pipe2 "
}

function setup_serial() {
	# 这太强了，所有的都总结了:
	# https://fadeevab.com/how-to-setup-qemu-output-to-console-and-automate-using-shell-script/#3inputoutputthroughanamedpipefile
	#
	# 用这个可以做一些有趣的实验
	# cat pipe1 和 pipe2 分别对应 ttyS0 和 ttyS1 ，如果 cat pipe1 的时候，
	# 加上 kernel cmdline 为 console=ttyS0 ，那么 dmesg 直接从 cat 那里输出出来
	# TODO ttyS0 是存在什么特殊地位来着
	#
	# 对于 pipe.in 来输出，对于 pipe.out 来输出从而进行交互
	# 不知道是靠什么实现的自动识别后面的 .in 和 .out 的
	# -virtio-serial 是 bus ，必须存在后面才可以有，有趣的
	# TODO 无法理解 pty 的效果是什么
	#
	# 使用 minicom 来连接: minicom -D /dev/pts/16
	# echo info chardev | nc -U -l qemu.mon | grep -E --line-buffered -o "/dev/pts/[0-9]*" | xargs -I PTS ln -fs PTS serial.pts &
	if [[ $(uname -m) == aarch64 ]]; then
		arg_serial=""
	else
		setup_pipe_serial
	fi
	arg_serial+=" -device virtio-serial -chardev pty,id=virtiocon0 -device virtconsole,chardev=virtiocon0"

}

# 将这个函数参数化，任何场景都是调用对应的
function setup_monitor() {
	# 1. 具体使用参考 man qemu(1) 中 -monitor -qmp 和 -mon ，其中 -monitor 和 -qmp 是 -mon 的 legacy 版本
	#	现在都是统一到 -mon 中
	# 2. 也是可以使用 unix domain socket 或者 tcp ，具体使用参考 -chardev 中描述
	# 3. -mon 可以反复设置多个接口

	arg_monitor+=" -chardev socket,id=mon3,path=$vm_dir/qmp-shell,server=on,wait=off"
	arg_monitor+=" -mon chardev=mon3,mode=control" # 如果使用 qmp shell ，必须将

	arg_monitor+=" -chardev socket,id=mon1,path=$vm_dir/qmp,server=on,wait=off"
	arg_monitor+=" -mon chardev=mon1,mode=control,pretty=on"

	arg_monitor+=" -chardev socket,id=mon2,path=$vm_dir/hmp,server=on,wait=off"
	arg_monitor+=" -mon chardev=mon2"
}

function setup_vnc() {
	# 将所有 vnc 都放到浏览器中，性能都非常差，还不如整齐一点
	vnc=$guest_id
	pueue clean
	pueue add -i -- novnc --vnc localhost:$((5900 + vnc)) --listen $((6000 + vnc))
	echo "http://127.0.0.1:$((vnc + 6000))/vnc.html"
	pueue add -i -- microsoft-edge http://127.0.0.1:$((vnc + 6000))/vnc.html
	arg_display+=" -vnc :$vnc,password=off"
}

function setup_display() {
	# TODO 如果只是单独的 -serial stdio ，但是没有 -display none 的时候
	# 在界面上和 stdio 上是存在两个登录入口的，不知道这两个登录入口有什么区别
	# 如果用 w 检查 user ，发现只有一个 user ，是不是一个是 vga 的串口，一个是 serial 的串口

	# 1. 如果单独的设置 -serial stdio 和 -monitor stdio ，qemu 会报错，所以没办法必须将 stdio 耦合到这里
	# 2. 使用 arg_display="-serial stdio -display none" ，stdio 中 ctrl-c 直接将 qemu 杀死了
	if [[ $replace_kernel == true ]]; then
		arg_display="-serial mon:stdio"
	else
		arg_display="-monitor stdio"
	fi

	if [[ $debug_qemu ]]; then
		# 为了让 qemu 界面和 guest console 的输出不要重合
		arg_display="-serial unix:$serial_socket_path,server,nowait"
	fi
	# arg_display="-vnc :0,password=off -monitor stdio"
	if [[ -s "$opt_dir"/nodisplay ]]; then
		arg_display+=" -display none"
		return
	fi

	setup_vnc

	# TODO 不知道为什么 -vga virtio 鼠标是不能用的
	# arg_display+=" -vga virtio"
	# arg_display+=" -vga std"
}

function setup_trace() {
	arg_trace=""
	local tracepoint=""
	# tracepoint+=" kvm_set_user_memory "
	# tracepoint+=" memory_region_ops_read "
	# tracepoint+=" amdvi_ir_intctl "
	if [[ -n $tracepoint ]]; then
		arg_trace="--trace $tracepoint"
	fi
}

function show_msg() {
	gum style --foreground 212 \
		--border-foreground 212 \
		--border double \
		--margin "1 2" \
		--padding "2 4" \
		"$1"
}

show_help() {
	show_msg "$(awk "/\shelp begin/,/help end/" "$0")"
	exit 0
}

function show_kernel_addr() {
	# TODO 怎么传递多个参数来着，should be easy
	sed -i '1i #!/usr/bin/env bash' "$kernel_dir"/scripts/faddr2line
	echo "$kernel_dir/scripts/faddr2line $kernel_dir/vmlinux $1"
	echo "$1"
	"$kernel_dir"/scripts/faddr2line "${kernel_dir}/vmlinux" "$1"
	exit 0
}

function setup_imgs() {
	# 创建额外的 disk 用于测试 nvme 和 scsi 等
	mkdir -p "${vm_dir}"/img/
	for ((i = 0; i < 20; i++)); do
		ext4_img="${vm_dir}/img/$((i + 1))"
		if [ ! -f "$ext4_img" ]; then
			# raw 格式的太浪费内存了
			# 但是可以通过这种方法预设内容
			# mount -o loop /path/to/data /mnt
			# dd if=/dev/null of="${ext4_img}" bs=1M seek=1000
			# mkfs.ext4 -F "${ext4_img}"
			qemu-img create -f qcow2 "${ext4_img}" 1000G
		fi
	done

}

function install_vm() {
	local iso_path
	local boot_disk_num
	set -x

	iso_path=$(find "${workstation}"/iso/*.iso | fzf)

	while true; do
		basename "${iso_path%.*}" >/tmp/martins3/vm_name
		nvim /tmp/martins3/vm_name
		vm_name="$(cat /tmp/martins3/vm_name)"
		vm_name=$(echo "$vm_name" | head -n1 | cut -d " " -f1)

		if grep -q $workstation/by-name "$vm_name" >/dev/null; then
			echo "vm name duplicated"
			continue
		fi
		break
	done
	vm_dir=${workstation}/vm/$vm_name
	echo "install to ${vm_dir}"

	rm -f $vm_dir_symbol
	mkdir -p "$vm_dir"
	ln -s "$vm_dir" "$vm_dir_symbol"
	setup_vm_dir

	boot_disk_num=$(gum choose 1 3)
	ln -s "$iso_path" "$vm_dir"/iso

	get_new_port_num

	for ((i = 1; i <= boot_disk_num; i = i + 1)); do
		qemu-img create -f qcow2 "$(disk_img $i)" 400G
	done

	echo "$boot_disk_num" >"$disk_num_file_path"

	setup_vnc
	setup_machine
	if [[ $(uname -m) == aarch64 ]]; then
		aarch64_setup_bios
	else
		arg_bios=""
	fi

	# 否则本地安装即可
	# arg_vnc=""
	cd "$vm_dir"
	git init
	echo '*.qcow2' >>.gitignore
	echo "img/" >>.gitignore
	git add -A
	git commit -m "init"

	if [[ $boot_disk_num == 3 ]]; then
		# shellcheck disable=2086
		$qemu -boot d -cdrom "$iso_path" \
			-device virtio-blk-pci,drive=boot_img1 -drive if=none,file="$(disk_img 1)",format=qcow2,id=boot_img1 \
			-device virtio-blk-pci,drive=boot_img2 -drive if=none,file="$(disk_img 2)",format=qcow2,id=boot_img2 \
			-device virtio-blk-pci,drive=boot_img3 -drive if=none,file="$(disk_img 3)",format=qcow2,id=boot_img3 \
			-m 8G -smp 32 $arg_display $arg_machine $arg_bios -monitor stdio
	else
		set -x
		# TOOD hda 到底是什么，现在一直有一个迷信，有些地方只能使用 -hda ，但是应该是 bootindex 导致的吧
		#
		# 不要给 arg_vnc 增加双引号，否则会存在 qemu-system-x86_64: : drive with bus=0, unit=0 (index=0) exists
		# 复现方法很简单，只是需要在终端中执行的下面命令之后增加 "" 即可
		# shellcheck disable=2086
		$qemu -boot d -cdrom "$iso_path" -hda "$(disk_img 1)" -m 8G -smp 8 $arg_display $arg_machine $arg_bios -monitor stdio
		# 这里的设置 是为了防止 aarch64 的环境下，arm 的 vnc 是 monitor
	fi

	exit 0
}

arg_check() {
	# 将互相冲突的参数放到这里?
	if [[ -n $cpu_limit && -n $cgroup_limit ]]; then
		echo "cpu_limit=$cpu_limit conflicts with cgroup_limit=$cgroup_limit"
		exit 0
	fi
}

# TODO 似乎 xfs 的 cow 性能很差，可以用 fio + perf 测试下到底在搞什么
function create_snapshot() {
	local snapshot_action="$1"
	if [[ -f $pid_file_path ]]; then
		echo "shutdown vm firstly to create snapshot"
		return
	fi

	if [[ $snapshot_action == "backup" ]]; then
		for i in "$vm_dir"/*.qcow2; do
			cp "$i" "$i".bak
		done
	elif [[ $snapshot_action == "restore" ]]; then
		for i in "$vm_dir"/*.qcow2.bak; do
			cp "$i" "${i%.bak}"
		done
	else
		echo "unknow action"
	fi
	exit 0
}

function dump_and_crash() {
	# 此外 qemu 给 gdb 增加了命令 dump-guest-memory 的命令:
	# https://github.com/qemu/qemu/blob/master/scripts/dump-guest-memory.py
	set -x
	if [[ -f $dump_guest_path ]]; then
		mv "$dump_guest_path" "$dump_guest_path".bak
	fi

	# TODO 直接传递 qmp 过去 ?
	# echo "help" | socat - unix-connect:$mon_socket_path
	# echo "dump-guest-memory -z $dump_guest_path"
	# socat -,echo=0,icanon=0 unix-connect:/tmp/martins3/hmp
	echo "dump-guest-memory -z $dump_guest_path" | socat - unix-connect:"$vm_dir"/hmp
	if [[ $replace_kernel != true ]]; then
		exit 0
	fi
	# 这里需要等待吗?
	/home/martins3/core/crash/crash "${kernel_dir}/vmlinux" "$dump_guest_path"
	exit 0
}

function gdb_debug_qemu() {
	set -x
	if [[ -f $pid_file_path && -d /proc/$(cat "$pid_file_path") ]]; then
		echo "qemu process already exist, attach to it"
		# cd "${qemu_dir}" || exit 1
		gdb -p "$(cat "$pid_file_path")"
		# amdvi_mem_ir_write
		exit 0
	fi
	debug_qemu='gdb -ex "handle SIGUSR1 nostop noprint" --args'
	# gdb 的时候，让 serial 输出从 unix domain socket 输出
	# https://unix.stackexchange.com/questions/426652/connect-to-running-qemu-instance-with-qemu-monitor
	cd "${qemu_dir}"
}

function gdb_debug_kernel_wait() {
	debug_kernel="-S -s"
}

function gdb_debug_kernel() {
	echo "debug kernel"
	# 原则上来说，是可以使用任意的目录，因为只是使用了其中 gdb scripts 而已
	cd "${kernel_dir}"
	# ext4.ko 似乎暂时无法打点，只是因为没有参考文档而已
	vmlinux="$vm_dir/vmlinux"
	if [[ ! -f $vmlinux ]]; then
		vmlinux=${kernel_dir}/vmlinux
	fi

	gdb "$vmlinux" -ex "target remote :1234" \
		-ex "hbreak start_kernel" -ex "hbreak __crash_kexec" -ex "continue"
	exit 0
}

function connect_to_console() {
	socat -,echo=0,icanon=0 unix-connect:"$serial_socket_path"
	exit 0
}

function connect_to_monitor() {
	case "$1" in
		hmp)
			socat -,echo=0,icanon=0 unix-connect:"$vm_dir"/hmp
			;;
		qmp)
			socat -,echo=0,icanon=0 unix-connect:"$vm_dir"/qmp
			;;
		shell)
			qmp_shell=${qemu_dir}/scripts/qmp/qmp-shell
			$qmp_shell "$vm_dir"/qmp-shell
			;;
		*)
			echo "qmp hmp shell"
			;;
	esac
	exit 0

}

function perf_qemu() {
	sudo perf record --call-graph dwarf -p "$(cat "$pid_file_path")" -- sleep 10
	exit 0
}

function rename() {
	set -x
	echo "rename"
	new_name=$(gum input --placeholder "name $(realpath "$vm_dir")")
	check_name_validity "$new_name"
	new_vm_dir=${workstation}/vm/$new_name

	port=$(cat "$vm_dir"/port)
	mv "$vm_dir" "$new_vm_dir"
	ln -sf "$new_vm_dir"/port $workstation/vm/ports/"$port"
	rm $vm_dir_symbol
	ln -sf "$new_vm_dir" $vm_dir_symbol
	exit 0
}

function get_new_port_num() {
	for ((i = 4020; i <= 5000; i = i + 1)); do
		link="$workstation"/vm/ports/$i
		# if found dead symbol link, remove it firstly
		# if [[ -L $link && ! -e $link ]]; then
		# 	rm $link
		# fi

		# skip
		if [[ -L $link ]]; then
			continue
		fi

		if ln -s "$ssh_file_path" $link; then
			break
		fi
	done
	if [[ $i == 5000 ]]; then
		echo "impossible"
		exit 1
	fi
	echo $i >"$ssh_file_path"
	guest_port=$i
	guest_id=$((guest_port - 4000))
}

function clone() {
	set -x
	cd ${workstation}/vm
	new_name=$(gum input --placeholder "clone name")
	check_name_validity "$new_name"
	new_vm_dir="${workstation}/vm/$new_name"
	real_vm_dir=$(realpath "$vm_dir")
	cp -r "$real_vm_dir" "$new_vm_dir"

	ln -s "$new_vm_dir" "$vm_dir_symbol"
	setup_vm_dir
	get_new_port_num

	exit 0
}

function default_vm() {
	set -x
	readarray -d '' dirs_array < <(find /home/martins3/hack/vm/ -maxdepth 1 -type d -print0)
	for i in "${dirs_array[@]}"; do
		if [[ -f $i/port ]]; then
			installed_vms+=("$i")
		fi
	done
	choice=$(printf "%s\n" "${installed_vms[@]}" | fzf)
	rm $vm_dir_symbol
	ln -s "$choice" $vm_dir_symbol
	exit 0
}

function setup_qemu() {
	qemu=${qemu_dir}/build/$(uname -m)-softmmu/qemu-system-$(uname -m)
	# qemu="qemu-system-$(uname -m)"
}

function edit_options() {
	if env | grep vim >/dev/null; then
		echo "Don't Run nvim in vim 😸"
		exit 0
	fi
	cd "$vm_dir"
	nvim
	exit
}

opt_gdb_debug_kernel_wait=false
opt_gdb_debug_qemu=false
opt_tcg_mode=false
opt_launch_migration_target=false
opt_cgroup=false

setup_vm_dir() {
	# 如果不存在符号链接 || 如果符号链接不合法
	if [[ ! -L ${vm_dir_symbol} ]] || [[ ! -e ${vm_dir_symbol} ]]; then
		return
	fi
	vm_dir="$(realpath $vm_dir_symbol)"
	vm_name=$(basename -- "$vm_dir")
	opt_dir=$vm_dir/opt
	mkdir -p "$opt_dir"
	echo $"vm dir : $vm_dir"
	pid_file_path=$vm_dir/pid
	cmd_file_path=$vm_dir/cmd.sh
	ssh_file_path=$vm_dir/port
	dump_guest_path=$vm_dir/dump
	disk_num_file_path=$vm_dir/disk_num
	serial_socket_path=$vm_dir/serial

	if [[ -f $disk_num_file_path ]]; then
		boot_disk_num=$(cat "$disk_num_file_path")
	fi
	if [[ -f $ssh_file_path ]]; then
		guest_port=$(cat "$ssh_file_path")
		guest_id=$((guest_port - 4000))
	fi
}

setup_vm_dir
setup_qemu # 因为 qemu 被独立的程序使用，所以放到 getopts 的上面
mkdir -p "$workstation"/vm/ports
opt_gdb_debug_kernel=false

while getopts "abcdefg:hijklm:nopqrstu" opt; do
	case $opt in
		# help begin
		a) opt_launch_migration_target=true ;;
		b) opt_cgroup=true ;;
		c) connect_to_console ;;
		d) opt_gdb_debug_qemu=true ;;
		u) dump_and_crash ;;
		f) cpu_limit="taskset -ac 0-1" ;;
		g) create_snapshot "${OPTARG}" ;;
		h) show_help ;;
		i) install_vm ;;
		j) show_kernel_addr "$2" ;;
		l) default_vm ;;
		k) opt_gdb_debug_kernel=true ;;
		m) connect_to_monitor "${OPTARG}" ;;
		o) clone ;;
		p) perf_qemu ;;
		r) rename ;;
		s) opt_gdb_debug_kernel_wait=true ;;
		t) opt_tcg_mode=true ;;
		e) edit_options ;;
		*) show_help ;;
			# help end
	esac
done

if [[ -s "$opt_dir"/replace_kernel ]]; then
	replace_kernel=true
fi

# 根据参数执行具体的动作
setup_imgs

if [[ $opt_tcg_mode == true ]]; then
	arg_machine="--accel tcg,thread=single"
	arg_mem_cpu="-m 8G" # cpu 数量最好还是 1，内存需要指定一下，默认 128m 无法启动的
	arg_cpu_model=""    # cpu model 不能支持 host 了
fi

# 根据参数调整执行参数
if [[ $opt_gdb_debug_qemu == true ]]; then
	gdb_debug_qemu
else
	debug_qemu=
fi

if [[ $opt_gdb_debug_kernel_wait == true ]]; then
	gdb_debug_kernel_wait
else
	debug_kernel=
fi

if [[ $opt_launch_migration_target == true ]]; then
	# @todo 丑陋的代码，从原则上将，option 应该在最上面的才对，修改参数
	# @todo 本地热迁移存在一个小问题，就是 domain 是相互冲突的
	# arg_qmp="-qmp tcp:localhost:5444,server,wait=off"
	arg_network="-netdev user,id=net1,hostfwd=tcp::5557-:22 -device e1000e,netdev=net1"
	arg_network="-netdev user,id=net1,hostfwd=tcp::5557-:22 -device virtio-net-pci,netdev=net1"
	# 需要保证迁移的两侧的 romfile 内容一致才可以
	# arg_network="$arg_network,romfile=/home/martins3/hack/vm/img1"
	arg_migration_target="-incoming tcp:0:4000"
	# @todo 测试发现，目标端是否为 smm 不影响迁移
	arg_machine="-machine pc,accel=kvm,kernel-irqchip=on,smm=on"
	# target 端是啥 cpu model 根本没影响啊
	arg_cpu_model="-cpu Broadwell-IBRS"
fi

if [[ $opt_cgroup == true ]]; then
	# 如果以后使用多个虚拟机的时候
	# 可以带上虚拟机唯一标识，而不是使用 mem
	qemu_cgroup=alpine
	if [[ ! -d /sys/fs/cgroup/"$qemu_cgroup" ]]; then
		sudo cgcreate -g memory:"$qemu_cgroup"
		# 尝试
		# sudo cgcreate -t "$USER":"$USER" -a "$USER":"$USER" -g memory:user.slice/user-1000.slice/user@1000.service/user.slice/qemu
		# sudo cgset -r memory.max=10G mem
	fi
	cgroup_limit="sudo cgexec -g memory:$qemu_cgroup"
	# TODO 感觉不是很好用
	# 1. 没有办法更加动态的控制
	# 2. 需要使用 root
fi

# 获取 PARTUUID 的方法: 在 guest 中，blkid 看根分区的
# @todo 这个检查太早了，导致如果是安装系统的时候总是被这个打断
arg_kernel=""
arg_initrd=""
arg_kernel_args=""
arg_storage=""

# @todo 这个地方应该调整下，源端和目标端冲突了
arg_pdifile=""
if [[ $hacking_migration == false ]]; then
	arg_pdifile="-pidfile $pid_file_path"
fi

arg_migration_target=
arg_hacking=""

if [[ $replace_kernel == true ]]; then
	setup_kernel_initrd
	setup_kernel_cmdline
fi

setup_qemu
setup_fs_share
setup_boot_img
setup_vfio
setup_bios
setup_iommu
setup_cpumodel
setup_machine
setup_memory
setup_limit
setup_nvme 1 2 18 19 20
# setup_sata 6 7
# setup_floppy 8
setup_scsi 9 10 11 12 13 14
setup_virtio_scsi 5 15 16 17
# setup_spdk
# 这里非常奇怪，如果将 setup_virtio_blk 3 4 放到 setup_virtio_scsi 5 15 前面，那么 kernel 就
setup_virtio_blk 3 4
setup_network
setup_monitor
setup_serial
setup_display
setup_trace

# 互相需要检查的参数
arg_check

if [[ $opt_gdb_debug_kernel == true ]]; then
	gdb_debug_kernel
fi

# @todo 话说为什么非要使用 eval 来着
cmd="${cpu_limit} ${cgroup_limit} ${debug_qemu} ${qemu} ${arg_trace} ${debug_kernel} ${arg_img} ${arg_mem_cpu}  \
  ${arg_kernel} ${arg_kernel_args} ${arg_bios} ${arg_network} \
  ${arg_machine} ${arg_monitor} ${arg_initrd} ${arg_mem_balloon} ${arg_hacking} \
   ${arg_vfio} ${arg_smbios} ${arg_migration_target} ${arg_share_dir} \
   ${arg_pdifile} ${arg_storage} ${arg_cpu_model} ${arg_display} ${arg_serial}"

echo "$cmd" >"$cmd_file_path"
sed -i 's/ -/ \\\n-/g' "$cmd_file_path"
sed -i '1i #!/usr/bin/env bash' "$cmd_file_path"
sed -i '2i set -E -e -u -o pipefail ' "$cmd_file_path"
chmod +x "$cmd_file_path"
# shfmt -w "$cmd_file_path"

# eval "$cmd"
# if [[ $cmd == "" ]]; then
# 	exit 0
# fi

# debug_qemu needs the output
# cgroup_limit may need password input
if [[ -z ${debug_qemu} && -z ${cgroup_limit} ]]; then
	pueue group add qemu || true
	pueue add -i -g qemu -- eval "\"$cmd\""
else
	eval "$cmd"
fi
