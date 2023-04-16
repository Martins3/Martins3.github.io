#!/usr/bin/env bash
set -E -e -u -o pipefail

# @todo 用 https://github.com/charmbracelet/gum 来重写这个项目
replace_kernel=false

hacking_memory="hotplug"
hacking_memory="virtio-pmem"
hacking_memory="none"
hacking_memory="virtio-mem"
hacking_memory="prealloc"
hacking_memory="sockets"
hacking_memory="numa"
hacking_memory="file"
hacking_memory="none"

share_memory_option="9p"
# share_memory_option="virtiofs"

hacking_migration=false
# @todo 尝试在 guest 中搭建一个 vIOMMU
hacking_vfio=false

use_ovmf=false
minimal=false
qmp_shell=true # 使用 qmp_shell 可以交互，否则就是输入 json

abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

# ----------------------- 配置区 -----------------------------------------------
kernel_dir=$(jq -r ".kernel_dir" <"$configuration")
# 为什么 -kernel 的时候 @todo 的 centos  kernel 不行啊
# kernel_dir=/home/martins3/kernel/centos-upstream
qemu_dir=$(jq -r ".qemu_dir" <"$configuration")
workstation="$(jq -r ".workstation" <"$configuration")"
# ------------------------------------------------------------------------------
# @todo 使用 3.10 内核，不知道为什么，最后无法正常启动
# kernel_dir=/home/martins3/kernel/centos-3.10.0-1160.11.1-x86_64

qemu=${qemu_dir}/build/x86_64-softmmu/qemu-system-x86_64
virtiofsd=${qemu_dir}/build/tools/virtiofsd/virtiofsd
kernel=${kernel_dir}/arch/x86/boot/bzImage
# kernel="/nix/store/g4zdxdxj8sfbv08grmpahzajrm1gm4s8-linux-5.15.97/bzImage"

in_guest=false
if grep hypervisor /proc/cpuinfo >/dev/null; then
	in_guest=true
	replace_kernel=false
	# @todo 需要让 guest 中安装的 kernel
	workstation="/root/hack/vm"
fi

distribution=openEuler-22.09-x86_64-dvd
# distribution=openEuler-20.03-LTS-SP3-x86_64-dvd
# distribution=CentOS-7-x86_64-DVD-2207-02
# distribution=ubuntu-22.04.1-live-server-amd64
# distribution=ubuntu2204
#
# @todo fedora 内核只能在 centos 上，不能在 oe 上安装，因为 linux-install 这个包的原因
# https://kojipkgs.fedoraproject.org/packages/kernel/4.19.16/200.fc28/

arch=$(uname -m)
if [[ $arch == aarch64 ]]; then
	distribution=
fi

if [[ $in_guest == true ]]; then
	distribution=CentOS-7-x86_64-DVD-2207-02
fi

cgroup_limit=""

guest_port=5556
qmp_port=4444
case $distribution in
	openEuler-22.09-x86_64-dvd)
		use_ovmf=false
		;;
	openEuler-20.03-LTS-SP3-x86_64-dvd)
		guest_port=5557
		qmp_port=4445
		replace_kernel=false
		;;
	CentOS-7-x86_64-DVD-2207-02)
		replace_kernel=false
		minimal=true
		guest_port=5558
		qmp_port=4446
		;;
	ubuntu-22.04.1-live-server-amd64)
		replace_kernel=false
		guest_port=5559
		qmp_port=4447
		use_ovmf=true
		;;
esac

iso=${workstation}/iso/${distribution}.iso
disk_img=${workstation}/vm/${distribution}.qcow2

# dir=/nix/store/rlf9m7zzhdcsp4mv78jfi2f6scfcvbp7-nixos-disk-image
# if [[ ! -f /tmp/nixos.qcow2 ]]; then
#   install -m644 $dir/nixos.qcow2 /tmp/nixos.qcow2
# fi
# disk_img=/tmp/nixos.qcow2 # nixos

# @todo 这个地方应该调整下，源端和目标端冲突了
arg_pdifile=""
if [[ $hacking_migration == false ]]; then
	mkdir -p /tmp/martins3-alpine
	arg_pdifile="-pidfile /tmp/martins3-alpine/qemu-pid"
fi

debug_qemu=
debug_kernel=
launch_gdb=false
arg_migration_target=

arg_hacking=""

