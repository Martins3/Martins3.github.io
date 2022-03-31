# busybox
https://busybox.net/

似乎是一个比较靠谱的文章:
https://www.cnblogs.com/hellogc/p/7482066.html
busybox 是用于提供 init 程序

这个方法 : 告知 drive + root 指定，使用 linuxrc 来启动，老的做法
> 这个方法成功了，那么 Ubuntu20 的还是没有办法，是不是因为 Ubuntu20 由于 ext4，所以，存在问题
> **可以尝试的内容 : 不使用 busybox，在其中只是简单的写入一个 init 程序 : 估计是不可以的**

> 1. 这个blog中间最后的部分，mount 各种文件夹的操作暂时没有实现
> 2. 关于创建 init 的部分，感觉不科学，maybe out of dated，内核的文档中间才说到 linurc 作为启动的不科学

cnblog 的那个，其中 tty 的报错，也许可以使用如下解决

```sh
sudo mkdir -p rootfs/dev
sudo mknod rootfs/dev/tty1 c 4 1
sudo mknod rootfs/dev/tty2 c 4 2
sudo mknod rootfs/dev/tty3 c 4 3
sudo mknod rootfs/dev/tty4 c 4 4
```

## 一个小问题
不知道为什么，当运行一个 hello world 的 initrd 的 kernel 参数是这个:

```c
  # arg_kernel_args="nokaslr console=ttyS0 root=/dev/ram rdinit=/hello.out"
```

而运行 initrd 的时候:

```c
  # arg_kernel_args="nokaslr console=ttyS0"
```

## 使用 busybox 可以观察到有意思的东西

## 网络
在启动 dhcp 的时候，可以检测到两次 irq_msi_compose_msg 和写入网络的操作。

```c
/*
#0  irq_msi_compose_msg (data=0xc71af010, msg=0xc7337d14) at arch/x86/kernel/apic/msi.c:29
#1  0xc1097bd7 in irq_chip_compose_msi_msg (data=<optimized out>, msg=<optimized out>) at kernel/irq/chip.c:1087
#2  0xc109a7c7 in msi_domain_activate (domain=<optimized out>, irq_data=0xc71af010) at kernel/irq/msi.c:88
#3  0xc1099525 in irq_domain_activate_irq (irq_data=0xc71af010) at kernel/irq/irqdomain.c:1303
#4  0xc1097358 in irq_startup (desc=0xc71af000, resend=true) at kernel/irq/chip.c:196
#5  0xc109616c in __setup_irq (irq=<optimized out>, desc=0xc71af000, new=0xc7326900) at kernel/irq/manage.c:1309
#6  0xc1096329 in request_threaded_irq (irq=24, handler=0xc155b7d0 <e1000_intr_msix_rx>, thread_fn=<optimized out>, irqflags=0, devname=<optimized out>, dev_id=0xc72a40
00) at kernel/irq/manage.c:1653
#7  0xc155fd2b in request_irq (dev=<optimized out>, name=<optimized out>, flags=<optimized out>, handler=<optimized out>, irq=<optimized out>) at include/linux/interrup
t.h:137
#8  e1000_request_msix (adapter=<optimized out>) at drivers/net/ethernet/intel/e1000e/netdev.c:2137
#9  e1000_request_irq (adapter=0xc72a4500) at drivers/net/ethernet/intel/e1000e/netdev.c:2185
#10 0xc1564877 in e1000_open (netdev=0xc72a4000) at drivers/net/ethernet/intel/e1000e/netdev.c:4578
#11 0xc16ba31e in __dev_open (dev=0xc72a4000) at net/core/dev.c:1335
#12 0xc16ba5af in __dev_change_flags (dev=0xc72a4000, flags=4099) at net/core/dev.c:6021
#13 0xc16ba687 in dev_change_flags (dev=0xc72a4000, flags=<optimized out>) at net/core/dev.c:6086
#14 0xc17372fb in devinet_ioctl (net=<optimized out>, cmd=<optimized out>, arg=<optimized out>) at net/ipv4/devinet.c:1052
#15 0xc17385ea in inet_ioctl (sock=<optimized out>, cmd=<optimized out>, arg=<optimized out>) at net/ipv4/af_inet.c:874
#16 0xc169d6dd in sock_do_ioctl (arg=<optimized out>, cmd=<optimized out>, sock=<optimized out>, net=<optimized out>) at net/socket.c:955
#17 sock_ioctl (file=<optimized out>, cmd=35092, arg=<optimized out>) at net/socket.c:955
#18 0xc1169a20 in vfs_ioctl (arg=<optimized out>, cmd=<optimized out>, filp=<optimized out>) at fs/ioctl.c:43
#19 file_ioctl (arg=<optimized out>, cmd=<optimized out>, filp=<optimized out>) at fs/ioctl.c:470
#20 do_vfs_ioctl (filp=0xc71afc00, fd=<optimized out>, cmd=<optimized out>, arg=3214201468) at fs/ioctl.c:605
#21 0xc1169ccf in SYSC_ioctl (arg=<optimized out>, cmd=<optimized out>, fd=<optimized out>) at fs/ioctl.c:622
#22 SyS_ioctl (fd=3, cmd=35092, arg=-1080765828) at fs/ioctl.c:613
#23 0xc1001aa1 in do_syscall_32_irqs_on (regs=<optimized out>) at arch/x86/entry/common.c:393
#24 do_fast_syscall_32 (regs=0xc7337fb4) at arch/x86/entry/common.c:460
#25 0xc18796a1 in entry_SYSENTER_32 () at arch/x86/entry/entry_32.S:312
#26 0x00000003 in ?? ()
#27 0x00008914 in ?? ()
#28 0xbf94d27c in ?? ()
Backtrace stopped: previous frame inner to this frame (corrupt stack?)
```

