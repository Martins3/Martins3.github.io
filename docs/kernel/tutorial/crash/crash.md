# crash utility

https://crash-utility.github.io/crash_whitepaper.html

## crash 常用命令
<!-- 54e3501f-b334-47fc-bfaf-68a332235fb5 -->

- foreach bt : 所有进程的 backtrace
- bt -a : 所有的 CPU 的 backtrace
- bt -FF  264 : CPU
  - [ ] -FF 的数据，好吧，需要重新理解 kmalloc 和 stack 的关系
- search sd_fops : 搜索 sd_fops，我靠，根本不能理解为什么这个东西的实现原理啊
- dev : 展示所有的 device
- kmem
  - `-s` : 展示 k

### backtrace
- foreach bt : 所有进程的 backtrace
- bt -a : 所有的 CPU 的 backtrace
- bt -FF 264 : CPU
  - [ ] -FF 的数据，好吧，需要重新理解 kmalloc 和 stack 的关系
- bt -FF -c 12

```txt
#6 [ff58499be3613d88] dev_printk at ffffffffbced1f0e
   ff58499be3613d90: .LC101+608       ff58499be3613da0
   ff58499be3613da0: ff8f004100000018 ff58499be3613e00
   ff58499be3613db0: ff58499be3613dc0 4f7b2ca0fd072e00
   ff58499be3613dc0: ff8f004105221b40 ff8f004105221b80
   ff58499be3613dd0: ff8f004105221bc0 00000000802039d0
   ff58499be3613de0: 0000000000002170 00000000802020c0
   ff58499be3613df0: [ff39c4d980202000:kmalloc-2k] lpfc_dev_loss_tmo_callbk.cold+96
#7 [ff58499be3613df8] lpfc_dev_loss_tmo_callbk.cold at ffffffffc0d1ba6c [lpfc]
   ff58499be3613e00: [ff39c4d980202140:kmalloc-2k] [ff39c4d9802020c0:kmalloc-2k]
   ff58499be3613e10: [ff39c4d980202480:kmalloc-2k] 00000000802021f0
   ff58499be3613e20: 000000000000ff39 0000000080202320
   ff58499be3613e30: 4f7b2ca0fd072e00 [ff39c4d8d6fc3e48:kmalloc-2k]
   ff58499be3613e40: [ff39c4f7cdf9a000:kmalloc-8k] [ff39c4d8d6fc3860:kmalloc-2k]
   ff58499be3613e50: [ff39c4d8d6fc3800:kmalloc-2k] [ff39c4f7c46ed000:kmalloc-4k]
   ff58499be3613e60: ff8a497c4049e315 fc_rport_final_delete+231
```

首先，我们来回顾一下 x86 处理 stack 的基本原理:
```txt
# 调用前（caller 函数内）
call dev_printk    ; 1. 将下一条指令地址（dev_printk+93）压栈
                   ; 2. RSP -= 8
                   ; 3. RIP = dev_printk 入口

# 进入 dev_printk 后
RSP = ff58499be3613d88   ← 栈指针指向刚压入的返回地址
[RSP] = dev_printk+93    ← 栈上存储的值（返回地址）
RIP = ffffffffbced1f0e   ← 当前执行指令（dev_printk 内部）
```

所以，这个就是标题而已，并不占用空间:
```txt
#7 [ff58499be3613df8] lpfc_dev_loss_tmo_callbk.cold at ffffffffc0d1ba6c [lpfc]
    描述 stack 开始的位置  符号				符号地址
```
而其 stack 的内容在标题下面。


### search
- search sd_fops : 搜索 sd_fops，我靠，根本不能理解为什么这个东西的实现原理啊
- search

### struct

- struct hrtimer 0xffff8faa7e095ee0
- struct -x o task_struct.group_leader
- struct vcpu_vmx.msr_ia32_feature_control -o 获取到 member 的 offset ?

仅仅打印一个结构体的定义:
ptype /o struct task_struct

### kmem

#### kmem -s
1. kmem -s : 一个地址属于那个 slab 的
```txt
crash> kmem -s ffff8040a10b8688
CACHE OBJSIZE ALLOCATED TOTAL SLABS SSIZE NAME
ffff80408001ac00 632 54215 77214 757 64k inode_cache
SLAB MEMORY NODE TOTAL ALLOCATED FREE
ffff7fe0102842c0 ffff8040a10b0000 0 102 12 90
FREE / [ALLOCATED]
[ffff8040a10b8480]
```

我猜测是通过遍历所有的 slub cache 来实现的。

