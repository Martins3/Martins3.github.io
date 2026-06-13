https://wiki.xenproject.org/wiki/Fedora_Host_Installation

为什么每次 make 似乎都是从头开始啊

先随便看看东西把
```txt
Installation targets:
  install               - build and install everything
  install-xen           - build and install the Xen hypervisor
  install-tools         - build and install the control tools
  install-stubdom       - build and install the stubdomain images
  install-docs          - build and install user documentation

Local dist targets:
  dist                  - build and install everything into local dist directory
  world                 - clean everything then make dist
  dist-xen              - build Xen hypervisor and install into local dist
  dist-tools            - build the tools and install into local dist
  dist-stubdom          - build the stubdomain images and install into local dist
  dist-docs             - build user documentation and install into local dist

Building targets:
  build                 - build everything
  build-xen             - build Xen hypervisor
  build-tools           - build the tools
  build-stubdom         - build the stubdomain images
  build-docs            - build user documentation

Cleaning targets:
  clean                 - clean the Xen, tools and docs
  distclean             - clean plus delete kernel build trees and
                          local downloaded files
  subtree-force-update  - Call *-force-update on all git subtrees (qemu, seabios, ovmf)

Miscellaneous targets:
  uninstall             - attempt to remove installed Xen tools
                          (use with extreme care!)

Package targets:
  src-tarball-release   - make a source tarball with xen and qemu tagged with a release
  src-tarball           - make a source tarball with xen and qemu tagged with git describe

Environment:
  [ this documentation is sadly not complete ]
```


make build-xen

## 难道环境中必须是 fedora 才可以吗?
https://www.qemu.org/docs/master/system/i386/xen.html

## fedora 环境中可以直接安装

sudo yum install xen ，然后 grub 从 xen 的位置启动

但是会发现系统盘无法启动，机器会一直卡主

## 还是可以观察一下

```txt
martins3@bogon:~$ rpm -ql xen-hypervisor
/boot/flask
/boot/flask/xenpolicy-4.19.2
/boot/xen-4.19.2.config
/boot/xen-4.19.2.gz
/usr/lib/debug/.build-id
/usr/lib/debug/.build-id/f7
/usr/lib/debug/.build-id/f7/6e14818bfa7a9e24fb683e2e35a27bfdee6d66
/usr/lib/debug/.build-id/f7/6e14818bfa7a9e24fb683e2e35a27bfdee6d66.debug
/usr/lib/debug/xen-4.19.2.efi.map
/usr/lib/debug/xen-syms-4.19.2
/usr/lib/debug/xen-syms-4.19.2.map
/usr/lib64/efi/xen-4.19.2.efi
/usr/lib64/efi/xen-4.19.2.notstripped.efi
```

```txt
martins3@bogon:~$ rpm -ql xen
/etc/sysconfig/xendomains
/etc/xen/auto
/usr/lib/.build-id
/usr/lib/.build-id/29
/usr/lib/.build-id/29/6d070de61cbaddcf234fe5cfe9f305787e1213
/usr/lib/.build-id/7f
/usr/lib/.build-id/7f/aa6576b064c0555816f1e0baf87a5ba9e9a2ec
/usr/lib/systemd/system/xendomains.service
/usr/lib64/python3.13/site-packages/xen
/usr/lib64/python3.13/site-packages/xen-3.0-py3.13.egg-info
/usr/lib64/python3.13/site-packages/xen-3.0-py3.13.egg-info/PKG-INFO
/usr/lib64/python3.13/site-packages/xen-3.0-py3.13.egg-info/SOURCES.txt
/usr/lib64/python3.13/site-packages/xen-3.0-py3.13.egg-info/dependency_links.txt
/usr/lib64/python3.13/site-packages/xen-3.0-py3.13.egg-info/top_level.txt
/usr/lib64/python3.13/site-packages/xen/__init__.py
/usr/lib64/python3.13/site-packages/xen/__pycache__
/usr/lib64/python3.13/site-packages/xen/__pycache__/__init__.cpython-313.opt-1.pyc
/usr/lib64/python3.13/site-packages/xen/__pycache__/__init__.cpython-313.pyc
/usr/lib64/python3.13/site-packages/xen/__pycache__/util.cpython-313.opt-1.pyc
/usr/lib64/python3.13/site-packages/xen/__pycache__/util.cpython-313.pyc
/usr/lib64/python3.13/site-packages/xen/lowlevel
/usr/lib64/python3.13/site-packages/xen/lowlevel/__init__.py
/usr/lib64/python3.13/site-packages/xen/lowlevel/__pycache__
/usr/lib64/python3.13/site-packages/xen/lowlevel/__pycache__/__init__.cpython-313.opt-1.pyc
/usr/lib64/python3.13/site-packages/xen/lowlevel/__pycache__/__init__.cpython-313.pyc
/usr/lib64/python3.13/site-packages/xen/lowlevel/xc.cpython-313-x86_64-linux-gnu.so
/usr/lib64/python3.13/site-packages/xen/lowlevel/xs.cpython-313-x86_64-linux-gnu.so
/usr/lib64/python3.13/site-packages/xen/migration
/usr/lib64/python3.13/site-packages/xen/migration/__init__.py
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/__init__.cpython-313.opt-1.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/__init__.cpython-313.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/legacy.cpython-313.opt-1.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/legacy.cpython-313.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/libxc.cpython-313.opt-1.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/libxc.cpython-313.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/libxl.cpython-313.opt-1.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/libxl.cpython-313.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/public.cpython-313.opt-1.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/public.cpython-313.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/tests.cpython-313.opt-1.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/tests.cpython-313.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/verify.cpython-313.opt-1.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/verify.cpython-313.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/xl.cpython-313.opt-1.pyc
/usr/lib64/python3.13/site-packages/xen/migration/__pycache__/xl.cpython-313.pyc
/usr/lib64/python3.13/site-packages/xen/migration/legacy.py
/usr/lib64/python3.13/site-packages/xen/migration/libxc.py
/usr/lib64/python3.13/site-packages/xen/migration/libxl.py
/usr/lib64/python3.13/site-packages/xen/migration/public.py
/usr/lib64/python3.13/site-packages/xen/migration/tests.py
/usr/lib64/python3.13/site-packages/xen/migration/verify.py
/usr/lib64/python3.13/site-packages/xen/migration/xl.py
/usr/lib64/python3.13/site-packages/xen/util.py
/usr/share/doc/xen
/usr/share/doc/xen/COPYING
/usr/share/doc/xen/README
```

