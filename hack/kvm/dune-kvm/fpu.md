# Enable dune with fpu

- [ ] 如何测试 ? 
- [ ] kvm 为什么需要在 fpu 上做特殊处理 ?
- [ ] 实际上，三个要一起处理.

- /home/maritns3/core/loongson-dune/cross/arch/mips/include/asm/fpu.h:lose_fpu_inatomic
- in dune, process always has fpu enabed.
- /home/maritns3/core/loongson-dune/cross/arch/mips/kvm/callback.c 定义了保存 fpu, msa 和 lasx 相关的函数

- [ ] 如何区分 fpu, msa, lasx 的选项:

- [ ] kvm_mips_handle_exit 中间处理了 cp0 不可用的 exception，那么什么类型的是在 guest 处理，什么是在 host 中间处理 ?

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

- kvm_trap_vz_handle_cop_unusable
- kvm_trap_vz_handle_msa_disabled
- 

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

找到证据:
- [ ] 当 host 进入到 guest 的时候，如果 guest 表示自己可能使用 fpu, 那么 host 一定会保存 fpu ?
- [ ] 如果 guest 表示自己可能使用 fpu, 但是 host 硬件保存的 fpu 并不是自己的，是不是存在什么机制来查看 fpu 是不是属于当前进程 ?
- [ ] 当 guest 使用 fpu, 那么是不是 host 的 process 也是需要标记当前进程为 fpu 不可用的状态 ?
- [ ] 为了防止 guest 中间错误的修改了 host process (比如 fpu) 中间的代码，当 host 检测到 guest 会 fpu 的时候，那么应该首先将 cp1 可用的 exception 关闭掉

- [ ] 因为浮点，向量使用的寄存器重叠，是不是在 enable 一个的时候，马上需要 disable 另一个 ?

kvm_own_fpu:
1. kvm_lose_fpu : 如果发现 MSA 或者 LASX 可用，那么首先保存一下
2. `__kvm_restore_fpu` :

- [ ] kvm_lose_fpu 的逻辑很奇怪啊

kvm_vz_vcpu_put 调用 lose_fpu
