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

## backtrace
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


## search
- search sd_fops : 搜索 sd_fops，我靠，根本不能理解为什么这个东西的实现原理啊
- search

## struct

- struct hrtimer 0xffff8faa7e095ee0
- struct -x o task_struct.group_leader
- struct vcpu_vmx.msr_ia32_feature_control -o 获取到 member 的 offset ?

仅仅打印一个结构体的定义:
ptype /o struct task_struct

## kmem

### kmem -s
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

### kmem -i : 系统的内存信息
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

### kmem -v

> -v  displays the mapped virtual memory regions allocated by vmalloc().

```txt
crash> kmem -v fffffdff6f810000

VMAP_AREA         VM_STRUCT         ADDRESS RANGE
ffff087ee2152c40  ffff087610ed2380  fffffdff6f5f0000 - fffffdff6fff0000
SIZE: 10485760

crash> struct vm_struct ffff087610ed2380

addr   = 0xfffffdff6f5f0000
size   = 10485760
caller = 0xffff8000402f6978 <pcpu_get_vm_areas>
```

## mod

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

## irq

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

## vtop

sudo /nix/store/x8wfndi0l82mjbzwplijbbqglkmlr1y3-crash-utility-unstable-2026-08-18/bin/crash vmlinux

aarch64 一个经典的 vtop 结果解析:
```txt
crash> vtop fffffdff6f810000
VIRTUAL           PHYSICAL
fffffdff6f810000  287f9bc10000

PAGE DIRECTORY: ffff8000413c9000
   PGD: ffff8000413c9fd8 => 12914003 ( pte => pte 中的内容)
   PUD: ffff000012914fe8 => 12915003
   PMD: ffff000012915be0 => 60287f9bc00791
  PAGE: 287f9bc00000  (2MB)

     PTE          PHYSICAL    FLAGS
60287f9bc00791  287f9bc00000  (VALID|RDONLY|SHARED|AF|PXN|UXN)

      PAGE         PHYSICAL      MAPPING       INDEX CNT FLAGS
fffffea1fe4f0400 287f9bc10000 ffff087ef2067ba0      ef9  3 17fffc00008001c uptodate,dirty,lru,swapbacked
(struct page)
```

### 理解 page table 的 walk 过程

```text
PGD: ffff8000413c9fd8 => 12914003
     └─PGD 表项地址       └─表项内容
```

对于 ARM64 4KB 页表：

```text
PGD 基地址：ffff8000413c9000
目标虚拟地址：fffffdff6f810000

PGD index = (VA >> 39) & 0x1ff
          = 0x1fb

表项地址 = ffff8000413c9000 + 0x1fb × 8
         = ffff8000413c9fd8
```

每个页表项占 8 字节，因此 crash 从 `ffff8000413c9fd8` 读取到：

```text
0x0000000012914003
```

将它分解：

```text
0x12914003
          ^^^ 低位属性
0x12914000     下一级页表的物理基地址
```

具体来说：

```text
bit 0 = 1：VALID，有效
bit 1 = 1：TABLE，指向下一级页表
```

所以低两位是 `0b11`，即 `0x3`。地址部分可以近似看成：

```text
0x12914003 & ~0xfff = 0x12914000
```

完整含义就是：

> 内核在 PGD 表的 `ffff8000413c9fd8` 位置找到一个有效的 table descriptor，它指向物理地址 `0x12914000` 的下一级 PUD 页表。

crash 随后通过内核线性映射访问这张物理页表：

```text
物理地址：12914000
内核虚拟地址：ffff000012914000
```

再根据 PUD index 找到：

```text
PUD: ffff000012914fe8 => 12915003
```

注意，`0x12914000` 不是最终数据页的物理地址，只是下一层页表所在的物理地址。
真正的数据物理地址要继续沿 `PUD → PMD` 查找。

### pte

fffffdff6f810000 是 PTE

MMU 先找到这个 2 MiB block，再加上虚拟地址在 block 内的偏移：

fffffdff6f810000 - fffffdff6f800000 = 0x10000

0x287f9bc00000 + 0x10000 = 0x287f9bc10000

这就得到：

fffffdff6f810000 -> 287f9bc10000

也就是 vtop 第一行显示的结果。

### 内核地址：2 MiB 大页与 4 KiB 普通页

下面在 x86_64 virtme live system 中对比两个真实内核地址。这里的“大页”是
内核页表中的 PMD huge mapping，不是 hugetlbfs 分配的用户态 HugeTLB 页面。

不要先复制下面的固定地址。每次启动 VM 后都按照“找候选地址，再用 `vtop`
确认”的顺序操作：

