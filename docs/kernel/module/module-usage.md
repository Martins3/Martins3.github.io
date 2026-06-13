## 关于 module 的基本使用
https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/9/html/managing_monitoring_and_updating_the_kernel/managing-kernel-modules_managing-monitoring-and-updating-the-kernel#doc-wrapper


## 常见工具
https://github.com/kmod-project/kmod 这个项目提供了
1. modprobe
2. lsmod
3. modinfo
4. depmod -a / -A


## weak modules 基本原理
- https://unix.stackexchange.com/questions/229521/weak-modules-from-module-init-tools-directory
- weak-modules 到底是做什么的，为什么安装的过程这么慢?

## 这是哪里有点问题吗?

加载的时候存在这个问题，怎么说
```txt
loading out-of-tree module taints kernel.
loading out-of-tree module taints kernel.
module verification failed: signature and/or required key missing - tainting kernel
```

## 模块相关工具总结

## request_module

## 检查下这个东西

`parse_args` is a routine that parses an input string with parameters in the form `name_variable=value`, looking for specific keywords and invoking the right handlers. `parse_args` is also used when loading a module, to parse the command-line parameters provided (if any).

When a piece of code is compiled as a module, the `__setup` macro is ignored (i.e., defined as a no-op).

The reason why start_kernel calls `parse_args` twice to parse the boot configuration
string is that boot-time options are actually divided into two classes, and each call
takes care of one class:

`early_param` 和 `__setup` 被分别分析


## 文档
1. man systemd-modules-load.service
2. man modules-load.d

## 需要分析下 kernel/module 目录的实现

## 如何知道是谁在引用内核模块
- rmmod nvme : 两个盘就消失了
- rmmod virtio_scsi : 被告知存在引用，但是不知道该引用在哪里

## kabi check
- https://access.redhat.com/solutions/444773
- https://wiki.ubuntu.com/KernelTeam/Stable_kABI

是因为这个原因，导致最后无法在 nixos 中插入 kvm 模块吗?

## 理解下 kmod version 到底咋回事?
magic ，function version 是如何区分的 ?

如果修改了函数的参数数量，或者函数参数结构体的定义，应该是不可以的，

1. 如果修改了一个函数的内容，导致 bzImage 中内容变化，kernel module 可以插入吗?
2. 如果修改了一个和函数的参数不相关的结构体，会吗？

## 如果使用 ftrace 正在 trace 一个模块中的符号，这个模块的引用也会增加


## /etc 配置相关
- /etc/modules-load.d/
- /etc/modprobe.d/


/etc/modules-load.d/nixos.conf
```txt
vfio_pci
vfio_iommu_type1
vmd
null_blk
scsi_debug
vhost_net
nvmet
nvmet-tcp
kvm-intel
tun
openvswitch
bridge
macvlan
tap
tun
tun
bridge
veth
br_netfilter
xt_nat
cpufreq_powersave
loop
atkbd
ctr
nvidia
nvidia_modeset
nvidia_drm
```

/etc/modprobe.d/ 下

```txt
 debian.conf   firmware.conf   nixos.conf   systemd.conf   ubuntu.conf
```
真的是给爷整笑了，直接抄袭 debian 和 ubuntu 的!


为什么 systemd 需要增加这个配置:
```txt
🧀  cat systemd.conf
#  SPDX-License-Identifier: LGPL-2.1-or-later
#
#  This file is part of systemd.
#
#  systemd is free software; you can redistribute it and/or modify it
#  under the terms of the GNU Lesser General Public License as published by
#  the Free Software Foundation; either version 2.1 of the License, or
#  (at your option) any later version.
#
# When bonding module is loaded, it creates bond0 by default due to max_bonds
# option default value 1. This interferes with the network configuration
# management / networkd, as it is not possible to detect whether this bond0 was
# intentionally configured by the user, or should be managed by
# networkd/NM/etc. Therefore disable bond0 creation.

options bonding max_bonds=0

# Do the same for dummy0.

options dummy numdummies=0
```

https://stackoverflow.com/questions/73811317/how-to-find-the-udev-rule-which-causes-the-loading-of-the-kernel-module-88xxau-k

## 是放到这里的吧
https://git.kernel.org/pub/scm/utils/kernel/kmod/kmod.git

## 总结下 module 的使用
- softdep 之类的


# 如何方便的再 nixos 上构建 kvm 模块


## 问题 1 : 这样执行完成之后，存在如下的警告
```txt
exe "make -j32"
exe "make M=./arch/x86/kvm/  modules -j32"
```

```txt
[ 1446.603548] kvm: version magic '6.4.9-g5962edea824b SMP preempt mod_unload ' should
 be '6.4.9 SMP preempt mod_unload '
```