2. 查询一个 slab 的统计信息:
```txt
crash> kmem -S ffffea00330eac40
CACHE             OBJSIZE  ALLOCATED     TOTAL  SLABS  SSIZE  NAME
ffff888100044780       32      11862     45824    358     4k  kmalloc-32
  SLAB              MEMORY            NODE  TOTAL  ALLOCATED  FREE
  ffffea00330eac40  ffff888cc3ab1000     5    128         20   108
  FREE / [ALLOCATED]
   ffff888cc3ab1000
   ffff888cc3ab1020
   ffff888cc3ab1040
   ffff888cc3ab1060
   ffff888cc3ab1080
   ffff888cc3ab10a0
   ffff888cc3ab10c0
   ffff888cc3ab10e0
   ffff888cc3ab1100
   ffff888cc3ab1120
   ffff888cc3ab1140
```

```txt
kmem -s ffff99d4de04690c
CACHE             OBJSIZE  ALLOCATED     TOTAL  SLABS  SSIZE  NAME
ffff99e419c2ca00      264          0        60      2     8k  tw_sock_TCPv6(1757:timemachine.service)
  SLAB              MEMORY            NODE  TOTAL  ALLOCATED  FREE
  fffff93444781180  ffff99d4de046000     0     30          0    30
  FREE / [ALLOCATED]
   ffff99d4de046880  (cpu 9 cache)
crash> struct tw_sock_TCPv6 ffff99d4de046880
```

#### 其他
kmem -i : 系统的内存信息

```txt
crash> kmem -i
                 PAGES        TOTAL      PERCENTAGE
    TOTAL MEM  65430267     249.6 GB         ----
         FREE  4829333      18.4 GB    7% of TOTAL MEM
         USED  60600934     231.2 GB   92% of TOTAL MEM
       SHARED  3444315      13.1 GB    5% of TOTAL MEM
      BUFFERS   153321     598.9 MB    0% of TOTAL MEM
       CACHED  25688175        98 GB   39% of TOTAL MEM
         SLAB   859636       3.3 GB    1% of TOTAL MEM

   TOTAL HUGE        0            0         ----
    HUGE FREE        0            0    0% of TOTAL HUGE

   TOTAL SWAP        0            0         ----
    SWAP USED        0            0    0% of TOTAL SWAP
    SWAP FREE        0            0    0% of TOTAL SWAP

 COMMIT LIMIT  32715133     124.8 GB         ----
    COMMITTED  65581527     250.2 GB  200% of TOTAL LIMIT
```

### mod

struct: invalid data structure reference: r5conf

https://stackoverflow.com/questions/58810201/how-to-find-a-symbol-file-and-tell-crash-about-it
需要加载对应的模块

mod -s ext2 path/to/ext2.ko.debug
mod -s raid1 lib/modules/3.10.0/kernel/drivers/md/raid1.ko
```txt
crash> lsmod
     MODULE       NAME                      TEXT_BASE         SIZE  OBJECT FILE
ffffffffc0403080  virtio_pci_legacy_dev  ffffffffc0401000    16384  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc040d0c0  virtio_pci_modern_dev  ffffffffc040b000    16384  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc0459200  sch_fq_codel           ffffffffc0412000    20480  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc041a3c0  virtio_pci             ffffffffc0415000    40960  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc043e0c0  fuse                   ffffffffc0422000   217088  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc046dc80  virtio_net             ffffffffc045e000   110592  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc0480300  9pnet_virtio           ffffffffc047d000    20480  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc0489300  virtio_scsi            ffffffffc0486000    28672  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc05a8bc0  dax                    ffffffffc0491000    53248  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc049b4c0  virtio_console         ffffffffc0496000    45056  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc04ad300  nvme_auth              ffffffffc04ab000    20480  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc04cf6c0  nvme_core              ffffffffc04b7000   233472  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc04f7280  crc32c_intel           ffffffffc04f4000    16384  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc0529200  netfs                  ffffffffc0507000   569344  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc05b9540  nvme                   ffffffffc05b2000    57344  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc0664300  kvm                    ffffffffc05ca000  1343488  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc0728700  dm_mod                 ffffffffc0713000   192512  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc074a780  configfs               ffffffffc0744000    61440  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc07570c0  iptable_nat            ffffffffc0755000    12288  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc075f540  configfs_sample        ffffffffc075d000    16384  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc0775c00  9pnet                  ffffffffc0769000   110592  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc0599f80  kvm_intel              ffffffffc077e000   409600  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc0800e00  nf_tables              ffffffffc07cf000   360448  (not loaded)  [CONFIG_KALLSYMS]
ffffffffc08325c0  null_blk               ffffffffc0829000    86016  (not loaded)  [CONFIG_KALLSYMS]
```

