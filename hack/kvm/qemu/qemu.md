# Qemu

- [ ] 需要使用 qemu 测试的内容:
  - kdump
  - qboot

## env
一般的编译方法
```
mkdir build
cd build
./configure --target-list=x86_64-softmmu,aarch64-softmmu,aarch64-linux-user
make
```

使用 ../configure --help 查看支持的系统

## 使用 Qemu 的参数
1. 调试内核:
  - https://blahcat.github.io/2018/01/07/building-a-debian-stretch-qemu-image-for-aarch64/
  - https://kennedy-han.github.io/2015/06/15/QEMU-arm64-guide.html
  - https://dev.to/alexeyden/quick-qemu-setup-for-linux-kernel-module-debugging-2nde

2. 各种自动化捕获 qemu 输出的方法:
  - https://fadeevab.com/how-to-setup-qemu-output-to-console-and-automate-using-shell-script/

-nographic does the same as "-serial stdio" and also hides a QEMU's graphical

-s  Shorthand for -gdb tcp::1234, i.e. open a gdbserver on TCP port 1234.


## 经典配置方案
1. 自己编译内核 + minimal 镜像: 使用 https://linux-kernel-labs.github.io/
2. 自己编译内核 + ubuntun 镜像:
3. ubuntun 镜像和内核:

##  和 Host 共享文件
首先保证 guest kernel 的配置:
- https://wiki.qemu.org/Documentation/9psetup
然后按照这个参数:
- https://askubuntu.com/questions/290668/how-to-share-a-folder-between-kvm-host-and-guest-using-virt-manager

## HMI
这里描述在 graphic 和 non-graphic 的模式下访问 HMI 的方法，并且说明了从 HMI 中间如何获取各种信息
https://web.archive.org/web/20180104171638/http://nairobi-embedded.org/qemu_monitor_console.html

自己尝试的效果:
```c
(qemu) info chardev
virtiocon0: filename=pty:/dev/pts/6
serial1: filename=pipe
parallel0: filename=vc
gdb: filename=disconnected:tcp:0.0.0.0:1234,server
compat_monitor0: filename=stdio
serial0: filename=pipe
```
如果什么都不配置，结果如下:
```c
(qemu) info chardev
parallel0: filename=vc
compat_monitor0: filename=stdio
serial0: filename=vc
```

## related project
- [Unicorn](https://github.com/unicorn-engine/unicorn) is a lightweight, multi-platform, multi-architecture CPU emulator framework based on QEMU.

