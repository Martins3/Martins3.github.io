# 计划给 kconfig 来来

Kconfig 的文档先看看吧

## 1 make menuconfig

```txt
config ROOT_NFS
	bool "Root file system on NFS"
	depends on NFS_FS=y && IP_PNP
	help
	  If you want your system to mount its root file system via NFS,
	  choose Y here.  This is common practice for managing systems
	  without local permanent storage.  For details, read
	  <file:Documentation/admin-guide/nfs/nfsroot.rst>.

	  Most people say N here.
```

修改为 nfs 应该没有问题吧

那么，这里就是 make menuconfig 的 bug 了啊
```txt
Symbol: ROOT_NFS [=n]
Type  : bool
Defined at fs/nfs/Kconfig:160
  Prompt: Root file system on NFS
  Depends on: NETWORK_FILESYSTEMS [=y] && NFS_FS [=m]=y [=y] && IP_PNP [=y]
  Location:
    -> File systems
(1)   -> Network File Systems (NETWORK_FILESYSTEMS [=y])
        -> Root file system on NFS (ROOT_NFS [=n])
```
这里展示了三个 yes ，但是实际上，ROOT_NFS 的条件还是不满足

```txt
Symbol: USB_KBD [=n]
Type  : tristate
Defined at drivers/hid/usbhid/Kconfig:51
  Prompt: USB HIDBP Keyboard (simple Boot) support
  Depends on: HID_SUPPORT [=y] && USB_HID [=m]!=y [=y] && EXPERT [=y] && USB [=m] && INPUT [=y]
  Location:
    -> Device Drivers
      -> HID bus support (HID_SUPPORT [=y])
        -> USB HID support
          -> USB HID Boot Protocol drivers
(1)         -> USB HIDBP Keyboard (simple Boot) support (USB_KBD [=n])
```
而且这导致了一个很奇怪的问题，那就是 USB_KBD 在 USB_HID 被设置为 m 之后
我们发现 CONFIG_USB_KBD 被自动关闭了，但是，实际上这个修改是可以再去手动打开的
```txt
#
# USB HID Boot Protocol drivers
#
# CONFIG_USB_KBD is not set
# CONFIG_USB_MOUSE is not set
# end of USB HID Boot Protocol drivers
```

而且奇怪的问题是，这里也没有写什么 USB_KBD 有这么多依赖啊，这些依赖都是哪里来的:
```txt
config USB_KBD
	tristate "USB HIDBP Keyboard (simple Boot) support"
	depends on USB && INPUT
	help
	  Say Y here only if you are absolutely sure that you don't want
	  to use the generic HID driver for your USB keyboard and prefer
	  to use the keyboard in its limited Boot Protocol mode instead.

	  This is almost certainly not what you want.  This is mostly
	  useful for embedded applications or simple keyboards.

	  To compile this driver as a module, choose M here: the
	  module will be called usbkbd.

	  If even remotely unsure, say N.
```


## KVM_EXTERNAL_WRITE_TRACKING 被 KVM select ，但是还是不会被使用

## 这个 select 真的有理解吗?

为什么有的 yes ，有的 no ，最后还是 no 的
```txt
  │ Symbol: LZ4_COMPRESS [=n]                                                                                                                        │
  │ Type  : tristate                                                                                                                                 │
  │ Defined at lib/Kconfig:308                                                                                                                       │
  │ Selected by [n]:                                                                                                                                 │
  │   - ZRAM_BACKEND_LZ4 [=n] && BLK_DEV [=y] && ZRAM [=m]                                                                                           │
  │   - DM_VDO [=n] && MD [=y] && 64BIT [=y] && BLK_DEV_DM [=m]                                                                                      │
  │   - DRM_GUD [=n] && HAS_IOMEM [=y] && DRM [=m] && USB [=m] && MMU [=y]                                                                           │
  │   - F2FS_FS [=n] && BLOCK [=y] && F2FS_FS_LZ4 [=n]                                                                                               │
  │   - BCACHEFS_FS [=n] && BLOCK [=y]                                                                                                               │
  │   - CRYPTO_LZ4 [=n] && CRYPTO [=y]                                                                                                               │
  │
```

既可以 select 又可以被 select 的如何理解?
```txt
  │ Symbol: NVME_AUTH [=n]                                                                                                                           │
  │ Type  : tristate                                                                                                                                 │
  │ Defined at drivers/nvme/common/Kconfig:7                                                                                                         │
  │ Selects: CRYPTO [=y] && CRYPTO_HMAC [=n] && CRYPTO_SHA256 [=m] && CRYPTO_SHA512 [=m] && CRYPTO_DH [=n] && CRYPTO_DH_RFC7919_GROUPS [=n]          │
  │ Selected by [n]:                                                                                                                                 │
  │   - NVME_HOST_AUTH [=n] && NVME_CORE [=m]                                                                                                        │
  │   - NVME_TARGET_AUTH [=n] && NVME_TARGET [=n]
```