## 问题 2 : 不知道为什么，在虚拟机中构建的时候 kvm 的时候
```txt
make[2]: *** No rule to make target 'arch/x86/kvm/kvm.ko', needed by '__modfinal'.  Stop.
make[1]: *** [/home/martins3/hack/nixos-kernel/linux-6.8.8/Makefile:1854: modules] Error 2
make: *** [Makefile:240: __sub-make] Error 2
```


## depmod 深入分析
只有 depmod 之后，modprobe 才可以正确的加载模块，进而导致 udev 不可以正确加载内核模块
所以安装新驱动的之后，都是需要执行一个 depmod 的。

看看 kernel source 中构建的时候，都有那些 pkg :

| 大小 | 文件名                    | 含义                            | 案例                                                                                                      |
|------|---------------------------|---------------------------------|----------------------------------------------------------------------------------------------------------|
| 16k  |  modules.builtin         | 描述被 builtin 的 kernel module | kernel/arch/x86/kvm/kvm.ko                                                                                |
| 173k |  modules.builtin.modinfo |  |
| 171  |  modules.order           | 和想象的不一样，只有两个        | arch/x86/events/amd/power.ko
| 791k |  Module.symvers          | 描述导出的符号                  | 0x2b81f3fa	__tracepoint_mlx5_fs_del_rule	drivers/net/ethernet/mellanox/mlx5/core/mlx5_core	EXPORT_SYMBOL |
| 11M  |  System.map              | 所有符号的地址 |
| 818k |  vmlinux.symvers         | 不知道和 Module.symvers 的关系，内容有重复|


### 测试的问题

在 openEuler 中 : /usr/lib/modules/6.6.0-28.0.0.34.oe2403.x86_64/modules.dep

```txt
➜  modules cat 6.6.0-28.0.0.34.oe2403.x86_64/modules.dep | grep kvm
kernel/arch/x86/kvm/kvm.ko.xz: kernel/virt/lib/irqbypass.ko.xz
kernel/arch/x86/kvm/kvm-intel.ko.xz: kernel/arch/x86/kvm/kvm.ko.xz kernel/virt/lib/irqbypass.ko.xz
kernel/arch/x86/kvm/kvm-amd.ko.xz: kernel/drivers/crypto/ccp/ccp.ko.xz kernel/arch/x86/kvm/kvm.ko.xz kernel/virt/lib/irqbypass.ko.xz
kernel/fs/proc/etmem_scan.ko.xz: kernel/arch/x86/kvm/kvm.ko.xz kernel/virt/lib/irqbypass.ko.xz
kernel/drivers/gpu/drm/i915/kvmgt.ko.xz: kernel/drivers/vfio/mdev/mdev.ko.xz kernel/drivers/vfio/vfio.ko.xz kernel/drivers/iommu/iommufd/iommufd.ko.xz kernel/drivers/gpu/drm/i915/i915.ko.xz kernel/drivers/char/agp/intel-gtt.ko.xz kernel/drivers/gpu/drm/drm_buddy.ko.xz kernel/drivers/i2c/algos/i2c-algo-bit.ko.xz kernel/drivers/gpu/drm/display/drm_display_helper.ko.xz kernel/drivers/media/cec/core/cec.ko.xz kernel/drivers/gpu/drm/ttm/ttm.ko.xz kernel/drivers/gpu/drm/drm_kms_helper.ko.xz kernel/drivers/gpu/drm/drm.ko.xz kernel/drivers/acpi/video.ko.xz kernel/drivers/platform/x86/wmi.ko.xz kernel/arch/x86/kvm/kvm.ko.xz kernel/virt/lib/irqbypass.ko.xz
kernel/drivers/ptp/ptp_kvm.ko.xz:
```


```sh
nix-build -E '(import <nixpkgs> {}).linuxPackages_latest.kernel.dev' --no-out-link
```

但是在 nixos 中，modprobe 是可以
```txt
🧀  modprobe kvm_intel --show-depends
insmod /run/booted-system/kernel-modules/lib/modules/6.11.0/kernel/arch/x86/kvm/kvm.ko.xz halt_poll_ns=0
insmod /run/booted-system/kernel-modules/lib/modules/6.11.0/kernel/arch/x86/kvm/kvm-intel.ko.xz
```

在 nixos 中 modules.dep
邪门，原来在这里的啊 : /run/current-system/kernel-modules/lib/modules