```text
2 MiB 候选：sym _stext -> 复制当次 _stext 地址 -> vtop 地址
4 KiB 候选：lsmod -> 复制第一列 MODULE 地址 -> vtop 地址
```

最终判断依据永远是 `vtop` 的实际 walk，而不是地址来自哪个区域。

#### 如何找到 2 MiB 候选：从 `_stext` 开始

`_stext` 是内建内核文本的起始符号。x86_64 通常尽量使用 2 MiB PMD 映射
内核文本，因此它是最容易获得的候选地址。即使启用了 KASLR，也应先让 crash
用符号表给出本次运行或 vmcore 中的实际地址：

```text
crash> sym _stext
ffffffff81000000 (T) _stext
```

复制当次输出的地址执行 `vtop`：

```text
crash> vtop ffffffff81000000
VIRTUAL           PHYSICAL
ffffffff81000000  1600000

PGD DIRECTORY: ffffffff82a3e000
PAGE DIRECTORY: 3041067
   PUD: 3041ff0 => 3042063
   PMD: 3042040 => 16001a1
  PAGE: 1600000  (2MB)

  PTE    PHYSICAL  FLAGS
16001a1   1600000  (PRESENT|ACCESSED|PSE|GLOBAL)
```

判断它是 2 MiB 大页有三个直接证据：

- 页表遍历在 `PMD` 层停止，没有继续出现 `PTE:` 遍历项。
- crash 明确显示 `PAGE: 1600000 (2MB)`。
- PMD 表项带 `PSE`，在 x86_64 的 PMD 层表示该表项本身映射一个 2 MiB 页面，
  而不是指向下一级 PTE 表。

注意，crash 在后面的最终表项摘要中仍使用通用列名 `PTE`；判断实际终止层级
应看上面的 walk。这里没有 `PTE:` walk 行，并且 PMD 带 `PSE`，所以叶子表项
实际是 PMD。

本例恰好选择 2 MiB 映射的起始地址，所以页内偏移为 0：

```text
虚拟大页基址 = ffffffff81000000
页内偏移     = 0

物理大页基址 = 0x1600000
最终物理地址 = 0x1600000
```

如果自己的 `_stext` 输出没有 `(2MB)` 和 `PSE`，说明该内核没有把这个区域
保留为 PMD 大页，不能因为它是内核文本就硬说它是大页，应另找候选地址。

#### 如何找到 4 KiB 候选：使用 `lsmod` 的 `MODULE` 列

模块同时包含文本和数据，不同区域可能采用不同页大小。运行 `lsmod`：

```text
crash> lsmod
     MODULE       NAME                    TEXT_BASE         SIZE
ffffffffc02010c0  virtio_pci_modern_dev  ffffffffc0400000  24576
ffffffffc020b080  virtio_pci_legacy_dev  ffffffffc0402000  16384
...
```

这里要复制第一列 `MODULE`，不要复制 `TEXT_BASE`。第一列是 `struct module`
地址，可以先验证对象名称：

```text
crash> struct module.name ffffffffc02010c0
  name = "virtio_pci_modern_dev\000...",
```

再用 `kmem -v` 确认它所在的 vmalloc 区域：

```text
crash> kmem -v ffffffffc02010c0
   VMAP_AREA         VM_STRUCT                 ADDRESS RANGE                SIZE
ffff8881015ceee8  ffff8881009e5e40  ffffffffc0201000 - ffffffffc0205000    16384
```

这个 `struct module` 位于 16 KiB 的 vmalloc 区域中。区域总长不是单个页的
大小，是否使用普通页仍要交给 `vtop` 判断：

```text
crash> vtop ffffffffc02010c0
VIRTUAL           PHYSICAL
ffffffffc02010c0  104a700c0

PGD DIRECTORY: ffffffff82a3e000
PAGE DIRECTORY: 3041067
   PUD: 3041ff8 => 3043067
   PMD: 3043008 => 1050f8067
   PTE: 1050f8008 => 8000000104a70163
  PAGE: 104a70000

      PTE         PHYSICAL   FLAGS
8000000104a70163  104a70000  (PRESENT|RW|ACCESSED|DIRTY|GLOBAL|NX)

      PAGE        PHYSICAL      MAPPING       INDEX CNT FLAGS
ffffea0004129c00 104a70000                0        0  1 2ffff0000000000
```

判断它是 4 KiB 普通页的依据是：

- `PMD` 表项没有 `PSE`，它指向物理地址 `0x1050f8000` 的 PTE 表。
- 页表遍历继续到 `PTE: 1050f8008`，由该 PTE 给出物理页基址
  `0x104a70000`。
- `PAGE` 后面没有 `(2MB)` 或 `(1GB)` 标记，因此使用基础页大小；该内核的
  `getconf PAGESIZE` 输出为 `4096`，所以基础页是 4 KiB。