arg_img="-drive aio=io_uring,file=${disk_img},format=qcow2,if=virtio"
arg_img="-drive aio=native,cache.direct=on,file=${disk_img},format=qcow2,if=virtio"
# 当前端是 nvme ，那么可以看到非常酷炫的结果
arg_img="-device nvme,drive=boot_img,serial=foo,bootindex=1 -drive if=none,file=${disk_img},format=qcow2,id=boot_img,aio=native,cache.direct=on"
# 如果不替换内核，那么就需要使用 bootindex=1 来指定，bootindex 是 -device 的参数，所以需要显示的指出 -device 的类型
# 这里的 virtio-blk-pci 也可以修改 scsi-hd，总之 qemu-system-x86_64 -device help  中的代码是可以看看的
arg_img="-device virtio-blk-pci,drive=boot_img,bootindex=1 -drive if=none,file=${disk_img},format=qcow2,id=boot_img,aio=native,cache.direct=on"
root=/dev/vdb2
# root=/dev/vda2 # nixos

arg_share_dir=""
case $share_memory_option in
	"9p")
		arg_share_dir="-virtfs local,path=$(pwd),mount_tag=host0,security_model=mapped,id=host0"
		;;
	"virtiofs")
		# @todo drop_supplementary_groups 让 virtiofd 的启动有问题
		# sudo /home/martins3/core/qemu//build/tools/virtiofsd/virtiofsd --socket-path=/tmp/vhostqemu -o source=/tmp -o cache=always
		# unshare 也导致的权限问题
		zellij run -- sudo "$virtiofsd" --socket-path=/tmp/vhostqemu -o source="$(pwd)" -o cache=always
		# sudo chown martins3 /tmp/vhostqemu ，否则 QEMU 需要 root 启动
		arg_share_dir="-device vhost-user-fs-pci,queue-size=1024,chardev=char0,tag=myfs"
		arg_share_dir="$arg_share_dir -chardev socket,id=char0,path=/tmp/vhostqemu"
		arg_share_dir="$arg_share_dir -m 4G -object memory-backend-file,id=mem,size=4G,mem-path=/dev/shm,share=on -numa node,memdev=mem"
		;;
esac

if [[ $hacking_migration == true ]]; then
	# @todo 遇到了这个报错，但是似乎之前没有遇到过
	# Error: Migration is disabled when VirtFS export path '/home/martins3/core/vn' is mounted in the guest using mount_tag 'host0'
	arg_share_dir=""
fi

# 在 guset 中使用 sudo dmidecode -t bios 查看
arg_smbios='-smbios type=0,vendor="martins3",version=12,date="2022-2-2", -smbios type=1,manufacturer="Martins3 Inc",product="Hacking Alpine",version=12,serial="1234-4567-abc"'
arg_hugetlb="default_hugepagesz=2M hugepagesz=1G hugepages=1 hugepagesz=2M hugepages=512"
arg_hugetlb="default_hugepagesz=2M"
arg_hugetlb=""
# 可选参数
# arg_mem_cpu="-m 12G  -smp $(($(getconf _NPROCESSORS_ONLN) - 1))"

arg_machine="-machine pc,accel=kvm,kernel-irqchip=on,smm=off"
# @todo 对于 smm 是否打开，热迁移没有检查，似乎这是一个 bug 吧
arg_mem_balloon="-device virtio-balloon,id=balloon0,deflate-on-oom=true,page-poison=true,free-page-reporting=false,free-page-hint=true,iothread=io1 -object iothread,id=io1"
arg_mem_balloon=""

arg_cpu_model="-cpu Skylake-Client-IBRS,hle=off,rtm=off"
# @todo 如果 see=off 或者 see2=off ，系统直接无法启动
# arg_cpu_model="-cpu Skylake-Client-IBRS,hle=off,rtm=off,sse4_2=off,sse4_1=off,ssse3=off,sep=off"
# arg_cpu_model="-cpu host"
arg_cpu_model="-cpu Skylake-Client-IBRS,vmx=on,hle=off,rtm=off"
arg_cpu_model="-cpu Broadwell-noTSX-IBRS,vmx=on,hle=off,rtm=off"
# arg_cpu_model="-cpu Denverton"
# arg_cpu_model="-cpu Broadwell-IBRS"
arg_cpu_model="-cpu host"

if [[ $in_guest == true ]]; then
	arg_cpu_model="$arg_cpu_model,vmx=off"
fi