```txt
dr-xr-xr-x     - root  1 Jan  1970   kernel
.r--r--r--  1.7M root  1 Jan  1970   modules.alias
.r--r--r--  4.4k root  1 Jan  1970   modules.builtin
.r--r--r--   45k root  1 Jan  1970   modules.builtin.modinfo
.r--r--r--  973k root  1 Jan  1970   modules.dep
.r--r--r--   498 root  1 Jan  1970   modules.devname
.r--r--r--  264k root  1 Jan  1970   modules.order
.r--r--r--  2.9k root  1 Jan  1970   modules.softdep
.r--r--r--  777k root  1 Jan  1970   modules.symbols
```

构建内核的时候会有:
- modules.order
- modules.builtin : 说明那些模块是 builtin 的
- modules.builtin.modinfo : 无实质内容

在 depmod 之后有
```txt
.rw-r--r--    30k martins3 17 Jan 17:38   modules.alias
.rw-r--r--    31k martins3 17 Jan 17:38   modules.alias.bin
.rw-r--r--    10k martins3 17 Jan 17:38   modules.builtin
.rw-r--r--    12k martins3 17 Jan 17:38   modules.builtin.alias.bin
.rw-r--r--    13k martins3 17 Jan 17:38   modules.builtin.bin
.rw-r--r--   108k martins3 17 Jan 17:38   modules.builtin.modinfo
.rw-r--r--   6.4k martins3 17 Jan 17:38   modules.dep
.rw-r--r--    15k martins3 17 Jan 17:38   modules.dep.bin
.rw-r--r--    221 martins3 17 Jan 17:38   modules.devname
.rw-r--r--   6.2k martins3 17 Jan 17:38   modules.order
.rw-r--r--    384 martins3 17 Jan 17:38   modules.softdep
.rw-r--r--   105k martins3 17 Jan 17:38   modules.symbols
.rw-r--r--   123k martins3 17 Jan 17:38   modules.symbols.bin
```

### modules.order
https://lwn.net/Articles/260856/

> When multiple built-in modules (especially drivers) provide the same
> capability, they're prioritized by link order specified by the order
> listed in Makefile.  This implicit ordering is lost for loadable
> modules.

### modules.symbols
告诉每一个模块都提供什么符号，例如:

```txt
alias symbol:kvm_init kvm
```

### modules.softdep
内容不多
```txt
# Soft dependencies extracted from modules themselves.
softdep bcachefs pre: xxhash
softdep bcachefs pre: poly1305
softdep bcachefs pre: chacha20
softdep bcachefs pre: sha256
softdep bcachefs pre: crc64
softdep bcachefs pre: crc32c
softdep xt_LOG pre: nf_log_syslog
softdep cxl_acpi pre: cxl_port
softdep cxl_mem pre: cxl_port
softdep vfio post: vfio_iommu_type1 vfio_iommu_spapr_tce
```

和模块中的这个有关
```c
MODULE_SOFTDEP("post: vfio_iommu_type1 vfio_iommu_spapr_tce");
```
### modules.devname

内容如下，估计是给 udev 用的:
```txt
# Device nodes to trigger on-demand module loading.
fuse fuse c10:229
cuse cuse c10:203
vhost_vsock vhost-vsock c10:241
dm_mod mapper/control c10:236
vhost_net vhost-net c10:238
tun net/tun c10:200
vfio vfio/vfio c10:196
```
### modules.dep
依赖关系，估计是 modprobe 用的
### modules.alias

所有的 module alias 的合计，估计还是 udev 用的
```txt
alias vport-type-4 vport_vxlan
alias pci:v00008086d000010D6sv*sd*bc*sc*i* igb
```

## 忽然想起来了，nv 的驱动就是 module ，然后链接一下

其实可以整理一下这种：

1. 静态库 : nv module 的构建
2. 动态库 : 插入一个 module
3. gdb 一个 ko 获取到符号和符号表

但是也是有一些不同的东西的，可以都整理一下


如果只是构建 bzImage ，然后对着这个目录构建模块，结果为:
具体原因是什么?
```txt
 make -C ~/data/linux-build M=/home/martins3/core/vn/code/nixos modules
 make[1]: Entering directory '/home/martins3/data/linux-build'
   MODPOST /home/martins3/core/vn/code/nixos/Module.symvers
 WARNING: Module.symvers is missing.
          Modules may not have dependencies or modversions.
          You may get many unresolved symbol errors.
          You can set KBUILD_MODPOST_WARN=1 to turn errors into warning
          if you want to proceed at your own risk.
 ERROR: modpost: "__fentry__" [/home/martins3/core/vn/code/nixos/hello.ko] undefined!
 ERROR: modpost: "_printk" [/home/martins3/core/vn/code/nixos/hello.ko] undefined!
 ERROR: modpost: "__x86_return_thunk" [/home/martins3/core/vn/code/nixos/hello.ko] undefined!
 ERROR: modpost: "module_layout" [/home/martins3/core/vn/code/nixos/hello.ko] undefined!
 make[3]: *** [scripts/Makefile.modpost:145: /home/martins3/core/vn/code/nixos/Module.symvers] Error 1
 make[2]: *** [/home/martins3/data/linux-build/Makefile:1870: modpost] Error 2
 make[1]: *** [Makefile:240: __sub-make] Error 2
 make[1]: Leaving directory '/home/martins3/data/linux-build'
		# make: *** [Makefile:19: default] Error 2
```