这几个模块，我靠，别说，streamline_config.pl 了，我自己都不知道如何处理了?
全部都是这种 select

### sleect 的又是模块，这合理吗，streamline_config 基本无法使用
CONFIG_GPIO_AMDPT=m
CONFIG_NVME_AUTH=m
CONFIG_SND_HWDEP=m
CONFIG_GPIO_GENERIC=m

snd_hwdep 这个模块是被谁构建的
```txt
snd_hwdep              20480  1 snd_hda_codec
snd                   167936  8 snd_hda_codec_generic,snd_hda_codec_hdmi,snd_hwdep,snd_hda_intel,snd_hda_codec,snd_hda_codec_realtek,
snd_timer,snd_pcm
```

记住，被 select 只有 yes 和 no ，没有

```txt
🧀  lsmod | grep nvme
nvme                   73728  3
nvme_core             266240  4 nvme
nvme_auth              28672  1 nvme_core
```

CONFIG_GPIO_AMDPT=m 只能是 yes ，但是 lsmod 可以看到:

```txt
gpio_amdpt             16384  0
```

## 这个问题真的可以好好分析

```txt
rdma_ucm
rdma_cm
crct10dif_pclmul
libcrc32c
crc64_rocksoft
crc32_pclmul
crc32c_intel
crc64_rocksoft_generic
```
rg rdma_ucm -g 'Makefile' -g Kconfig
还可以看到

但是这个就完全看不到了
rg crc64 -g 'Makefile' -g Kconfig


难道 libcrc32c.ko 不见了?
```c
static __wsum warn_crc32c_csum_update(const void *buff, int len, __wsum sum)
{
	net_warn_ratelimited(
		"%s: attempt to compute crc32c without libcrc32c.ko\n",
		__func__);
	return 0;
}

static __wsum warn_crc32c_csum_combine(__wsum csum, __wsum csum2,
				       int offset, int len)
{
	net_warn_ratelimited(
		"%s: attempt to compute crc32c without libcrc32c.ko\n",
		__func__);
	return 0;
}
```
那么这两个注释改修改一下了。

## 使用 make olddefconfig 的警告是如何产生的

随便从这个机器上拷贝过来原来的配置
```txt
🧀  make olddefconfig
  HOSTCC  scripts/basic/fixdep
  HOSTCC  scripts/kconfig/conf.o
  HOSTCC  scripts/kconfig/confdata.o
  HOSTCC  scripts/kconfig/expr.o
  LEX     scripts/kconfig/lexer.lex.c
  YACC    scripts/kconfig/parser.tab.[ch]
  HOSTCC  scripts/kconfig/lexer.lex.o
  HOSTCC  scripts/kconfig/menu.o
  HOSTCC  scripts/kconfig/parser.tab.o
  HOSTCC  scripts/kconfig/preprocess.o
  HOSTCC  scripts/kconfig/symbol.o
  HOSTCC  scripts/kconfig/util.o
  HOSTLD  scripts/kconfig/conf
.config:378:warning: symbol value 'm' invalid for I8K
.config:868:warning: symbol value '0' invalid for BASE_SMALL
.config:1200:warning: symbol value 'm' invalid for NF_CT_PROTO_GRE
.config:3057:warning: symbol value 'm' invalid for ISDN_CAPI
.config:3596:warning: symbol value 'm' invalid for PINCTRL_AMD
.config:4764:warning: symbol value 'm' invalid for HSA_AMD
.config:5960:warning: symbol value 'm' invalid for VFIO_VIRQFD
.config:6697:warning: symbol value 'm' invalid for FSCACHE
#
# configuration written to .config
#
```
scripts/kconfig/confdata.c 报这错误的，所以，应该让内核中

## 在 arm 环境中，在 def/plus 中添加这个几个配置

```txt
CONFIG_KVM_X86=m
CONFIG_KVM_INTEL=m
CONFIG_KVM_AMD=m
```
结果既没有报错，也没有警告，config 到底如何工作的

## 记录一个现象

由于 make localmodconfig 导致 vhost 模块没有构建进去，
通过 make menuconfig 打开，然后 make -j128