case $hacking_memory in
	"none")
		ramsize=12G
		arg_mem_cpu="-smp $(($(getconf _NPROCESSORS_ONLN) - 1))"
		arg_mem_cpu="-smp 1"
		arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,id=pc.ram,size=$ramsize,prealloc=off,share=on -machine memory-backend=pc.ram -m $ramsize"
		;;
	"file")
		# 只有使用这种方式才会启动 async page fault
		ramsize=12G
		arg_mem_cpu="-smp $(($(getconf _NPROCESSORS_ONLN) - 1))"
		arg_mem_cpu="$arg_mem_cpu -object memory-backend-file,id=id0,size=$ramsize,mem-path=$workstation/qemu.ram -machine memory-backend=id0 -m $ramsize"
		;;
	"numa")
		# 通过 reserve = false 让 mmap 携带参数 MAP_NORESERVE，从而可以模拟超级大内存的 Guest
		arg_mem_cpu=" -m 8G -smp cpus=6"
		arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,size=2G,id=m0,reserve=false -numa node,memdev=m0,cpus=0-1,nodeid=0"
		arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,size=2G,id=m1 -numa node,memdev=m1,cpus=2-3,nodeid=1"
		arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,size=4G,id=m2 -numa node,memdev=m2,cpus=4,nodeid=2"
		arg_mem_cpu="$arg_mem_cpu -numa node,cpus=5,nodeid=3" # 只有 CPU ，但是没有内存
		;;
	"prealloc")
		arg_mem_cpu=" -m 1G -smp cpus=1"
		arg_mem_cpu="$arg_mem_cpu -object memory-backend-file,size=1G,prealloc=on,share=on,id=m2,mem-path=/dev/hugepages -numa node,memdev=m2,cpus=0,nodeid=0"
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
		arg_mem_cpu="$arg_mem_cpu -numa node,nodeid=0,cpus=0-1,nodeid=0,memdev=mem0 -numa node,nodeid=1,cpus=2-3,nodeid=1,memdev=mem1"
		arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,id=mem0,size=2G"
		arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,id=mem1,size=2G"
		arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,id=mem2,size=2G"
		arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,id=mem3,size=2G"
		arg_mem_cpu="$arg_mem_cpu -device virtio-mem-pci,id=vm0,memdev=mem2,node=0,requested-size=1G"
		arg_mem_cpu="$arg_mem_cpu -device virtio-mem-pci,id=vm1,memdev=mem3,node=1,requested-size=1G"

		arg_hugetlb="crashkernel=300M"
		;;

	"hotplug")
		arg_mem_cpu="-m 1G,slots=7,maxmem=8G"
		arg_hugetlb=""
		;;
	"virtio-pmem")
		# @todo 似乎这一行不能去掉
		# memory devices (e.g. for memory hotplug) are not enabled, please specify the maxmem option
		# 还有其他问题
		arg_mem_cpu="-m 1G,slots=7,maxmem=8G"
		pmem_img=${workstation}/virtio_pmem.img
		arg_hacking="${arg_hacking} -object memory-backend-file,id=nvmem1,share=on,mem-path=${pmem_img},size=4G"
		arg_hacking="${arg_hacking} -device virtio-pmem-pci,memdev=nvmem1,id=nv1"
		;;
	"sockets")
		arg_mem_cpu="-m 8G -smp 8,sockets=2,cores=2,threads=2,maxcpus=8"
		;;
esac

arg_bridge="-device pci-bridge,id=mybridge,chassis_nr=1"
# seabios=/home/martins3/core/seabios/out/bios.bin
if [[ -z ${seabios+x} ]]; then
	arg_seabios=""
else
	arg_seabios="-chardev file,path=/tmp/seabios.log,id=seabios -device isa-debugcon,iobase=0x402,chardev=seabios -bios ${seabios}"
fi

# @todo 不知道为什么现在使用 ovmf 界面没有办法正常刷新了
# 将 ovmf 的环境一起集成过来，让 gdb 也可以调试 ovmf
# 使用 ovmf 的时候，windows 也是无法启动
#
# 但是还可以正常使用的机器
# 当然，如果是 ubuntu ，问题更加严重，直接卡在哪里了
if [[ $use_ovmf == true ]]; then
	# sudo cp /run/libvirt/nix-ovmf/OVMF_VARS.fd /tmp/OVMF_VARS.fd
	# sudo chmod 666 /tmp/OVMF_VARS.fd
	ovmf_code=/run/libvirt/nix-ovmf/OVMF_CODE.fd
	ovmf_code=$workstation/OVMF.fd
	ovmf_var=/tmp/OVMF_VARS.fd
	arg_seabios="-drive file=$ovmf_code,if=pflash,format=qcow2,unit=0,readonly=on"
	arg_seabios="$arg_seabios -drive file=$ovmf_var,if=pflash,format=qcow2,unit=1"

	# arg_seabios="-bios $workstation/OVMF.fd"
