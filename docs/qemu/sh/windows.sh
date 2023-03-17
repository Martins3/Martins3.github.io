#!/usr/bin/env bash

set -eux
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

qemu_dir=$(jq -r ".qemu_dir" <"$configuration")
workstation=$(jq -r ".workstation" <"$configuration")
version=10
arg_win11=""
case $version in
8)
  iso="$workstation/Win8.1_Chinese(Simplified)_x64.iso"
  img="$workstation/windows8.img"
  ;;
10)
  iso="$workstation/Win10_22H2_Chinese_Simplified_x64.iso"
  img="$workstation/windows10.img"
  ;;
11)
  iso="$workstation/Win11_22H2_Chinese_Simplified_x64v1.iso"
  img="$workstation/windows11.img"
  arg_win11="-chardev socket,id=chrtpm,path=/tmp/emulated_tpm/swtpm-sock -tpmdev emulator,id=tpm0,chardev=chrtpm -device tpm-tis,tpmdev=tpm0"
  # 关于 secure boot : https://wiki.debian.org/SecureBoot/VirtualMachine
  # swtpm socket --tpmstate dir=/tmp/emulated_tpm --ctrl type=unixio,path=/tmp/emulated_tpm/swtpm-sock --log level=20 --tpm2
  # nixos 的不行，直接下载一个就可以了: http://mirror.centos.org/centos/8-stream/AppStream/x86_64/os/Packages/
  # arg_win11="$arg_win11 -machine q35,smm=on"
  arg_win11="$arg_win11 -global driver=cfi.pflash01,property=secure,value=on -drive if=pflash,format=raw,unit=0,file=/home/martins3/hack/vm/ovmf/usr/share/edk2/ovmf/OVMF_CODE.secboot.fd,readonly=on -drive if=pflash,format=raw,unit=1,file=/home/martins3/hack/vm/ovmf/usr/share/edk2/ovmf/OVMF_VARS.secboot.fd"
  ;;
esac

arg_img="-drive aio=native,cache.direct=on,file=${img},format=qcow2,media=disk,index=0"

arg_vfio=""
if [[ $version == 10 ]]; then
  img=/home/martins3/hack/mnt/windows10.img.cur
  # @todo 修改点
  arg_img="-drive aio=native,cache.direct=on,file=/home/martins3/hack/mnt/windows10.img.cur,format=qcow2,media=disk,index=0"
  # arg_img="-drive aio=native,cache.direct=on,file=/dev/nvme1n1,format=raw,media=disk,index=3"
  # 发现了一个事情，当安装的操作系统的时候，需要先使用直通，之后可以整个 disk 访问的
  # 这个是可以复现的
  #
  # 使用 disk 是需要将 participation 的信息清理一下
  # sudo dd if=/dev/zero of=/dev/nvme1n1  bs=512  count=1

  # @todo 修改点
  # use_vfio=false
  # if [[ $use_vfio == true ]]; then
  #   arg_vfio="-device vfio-pci,host=03:00.0"
  #   arg_img=""
  # else
  #   arg_vfio=""
  #   arg_img="-drive file=/dev/nvme1n1,format=raw"
  # fi
fi

if [[ $version == 8 ]]; then
  img=/home/martins3/hack/mnt/windows8.img
fi

if [[ ! -f ${iso} ]] && [[ ! -f ${img} ]]; then
  echo "Download the ISO from
https://www.microsoft.com/en-us/software-download/windows10ISO
and rename it as $iso"
fi

# arg_vnc="-vnc :0,password=on"
arg_monitor="-monitor stdio"

# @todo 修改点
# qemu-system-x86_64 $arg_vfio -cdrom "$iso" $arg_img -m 16G -smp 16 -cpu host --enable-kvm $arg_monitor $arg_win11
# exit 0

# if [ ! -f "$img" ]; then
#   qemu-img create -f qcow2 "$img" 200G
#   qemu-system-x86_64 -cdrom "$iso" $arg_img -m 16G -smp 16 -cpu host --enable-kvm $arg_monitor $arg_win11
#   exit 0
# fi

cat <<'_EOF_'
# @todo add this to systemd scripts
# echo 0000:01:00.0 | sudo tee /sys/bus/pci/devices/0000:01:00.0/driver/unbind
echo 10de 1c02 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
echo 0000:01:00.1 | sudo tee /sys/bus/pci/devices/0000:01:00.1/driver/unbind
echo 10de 10f1 | sudo tee /sys/bus/pci/drivers/vfio-pci/new_id
sudo chown martins3 /dev/vfio/15
_EOF_

# arg_vfio="$arg_vfio -device vfio-pci,host=01:00.0 -device vfio-pci,host=01:00.1"
# https://gist.github.com/ichisadashioko/cfc6446764516bf7eccaffdb3799f041
# arg_usb="-usb -device usb-host,bus=usb-bus.0,hostbus=1,hostport=1"
arg_usb=""

# "$QEMU" -hda "${img}" -enable-kvm -m 8G -smp 8 -vga virtio -soundhw
arg_cpu="-cpu host,-hypervisor,+kvm_pv_unhalt,+kvm_pv_eoi,hv_spinlocks=0x1fff,hv_vapic,hv_time,hv_reset,hv_vpindex,hv_runtime,hv_relaxed,kvm=off,hv_vendor_id=intel"
# arg_qemu_mon="-vga virtio -display gtk,gl=on" # 打开 gl=on 只是显示 1/4，比较怀疑是整体缩放之后导致的
arg_qemu_mon="-vga virtio -display gtk"

qemu="qemu-system-x86_64"
arg_mem_balloon="-device virtio-balloon-pci,id=balloon0,deflate-on-oom=true"

# @todo 不知道为什么，windows 启动之后，所有的内存都会被踩一遍
ramsize=12G
arg_mem="-object memory-backend-ram,id=pc.ram,size=$ramsize,prealloc=off,share=off -machine memory-backend=pc.ram -m $ramsize "

arg_qmp="-qmp tcp:localhost:4445,server,wait=off"

# https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/archive-virtio/virtio-win-0.1.208-1/
arg_virtio="-drive aio=native,cache.direct=on,file=$HOME/hack/iso/virtio-win-0.1.208.iso,media=cdrom,index=2"
# https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/archive-virtio/virtio-win-0.1.208-1/virtio-win-0.1.208.iso
"$qemu" $arg_img $arg_mem -smp $(($(getconf _NPROCESSORS_ONLN) - 1)) $arg_qemu_mon --enable-kvm $arg_cpu $arg_monitor $arg_mem_balloon $arg_qmp $arg_virtio $arg_win11 $arg_vfio $arg_usb
# "$QEMU" -drive file=/dev/nvme0n1p2,format=raw -drive file=/dev/nvme1n1p1,format=raw,readonly=on -m 8G -smp 8 -device vfio-pci,host=01:00.0 -machine type=q35,accel=kvm
