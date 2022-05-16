# QEMU 使用入门


## 给一个硬盘安装操作系统

## 到底是 sdb3 ，其中的基本原理是什么
- 为什么不能作为启动盘啊
- 将之前的几个 ubuntu imge 之类重新整理一下

## 所以 virtue box 是如何实现的

## 安装 Windows

## img 制作方法

### [ ] Hello World

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

### [ ] Ubuntu Server

### [ ] Ubuntu Desktop

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

## 直通一个设备

## 共享目录

### [ ] virtiofs

### [ ] 9p

### [ ] ssh

### [ ] ftp

## [ ] 让人迷惑的东西

- [ ] 从 Linux-Kernel-Labs.sh 中抽出来的，莫名其妙的，其他的内容也全部整合一下
```sh
-device virtio-serial-pci -chardev pty,id=virtiocon0 -device virtconsole,chardev=virtiocon0 \
```