参考 https://crash-utility.github.io/help_pages/mod.html
需要加载 kernel module 的 debuginfo


- https://stackoverflow.com/questions/32069887/not-able-to-load-my-module-symbols-in-crash-utility
- https://walac.github.io/kernel-crashes/

加载一个外部内核模块:
```txt
 mod -s hct ../usr/lib/debug/lib/modules/martins3-5.10.x86_64/kernel/kvm.debug
     MODULE       NAME                        BASE            SIZE  OBJECT FILE
```

### irq

用来学习内核还是不错的:
```txt
This command collaborates the data in an irq_desc_t, along with its
associated hw_interrupt_type and irqaction structure data, into a
consolidated per-IRQ display.  For kernel versions 2.6.37 and later
the display consists of the irq_desc/irq_data address, its irqaction
address(es), and the irqaction name strings.  Alternatively, the
intel interrupt descriptor table, bottom half data, cpu affinity for
in-use irqs, or kernel irq stats may be displayed.  If no index value
argument(s) nor any options are entered, the IRQ data for all IRQs will
be displayed.

  index   a valid IRQ index.
     -u   dump data for in-use IRQs only.
     -d   dump the intel interrupt descriptor table.
     -b   dump bottom half data.
     -a   dump cpu affinity for in-use IRQs.
     -s   dump the kernel irq stats; if no cpu specified with -c, the
          irq stats of all cpus will be displayed.
 -c cpu   only usable with the -s option, dump the irq stats of the
          specified cpu[s]; cpu can be specified as "1,3,5", "1-3",
          "1,3,5-7,10", "all", or "a" (shortcut for "all").
```
只是从 vector 的角度来分析的，对于我好奇的 irqdomain 相关的，没有作用

### dev
调试内核到很少使用，但是用来理解内核倒是非常不错的:

支持多个命令，例如:

```txt
    -i  display I/O port usage; on 2.4 kernels, also display I/O memory usage.
    -p  display PCI device data.
    -d  display disk I/O statistics:
         TOTAL: total number of allocated in-progress I/O requests
          SYNC: I/O requests that are synchronous
         ASYNC: I/O requests that are asynchronous
          READ: I/O requests that are reads (older kernels)
         WRITE: I/O requests that are writes (older kernels)
           DRV: I/O requests that are in-flight in the device driver.
                If the device driver uses blk-mq interface, this field
                shows N/A(MQ).  If not available, this column is not shown.
    -D  same as -d, but filter out disks with no in-progress I/O requests.

  If the dumpfile contains device dumps:
        -V  display an indexed list of all device dumps present in the vmcore,
            showing their file offset, size and name.
  -v index  select and display one device dump based upon an index value
            shown by the -V option, shown in a default human-readable format;
            alternatively, the "rd -f" option along with its various format
            options may be used to further tailor the output.
      file  only used with -v, copy the device dump data to a file.
```


dev
```txt
CHRDEV    NAME                 CDEV        OPERATIONS
   1      mem            ffff888101b11380  memory_fops
   2      pty            ffff888101b15580  tty_fops
   3      ttyp           ffff888101b15600  tty_fops
   4      /dev/vc/0      ffffffff83e6b340  console_fops
   4      tty            ffff888101b11480  tty_fops
   4      ttyS           ffff888101b15800  tty_fops
   5      /dev/tty       ffffffff83e697c0  tty_fops
   5      /dev/console   ffffffff83e69740  console_fops
   5      /dev/ptmx      ffffffff83e699e0  ptmx_fops
   7      vcs            ffff888101b11400  vcs_fops
  10      misc           ffff888101b0a880  misc_fops
  13      input               (none)
  29      fb             ffff888103635d00  fb_fops
 128      ptm            ffff888101b15680  tty_fops
 136      pts            ffff888101b15700  tty_fops
 226      drm            ffff888103621c80  drm_stub_fops
 229      hvc            ffff88810ef18480  tty_fops
 249      virtio-portsdev  ffff8881074b1e00  portdev_fops
 250      vfio                (none)
 251      mei                 (none)
 252      bsg            ffff8881109e24a0  bsg_fops
 253      ptp            ffff88810982c050  posix_clock_file_operations
 254      pps            ffff888101b0aa00  pps_cdev_fops

BLKDEV    NAME                GENDISK      OPERATIONS
 259      blkext              (none)
   7      loop           ffff888109827800  lo_fops
   8      sd             ffff88810ee7f000  sd_fops
  11      sr                  (none)
  65      sd                  (none)
  66      sd                  (none)
  67      sd                  (none)
  68      sd                  (none)
  69      sd                  (none)
  70      sd                  (none)
  71      sd                  (none)
 128      sd                  (none)
 129      sd                  (none)
 130      sd                  (none)
 131      sd                  (none)
 132      sd                  (none)
 133      sd                  (none)
 134      sd                  (none)
 135      sd                  (none)
 251      virtblk        ffff88811aeef800  virtblk_fops
 252      zram           ffff8881062b4000  zram_devops
 253      device-mapper  ffff8881075a6000  dm_blk_dops
 254      nullb          ffff888113b9b000  null_ops
```