fi

# arg_debug_memblock="memblock=debug"
arg_cgroupv2="systemd.unified_cgroup_hierarchy=1"
# scsi_mod.scsi_logging_level=0x3fffffff
# @todo 这个 function graph 为什么没有办法打印全部啊
arg_boot_trace="ftrace=function_graph ftrace_filter=arch_freq_get_on_cpu"
arg_kernel_args="root=$root nokaslr console=ttyS0,9600 earlyprink=serial $arg_boot_trace $arg_hugetlb $arg_cgroupv2 transparent_hugepage=always"
# @todo 可以看到，先会启动 initramfs 才会开始执行 /bin/bash 的
# arg_kernel_args="root=$root nokaslr console=ttyS0,9600 earlyprink=serial init=/bin/bash"
arg_kernel="--kernel ${kernel} -append \"${arg_kernel_args}\""

if [[ $hacking_migration == true ]]; then
	arg_nvme=""
else
	arg_nvme="-device nvme,drive=nvme1,serial=foo,bus=mybridge,addr=0x1 -drive file=${workstation}/img1,format=qcow2,if=none,id=nvme1"
	# @todo virtio-blk-pci vs virtio-blk-device ?
fi
arg_disk="-device virtio-blk-pci,drive=nvme2,iothread=io0 -drive file=${workstation}/img2,format=qcow2,if=none,id=nvme2 -object iothread,id=io0"
arg_disk="$arg_disk -device virtio-blk-pci,drive=d2 -drive file=${workstation}/img9,format=qcow2,if=none,id=d2"
arg_scsi="-device virtio-scsi-pci,id=scsi0,bus=pci.0,addr=0xa  -device scsi-hd,bus=scsi0.0,channel=0,scsi-id=0,lun=0,drive=scsi-drive -drive file=${workstation}/img3,format=qcow2,id=scsi-drive,if=none"

arg_sata="-drive file=${workstation}/img4,media=disk,format=qcow2"
arg_sata="$arg_sata -drive file=${workstation}/img5,media=disk,format=qcow2"
arg_sata="$arg_sata -drive file=${workstation}/img6,media=disk,format=qcow2"
arg_sata="$arg_sata -drive file=${workstation}/img7,media=disk,format=qcow2"

# arg_sata="-device scsi-hd,drive=jj,bootindex=10 -drive if=none,file=${workstation}/img4,format=qcow2,id=jj"

# @todo 尝试一下这个
# -netdev tap,id=nd0,ifname=tap0 -device e1000,netdev=nd0
# arg_network="-netdev user,id=net1,hostfwd=tcp::$guest_port-:22 -device e1000e,netdev=net1"
# arg_network="-netdev user,id=net1,$arg_fwd -device virtio-net-pci,netdev=net1,romfile=/home/martins3/core/zsh/README.md" # 替代 romfile
# @todo 做成一个计数器吧，自动增加访问的接口
arg_fwd="hostfwd=tcp::5556-:22"
if [[ $in_guest == false ]]; then
	arg_fwd="hostfwd=tcp::$guest_port-:22"     # @todo 最好是 guest 和 host 都是相同内容
	arg_fwd="$arg_fwd,hostfwd=tcp::5900-:5900" # 为了让 guest 中 vnc 穿透出来
fi
arg_network="-netdev user,id=net1,$arg_fwd -device virtio-net-pci,netdev=net1"

arg_qmp="-qmp tcp:localhost:$qmp_port,server,wait=off"
if [[ $qmp_shell == true ]]; then
	arg_qmp="-qmp unix:/tmp/qmp-sock,server,wait=off"
fi