地址换算如下：

```text
虚拟页基址 = ffffffffc0201000
页内偏移   = ffffffffc02010c0 - ffffffffc0201000
           = 0xc0

物理页基址 = 0x104a70000
最终物理地址 = 0x104a70000 + 0xc0
             = 0x104a700c0
```

为什么不能从 `lsmod` 复制 `TEXT_BASE` 来找普通页？当前内核的模块文本恰好
也使用 2 MiB PMD 映射：

```text
crash> vtop ffffffffc0400000
VIRTUAL           PHYSICAL
ffffffffc0400000  105a00000

   PUD: 3041ff8 => 3043067
   PMD: 3043010 => 105a001a1
  PAGE: 105a00000  (2MB)

   PTE     PHYSICAL   FLAGS
105a001a1  105a00000  (PRESENT|ACCESSED|PSE|GLOBAL)
```

这正好说明：`TEXT_BASE`、vmalloc、direct map 等名称只能帮助选择候选地址，
不能代替页表 walk。若一个 `MODULE` 地址没有走到 PTE，就从 `lsmod` 换一个
模块继续验证；当前 virtme 的第一项可以稳定得到 4 KiB 示例。

如果待分析环境没有任何模块，可以运行 `kmem -v`，从较小的 `VMAP_AREA`
中选择 `ADDRESS RANGE` 的起始地址作为 4 KiB 候选，再用 `vtop` 验证。遇到
`not accessible`、`(2MB)` 或 `(1GB)` 就换下一个，直到 walk 出现 `PTE:`。

#### 每次重启后的最短复现流程

地址可能随内核构建、KASLR、模块加载顺序和重启发生变化。只需要重新执行：

```text
crash> sym _stext
ffffffff81000000 (T) _stext
crash> vtop ffffffff81000000
... PAGE: 1600000 (2MB)
... (PRESENT|ACCESSED|PSE|GLOBAL)

crash> lsmod
     MODULE       NAME                    TEXT_BASE         SIZE
ffffffffc02010c0  virtio_pci_modern_dev  ffffffffc0400000  24576
crash> struct module.name ffffffffc02010c0
  name = "virtio_pci_modern_dev\000...",
crash> vtop ffffffffc02010c0
... PTE: 1050f8008 => 8000000104a70163
... PAGE: 104a70000
```

不要记住示例里的十六进制地址，只记住两个入口：`sym _stext` 和 `lsmod` 的
第一列 `MODULE`。

两个例子放在一起时，最实用的判断方法就是观察 walk 在哪一层停止：

```text
2 MiB 大页：PGD/PUD -> PMD(PSE，叶子表项) -> 物理地址
4 KiB 普通页：PGD/PUD -> PMD -> PTE(叶子表项) -> 物理地址
```

### 在 collei virtme 中上手 `vtop`

下面用 x86_64 live system 做一个可以重复的实验：两个进程都映射虚拟地址
`0x700000000000`，但各自写入不同内容。这样可以直接观察到“同一个虚拟地址，
在不同进程页表中对应不同物理页”。

在 host 的第一个终端启动 virtme（QEMU 前台运行，这个终端会被占用）：

```bash
cd ~/data/vn
./collei/scripts/collei-action.py -a run -n virtme
```

等串口显示初始化完成后，在 host 的第二个终端登录 guest：

```bash
cd ~/data/vn
./collei/scripts/collei-action.py -a ssh -n virtme
```

当前 virtme 使用 `~/data/kernel/linux-drm` 构建树。进入 guest 后先确认运行内核
与 `vmlinux` 匹配：

```bash
uname -r
file ~/data/kernel/linux-drm/vmlinux
```

本次实验使用的内核版本是 `7.2.0-00001-geb5a10dc0e00`，`vmlinux` 带有
`debug_info`。分析 vmcore 时也必须使用生成该 vmcore 的同一份 `vmlinux`。

#### 构造两个具有相同虚拟地址的进程

将下面的程序保存为 `/tmp/vtop-demo.c`：

```c
#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	void *requested = (void *)0x700000000000UL;
	char *page;
	const char *label = argc > 1 ? argv[1] : "demo";

	page = mmap(requested, 4096, PROT_READ | PROT_WRITE,
		    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
	if (page == MAP_FAILED) {
		fprintf(stderr, "mmap: %s\n", strerror(errno));
		return EXIT_FAILURE;
	}

	snprintf(page, 4096, "vtop demo: %s", label);
	printf("pid=%d address=%p value=\"%s\"\n", getpid(), page, page);
	fflush(stdout);
	pause();
	return EXIT_SUCCESS;
}
```

编译并运行两个实例：