### 杂项

#### task
task -R state,comm,pid,thread_info ffff8080432bf000

#### waitq
很简单的功能
https://crash-utility.github.io/help_pages/waitq.html
#### files
#### ipcs
#### swap


#### ps
#### sys
#### vm
#### vtop

vtop -u addres

- 为什么 vtop -u 是怎么区分这是那个程序的地址 ?
  - [ ] 看看 `vm` 这个程序吧

#### ptov

#### rd : 读取内存

展示一个位置上的内存是什么
rd 0xffffa0428fa70000 8


#### ps

1. 直接使用名称，而且支持正则
注意: 是单引号

```txt
crash> ps 'tmux*'
      PID    PPID  CPU       TASK        ST  %MEM      VSZ      RSS  COMM
     1752    1665  28  ffff88800a8317c0  IN   0.4    23648     3788  tmux: client
     1754       1   0  ffff888012852f80  IN   0.4    24188     4060  tmux: server
```
2. ps -y 限制 policy


## 几个经典用法

### 调试当前机器的 kvm
```txt
crash vmlinux
mod -s kvm_intel ./kernel/arch/x86/kvm/kvm-intel.ko.debug
mod -s kvm ./kernel/arch/x86/kvm/kvm.ko.debug
```

### libvirt
virsh dump --memory-only --live e5fb54af-98ec-46d7-a69b-5a8fb6b52996 g.dump

## 获取函数地址
参考:
- https://www.kernel.org/doc/html/latest/admin-guide/bug-hunting.html
- https://www.kernel.org/doc/html/latest/admin-guide/quickly-build-trimmed-linux.html#backup

1. 方法 1，通过 EIP 可以获取到
```txt
$ gdb vmlinux
(gdb) l *0xc021e50e
```

2. 对于 `EIP is at vt_ioctl+0xda8/0x1482`
```txt
l *vt_ioctl+0xda8
```
这个方法对于 module 也是可以的

先 gdb debuginfo ，这里的 debuginfo 可以替换一下:
```txt
gdb usr/lib/debug/lib/modules/txgbe.ko.debug
```
然后可以
```txt
$ l *txgbe_clean_tx_irq+0x100
0x2f50 is in txgbe_clean_tx_irq (drivers/net/ethernet/netswift/txgbe/txgbe_main.c:538).
```

3. 这个方法对于 crash 也是适用的，不过注意要提前加载内核模块
```txt
mod -s kvm ../usr/lib/debug/lib/modules/path/to/kvm.debug

l  *(hct_get_page+114)                                                                                                                0xffffffffc11303b2 is in hct_get_page (drivers/crypto/ccp/hygon/hct.c:1823).
```

在例如:
```txt
[10361.550742]  panic+0x358/0x3b0
[10361.550742]  ? _printk+0x64/0x80
[10361.550742]  ? __wake_up_klogd.part.0+0x4c/0x70
[10361.550742]  sysrq_handle_crash+0x1a/0x20
[10361.550742]  __handle_sysrq+0xd4/0x190
[10361.550742]  write_sysrq_trigger+0x59/0x80
[10361.550742]  proc_reg_write+0x56/0xa0
[10361.550742]  vfs_write+0xfa/0x470
[10361.550742]  ksys_write+0x6f/0xf0
[10361.550742]  do_syscall_64+0xbc/0x210
[10361.550742]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
[10361.550742] RIP: 0033:0x7fe4f724c9b4
```

gdb vmlinux ，然后可以直接：
```txt
$ l *write_sysrq_trigger+0x59
0xffffffff817e87f9 is in write_sysrq_trigger (drivers/tty/sysrq.c:1184).
1179                    if (c == '_')
1180                            bulk = true;
1181                    else
1182                            __handle_sysrq(c, false);
1183
1184                    if (!bulk)
1185                            break;
1186            }
1187
1188            return count;
```


