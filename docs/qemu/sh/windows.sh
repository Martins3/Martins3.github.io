#!/usr/bin/env bash

set -eux
abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json

qemu_dir=$(jq -r ".qemu_dir" <"$configuration")
workstation=$(jq -r ".workstation" <"$configuration")
version=8
if [[ $version == "8" ]]; then
  iso="$workstation/Win8.1_Chinese(Simplified)_x64.iso"
  img="$workstation/windows8.img"
else
  iso="$workstation/Win10_22H2_Chinese_Simplified_x64.iso"
  img="$workstation/windows10.img"
fi

if [[ ! -f ${iso} ]] && [[ ! -f ${img} ]]; then
  echo "Download the ISO from
https://www.microsoft.com/en-us/software-download/windows10ISO
and rename it as $iso"
fi

# arg_vnc="-vnc :0,password=on"
arg_monitor="-monitor stdio"
if [ ! -f "$img" ]; then
  qemu-img create -f qcow2 "$img" 100G
  qemu-system-x86_64 -cdrom "$iso" -hda "$img" -m 8G -smp 8 -cpu host -machine type=q35,accel=kvm $arg_monitor
  exit 0
fi

# "$QEMU" -hda "${img}" -enable-kvm -m 8G -smp 8 -vga virtio -soundhw

qemu=${qemu_dir}/build/x86_64-softmmu/qemu-system-x86_64
arg_mem_balloon="-device virtio-balloon-pci,id=balloon0,deflate-on-oom=true"
arg_qmp="-qmp tcp:localhost:4444,server,wait=off"
arg_img="-drive aio=native,cache.direct=on,file=${img},format=qcow2,media=disk,index=0"
# https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/archive-virtio/virtio-win-0.1.208-1/
arg_virtio="-drive aio=native,cache.direct=on,file=$workstation/virtio-win-0.1.208.iso,media=cdrom,index=2"
# https://fedorapeople.org/groups/virt/virtio-win/direct-downloads/archive-virtio/virtio-win-0.1.208-1/virtio-win-0.1.208.iso
"$qemu" $arg_img -m 16G -smp 24 -machine type=q35,accel=kvm -cpu host $arg_monitor $arg_mem_balloon $arg_qmp $arg_virtio -vga std -display gtk
# "$QEMU" -drive file=/dev/nvme0n1p2,format=raw -drive file=/dev/nvme1n1p1,format=raw,readonly=on -m 8G -smp 8 -device vfio-pci,host=01:00.0 -machine type=q35,accel=kvm -soundhw hda
