#!/usr/bin/env bash
set -e

# @todo 用 https://github.com/charmbracelet/gum 来重写这个项目
use_nvme_as_root=false # @todo nvme 的这个事情走通一下
replace_kernel=true

hacking_memory="hotplug"
hacking_memory="virtio-pmem"
hacking_memory="none"
hacking_memory="virtio-mem"
hacking_memory="prealloc"
hacking_memory="sockets"
hacking_memory="numa"
hacking_memory="none"

hacking_migration=false
# @todo 尝试在 guest 中搭建一个 vIOMMU
#
if [[ $hacking_migration = true ]]; then
  use_nvme_as_root=false
fi

use_ovmf=false

abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

# ----------------------- 配置区 -----------------------------------------------
kernel_dir=$(jq -r ".kernel_dir" <"$configuration")
qemu_dir=$(jq -r ".qemu_dir" <"$configuration")
workstation="$(jq -r ".workstation" <"$configuration")"
# ------------------------------------------------------------------------------
qemu=${qemu_dir}/build/x86_64-softmmu/qemu-system-x86_64
kernel=${kernel_dir}/arch/x86/boot/bzImage

distribution=openEuler-22.09-x86_64-dvd
# distribution=openEuler-20.03-LTS-SP3-x86_64-dvd
# distribution=CentOS-7-x86_64-DVD-2207-02
# distribution=ubuntu-22.04.1-live-server-amd64

guest_port=5556
qmp_port=4444
case $distribution in
openEuler-22.09-x86_64-dvd)
  use_ovmf=false;
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

iso=${workstation}/${distribution}.iso
disk_img=${workstation}/${distribution}.qcow2

debug_qemu=
debug_kernel=
launch_gdb=false
arg_migration_target=

arg_hacking=""
arg_img="-drive aio=io_uring,file=${disk_img},format=qcow2,if=virtio"
arg_img="-drive aio=native,cache.direct=on,file=${disk_img},format=qcow2,if=virtio"
root=/dev/vdb2

arg_share_dir="-virtfs local,path=$(pwd),mount_tag=host0,security_model=mapped,id=host0"

if [[ $use_nvme_as_root = true ]]; then
  # @todo 这个应该只是缺少 bootindex 吧？
  arg_img="-device nvme,drive=nvme3,serial=foo -drive file=${disk_img},format=qcow2,if=none,id=nvme3"
  root=/dev/nvme1n1
fi

# 在 guset 中使用 sudo dmidecode -t bios 查看
arg_smbios='-smbios type=0,vendor="martins3",version=12,date="2022-2-2", -smbios type=1,manufacturer="Martins3 Inc",product="Hacking Alpine",version=12,serial="1234-4567-abc"'
arg_hugetlb="default_hugepagesz=2M hugepagesz=1G hugepages=1 hugepagesz=2M hugepages=512"
arg_hugetlb="default_hugepagesz=2M"
arg_hugetlb=""
# 可选参数
arg_mem_cpu="-m 12G -cpu host -smp $(($(getconf _NPROCESSORS_ONLN) - 1))"
arg_machine="-machine pc,accel=kvm,kernel-irqchip=on"
arg_mem_balloon="-device virtio-balloon,id=balloon0,deflate-on-oom=true,page-poison=true,free-page-reporting=false,free-page-hint=true,iothread=io1 -object iothread,id=io1"
arg_mem_balloon=""

case $hacking_memory in
"none")
  ;;
"numa")
  # 通过 reserve = false 让 mmap 携带参数 MAP_NORESERVE，从而可以模拟超级大内存的 Guest
  arg_mem_cpu="-cpu host -m 8G -smp cpus=6"
  arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,size=2G,id=m0,reserve=false -numa node,memdev=m0,cpus=0-1,nodeid=0"
  arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,size=2G,id=m1 -numa node,memdev=m1,cpus=2-3,nodeid=1"
  arg_mem_cpu="$arg_mem_cpu -object memory-backend-ram,size=4G,id=m2 -numa node,memdev=m2,cpus=4,nodeid=2"
  arg_mem_cpu="$arg_mem_cpu -numa node,cpus=5,nodeid=3" # 只有 CPU ，但是没有内存
  ;;
"prealloc")
  arg_mem_cpu="-cpu host -m 1G -smp cpus=1"
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
  arg_seabios="-drive file=$ovmf_code,if=pflash,format=raw,unit=0,readonly=on"
  arg_seabios="$arg_seabios -drive file=$ovmf_var,if=pflash,format=raw,unit=1"

  # arg_seabios="-bios $workstation/OVMF.fd"
fi

arg_cgroupv2="systemd.unified_cgroup_hierarchy=1"
# scsi_mod.scsi_logging_level=0x3fffffff
arg_kernel_args="root=$root nokaslr console=ttyS0,9600 earlyprink=serial $arg_hugetlb $arg_cgroupv2 transparent_hugepage=always"
arg_kernel="--kernel ${kernel} -append \"${arg_kernel_args}\""

if [[ $hacking_migration = true ]]; then
  arg_nvme=""
else
  arg_nvme="-device nvme,drive=nvme1,serial=foo,bus=mybridge,addr=0x1 -drive file=${workstation}/img1,format=raw,if=none,id=nvme1"
  # @todo virtio-blk-pci vs virtio-blk-device ?
fi
arg_disk="-device virtio-blk-pci,drive=nvme2,iothread=io0 -drive file=${workstation}/img2,format=raw,if=none,id=nvme2 -object iothread,id=io0"
arg_scsi="-device virtio-scsi-pci,id=scsi0,bus=pci.0,addr=0xa -device scsi-hd,bus=scsi0.0,channel=0,scsi-id=0,lun=0,drive=scsi-drive -drive file=${workstation}/img3,format=raw,id=scsi-drive,if=none"
arg_sata="-drive file=${workstation}/img4,media=disk,format=raw"
arg_sata="$arg_sata -drive file=${workstation}/img5,media=disk,format=raw"

