# Loongarch : Make MIPS Great Again

当其中的数值
```
➜  mips git:(dune) ✗ cloc .
    1973 text files.
    1968 unique files.
     254 files ignored.

github.com/AlDanial/cloc v 1.82  T=1.23 s (1393.0 files/s, 316854.3 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              786          28472          32424         148330
C/C++ Header                   767          15374          25471         124609
Assembly                        57           1363           3794           8723
make                           108            393            578           1376
Bourne Shell                     1              6             37             45
-------------------------------------------------------------------------------
SUM:                          1719          45608          62304         283083
-------------------------------------------------------------------------------

➜  arch git:(dune) ✗ cloc loongarch
     733 text files.
     707 unique files.
     222 files ignored.

github.com/AlDanial/cloc v 1.82  T=0.42 s (1289.9 files/s, 246246.2 lines/s)
-------------------------------------------------------------------------------
Language                     files          blank        comment           code
-------------------------------------------------------------------------------
C                              123           4918           4373          26232
C/C++ Header                   235           2879           4055          14750
Assembly                        29            567           1491           3334
DOS Batch                       31             28              0           1298
make                            19            167            167            496
-------------------------------------------------------------------------------
SUM:                           540           8559          10086          84445
-------------------------------------------------------------------------------
```
代码量只有以前的一个零头了 !

## Kernel
- [ ] 分析一下内核的启动和依赖的驱动

syscall 的处理流程:
- traps.c:trap_init 中间初始化
- genex.S:handle_sys_wrap 是最开始的入口
- 在 arch/loongarch/kernel/scall64-64.S 定义 syscall 的处理, 和常规函数感觉其实没有什么区别

```asm
NESTED(handle_sys_wrap, 0, sp)
	la.abs	t0, handle_sys
	jirl    zero, t0, 0
	END(handle_sys_wrap)
```

- [ ] 为什么使用 handle_sys_wrap 进行间接跳转
- [ ] kernelsp 如何使用的

关于 exception 的实现:
- build_tlb_refill_handler :  将代码拷贝到 refill_ebase 中间
- configure_exception_vector : ebase 和 refill_ebase 写入到 csr 中间
- trap_init : refill_ebase = ebase, 调用各种 handler 设置

从内核代码 和 csr 寄存器数值分析，vector 的距离是 512
需要分配的空间是: ( size = (64 + 14) * vec_size;)

## abi
- [ ] 分析手册
  - [ ] 几种 la 和 la.abs 等之类的指令的区别是什么?

## 手册分析
- [ ] 手册中的 typo 和清楚的
  - [ ] 中断 ecode + 64
  - [ ] mcsr

- [ ] IMCTL1 的作用是什么?
- [ ] IMCTL2 的作用是什么?

- [ ] 在什么情况下 ERTN 需要控制 LLBCTL 的位置
  - [ ] LLBCTL 的 ROLLB 返回当前 LLBit 的值, 内核的测试中间显示，每次返回的都是 1
  
一共三个 exception 入口: TLB, general, reset

中断信号被采样到 CSR.ESTA.IS 和 CSR.ECFG.LIE, 得到 13 bit 的中断向量

GTLBC 来控制 guest MTLB 的数量，只是为了调节性能参数吧

LA 中间取消掉了基本函数 arch/loongarch/include/asm/asm.h


在 arch/loongarch/include/asm/loongarchregs.h
中间定义了一堆 IOCSR 寄存器，IOCSR 只是用于各种地址空间的。
- [x] 如果不正确设置 IOCSR，对于 dune 的影响是什么?
  - 因为外设本身不需要我们关系，所以应该没有什么影响
- [x] LOONGARCH_CSR_TMID 和 LOONGARCH_IOCSR_TIMER_CFG 的关系是什么?
 - 应该是 node 的时钟 和 本地时钟的关系吧


在 kvm_vz_vcpu_setup 只是设置的 `vcpu->arch.csr`, 但是 kvm_vz_set_one_reg 中间是直接写入到 gcsr 中间的
即使是在 kvm_vz_set_one_reg 中间，有时候也是写入 `vcpu->arch.csr` 中间的

- [x] 在 kvm_own_fpu 中间， 读取的是 `vcpu->arch.csr` , 但是 kvm_vz_set_one_reg 中间写入的是直接到 cpu 中间的
  - [x] 因为 arch/loongarch/include/asm/kvm_host.h 下 macro 写的太混乱，这个实际上是还是读去虚拟机的状态

- [x] 很多状态的书写，其实没有必要注入到 Guest 中间, 而是保存在 `vcpu->arch.csr` 中间了

很多微结构控制寄存器都是不需要设置，下面的是直接跳过了:
```c
#define INIT_VALUE_PRCFG1 0x72f8
// TLB 支持的页大小 [12, 29], 从 4k 到 512M 的大小
#define INIT_VALUE_PRCFG2 0x3ffff000
// TLB 的物理参数
#define INIT_VALUE_PRCFG3 0x8073f2
```