然后可以手动的安装这三个模块:
```txt
🧀  sudo insmod lib/modules/6.15.4/kernel/drivers/vhost/vhost_iotlb.ko
linux-full/install-6.15.4 on  yyds [?] via ❄️  impure (yyds-env) 🐱
🧀  sudo insmod lib/modules/6.15.4/kernel/drivers/vhost/vhost.ko
linux-full/install-6.15.4 on  yyds [?] via ❄️  impure (yyds-env) 🐱
🧀  sudo insmod lib/modules/6.15.4/kernel/drivers/vhost/vhost_net.ko
linux-full/install-6.15.4 on  yyds [?] via ❄️  impure (yyds-env) 🐱
```

所以，这种完全没有外部依赖的模块还是很清爽的。


## make menuconfig 中的 {} 是什么意思

```txt
{M}   Failover driver
```

## 给 make menuconfig 添加一个搜索功能无法看父目录，其实可以作为一个 fs 的 tree 一样。

可以写一个 fuse 来实现 make menuconfig 中功能，多好啊

## 为了 nixos 而搞的逆天操作

但是，为什么需要版本号一模一样才可以插入内核模块？
```diff
diff --git a/scripts/setlocalversion b/scripts/setlocalversion
index 28169d7e143b..a60929d71c72 100755
--- a/scripts/setlocalversion
+++ b/scripts/setlocalversion
@@ -209,4 +209,4 @@ elif [ "${LOCALVERSION+set}" != "set" ]; then
 	scm_version="$(scm_version --short)"
 fi

-echo "${KERNELVERSION}${file_localversion}${config_localversion}${LOCALVERSION}${scm_version}"
+echo "${KERNELVERSION}"
--
2.49.0
```


## 再说一个经典例子

请问，到底该使用 selects 还是 depends on :

(应该是 ublk 错误了，不该 select io uring )

drivers/block/Kconfig 中:
```txt
config BLK_DEV_UBLK
	tristate "Userspace block driver (Experimental)"
	select IO_URING
	help
	  io_uring based userspace block driver. Together with ublk server, ublk
	  has been working well, but interface with userspace or command data
	  definition isn't finalized yet, and might change according to future
	  requirement, so mark is as experimental now.
```

```txt
config FUSE_IO_URING
	bool "FUSE communication over io-uring"
	default y
	depends on FUSE_FS
	depends on IO_URING
	help
	  This allows sending FUSE requests over the io-uring interface and
          also adds request core affinity.

	  If you want to allow fuse server/client communication through io-uring,
	  answer Y
```

当时在 nixos 环境构建整个 qemu 用的失败的技巧:
```sh
# TODO 显然，我们失败了，即使是从 /nix 拷贝正确的 Module.symvers 也不可以
# 还是需要将整个 kernel 构建一次
function build_quickly() {
	version=$(uname -r)
	nixos_version_tmp=${version:0:3} # 将 6.8.1 转换为 6_8
	nixos_version=${nixos_version_tmp/\./_}
	look=$(nix-build -E "(import <nixpkgs> {}).linuxPackages_$nixos_version.kernel.dev" --no-out-link)
	echo "$look"
	if [[ ! -f Module.symvers ]]; then
		cp "$look"/lib/modules/*/build/Module.symvers .
	fi
	make bzImage
}

```

## 如果 kernel module 是基于错误的 kernel 构建的，那么就会有这个报错
```txt
module tty0tty_mod: .gnu.linkonce.this_module section size must match the kernel's built struct module size at run time
module tty0tty_mod: .gnu.linkonce.this_module section size must match the kernel's built struct module size at run time
```
例如，现在虚拟机是通过 linux-build 启动的，然后构建模块又是基于 linux-vmtest 构建的，就会遇到问题。

## 记录一个问题，我现在在一个物理机上，用 linux-full
然后，发现 tap 设备没有，那么自然，添加如下 config
```txt
# 需要 tap 设备
CONFIG_TUN=m
CONFIG_MACVLAN=m
CONFIG_MACVTAP=m
CONFIG_IPVLAN=m
CONFIG_IPVTAP=m
CONFIG_PACKET=m
```
但是构建模块，直接 sudo insmod drivers/net/tap.ko ，会有这个错误
```txt
[74912.387983] tap: module verification failed: signature and/or required key missing - tainting kernel
[74912.389908] BPF:      type_id=120561 bits_offset=16960
[74912.396392] BPF:
[74912.399644] BPF: Invalid name
[74912.403954] BPF:
[74912.407077] failed to validate module [tap] BTF: -22
```
之前理解的东西，我只是添加了模块，那么就不该出现问题啊


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
