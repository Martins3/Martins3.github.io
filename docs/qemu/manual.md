# QEMU 使用入门

## 如何编译 QEMU
编译 x86 32bit 的方法
```sh
cd path_to_qemu || exit 1
mkdir 32bit
cd 32bit || exit 1
../configure --target-list=i386-softmmu
make -j
```

## 给一个硬盘安装操作系统

## 到底是 sdb3 ，其中的基本原理是什么
- 为什么不能作为启动盘啊
- 将之前的几个 ubuntu imge 之类重新整理一下

## 所以 virtue box 是如何实现的

## 安装 Windows

## img 制作方法

### [ ] hello world
```c
  /*
   - use hello.out
      - "console=ttyS0 earlyprintk=serial debug root=/dev/ram rdinit=/hello.out"
   - use busybox
      - "console=ttyS0 earlyprintk=serial debug";
  */
```

### [ ] busybox

### Alpine
所以我写了一个[脚本](https://github.com/Martins3/Martins3.github.io/blob/master/docs/qemu/sh/alpine.sh), 简单解释几个点:
1. 其中采用 alpine 作为镜像，因为 alpine 是 Docker 选择的轻量级镜像，比 Yocto 功能齐全(包管理器)，而且比 Ubuntu 简单
2. 第一步使用 iso 来安装镜像，这次运行的是 iso 中是包含了一个默认内核, 安装镜像之后，使用 -kernel 指定内核
3. 在 [How to use custom image kernel for ubuntu in qemu?](https://stackoverflow.com/questions/65951475/how-to-use-custom-image-kernel-for-ubuntu-in-qemu) 的这个问题中，我回答了如何设置内核参数 sda

其中几乎所有的操作使用脚本，除了镜像的安装需要手动操作
1. 使用 root 登录
2. 执行 setup-alpine 来进行安装, 所有的都是默认的, 一路 enter (包括密码，之后登录直接 enter) ，除了下面的两个
    - 选择 image 的时候让其自动选择最快的，一般是清华的
    - 安装磁盘选择创建的

> 默认 root 登录
![](./img/x-2.png)

> 选择 f 也即是自动选择最快的
![](./img/x-1.png)

> 将系统安装到脚本制作的 image 中
![](./img/x-3.png)

构建好了之后，就可以像是调试普通进程一样调试内核了，非常地好用。

```sh
apk add pciutils
```

### [ ] Ubuntu Server

### [ ] Ubuntu Desktop

## communicate with the guest

- [ ] TODO verify this one by one

I came across the same problem and fix it after struggling, here is my solution.

1. create the qcow2 image from iso[^1]
```sh
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
```sh
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



### [ ] NixOS

## [ ] 增加一个网络设备
https://www.collabora.com/news-and-blog/blog/2017/03/13/kernel-debugging-with-qemu-overview-tools-available/

似乎这个配置是无法启动网络的，但是 Linux-kernel-labs 中可以，也许是 `create_net.sh` 的原因
```sh
#!/bin/bash
# FIXME 也许是 dhcp 没有配置，所以没有办法正常链接网络
QEMU=/home/maritns3/core/kvmqemu/build/qemu-system-x86_64
$QEMU -kernel /home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage \
  -enable-kvm \
  -drive file=/home/maritns3/core/vn/hack/qemu/mini-img/core-image-minimal-qemux86-64.ext4,if=virtio,format=raw \
  -device virtio-serial-pci -chardev pty,id=virtiocon0 -device virtconsole,chardev=virtiocon0 \
  --append "root=/dev/vda loglevel=15 console=ttyS0" \
  -device e1000,netdev=net0 \
  -netdev user,id=net0,hostfwd=tcp::8080-:80 \
  -monitor stdio
```

- [ ] 使用 alpine 直接就可以 ssh 到远程，很神奇

- 似乎增加一个网络设备并不是很难，难的是如何正确配置
network 的事情参考这个部分：
https://www.digi.com/resources/documentation/digidocs/90001515/task/yocto/t_configure_network.htm
基础的部分参考这个：
https://www.cnblogs.com/wipan/p/9272255.html
https://gist.github.com/chrisdone/02e165a0004be33734ac2334f215380e
https://www.digi.com/resources/documentation/digidocs/90001515/task/yocto/t_configure_network.htm

## 直通一个设备

## 共享目录

- Through `qemu-nbd` to write/read guest's file
- Through libguestfs to mount/unmount qcow2 image
  - `sudo apt-get install libguestfs-tools`
  - `sudo yum install libguestfs-tools`
    - not wotk on Loongson because of the lack of supermin package
    - works fine on Ubuntu 16.04 AMD64
- Through interrnet
  - On default, the guest is connected to the interrnet and is able to `git clone` ...
  - But the guest is inside another Internal network and can not be reached by the host directly, such as
    - Guest IP : 10.0.2.15
    - Host IP: 192.168.1.103
- Through `-net` to make the guest connect to the host
  - may need the `tun` kernel module
    - `sudo modprobe tun`

- With 9p and virtio:
  - https://askubuntu.com/questions/290668/how-to-share-a-folder-between-kvm-host-and-guest-using-virt-manager/1274315#1274315

### [ ] virtiofs

### [ ] 9p

### [ ] ssh

### [ ] ftp

## [ ] 让人迷惑的东西

- [ ] 从 Linux-Kernel-Labs.sh 中抽出来的，莫名其妙的，其他的内容也全部整合一下
```sh
-device virtio-serial-pci -chardev pty,id=virtiocon0 -device virtconsole,chardev=virtiocon0 \
```

## PCI bridge : 构建更加复杂的 topo 结构

首先运行 `lspci`，观察到 00:03.0 是一个 PCI bridge
```plain
00:00.0 Host bridge: Intel Corporation 440FX - 82441FX PMC [Natoma] (rev 02)
00:01.0 ISA bridge: Intel Corporation 82371SB PIIX3 ISA [Natoma/Triton II]
00:01.1 IDE interface: Intel Corporation 82371SB PIIX3 IDE [Natoma/Triton II]
00:01.3 Bridge: Intel Corporation 82371AB/EB/MB PIIX4 ACPI (rev 03)
00:02.0 VGA compatible controller: Device 1234:1111 (rev 02)
00:03.0 PCI bridge: Red Hat, Inc. QEMU PCI-PCI bridge
00:04.0 SCSI storage controller: Red Hat, Inc. Virtio block device
00:05.0 Ethernet controller: Intel Corporation 82574L Gigabit Network Connection
00:06.0 Unclassified device [0002]: Red Hat, Inc. Virtio filesystem
01:01.0 Non-Volatile memory controller: Red Hat, Inc. QEMU NVM Express Controller (rev 02)
```

然后运行 `ls -l /sys/bus/pci/devices`，可以观察到 01:01.0 是挂载到 00:03.0 上的

```txt
lrwxrwxrwx    1 root     root             0 May 16 08:50 0000:00:00.0 -> ../../../devices/pci0000:00/0000:00:00.0
lrwxrwxrwx    1 root     root             0 May 16 08:50 0000:00:01.0 -> ../../../devices/pci0000:00/0000:00:01.0
lrwxrwxrwx    1 root     root             0 May 16 08:50 0000:00:01.1 -> ../../../devices/pci0000:00/0000:00:01.1
lrwxrwxrwx    1 root     root             0 May 16 08:50 0000:00:01.3 -> ../../../devices/pci0000:00/0000:00:01.3
lrwxrwxrwx    1 root     root             0 May 16 08:50 0000:00:02.0 -> ../../../devices/pci0000:00/0000:00:02.0
lrwxrwxrwx    1 root     root             0 May 16 08:50 0000:00:03.0 -> ../../../devices/pci0000:00/0000:00:03.0
lrwxrwxrwx    1 root     root             0 May 16 08:50 0000:00:04.0 -> ../../../devices/pci0000:00/0000:00:04.0
lrwxrwxrwx    1 root     root             0 May 16 08:50 0000:00:05.0 -> ../../../devices/pci0000:00/0000:00:05.0
lrwxrwxrwx    1 root     root             0 May 16 08:50 0000:00:06.0 -> ../../../devices/pci0000:00/0000:00:06.0
lrwxrwxrwx    1 root     root             0 May 16 08:50 0000:01:01.0 -> ../../../devices/pci0000:00/0000:00:03.0/0000:01:01.0
```
