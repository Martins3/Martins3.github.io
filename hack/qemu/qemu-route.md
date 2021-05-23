## TODO
- [ ] 编译系统 : https://qemu.readthedocs.io/en/latest/devel/build-system.html

- [ ] x86 上原生的运行 x86 tcg, 大致流程是什么样子的?

- [ ] 图形系统是如何实现的 ?
  - `-vga virtio` 为什么不需要在 kernel 那一侧的支持 ?

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

编译 --disable-werror 目前是必须的, 很难受

## 在 Loongarch 上搭建 Qemu 开发环境
1. 参考 ccls.md 中搭建 Qemu 阅读环境
2. 似乎是可以直接在 x86 的机器上运行 Loongarch 的代码的


## 使用 Qemu 的参数
1. 调试内核:
  - https://blahcat.github.io/2018/01/07/building-a-debian-stretch-qemu-image-for-aarch64/
  - https://kennedy-han.github.io/2015/06/15/QEMU-arm64-guide.html
  - https://dev.to/alexeyden/quick-qemu-setup-for-linux-kernel-module-debugging-2nde

2. 各种自动化捕获 qemu 输出的方法:
  - https://fadeevab.com/how-to-setup-qemu-output-to-console-and-automate-using-shell-script/

##  和 Host 共享文件
1. **首先保证 guest kernel 的配置**:
  - https://wiki.qemu.org/Documentation/9psetup
  - 首先 NET_9P，然后 9P_FS，等等
2. 然后按照这个参数:
  - https://askubuntu.com/questions/290668/how-to-share-a-folder-between-kvm-host-and-guest-using-virt-manager

# 资源
- https://news.ycombinator.com/item?id=26941744
  - https://zserge.com/posts/kvm/ : 小文章
  - [qemu blog](https://airbus-seclab.github.io/qemu_blog/)
  - 可能有用的书 和 文章:
    - Foundations of Libvirt Development: How to Set Up and Maintain a Virtual Machine Environment with Python
    - https://www.morganclaypool.com/doi/abs/10.2200/S00754ED1V01Y201701CAC038
    - https://dl.acm.org/doi/abs/10.1145/2382553.2382554

- [ ] https://www.qemu-advent-calendar.org/2020/ : pulished every two year, adventure for dune

## debug kernel with qemu
- http://nickdesaulniers.github.io/blog/2018/10/24/booting-a-custom-linux-kernel-in-qemu-and-debugging-it-with-gdb/
- https://www.kernel.org/doc/html/latest/dev-tools/gdb-kernel-debugging.html

- https://stefano-garzarella.github.io/posts/2019-08-23-qemu-linux-kernel-pvh/
  - 让内核不要被压缩, 根本不是一个东西

## related project
- [Unicorn](https://github.com/unicorn-engine/unicorn) is a lightweight, multi-platform, multi-architecture CPU emulator framework based on QEMU.

