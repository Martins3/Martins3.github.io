# Enable dune with fpu

lazy mode 其实非常的麻烦, 默认是 eager mode, 为此 x86 添加专门的指令来处理 FPU save.[^1]

- [ ] 如何测试 ? 
- [ ] kvm 为什么需要在 fpu 上做特殊处理 ?
- [ ] 实际上，三个要一起处理.

- /home/maritns3/core/loongson-dune/cross/arch/mips/include/asm/fpu.h:lose_fpu_inatomic
- in dune, process always has fpu enabed.
- /home/maritns3/core/loongson-dune/cross/arch/mips/kvm/callback.c 定义了保存 fpu, msa 和 lasx 相关的函数

- [ ] 如何区分 fpu, msa, lasx 的选项:

## 设计方案
- fpu lazy 机制 kvm 已经实现好了，如果 guest 进入之后，没有再次使用 fpu, 那么 fpu 就不需要被加载了。
- 所以，只需要在进入的时候，设置好 fpu,  msa 和 lasx 相关的寄存器即可。

## lazy load, eager store
在 switch_to 中间总是会调用 lose_fpu_inatomic, 所以上一个 process 的环境总是保存的，但是下一个 process 则不一定。 其他的调用 lose_fpu 的位置:
```c
void start_thread(struct pt_regs * regs, unsigned long pc, unsigned long sp)
int arch_dup_task_struct(struct task_struct *dst, struct task_struct *src)
```

所以， 从 guest 进入到 host 中间的时候，host 中间的 fpu 并不会主动加载


## kvm_lose_fpu && kvm_vcpu_arch:aux_inuse
kvm_arch_vcpu_ioctl_run 调用 lose_fpu(1) 导致当前 host 的 fpu 被保存下来, 而程序结束的时候，调用 `vcpu_put` 将 guest 的 fpu 进行保存。
将 host 的 fpu 进行保存，并且设置当前的 host fpu 不可用。注意和 kvm_lose_fpu 相区分, 整个 kvm 模块仅仅在此处调用过一次 lose_fpu, 其余的情况都是
调用 kvm_lose_fpu 来进行 fpu 的保存。

kvm_vz_vcpu_put 调用 lose_fpu, 其意义在于，当 vcpu 使用物理 cpu 被换出的时候，因为物理 fpu 中间保存的是 guest 的内容，
所以，如果 guste 使用物理 fpu, 那么需要保存。 通过 vcpu_put 机制，可以保证，guest 如果使用了 fpu , 总是可以被保存下来的。

而决定 guest 的保存则是看 kvm_vcpu_arch:aux_inuse 的。

向 kvm_vcpu_arch:aux_inuse 插入 flag 位置是
1. kvm_own_msa
2. kvm_own_lasx
3. kvm_own_fpu

向 kvm_vcpu_arch:aux_inuse 去掉 flag 位置是
1. kvm_lose_fpu


而这几个 own 函数则是被调用:
- kvm_trap_vz_handle_cop_unusable
- kvm_trap_vz_handle_msa_disabled
- kvm_trap_vz_handle_lasx_disabled

## fpu/msa/lasx exception
```c
#define KVM_MIPS_AUX_FPU	0x1
#define KVM_MIPS_AUX_MSA	0x2
#define KVM_MIPS_AUX_LASX	0x4

/* Save and disable FPU & MSA & LASX */
void kvm_lose_fpu(struct kvm_vcpu *vcpu)
{
	/*
	 * With T&E, FPU & MSA & LASX get disabled in root context (hardware) when it
	 * is disabled in guest context (software), but the register state in
	 * the hardware may still be in use.
	 * This is why we explicitly re-enable the hardware before saving.
	 */

/*
 * It would be nice to add some more fields for emulator statistics,
 * the additional information is private to the FPU emulator for now.
 * See arch/mips/include/asm/fpu_emulator.h.
 */


#if defined(CONFIG_CPU_HAS_LASX)
# define FPU_REG_WIDTH	256
#elif defined(CONFIG_CPU_HAS_MSA)
# define FPU_REG_WIDTH	128
#else
# define FPU_REG_WIDTH	64
#endif

union fpureg {
	__u32	val32[FPU_REG_WIDTH / 32];
	__u64	val64[FPU_REG_WIDTH / 64];
};

struct mips_fpu_struct {
	union fpureg	fpr[NUM_FPU_REGS];
	unsigned int	fcr31;
	unsigned int	msacsr;
};
```

```c
/*
 * Return value is in the form (errcode<<2 | RESUME_FLAG_HOST | RESUME_FLAG_NV)
 */
int kvm_mips_handle_exit(struct kvm_run *run, struct kvm_vcpu *vcpu)
{
	case EXCCODE_CPU:
		kvm_debug("EXCCODE_CPU: @ PC: %p\n", opc);

		++vcpu->stat.cop_unusable_exits;
		ret = kvm_mips_callbacks->handle_cop_unusable(vcpu);
		/* XXXKYMA: Might need to return to user space */
		if (run->exit_reason == KVM_EXIT_IRQ_WINDOW_OPEN)
			ret = RESUME_HOST;
		break;
	case EXCCODE_LASX:
		if ((((vcpu->arch.host_cp0_gscause &
				 0x7c) >> 2) & 0x1f) == 7) {
			++vcpu->stat.lasx_disabled_exits;
			ret = kvm_mips_callbacks->handle_lasx_disabled(vcpu);
		} else
			kvm_err("Unhandled GSEXC %x @ %lx\n",
			 (((vcpu->arch.host_cp0_gscause) & 0x7c) >> 2), vcpu->arch.pc);
		break;

	case EXCCODE_MSADIS:
		++vcpu->stat.msa_disabled_exits;
		ret = kvm_mips_callbacks->handle_msa_disabled(vcpu);
		break;
```

- [x] 触发上面三种错误的原因是 : guest 允许使用 fpu / msa / lasx, 但是 host 不可以。证据在 Virtulization Manual 4.9.4 中间。


kvm_own_fpu:
1. kvm_lose_fpu : 如果发现 MSA 或者 LASX 可用，那么首先保存一下
2. `__kvm_restore_fpu`



[^1]: https://tthtlc.wordpress.com/2016/12/17/understanding-fpu-usage-in-linux-kernel/
