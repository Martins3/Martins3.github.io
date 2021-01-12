## interrupt && exception

- [ ] 检查一下 ebase 在其中的作用

- [ ] 可以调整 ebase 的位置吗 ?


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

- [ ] context switch 中间，地址空间的切换 cr3 在 ?


[^1]: 用"芯"探核 基于龙芯的 Linux 内核探索解析
[^2]: See Mips Run 2nd Edition