```c
#0  irq_msi_compose_msg (data=0xc71af010, msg=0xc7337d0c) at arch/x86/kernel/apic/msi.c:29
#1  0xc1097bd7 in irq_chip_compose_msi_msg (data=<optimized out>, msg=<optimized out>) at kernel/irq/chip.c:1087
#2  0xc109a782 in msi_domain_set_affinity (irq_data=0xc71af010, mask=<optimized out>, force=<optimized out>) at kernel/irq/msi.c:76
#3  0xc10957bf in irq_do_set_affinity (data=<optimized out>, mask=0xc7337d78, force=<optimized out>) at kernel/irq/manage.c:191
#4  0xc109585f in setup_affinity (desc=<optimized out>, mask=<optimized out>) at kernel/irq/manage.c:362
#5  0xc1095f1d in __setup_irq (irq=<optimized out>, desc=0xc71af000, new=0xc7326900) at kernel/irq/manage.c:1321
#6  0xc1096329 in request_threaded_irq (irq=24, handler=0xc155b7d0 <e1000_intr_msix_rx>, thread_fn=<optimized out>, irqflags=0, devname=<optimized out>, dev_id=0xc72a40
00) at kernel/irq/manage.c:1653
#7  0xc155fd2b in request_irq (dev=<optimized out>, name=<optimized out>, flags=<optimized out>, handler=<optimized out>, irq=<optimized out>) at include/linux/interrup
t.h:137
#8  e1000_request_msix (adapter=<optimized out>) at drivers/net/ethernet/intel/e1000e/netdev.c:2137
#9  e1000_request_irq (adapter=0xc72a4500) at drivers/net/ethernet/intel/e1000e/netdev.c:2185
#10 0xc1564877 in e1000_open (netdev=0xc72a4000) at drivers/net/ethernet/intel/e1000e/netdev.c:4578
#11 0xc16ba31e in __dev_open (dev=0xc72a4000) at net/core/dev.c:1335
#12 0xc16ba5af in __dev_change_flags (dev=0xc72a4000, flags=4099) at net/core/dev.c:6021
#13 0xc16ba687 in dev_change_flags (dev=0xc72a4000, flags=<optimized out>) at net/core/dev.c:6086
#14 0xc17372fb in devinet_ioctl (net=<optimized out>, cmd=<optimized out>, arg=<optimized out>) at net/ipv4/devinet.c:1052
#15 0xc17385ea in inet_ioctl (sock=<optimized out>, cmd=<optimized out>, arg=<optimized out>) at net/ipv4/af_inet.c:874
#16 0xc169d6dd in sock_do_ioctl (arg=<optimized out>, cmd=<optimized out>, sock=<optimized out>, net=<optimized out>) at net/socket.c:955
#17 sock_ioctl (file=<optimized out>, cmd=35092, arg=<optimized out>) at net/socket.c:955
#18 0xc1169a20 in vfs_ioctl (arg=<optimized out>, cmd=<optimized out>, filp=<optimized out>) at fs/ioctl.c:43
#19 file_ioctl (arg=<optimized out>, cmd=<optimized out>, filp=<optimized out>) at fs/ioctl.c:470
#20 do_vfs_ioctl (filp=0xc71afc00, fd=<optimized out>, cmd=<optimized out>, arg=3214201468) at fs/ioctl.c:605
#21 0xc1169ccf in SYSC_ioctl (arg=<optimized out>, cmd=<optimized out>, fd=<optimized out>) at fs/ioctl.c:622
#22 SyS_ioctl (fd=3, cmd=35092, arg=-1080765828) at fs/ioctl.c:613
#23 0xc1001aa1 in do_syscall_32_irqs_on (regs=<optimized out>) at arch/x86/entry/common.c:393
#24 do_fast_syscall_32 (regs=0xc7337fb4) at arch/x86/entry/common.c:460
#25 0xc18796a1 in entry_SYSENTER_32 () at arch/x86/entry/entry_32.S:312
#26 0x00000003 in ?? ()
#27 0x00008914 in ?? ()
#28 0xbf94d27c in ?? ()
Backtrace stopped: previous frame inner to this frame (corrupt stack?)
```
