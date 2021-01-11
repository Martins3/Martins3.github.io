# MIPS arch checksheet


## coprocessor 0

### Status
[^1]59

```
|31    28|27|26|25  24| 23 | 22 | 21| 20 | 19  |18  16| 15    8 | 
+--------+--+--+------+----+----+---+-----------------+---------+--------------------------------------+
| CU3-CU0|0 |FR|  0   | PX | BEV| 0 | SR | NMI |   0  | IM7-IM0 | KX | SX | UX | KSU | ERL | EXL | IE  |
+--------+--+--+------+----+----+---------------------+------------------------------------------------+
```
- IE : 全局中断使能位，1 表示开中断
- EXL : 1 表示 CPU 处于异常模式，即 复位，NMI 和 Cache 错误之外的某种异常
- ERL : 1 表示 CPU 处于错误模式，即 复位，NMI 和 Cache 错误中的某种异常
- KSU : 0 内核态，1 管理态，2 用户态，3 未定义

- [ ] 后面都抄一遍，还是很容易理解的

龙芯启动对于 status 的设置分为两个截断:
1. setup_c0_status ST0_KX|ST0_MM [^1]P59
2. trap_init => per_cpu_trap_init => configure_status => change_c0_status [^1]P85


### Context
清零是为了保证初始值合法 [^1]P57

## virtual address space

[^1]P81~P82

对于 32 位空间，虚拟地址的高位来标志类型，最高两位 00 或者 11 表示缓存+分页，最高三位 100 缓存但是不分页，101 表示 不缓存但是分页。
```
                   +---------------------> ------------------+
                   |                      |   CKSEG2         |
                   |                      |   CKSEG1         |
                   |                      |   CKSEG0         |
                   |  +--------------------------------------+
                   |  |                   |   XKSEG          |
                   |  |                   +------------------+
+------------------+  |                   |                  |
|     KSEG2(1G)    |  |                   |   XKPHYS         |
+------------------+  |                   |                  |
|     XSEG1(512M)  |  |                   +------------------+
+------------------+  |                   |                  |
|     KSEG0(512M)  |  |                   |   XSSEG          |
+---------------------+                   |                  |
|     USEG(2G)     |  |                   +------------------+
+------------------+  |                   |                  |
                   |  |                   |   XUSEG          |
                   |  +--------------------------------------+
                   |                      |   CUSEG          |
                   +----------------------+------------------+
```

- XKPHYS 仅仅在内核态可以访问，**不分页**，是否缓存由地址中间的 59-61 来决定(值为2表示不缓存，3表示缓存，7表示写合并)
- XKSEG 既缓存，又分页。
- XUSEG 最大可以扩展到 4EB 空间，当前龙芯实现 48 bit 空间，也就是 256 TB


在缺省的情况下，龙芯 3 号支持 44 位物理地址，但对于多芯片互联的 CC-NUMA 系统来说，NUMA 节点的编号被编入内存地址的第 44 ~ 47 位, 因此总共需要 48 位物理地址。[^1]58
> - [ ] 有点问题啊!
>   - [ ] 代码在缺省情况下，使用的是 48 位, 根本没有探测当前系统是不是 CC-NUMA
>   - [ ] 好家伙，是支持 48 位的物理地址吗 ?
>   - [ ] 可不可以让 44-47 处理 NUMA，从而可以实现虚拟地址和物理地址的范围完全相同


## tlbex
```
 History:        #0
 Commit:         380cd582c08831217ae693c86411902e6300ba6b
 Author:         Huacai Chen <chenhc@lemote.com>
 Committer:      Ralf Baechle <ralf@linux-mips.org>
 Author Date:    Thu 03 Mar 2016 09:45:12 AM CST
 Committer Date: Fri 13 May 2016 08:02:15 PM CST

 MIPS: Loongson-3: Fast TLB refill handler

 Loongson-3A R2 has pwbase/pwfield/pwsize/pwctl registers in CP0 (this
 is very similar to HTW) and lwdir/lwpte/lddir/ldpte instructions which
 can be used for fast TLB refill.
```

MIPS64-III-Priviledge :

- [ ] lddir : get pte specified by `PWCtl` `PWBase` `PWField`, `PWSize` and pgd pointer
  - [ ] PWBase : *virtual*  base directy address 
  - loongson manual says : load pte as PWField and PWsize indicates

- [ ] build_loongson3_tlb_refill_handler
- [ ] kmap

## gpr
-  在内核汇编中间见到 $28, 那就是 thread_union / thread_info

## syscall
[^1]P135

- [ ] 使用那些寄存器传递参数的
- [ ] syscall 是 interrupt 的一部分而已


## context switch
[^1]P363

- [ ] kernelsp 数组是怎么回事 ?
    - 应该是类似于 x86 中间的 TSS 寄存器，用于*切换到内核态的时候*初始化内核 stack
- [ ] 找到 pt regs 的定义
- [ ] 检查一下其中 save regs 相关的代码

## interrupt && exception

[^1]: 用"芯"探核 基于龙芯的 Linux 内核探索解析