mon_socket_path=/tmp/qemu-monitor-socket
serial_socket_path=/tmp/qemu-serial-socket
arg_monitor="-serial mon:stdio -display none"
# @todo 原来这个选项不打开，内核无法启动啊
# @todo 才意识到，这个打开之后，在 kernel cmdline 中的 init=/bin/bash 是无效的
# @todo 为什么配合 3.10 内核无法正常使用
arg_initrd="-initrd /home/martins3/hack/vm/initramfs-6.3.0-rc2.img"
# arg_initrd="-initrd /nix/store/kfaz0nv43qwyvj4s7c5ak4lgdyzdf51s-initrd/initrd" # nixos 的 initrd
# arg_initrd=""
tracepoint=()
arg_trace=""
# tracepoint+=(kvm_set_user_memory)
# tracepoint+=(memory_region_ops_read)
if [[ ${#tracepoint[@]} -gt 0 ]]; then
	echo "${#tracepoint[@]}"
	arg_trace="--trace"
	for i in "${tracepoint[@]}"; do
		arg_trace+=" '$i' "
	done
fi

arg_vfio=""
if [[ $hacking_vfio == true ]]; then
	# 直通一个 nvme 盘进去
	cat <<_EOF_
lspci -nn
# 03:00.0 Non-Volatile memory controller [0108]: Yangtze Memory Technologies Co.,Ltd Device [1e49:0071] (rev 01)
echo 0000:03:00.0 | sudo tee /sys/bus/pci/devices/0000:03:00.0/driver/unbind
echo 1e49 0071 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
sudo chown martins3 /dev/vfio/17
## 恢复方法
echo 0000:03:00.0 | sudo tee /sys/bus/pci/devices/0000:03:00.0/driver/unbind
echo 1e49 0071 | sudo tee /sys/bus/pci/drivers/nvme/new_id
_EOF_
	arg_vfio="-device vfio-pci,host=03:00.0"
fi

# -soundhw pcspk

show_help() {
	echo "------ 配置参数 ---------"
	echo "kernel=${kernel}"
	echo "qemu=${qemu}"
	echo "seabios=${seabios}"
	echo "-------------------------"
	echo ""
	echo "-h 展示本消息"
	echo "-s 调试内核，启动 QEMU 部分"
	echo "-k 调试内核，启动 gdb 部分"
	echo "-t 使用 tcg 作为执行引擎而不是 kvm"
	echo "-d 调试 QEMU"
	echo "   -m 调试 QEMU 的时候，打开 monitor"
	echo "   -c 调试 QEMU 的时候，打开 console"
	echo "-q 连接上 QEMU 的 qmp"
	echo "-a 表示作为热迁移的 target 端，此时需要将 hacking_migration 设置为 true"
	exit 0
}

while getopts "abcdhkmpqst" opt; do
	case $opt in
		a)
			# @todo 丑陋的代码，从原则上将，option 应该在最上面的才对，修改参数
			arg_qmp="-qmp tcp:localhost:5444,server,wait=off"
			arg_network="-netdev user,id=net1,hostfwd=tcp::5557-:22 -device e1000e,netdev=net1"
			arg_network="-netdev user,id=net1,hostfwd=tcp::5557-:22 -device virtio-net-pci,netdev=net1"
			# 需要保证迁移的两侧的 romfile 内容一致才可以
			# arg_network="$arg_network,romfile=/home/martins3/hack/vm/img1"
			arg_migration_target="-incoming tcp:0:4000"
			# @todo 测试发现，目标端是否为 smm 不影响迁移
			arg_machine="-machine pc,accel=kvm,kernel-irqchip=on,smm=on"
			# target 端是啥 cpu model 根本没影响啊
			arg_cpu_model="-cpu Broadwell-IBRS"
			;;
		b)
			# 可以带上虚拟机唯一标识，而不是使用 mem
			if [[ ! -d /sys/fs/cgroup/mem ]]; then
				sudo cgcreate -g memory:mem
				sudo cgset -r memory.max=10G mem
			fi
			cgroup_limit="sudo cgexec -g memory:mem"
			;;
		c)
			socat -,echo=0,icanon=0 unix-connect:$serial_socket_path
			exit 0
			;;
		d)
			debug_qemu='gdb -ex "handle SIGUSR1 nostop noprint" --args'
			# gdb 的时候，让 serial 输出从 unix domain socket 输出
			# https://unix.stackexchange.com/questions/426652/connect-to-running-qemu-instance-with-qemu-monitor
			arg_monitor="-serial unix:$serial_socket_path,server,nowait"
			arg_monitor="$arg_monitor -monitor unix:$mon_socket_path,server,nowait"
			arg_monitor="$arg_monitor -display none"
			cd "${qemu_dir}" || exit 1
			;;
		p) debug_qemu="perf record -F 1000" ;;
		s) debug_kernel="-S -s" ;;
		k) launch_gdb=true ;;
		t)
			arg_machine="--accel tcg,thread=single"
			arg_mem_cpu="-m 8G" # cpu 数量最好还是 1，内存需要指定一下，不然就
			arg_cpu_model=""    # cpu model 不能支持 host 了
			;;
		h) show_help ;;
		m)
			socat -,echo=0,icanon=0 unix-connect:$mon_socket_path
			exit 0
			;;
		q)

			if [[ $qmp_shell == true ]]; then
				qmp_shell=${qemu_dir}/scripts/qmp/qmp-shell
				$qmp_shell /tmp/qmp-sock
			else
				telnet localhost $qmp_port
			fi
			exit 0
			;;
		*) exit 0 ;;
	esac