一些特殊的CSR:
- 有些 CSR 不会注入到 guest 中间的
- PRCFG1 之类的是常量

1.4.4 / 1.4.5 软中断 和 硬中断的描述
HWIC : 用 host 的硬件撤销清除 Guest 中断
HWIP : 直接相连
HWIS : 中断注入
其实就是中断可以直接注入，也可以完全转发，或者部分转发

STLB 和 MTLB 的共享
- [x] 如果在 guest 中间设置的 STB 的 size 和 host 不同, 两者还可以共享吗?(是的，所以保证 STLBSZ 相同)
- [x] GID 的使用逻辑 1.2.3 中描述
- [x] VMM page : gpa 到 hpa 映射也是放到 TLB 中间的

问题是靠什么区分 VMM page 和 host page 的 TLB
- gpa 和 hva 的作为索引根本无法区分两者
- 猜测在 guest mode 中间，当进行使用 Host 的, TLB refill 是自动的，并且自动使用 pgd.mm 的内容

TRGP 是虚拟化扩展指令:
TLBRD : TLBRD 是根据 Index 获取 TLB 的信息, 将 GuestID 相关信息放到此处，实际上 TRGP 并没有在 kvm 中使用上。

为什么 TLBRPRMD 中间含有 PGM，但是在 PRMD 中间没有
1.7.1 : 这个记录了之前是否是客户机，如果是，那么就执行 eret 恢复客户机的状态 当 eret 的时候，硬件用于区分当前是否在 Host 的状态 还是 Guest 的

- 为什么存在两个 PS : TLBHI 和 TLNINDEX
- 如果是 TLB refill，那么在 TLBHI 中间处理

- [x] 之前 dune 实现中没有区分 STLB 和 MTLB 的，为什么可以正常工作的啊?
- 因为填写的首先指定了大小, 然后可以自动忽视 STLB

GSTAT::GID 和 GTLBC::TGID 的关系:
1. 一个是设置给虚拟机的 GID
2. 一个是如果想要在 host 中控制虚拟机的 TLB, 那么需要提前设置该数值

5.2 直接翻译模式 和 映射翻译模式, 在映射翻译模式下，又存在直接映射窗口
- 当 MMU 处于直接翻译模式的时候，所有的指令都是按照 CRMD 决定的
- 当 MMU 处于非映射模拟，如果在直接映射窗口，那么按照窗口，否则按照通过页表项。 所以，使用一致缓存就好了
  - 页表项中间的 TLB 的 MAT 从页表项中间获取
- 从 CRMD 的说明看，还是进入到映射模式，看来直接映射模式是给机器重启使用的
- [x] 在 TLB refill 的时候，会自动进入到 直接翻译模式 吗?
  - 确实，

- [x] TLBRERA / TLBRPRMD 按道理来说都是只读信息才对啊, 或者根本没有必要通过 csr 寄存器暴露出来
- [x] 为什么 TLB refill exception 需要单独保存 和 返回地址
- [x] 那些 TLB refill 在需要处理这些，那么，请问，走普通入口的需要这些吗 ?

- [x] 为什么需要设计成为两种 TLB 啊(历史原因)


1. LOONGARCH_CSR_EBASE : 0xc
2. LOONGARCH_CSR_TLBREBASE : 0x88
3. LOONGARCH_CSR_ERREBASE : 0x93
ERREBASE 入口现在的内核并没有注册

TLS 寄存器就是放到 reg[2]

- TLBIDX bit 位置 NE 和 TLBRERA 的关系。

PGDL 和 PGDH : 提供给 GPD 的两个地址
PWCL 和 PWCH : 描述 pagewalk 的地址，虚拟地址用于在各个级别进行所以的位宽和开始范围

badvaddr 从哪里找，取决于是否是 TLB refill exception 的 PGD 的取值取决于是否出错的地址
### 向量指令
3.1.3 : IR 只读, CSR : 浮点 eflags, FCCR : 控制寄存器 

两个配置寄存器的作用:
- [ ] KVM_REG_LOONGARCH_VIR
- [ ] KVM_REG_LOONGARCH_FCR_IR

MCSR 的作用是什么?
```c
#define INIT_VALUE_MCSR0 0x3f2f2fe0014c010
#define INIT_VALUE_MCSR1 0xfcff007ccfc7
#define INIT_VALUE_MCSR2 0x1000105f5e100
#define INIT_VALUE_MCSR3 0x7f33
#define INIT_VALUE_MCSR8 0x608000300002c3d
#define INIT_VALUE_MCSR9 0x608000f06080003
#define INIT_VALUE_MCSR10 0x60e000f
#define INIT_VALUE_MCSR24 0xe
```
IMPCTL
```c
#define INIT_VALUE_IMPCTL1 0x343c3
#define INIT_VALUE_IMPCTL2 0x0
```

