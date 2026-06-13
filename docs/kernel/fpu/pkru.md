# pkru

pkru 为什么会特殊处理来着?

## 如何使用
- [x] https://man7.org/linux/man-pages/man7/pkeys.7.html

仅仅是看 man 机制就是不错的了，主要不是作为一个安全机制的，
而是对于传统的 READ WRITE EXEC 机制的扩展。

- [ ] https://liujunming.top/2020/03/07/Introduction-to-pkeys/

似乎基本的实现就是这样的
```c
static inline void wrpkru(unsigned int pkru)
{
	unsigned int eax = pkru;
	unsigned int ecx = 0;
	unsigned int edx = 0;

	asm volatile(".byte 0x0f,0x01,0xef\n\t"
		     :
		     : "a"(eax), "c"(ecx), "d"(edx));
}

int pkey_set(int pkey, unsigned long rights, unsigned long flags)
{
	unsigned int pkru = (rights << (2 * pkey));
	return wrpkru(pkru);
}

int pkey_mprotect(void *ptr, size_t size, unsigned long orig_prot,
		  unsigned long pkey)
{
	return syscall(SYS_pkey_mprotect, ptr, size, orig_prot, pkey);
}


int pkey_free(unsigned long pkey)
{
	return syscall(SYS_pkey_free, pkey);
}
```

https://github.com/bminor/glibc/blob/master/sysdeps/unix/sysv/linux/x86/arch-pkey.h

的确是那么回事，就是读写的 pkru 寄存器即可。

- [ ] https://docs.kernel.org/core-api/protection-keys.html
- [ ] https://taesoo.kim/pubs/2023/park:mpkfacts.pdf
- [ ] https://charlycst.github.io/posts/mpk/

## 和 xsave 的关系

至少在 qemu 的 x86_cpu_xrstor_all_areas 中看，这个也是区域中的一个:

```c
    e = &x86_ext_save_areas[XSTATE_PKRU_BIT];
    if (e->size && e->offset) {
        const XSavePKRU *pkru;

        pkru = buf + e->offset;
        memcpy(&env->pkru, pkru, sizeof(env->pkru));
    }
```

## 虚拟机中到底支持 pkru 么?

## 为什么 pkru 是放到 fpu 中保存切换上下文的
> [!NOTE]
> 参考 Deepseeek ，有待验证

PKRU（Protection Key Rights Register for Userspace）：
是 x86 的内存保护密钥（Memory Protection Keys, MPK）机制的一部分。
虽然 PKRU 在逻辑上属于 FPU 状态（因为它是通过 XSAVE/XRSTOR 保存的），但出于安全和性能考虑，KVM 单独处理 PKRU 的切换。

在 VM-Entry/VM-Exit 时单独切换 PKRU：
即使 FPU 状态整体未切换（例如 lazy FPU 优化），PKRU 也会被显式保存和恢复。

这是因为 PKRU 控制内存访问权限，若错误地保留宿主机 PKRU 值，可能导致客户机访问非法内存或泄露信息。
guest_fpstate 包含客户机 FPU，但 PKRU 仍是宿主机的：
这句话有点反直觉，但意思是：guest_fpu 结构体中保存的是客户机的 FPU 寄存器值（如 XMM、YMM 等），但其中的 PKRU 字段并未更新为客户机的值，而是仍保留宿主机的 PKRU。
实际客户机的 PKRU 值由 KVM 单独维护（通常在 vcpu->arch.pkru 或类似字段中），并在 VM-Entry 时写入 CPU 的 PKRU 寄存器。


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