done

sure() {
	read -r -p "$1? (y/n)" yn
	case $yn in
		[Yy]*) return ;;
		[Nn]*) exit ;;
		*) echo "Please answer yes or no." ;;
	esac
}

if [ ! -f "$iso" ] && [ ! -f $disk_img ]; then
	echo "please download ${distribution}"
	# wget http://mirrors.ustc.edu.cn/centos/8-stream/isos/x86_64/CentOS-Stream-8-x86_64-latest-boot.iso
	exit 0
fi

# 创建额外的 disk 用于测试 nvme 和 scsi 等
for ((i = 0; i < 10; i++)); do
	ext4_img="${workstation}/img$((i + 1))"
	if [ ! -f "$ext4_img" ]; then
		# raw 格式的太浪费内存了
		# 但是可以通过这种方法预设内容
		# mount -o loop /path/to/data /mnt
		# dd if=/dev/null of="${ext4_img}" bs=1M seek=1000
		# mkfs.ext4 -F "${ext4_img}"
		qemu-img create -f qcow2 "${ext4_img}" 100G
	fi
done

if [ ! -f "${disk_img}" ]; then
	sure "use ${iso} install ${disk_img}"
	qemu-img create -f qcow2 "${disk_img}" 100G
	# 很多发行版的安装必须使用图形界面，如果在远程，那么需要 vnc
	arg_monitor="-vnc :0,password=on -monitor stdio"
	arg_monitor=""
	qemu-system-x86_64 \
		-boot d \
		-cdrom "$iso" \
		-hda "${disk_img}" \
		-enable-kvm \
		-m 2G \
		-smp 2 $arg_monitor
	exit 0
fi

if [ $launch_gdb = true ]; then
	echo "debug kernel"
	cd "${kernel_dir}" || exit 1
	# gdb /home/martins3/kernel/openeuler-4.19.90-2112.8.0.0131-x86_64/vmlinux -ex "target remote :1234" -ex "hbreak start_kernel" -ex "continue"
	# 才意识到，cd 到不同的位置，最后展示出来的代码是不同的，vmlinux 中不存放源代码的
	gdb vmlinux -ex "target remote :1234" -ex "hbreak start_kernel" -ex "continue"
	exit 0
fi

# @todo 现在编译的 qemu 存在这个问题
# (qemu) qemu-system-x86_64: -vnc :0,password=on: Cipher backend does not support DES algorithm
if [[ $in_guest == true ]]; then
	arg_monitor="-vnc :0,password=on -monitor stdio"
	qemu="qemu-system-x86_64"
fi

if [[ ${minimal} == true ]]; then
	arg_monitor="-vnc :0 -monitor stdio"
	# arg_monitor="-nographic"
	# arg_monitor="-monitor stdio"
	# arg_monitor="-serial mon:stdio -display none"
	${qemu} \
		$arg_cpu_model \
		$arg_img \
		-enable-kvm \
		-m 2G \
		-smp 2 $arg_monitor
	# @todo 不知道为什么 guest 中不能携带 arg_network 了
	exit 0
fi

if [[ ${replace_kernel} == false ]]; then
	arg_kernel=""
	arg_initrd=""
	# 如果不修改如下选项 ，那么 grub 是不是没有办法选择
	arg_monitor="-monitor stdio"
fi

# @todo 将这个图形在终端中更加清晰的输出出来
cmd="${cgroup_limit} ${debug_qemu} ${qemu} ${arg_trace} ${debug_kernel} ${arg_img} ${arg_mem_cpu}  \
  ${arg_kernel} ${arg_seabios} ${arg_bridge} ${arg_network} \
  ${arg_machine} ${arg_monitor} ${arg_initrd} ${arg_mem_balloon} ${arg_hacking} \
  ${arg_qmp} ${arg_vfio} ${arg_smbios} ${arg_migration_target} ${arg_share_dir} ${arg_sata} ${arg_scsi} ${arg_nvme} ${arg_disk} ${arg_pdifile} \
  ${arg_cpu_model}"
echo "$cmd"
eval "$cmd"
