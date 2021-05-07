I came across the same problem and fix it after struggling, here is my solution.

1. create the qcow2 image from iso[^1]
```bash
disk_img=ubuntu.qcow2
iso_name=ubuntu-21.04-desktop-amd64.iso

qemu-img create -f qcow2 "$disk_img" 1T

qemu-system-x86_64 \
		-cdrom "$iso" \
		-drive "file=${disk_img},format=qcow2" \
		-enable-kvm \
		-m 2G \
		-smp 2 \
		;
```
Then, you install ubuntu and reboot.

2. Run the Ubuntu with built-in kernel.
```bash
qemu-system-x86_64 \
  -drive "file=${disk_img},format=qcow2" \
  -enable-kvm \
  -m 8G \
  -smp 8
```
It should works well, open a terminal and use `df -h` to find out from which drive your ubuntu boot.
On my computer, it's "/dev/sda3".

3. compile the kernel
```bash
cd /kernel/src/path
git reset --hard origin/master
make defconfig
make -j4
```

4. Run the Ubuntu with the newly compiled kernel with hard drive specified
```bash
qemu-system-x86_64 \
  -hda ${disk_img} \
  -enable-kvm \
  -append "root=/dev/sda3" \
  -kernel /kernel/src/path/arch/x86/boot/bzImage \
  -cpu host \
  -m 8G \
  -smp 8
```

The key point is to inform the kernel 'root=/dev/sda3', as can be obtained step 2.

[^1]: https://github.com/cirosantilli/linux-cheat/blob/4c8ee243e0121f9bbd37f0ab85294d74fb6f3aec/ubuntu-18.04.1-desktop-amd64.sh

