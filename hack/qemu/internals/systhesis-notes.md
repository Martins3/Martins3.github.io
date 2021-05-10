# Qemu
> 这里应该放置 synthesis 的内容

## TODO
- [ ] 编译系统 : https://qemu.readthedocs.io/en/latest/devel/build-system.html

## 使用
- [ ] 需要使用 qemu 测试的内容:
  - kdump
  - [x] qboot
- [ ] 按照文档测试一下 slirp 那个用户态网络库

## compile
一般的编译方法
```
mkdir build
cd build
../configure --target-list=x86_64-softmmu,aarch64-softmmu,aarch64-linux-user
../configure --target-list=aarch64-softmmu
make
```
使用 ../configure --help 查看支持的系统

为了生成的 compile_commands.json 可以正常使用，--target-list 最好不要同时支持多个，否则会出现一些诡异的问题。

编译一个仅仅支持 kvm 的代码:
```c
../configure --target-list=x86_64-softmmu  --disable-werror
```

编译 --disable-werror 目前是必须的, 着很难受

## 在 Loongarch 上搭建 Qemu 开发环境

## 使用 Qemu 的参数
1. 调试内核:
  - https://blahcat.github.io/2018/01/07/building-a-debian-stretch-qemu-image-for-aarch64/
  - https://kennedy-han.github.io/2015/06/15/QEMU-arm64-guide.html
  - https://dev.to/alexeyden/quick-qemu-setup-for-linux-kernel-module-debugging-2nde

2. 各种自动化捕获 qemu 输出的方法:
  - https://fadeevab.com/how-to-setup-qemu-output-to-console-and-automate-using-shell-script/

-nographic does the same as "-serial stdio" and also hides a QEMU's graphical

-s  Shorthand for -gdb tcp::1234, i.e. open a gdbserver on TCP port 1234.

##  和 Host 共享文件
首先保证 guest kernel 的配置:
- https://wiki.qemu.org/Documentation/9psetup
然后按照这个参数:
- https://askubuntu.com/questions/290668/how-to-share-a-folder-between-kvm-host-and-guest-using-virt-manager

## related project
- [Unicorn](https://github.com/unicorn-engine/unicorn) is a lightweight, multi-platform, multi-architecture CPU emulator framework based on QEMU.