# @todo 尝试一下这个
# -netdev tap,id=nd0,ifname=tap0 -device e1000,netdev=nd0
# @todo 做成一个计数器吧，自动增加访问的接口
arg_network="-netdev user,id=net1,hostfwd=tcp::$guest_port-:22 -device e1000e,netdev=net1"
arg_network="-netdev user,id=net1,hostfwd=tcp::$guest_port-:22 -device virtio-net-pci,netdev=net1,romfile=/home/martins3/core/zsh/README.md"
arg_qmp="-qmp tcp:localhost:$qmp_port,server,wait=off"

mon_socket_path=/tmp/qemu-monitor-socket
serial_socket_path=/tmp/qemu-serial-socket
arg_monitor="-serial mon:stdio -display none"
arg_initrd="-initrd /home/martins3/hack/vm/initramfs-6.1.0-rc7-00200-gc2bf05db6c78-dirty.img"
# arg_initrd=""
arg_trace="--trace 'memory_region_ops_read'" # 打开这个选项，输出内容很多
arg_trace=""

arg_vfio="-device vfio-pci,host=02:00.0" # 将音频设备直通到 Guest 中
arg_vfio=""
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
  echo "-a 表示作为热迁移的 target 端"
  exit 0
}

while getopts "adskthpcmqr" opt; do
  case $opt in
  d)
    debug_qemu="gdb -ex \"handle SIGUSR1 nostop noprint\" --args"
    # gdb 的时候，让 serial 输出从 unix domain socket 输出
    # https://unix.stackexchange.com/questions/426652/connect-to-running-qemu-instance-with-qemu-monitor
    arg_monitor="-serial unix:$serial_socket_path,server,nowait -display none"
    cd "${qemu_dir}" || exit 1
    ;;
  r)
    # @todo 不知道为什么，-serial stdio:monitor 的时候会失败
    arg_monitor="-serial stdio -display none"
    arg_monitor="$arg_monitor -monitor unix:$mon_socket_path,server,nowait"
    ;;
  p) debug_qemu="perf record -F 1000" ;;
  s) debug_kernel="-S -s" ;;
  k) launch_gdb=true ;;
  t) arg_machine="--accel tcg,thread=single" arg_mem_cpu="" ;;
  h) show_help ;;
  c)
    socat -,echo=0,icanon=0 unix-connect:$serial_socket_path
    exit 0
    ;;
  m)
    socat -,echo=0,icanon=0 unix-connect:$mon_socket_path
    exit 0
    ;;
  q)
    telnet localhost $qmp_port
    exit 0
    ;;
  a)
    # @todo 丑陋的代码，从原则上将，option 应该在最上面的才对，修改参数
    arg_qmp="-qmp tcp:localhost:5444,server,wait=off"
    arg_network="-netdev user,id=net1,hostfwd=tcp::5557-:22 -device e1000e,netdev=net1"
    arg_network="-netdev user,id=net1,hostfwd=tcp::5557-:22 -device virtio-net-pci,netdev=net1,romfile=/home/martins3/hack/vm/img1"
    arg_migration_target="-incoming tcp:0:4000"
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

# 创建额外的两个 disk 用于测试 nvme 和 scsi 等
# mount -o loop /path/to/data /mnt
for ((i = 0; i < 5; i++)); do
  ext4_img="${workstation}/img$((i + 1))"
  if [ ! -f "$ext4_img" ]; then
    dd if=/dev/null of="${ext4_img}" bs=1M seek=1000
    mkfs.ext4 -F "${ext4_img}"
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
    -cpu host \
    -hda "${disk_img}" \
    -enable-kvm \
    -m 2G \
    -smp 2 $arg_monitor
  exit 0
fi

if [ $launch_gdb = true ]; then
  echo "debug kernel"
  cd "${kernel_dir}" || exit 1
  gdb vmlinux -ex "target remote :1234" -ex "hbreak start_kernel" -ex "continue"
  exit 0
fi

if [[ ${minimal} = true ]]; then
  arg_monitor="-vnc :0,password=on -monitor stdio"
  # arg_monitor="-nographic"
  arg_monitor="-monitor stdio"
  ${qemu} \
    -cpu SandyBridge,vmx=on \
    $arg_img \
    -enable-kvm \
    -m 2G \
    -smp 2 $arg_monitor $arg_network
  exit 0
fi

if [[ ${replace_kernel} == false ]]; then
  arg_kernel=""
  arg_initrd=""
  arg_monitor="-monitor stdio"

  # @todo 不知道为什么需要将无关的 storage 设备都去掉，才可以正确启动
  # @todo lsblk 的为什么还有一个 sda 和 sr0 啊？
  arg_sata=""
  arg_scsi=""
  arg_nvme=""
  arg_disk=""
fi

# @todo 将这个图形在终端中更加清晰的输出出来
cmd="${debug_qemu} ${qemu} ${arg_trace} ${debug_kernel} ${arg_img} ${arg_mem_cpu}  \
  ${arg_kernel} ${arg_seabios} ${arg_bridge} ${arg_network} \
  ${arg_machine} ${arg_monitor} ${arg_initrd} ${arg_mem_balloon} ${arg_hacking} \
  ${arg_qmp} ${arg_vfio} ${arg_smbios} ${arg_migration_target} ${arg_share_dir} ${arg_sata} ${arg_scsi} ${arg_nvme} ${arg_disk} "
echo "$cmd"
eval "$cmd"