### 使用工具 scripts/decode_stacktrace.sh
scripts/decode_stacktrace.sh 中有注释:
```txt
	# Let's start doing the math to get the exact address into the
	# symbol. First, strip out the symbol total length.
```
所以 `write_sysrq_trigger+0x59/0x80` 中的 0x80 就是 symbol total length 了发

```txt
# 这个
🧀  /home/martins3/data/linux/scripts/decode_stacktrace.sh ~/data/linux-build/vmlinux < a

[10361.550742] Call Trace:
[10361.550742]  <TASK>
[10361.550742] panic (kernel/panic.c:354)
[10361.550742] ? _printk (kernel/printk/printk.c:2436)
[10361.550742] ? __wake_up_klogd.part.0 (kernel/printk/printk.c:4495 (discriminator 3))
[10361.550742] sysrq_handle_crash (drivers/tty/sysrq.c:154)
[10361.550742] __handle_sysrq (drivers/tty/sysrq.c:613)
[10361.550742] write_sysrq_trigger (drivers/tty/sysrq.c:1184)
[10361.550742] proc_reg_write (fs/proc/inode.c:330 fs/proc/inode.c:342)
[10361.550742] vfs_write (fs/read_write.c:681)
[10361.550742] ksys_write (fs/read_write.c:736)
[10361.550742] do_syscall_64 (arch/x86/entry/common.c:52 (discriminator 1) arch/x86/entry/common.c:83 (discriminator 1))
[10361.550742] entry_SYSCALL_64_after_hwframe (arch/x86/entry/entry_64.S:130)
[10361.550742] RIP: 0033:0x7fe4f724c9b4
[10361.550742] Code: c7 00 16 00 00 00 b8 ff ff ff ff c3 66 2e 0f 1f 84 00 00 00 00 00 f3 0f
1e fa 80 3d b5 a9 0d 00 00

Code starting with the faulting instruction
===========================================
   0:   c7 00 16 00 00 00       movl   $0x16,(%rax)
   6:   b8 ff ff ff ff          mov    $0xffffffff,%eax
   b:   c3                      ret
   c:   66 2e 0f 1f 84 00 00    cs nopw 0x0(%rax,%rax,1)
  13:   00 00 00
  16:   f3 0f 1e fa             endbr64
  1a:   80 3d b5 a9 0d 00 00    cmpb   $0x a
```

所以，bpftrace 输出类似这种的，如果有 vmlinux ，那么也可以获取到每一个函数的调用的:

```txt
        ffffffff813ff8f1 do_sys_openat2+1
        ffffffff813fffd5 __x64_sys_openat2+149
        ffffffff821068fc do_syscall_64+188
        ffffffff82200130 entry_SYSCALL_64_after_hwframe+119
```

### [ ] 有待整理的东西
使用内核下的 : scripts/faddr2line
https://serverfault.com/questions/605946/kernel-stack-trace-to-source-code-lines

这个方法对于模块没用，使用 crash 可以实现
```txt
crash> sym proc_reg_write
ffffffffa99fc5d0 (t) proc_reg_write /usr/src/debug/kernel-5.14.0-70.16.1.el9_0/linux-5.14.0-70.16.1.el9_0.x86_64/fs/proc/inode.c: 340
crash> dis -s proc_reg_write
crash> dis -s ffffffffa99fc5d0
```

使用
crash $(find usr -name vmlinux) vmcore --src ./linux-3.10.0-957.21.3.el7 --mod ./usr
还是存在好几个问题:
1. --src 可以解决这个问题
```txt
crash> dis -s sys_signal
FILE: kernel/signal.c
LINE: 3554

dis: sys_signal: source code is not available
```
2. 但是 mod -s 之后，dis -s ext4_read_page 还是没有 FILE 和 LINE ，这导致

## 处理特殊符号
一般来说，可以直接使用 gdb

但是如果是特殊后缀的，那么可以用 addr2line 来辅助:
```sh
addr2line -e arch/x86/kvm/kvm.ko -f -p "vcpu_enter_guest.constprop.0+1623"
```

### 最后，回答这个问题
https://stackoverflow.com/questions/74196308/how-to-get-source-line-numbers-with-crash-utility-in-kernel-crash-debugging

## 经典分析案例
https://access.redhat.com/solutions/5375971
https://access.redhat.com/solutions/6233161
使用一个案例，分析如何解决死锁
https://access.redhat.com/solutions/5534961

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
