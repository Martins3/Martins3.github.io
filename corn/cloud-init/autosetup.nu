# https://ubuntu.com/server/docs/install/autoinstall-quickstart
let image_dir = $"($env.HOME)/hack/image"
let iso_dir = $"($env.HOME)/hack/iso"
let iso_mnt_dir = $"($iso_dir)/mnt"
let config_dir = $env.FILE_PWD

let qemu_dir = "/home/martins3/core/qemu"
let qemu = $"($qemu_dir)/build/qemu-system-x86_64"

let kernel_dir = "/home/martins3/core/kernel"
let kernel_bzimage = $"($kernel_dir)/arch/x86/boot/bzImage"

def setup [ ] {
  mkdir $image_dir
  mkdir $iso_dir
  mkdir $iso_mnt_dir
}

def get_iso [vm:string] {
  let vm_config = $"($config_dir)/($vm).yaml"
  print $vm_config
  let iso_name = (open $vm_config | transpose key value | where key == iso | get value).0
  let iso = ($iso_dir | path join $iso_name)
  return $iso
}

def mnt_ios [vm:string] {
  let iso = (get_iso vm)
  sudo mount -r $iso  $iso_mnt_dir
}

def cloud_init [] {
  let cloud_init_dir = "/tmp/corn"
  mkdir $cloud_init_dir
  let config = '
  #cloud-config
  autoinstall:
    version: 1
    identity:
      hostname: ubuntu-server
      password: "$6$exDY1mhS4KUYCE/2$zmn9ToZwTKLhCw.b4/b.ZRTIZM30JZ4QrOQ2aOXJ8yk96xpcCof0kxKwuX1kqLG/ygbJ1f8wxED22bTL4F46P0"
      username: ubuntu'
  # cd $cloud_init_dir
  echo $config | save --raw -f  $"($cloud_init_dir)/user-data"
  touch  $"($cloud_init_dir)/meta-data"
  cd $cloud_init_dir
  if (port 3003) == 3003 {
    zellij run -- python3 -m http.server 3003
  }
}

def iso_install [vm:string] {
  let img_name = $"($image_dir)/($vm).qcow2"
  qemu-img create -f qcow2 $img_name 100G
  let iso = (get_iso $vm)
  (run-external $qemu
  "-no-reboot" "-m" "2048" "-drive" $"file=($img_name),format=qcow2,cache=none,if=virtio"
  "-cdrom" $"($iso)"
  "-kernel" $"($iso_mnt_dir)/casper/vmlinuz"
  "-initrd" $"($iso_mnt_dir)/casper/initrd"
  "-append" "'autoinstall ds=nocloud-net;s=http://_gateway:3003/'"
)
}

def run [vm:string] {
  let img_name = $"($image_dir)/($vm).qcow2"
  (run-external $qemu "-no-reboot" "-m" "2048" "-drive" $"file=($img_name),format=qcow2,cache=none,if=virtio")
}

# setup
# create ubuntu2204
# prepare
# cloud_init
# print $qemu
# iso_install ubuntu2204
run ubuntu2204
