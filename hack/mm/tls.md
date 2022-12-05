## TLS
[A Deep dive into (implicit) Thread Local Storage](https://chao-tic.github.io/blog/2018/12/25/tls)

On x86-64, user land programs (Ring 3) can retrieve FS and GS, but they are not allowed to change the addresses stored in FS and GS. It’s typically the operating systems’ job (Ring 1) to provide facility to manipulate them indirectly.

On the x86-64 kernel, the address stored at FS is managed by a Model Specific Register (MSR) called `MSR_FS_BASE2`. The kernel provides a syscall arch_prctl with which user land programs can use to change FS and GS for the currently running thread.

Digging around a bit more, it can also be seen that when the kernel does context switch, the switch code loads the next task’s FS to the CPU’s MSR. Confirming that whatever FS points to is on a per thread/task basis.

- [ ] so, is fs used for tls both in userland and kernel ? what about gs ?

Based on what we’ve known so far, we may have this hypothesis that the runtime must be using some form of FS manipulation routine (such as arch_prctl) to bind the TLS to the current thread.
And the kernel keeps track of this binding by swap in the right FS value when doing context switch.

This finding confirms our previous hypothesis that the **dynamic linker** runtime allocates and sets up the TCB or struct pthread and then uses `arch_prctl` to bind the TLS to at least the main thread.


Depending on where a TLS variable is defined and accessed, there are four different cases to examine:

- TLS variable locally defined and used within an executable
- TLS variable externally defined in a shared object but used in a executable
- TLS variable locally defined in a shared object and used in the same shared object
- TLS variable externally defined in a shared object and used in an arbitrary shared object

- [ ] skip latter half contents of this article which mostly foucus on how this four cases work. Just details of linker.

- [ ] syscall `get_thread_area` and `set_thread_area` in  `arch/x86/kernel/tls.c` , there is no complete example for it.

[kernel doc](https://www.kernel.org/doc/html/latest/x86/x86_64/fsgs.html)
In 64-bit mode the CS/SS/DS/ES segments are ignored and the base address is always 0 to provide a full 64bit address space. The FS and GS segments are still functional in 64-bit mode.

The FS segment is commonly used to address Thread Local Storage (TLS).

The GS segment has no common use and can be used freely by applications. GCC and Clang support GS based addressing via address space identifiers.

- [ ] https://wiki.osdev.org/Thread_Local_Storage

## 绝对权威的资料: maskray 的 https://maskray.me/blog/2021-02-14-all-about-thread-local-storage
- [ ]  TODO

## 自己的探索

- [ ] 到底修改 mips 中间的 userlocal storage 的时候是哪里 ?

```c
#define _GNU_SOURCE
#include <sched.h>

int clone(int (*fn)(void *), void *stack, int flags, void *arg, ...
         /* pid_t *parent_tid, void *tls, pid_t *child_tid */ );
```

- [ ] clone 的参数 parent_tid 和 child_tid 分别在 parent 和 child 的地址空间中间设置 pid，还有一些实现 futex wake 的机制[^1]

- [x] tls

- 设置 task_thread_info-> tp, 两个位置，都是架构相关的代码
  - copy_thread_tls 
  - set_thread_area 
- 更新硬件寄存器，在 context_switch 中间

```c
SYSCALL_DEFINE1(set_thread_area, unsigned long, addr)
{
	struct thread_info *ti = task_thread_info(current);

	ti->tp_value = addr;
	if (cpu_has_userlocal)
		write_c0_userlocal(addr);

	return 0;
}

/*
 * For newly created kernel threads switch_to() will return to
 * ret_from_kernel_thread, newly created user threads to ret_from_fork.
 * That is, everything following resume() will be skipped for new threads.
 * So everything that matters to new threads should be placed before resume().
 */
#define switch_to(prev, next, last)					\
do {									\
	__mips_mt_fpaff_switch_to(prev);				\
	lose_fpu_inatomic(1, prev);					\
	if (tsk_used_math(next))					\
		__sanitize_fcr31(next);					\
	if (cpu_has_dsp) {						\
		__save_dsp(prev);					\
		__restore_dsp(next);					\
	}								\
	if (cop2_present) {						\
		set_c0_status(ST0_CU2);					\
		if ((KSTK_STATUS(prev) & ST0_CU2)) {			\
			if (cop2_lazy_restore)				\
				KSTK_STATUS(prev) &= ~ST0_CU2;		\
			cop2_save(prev);				\
		}							\
		if (KSTK_STATUS(next) & ST0_CU2 &&			\
		    !cop2_lazy_restore) {				\
			cop2_restore(next);				\
		}							\
		clear_c0_status(ST0_CU2);				\
	}								\
	__clear_r6_hw_ll_bit();						\
	__clear_software_ll_bit();					\
	if (cpu_has_userlocal)						\
		write_c0_userlocal(task_thread_info(next)->tp_value);	\
	__restore_watch(next);						\
	(last) = resume(prev, next, task_thread_info(next));		\
} while (0)
```


[^1]: https://stackoverflow.com/questions/6975098/when-is-the-system-call-set-tid-address-used