## 一些基本概念都没有搞清楚，就不要先瞎鸡儿乱测试了

- Dom0 和 DomU 是什么意思
- pv 和 hvm 模式都是如何运行的
- 内核中需要提供哪些 xen 的模块，看上去是把 xen 作为一个 hypervisor ，然后伪装了 linux 内核实现的效果。

似乎是这个原因啊

## 想不到内核中需要有这么多代码来支持 xen 啊
- arch/x86/kvm/xen.c
- arch/x86/xen/

- arch/arm64/xen/hypercall.S

而且，似乎 x86 相对于 arm64 的支持明显要多很多的


这个文件的 kconfig 看懂一下: arch/x86/xen/Kconfig


## 需要 qemu 打开这两个 option 么
  xen             Xen backend support
  xen-pci-passthrough
                  Xen PCI passthrough support

原来 qemu 可以运行在 xen 上啊
```txt
config WHPX
    bool

config NVMM
    bool

config HVF
    bool

config TCG
    bool

config KVM
    bool

config XEN
    bool
    select FSDEV_9P if VIRTFS
    select PCI_EXPRESS_GENERIC_BRIDGE
    select XEN_BUS
```
这里居然没有 hyper-V 的支持。

## 基本介绍
https://wiki.xenproject.org/wiki/Xen_Project_Beginners_Guide


## 基本驱动的使用
- drivers/block/xen-blkback/blkback.c
- drivers/block/xen-blkfront.c


https://wiki.xenproject.org/wiki/Frontend_Driver

> To access devices that are to be shared between domains, like the disks and
> network interfaces, the DomUs must communicate with Dom0. This is done by
> using a two-part driver. The FrontendDriver must be written for the OS used in
> the DomU, and uses XenBus, XenStore, shared pages, and event notifications to
> communicate with the BackendDriver, which lives in Dom0 and fulfils requests.
> To the applications and the rest of the kernel, the FrontendDriver just looks
> like a normal network interface, disk, or whatever.

FrontendDriver 在 DomU 中，而 BackendDriver 在 Dom0 中。


## 可以尝试一下这个东西
https://fnordig.de/2016/12/02/xen-a-backend-frontend-driver-example/

## 想不到是 fedora 的问题，而是 VIRTIO_F_ACCESS_PLATFORM

xen_guest_init 中:
```c
	if (IS_ENABLED(CONFIG_XEN_VIRTIO))
		virtio_set_mem_acc_cb(xen_virtio_restricted_mem_acc);
```

似乎是在这里检查出现错误，然后就没有然后了:
```c
static int virtio_features_ok(struct virtio_device *dev)
{
	unsigned int status;

	might_sleep();

	if (virtio_check_mem_acc_cb(dev)) {
		if (!virtio_has_feature(dev, VIRTIO_F_VERSION_1)) {
			dev_warn(&dev->dev,
				 "device must provide VIRTIO_F_VERSION_1\n");
			return -ENODEV;
		}

		if (!virtio_has_feature(dev, VIRTIO_F_ACCESS_PLATFORM)) {
			dev_warn(&dev->dev,
				 "device must provide VIRTIO_F_ACCESS_PLATFORM\n");
			return -ENODEV;
		}
	}
```
并没有那么容易，可以发现:
1. 自己构建的内核，现在连键盘鼠标都不可以使用了

## CONFIG_XEN_PVH 和 CONFIG_XEN_PVHVM 有什么区别?

```txt
config XEN_PVHVM
	def_bool y
	depends on XEN && X86_LOCAL_APIC

config XEN_PVH
	bool "Xen PVH guest support"
	depends on XEN && XEN_PVHVM && ACPI
	select PVH
	help
	  Support for running as a Xen PVH guest.
```
他们的含义解释在:
https://wiki.xenproject.org/wiki/Understanding_the_Virtualization_Spectrum#Enhancements_to_PV

## xen 和内核的基本配置

- https://wiki.xenproject.org/wiki/Mainline_Linux_Kernel_Configs

## 计划
- 先使用我们自己的 kernel ，构建
- 然后自己来构建项目内容

## 看看这个
https://brieflyx.me/2024/2024-xz-salon/

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
