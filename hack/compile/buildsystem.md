# build system
[各种 make defconfig 生成的过程 .config 的过程是什么?](https://stackoverflow.com/questions/41885015/what-exactly-does-linux-kernels-make-defconfig-do)
简单来说，每一个config 项都是默认项目的，如果在 /arch/x86/configs/x86_64_defconfig 中间存在这个选项，那么就使用该选项，否则使用默认选项。

[make olddefconfig 的作用](https://lore.kernel.org/patchwork/patch/267098/)
将想要的选项放到 .config，比如 virtio 的，然后将 make olddefconfig ，其其余的选项都是自动采用默认选项.
这里存在一个很诡异的地方是 : 在 .config 放下面的语句, 会让配置变为 32bit x86
```
# CONFIG_64BIT is not set
```

- [ ] CONFIG_VIRTIO_BLK 之类的存在依赖，make olddefconfig 可以自动处理吗?
  - [ ] 比如 B 依赖 A, 如果 CONFIG_B=Y, 那么 CONFIG_A=Y 会被自动配置
  - [ ] 比如 B 依赖 A, C 要求 A 不能打开，同时配置 CONFIG_B=Y CONFIG_C=Y 会怎么样?


[vmlinux bzImage zImage 的关系是什么?](https://unix.stackexchange.com/questions/5518/what-is-the-difference-between-the-following-kernel-makefile-terms-vmlinux-vml)
1. vmlinux 将内核编译为静态的 ELF 格式，可以用于调试
2. vmlinux.bin : 将 vmlinux 中的所有符号和重定向信息去掉
3. vmlinuz : 压缩版本
4. zImage 和 bzImage : By adding further boot and decompression capabilities to vmlinuz, the image can be used to boot a system with the vmlinux kernel. 
  - 其中 zImage 和 bzImage 在于 b 也就是 big，大小是否大于 512KB

## 问题
1. vmlinux 到底包括什么东西，包括各种 ko 吗 ? 为什么有的驱动被编译为 ko 了 ?
2. make modules_install 是做什么的 ? 如果各种 ko 不是和 modules_install 分开的，那么为什么存在 make modules_install 
    1. 根本不能理解 make modules 是做什么的
```
➜  linux git:(master) ✗ make modules
  CALL    scripts/checksyscalls.sh
  CALL    scripts/atomic/check-atomics.sh
  DESCEND  objtool
  MODPOST 12 modules
```
2. 下面这些安装的库，首先需要 make modules 才可以被生成
```
➜  linux git:(master) ✗  make modules_install INSTALL_MOD_PATH=./img 
  INSTALL drivers/thermal/intel/x86_pkg_temp_thermal.ko
  INSTALL fs/efivarfs/efivarfs.ko
  INSTALL net/ipv4/netfilter/iptable_nat.ko
  INSTALL net/ipv4/netfilter/nf_log_arp.ko
  INSTALL net/ipv4/netfilter/nf_log_ipv4.ko
  INSTALL net/ipv6/netfilter/nf_log_ipv6.ko
  INSTALL net/netfilter/nf_log_common.ko
  INSTALL net/netfilter/xt_LOG.ko
  INSTALL net/netfilter/xt_MASQUERADE.ko
  INSTALL net/netfilter/xt_addrtype.ko
  INSTALL net/netfilter/xt_mark.ko
  INSTALL net/netfilter/xt_nat.ko
  DEPMOD  5.7.0-rc7+
```
3. 利用 kconfig  能不能构建更加小的项目。


- [ ] 通过这种方法了解一下 Kconfig 的使用方法 : 在内核的source tree 中间，添加一个 hello world 的程序，然后加以编译执行。
- [ ]  如果存在部分 module 是单独分开安装的，那么，在 Ubuntu 的 img 重新指定任意的版本的内核就应该是不可能的事情了
## make modules

- [^2] make menuconfig 存在的两个框框 [] <>，前者只能选择为y 或者 n，后者还多出了一个 m，在 .config 中间也是存在对应的描述 =y =m，被注释掉


## compiler
https://lwn.net/Articles/512548/ : 函数前 `__visible` 的作用

[^1]: https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html
[^2]: https://unix.stackexchange.com/questions/20864/what-happens-in-each-step-of-the-linux-kernel-building-process