## kvm
进行 gpa 到 gpa 之间的映射，一致都是 kvm 在维护的, 类似于 ept 的 page table, 这里也存在一个 `kvm->arch.gpa_mm.pgd`

从手册上看 gintc 可以将中断直接连到 glibc 中间，但是 kvm 并没有将这个事情利用上:
1. 设置 KVM_CSR_ESTAT 的作用:
		case KVM_CSR_ESTAT:
		write_gcsr_estat(v);
		write_csr_gintc((v & 0x3fc) >> 2);
		
2. 在 vcpu_load 和 vcpu_set 的时候存在一些作用
	- kvm_loongarch_deliver_interrupts

分析完成 kvm_loongarch_deliver_interrupts 的调用之后, 目前的 kvm 实际上并没有用上这些的高级功能，只是可以通过 ioctl 将中断注入到
其中的方法。

- [ ] kvm/entry.S : 从多个层次都看过一遍了，应该整理一下


3. kvm_vz_vcpu_load 中间分析了到底那些 csr 寄存器实际上对于 guest 是有用的
  - 对于 timer 的设置比较复杂

- [ ] 难道 guest 中间无法使用 load / store 监视吗, 应该是可以的
  - [ ] 似乎只是现在的 kvm 对于这个事情没有增加任何支持的 
  - [ ] 所以 load / store 都是最近才支持的吗 ?

- [ ] STLBSZ 为什么没有出现 vcpu_load 和 set 中间
  - [ ] 设置 STLBSZ 的语义是什么?

- [ ] kscratch 寄存器实际上正确的设置了, 没有考虑到偏移量

```c
/*
 * Common Vectored Interrupt code
 * Complete the register saves and invoke the handler which is passed in $v0
 */
NESTED(except_vec_vi_handler, 0, sp)
	SAVE_TEMP
	SAVE_STATIC
	CLI
#ifdef CONFIG_TRACE_IRQFLAGS
	move	s0, v0
	TRACE_IRQS_OFF
	move	v0, s0
#endif

	LONG_L	s0, tp, TI_REGS
	LONG_S	sp, tp, TI_REGS

	/*
	 * SAVE_ALL ensures we are using a valid kernel stack for the thread.
	 * Check if we are already using the IRQ stack.
	 */
	move	s1, sp # Preserve the sp

	/* Get IRQ stack for this CPU */
	csrrd	t0, LOONGARCH_CSR_TMID
	la		t1, irq_stack
	LONG_SLL	t0, t0, LONGLOG
	LONG_ADDU	t1, t1, t0
	LONG_L		t0, t1, 0

	# Check if already on IRQ stack
	PTR_LI		t1, ~(_THREAD_SIZE-1)
	and		t1, t1, sp
	beq		t0, t1, 2f

	/* Switch to IRQ stack */
	li		t1, _IRQ_STACK_START
	PTR_ADDU	sp, t0, t1

	/* Save task's sp on IRQ stack so that unwinding can follow it */
	LONG_S		s1, sp, 0
2:
	jirl    	ra, v0, 0

	/* Restore sp */
	move	sp, s1

	la	t0, ret_from_irq
	jirl    zero, t0, 0
	END(except_vec_vi_handler)
```

```diff
History: #0
Commit:  2b7f254f25d5cd37c3be2a535b2c6b693165ce1d
Author:  Wang liupu <wangliupu@loongson.cn>
Date:    Thu 08 Apr 2021 08:02:49 PM CST

LoongArch:support percpu

1.kernelsp and irq_stack support percpu
2.move set_saved_sp place in head.S

Change-Id: Ibe640a7076b5891827356cc800280e9c7757fc65
Signed-off-by: Wang liupu <wangliupu@loongson.cn>

diff --git a/arch/loongarch/kernel/genex.S b/arch/loongarch/kernel/genex.S
index 1c7e7d53820a..8c656910d349 100644
--- a/arch/loongarch/kernel/genex.S
+++ b/arch/loongarch/kernel/genex.S
@@ -84,10 +84,8 @@ NESTED(except_vec_vi_handler, 0, sp)
 	move	s1, sp # Preserve the sp
 
 	/* Get IRQ stack for this CPU */
-	csrrd	t0, LOONGARCH_CSR_TMID
 	la		t1, irq_stack
-	LONG_SLL	t0, t0, LONGLOG
-	LONG_ADDU	t1, t1, t0
+	LONG_ADDU	t1, t1, $r21
 	LONG_L		t0, t1, 0
 
 	# Check if already on IRQ stack
```
似乎将 $r21 作为内核的 percpu 使用了!

## 参考资料
- [编译器版本维护列表](http://sysdev.loongson.cn/projects/compiler/wiki)
- [abi](http://10.20.5.50/xch/loongarch_psabi/loongarch_psabi/loongarch_psabi.html)
- [kernel](kernel link)

Watch https://github.com/loongson-community/docs,  don't release the notes until it does it firstly.