## 所以，为什么 x86 和 aarch 的一个叫 bzImage ，一个叫 Image ?
```sh
	 make $use_llvm CC='ccache clang' bzImage -j$cores
	 make $use_llvm CC='ccache $cc' Image -j$cores
	 make $use_llvm CC='ccache $cc' modules -j$cores
```

## 版本的问题解决之后，希望可以实现的效果
如果只是修改了 builtin ，那么仅仅去替换 /boo/t bzImage 就可以了

所以就没有必要去不断的制作 rpm ，然后去安装覆盖。

也没有必要重新制作 initramfs

其实就是简单的 rpm install 就可以了。

## 仔细看看这个问题
https://mp.weixin.qq.com/s/09qlJ2eGXQceRKs0vVxJgg

## depmod 的理解
mv ccp.ko /lib/modules/$version/extra/
depmod
然后 modprobe kvm 那么 ccp 可以自动被加载，而且是加载被替换过的

## 原来如此哦
https://askubuntu.com/questions/1372267/if-i-ve-already-blacklisted-nouveau-why-is-options-nouveau-modeset-0-necessar

## 现在看，似乎没那么难了
- https://superuser.com/questions/445507/how-do-i-compress-a-vmlinux-kernel-into-vmlinuz


## module 的 version
https://www.kernel.org/doc/html/next/kbuild/modules.html#module-versioning

> Module versioning is enabled by the CONFIG_MODVERSIONS tag, and is used as a simple ABI consistency check. A CRC value of the full prototype for an exported symbol is created. When a module is loaded/used, the CRC values contained in the kernel are compared with similar values in the module; if they are not equal, the kernel refuses to load the module.

好像也就是这了，简单的 prototype 的 CRC 而已。


## 忘记修改了什么，然后就是这个问题
```txt
[72158.776647] module martins3: .gnu.linkonce.this_module section size must match the kernel's built struct module size at run time
[72175.741768] module martins3: .gnu.linkonce.this_module section size must match the kernel's built struct module size at run time
```

## 不要忘记了，除了 srcversion 还有 version 可以看的

```txt
[root@localhost 15:27:56 module]$ ls -la /sys/module/*/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/ata_generic/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/ata_piix/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/configfs/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/hinic3/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/ipmi_msghandler/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/libata/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/scsi_dh_alua/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/scsi_dh_rdac/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/sfc_driverlink/version
-r--r--r-- 1 root root 4096 Dec 29 15:27 /sys/module/sfc/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/sg/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/tcp_cubic/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/tpm_crb/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/tpm_tis_core/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/tpm_tis/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/tpm/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/vfio_iommu_type1/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/vfio_pci/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/vfio/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/vfio_virqfd/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/virtio_pci_modern_dev/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/virtio_pci/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/vmd/version
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/xz_dec/version
[root@localhost 15:28:07 module]$ ls -la /sys/module/*/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/acpi_ipmi/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/ata_generic/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/ata_piix/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/cdrom/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/cec/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/cirrus/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/crc32c_intel/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/crc32_pclmul/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/crct10dif_pclmul/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/drm_kms_helper/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/drm/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/ext4/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/failover/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/fb_sys_fops/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/fuse/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/ghash_clmulni_intel/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/hinic3/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/i2c_piix4/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/intel_rapl_common/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/intel_rapl_msr/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/ipmi_devintf/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/ipmi_msghandler/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/ip_tables/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/irqbypass/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/jbd2/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/joydev/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/libata/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/libcrc32c/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/libnvdimm/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/mbcache/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/mdio/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/mtd/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/net_failover/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/nf_conncount/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/nf_conntrack/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/nf_defrag_ipv4/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/nf_defrag_ipv6/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/nfit/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/nf_nat/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/openvswitch/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/pcspkr/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/serio_raw/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/sfc_driverlink/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:27 /sys/module/sfc/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/sg/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/sr_mod/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/sunrpc/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/syscopyarea/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/sysfillrect/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/sysimgblt/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/vfio_iommu_type1/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/vfio_pci/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/vfio/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/vfio_virqfd/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/virtio_balloon/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/virtio_blk/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/virtio_console/srcversion
-r--r--r-- 1 root root 4096 Dec 29 15:28 /sys/module/virtio_net/srcversion
```

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
