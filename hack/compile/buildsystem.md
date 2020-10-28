# build system

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
4. 各种 make defconfig 生成的过程 .config 的过程是什么 ?



## make modules
1. 在内核的source tree 中间，添加一个 hello world 的程序，然后加以编译执行。
2. 如果存在部分 module 是单独分开安装的，那么，在 Ubuntu 的 img 重新指定任意的版本的内核就应该是不可能的事情了

- [^2] make menuconfig 存在的两个框框 [] <>，前者只能选择为y 或者 n，后者还多出了一个 m，在 .config 中间也是存在对应的描述 =y =m，被注释掉



## compiler
https://lwn.net/Articles/512548/ : 函数前 `__visible` 的作用



[^1]: https://www.kernel.org/doc/html/latest/kbuild/kconfig-language.html
[^2]: https://unix.stackexchange.com/questions/20864/what-happens-in-each-step-of-the-linux-kernel-building-process