```bash
gcc -Wall -Wextra -O0 -g -o /tmp/vtop-demo.out /tmp/vtop-demo.c
/tmp/vtop-demo.out A &
/tmp/vtop-demo.out B &
```

本次运行得到：

```text
pid=404 address=0x700000000000 value="vtop demo: A"
pid=405 address=0x700000000000 value="vtop demo: B"
```

PID 每次都会变化，后面的命令要替换为自己实际得到的 PID。

#### 启动 live crash

另开一个 guest SSH 终端，在构建树中执行：

```bash
cd ~/data/kernel/linux-drm
sudo /nix/store/x8wfndi0l82mjbzwplijbbqglkmlr1y3-crash-utility-unstable-2026-08-18/bin/crash vmlinux
```

没有显式指定 dumpfile 时，crash 使用 `/proc/kcore` 分析当前运行的内核。

先故意不指定进程：

```text
crash> vtop -u 700000000000
VIRTUAL     PHYSICAL
700000000000  (not accessible)
```

这是因为 crash 启动后的 current context 是 crash 进程，而它没有映射这个地址。
`-u` 只说明参数是用户虚拟地址，并不负责选择进程。

显式指定 PID 后即可得到两个不同的翻译结果：

```text
crash> vtop -c 404 700000000000
VIRTUAL     PHYSICAL
700000000000  17bbf3000

   PGD: 101bb6700 => 102b50067
   PUD: 102b50000 => 103d45067
   PMD: 103d45000 => 102bd7067
   PTE: 102bd7000 => 800000017bbf3067
  PAGE: 17bbf3000

      PTE         PHYSICAL   FLAGS
800000017bbf3067  17bbf3000  (PRESENT|RW|USER|ACCESSED|DIRTY|NX)

      VMA           START       END     FLAGS FILE
ffff8881097ab800 700000000000 700000001000 100073

crash> vtop -c 405 700000000000
VIRTUAL     PHYSICAL
700000000000  17c224000

   PGD: 109aac700 => 10989a067
   PUD: 10989a000 => 101b10067
   PMD: 101b10000 => 102ba2067
   PTE: 102ba2000 => 800000017c224067
  PAGE: 17c224000

      PTE         PHYSICAL   FLAGS
800000017c224067  17c224000  (PRESENT|RW|USER|ACCESSED|DIRTY|NX)

      VMA           START       END     FLAGS FILE
ffff888104ee9900 700000000000 700000001000 100073
```

结果中最重要的是：

- 两个进程的虚拟地址都是 `0x700000000000`。
- PID 404 的页表将它翻译为物理地址 `0x17bbf3000`。
- PID 405 的页表将它翻译为物理地址 `0x17c224000`。
- `PGD` 起点及后续 `PUD/PMD/PTE` 都不同，因为两个进程有各自的 `mm` 和用户页表。
- `VMA` 表明该地址位于 `[0x700000000000, 0x700000001000)` 这一页匿名映射内。

还可以按物理地址读取内存，闭环验证翻译结果：

```text
crash> rd -p -8 17bbf3000 16
       17bbf3000:  76 74 6f 70 20 64 65 6d 6f 3a 20 41 00 00 00 00   vtop demo: A....
crash> rd -p -8 17c224000 16
       17c224000:  76 74 6f 70 20 64 65 6d 6f 3a 20 42 00 00 00 00   vtop demo: B....
```

其中 `rd -p` 表示把输入地址当作物理地址，`-8` 表示按字节显示。

#### `-c`、`set` 与 `-u` 的关系

以下两种写法等价：

```text
crash> vtop -c 404 700000000000
```

```text
crash> set 404
crash> vtop -u 700000000000
```

可以这样记：

- `-c PID`：仅为本次 `vtop` 指定使用哪个进程的页表。
- `set PID`：切换 crash 的 current context，后续用户地址翻译默认使用该进程的页表。
- `-u`：声明该地址是用户地址；它不是 PID，也不会寻找“地址属于哪个进程”。
- `-k`：声明该地址是内核地址。x86_64 的用户/内核地址范围本身可以区分，所以
  `-u` 和 `-k` 通常可以省略；在地址空间重叠的架构上才是必需的。

对于 vmcore，用法完全相同；区别只是启动 crash 时在 `vmlinux` 后面再传入
`vmcore`，进程和页表状态固定在崩溃时刻。

## dev
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

## 杂项

### task
task -R state,comm,pid,thread_info ffff8080432bf000

### waitq
很简单的功能
https://crash-utility.github.io/help_pages/waitq.html
### files
### ipcs
### swap


### ps
### sys
### vm

### ptov

### rd : 读取内存

展示一个位置上的内存是什么
rd 0xffffa0428fa70000 8


### ps

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
