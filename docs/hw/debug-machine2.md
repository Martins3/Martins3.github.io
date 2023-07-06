
似乎 amd 是知道自己有 bug 的
```c
/*
 * AMD Erratum 400 aware idle routine. We handle it the same way as C3 power
 * states (local apic timer and TSC stop).
 *
 * XXX this function is completely buggered vs RCU and tracing.
 */
static void amd_e400_idle(void)
{
	/*
	 * We cannot use static_cpu_has_bug() here because X86_BUG_AMD_APIC_C1E
	 * gets set after static_cpu_has() places have been converted via
	 * alternatives.
	 */
	if (!boot_cpu_has_bug(X86_BUG_AMD_APIC_C1E)) {
		default_idle();
		return;
	}

	tick_broadcast_enter();

	default_idle();

	tick_broadcast_exit();
}
```



```c
/*
 * Check if the CPU can handle C2 and deeper
 */
static inline unsigned int acpi_processor_cstate_check(unsigned int max_cstate)
{
	/*
	 * Early models (<=5) of AMD Opterons are not supposed to go into
	 * C2 state.
	 *
	 * Steppings 0x0A and later are good
	 */
	if (boot_cpu_data.x86 == 0x0F &&
	    boot_cpu_data.x86_vendor == X86_VENDOR_AMD &&
	    boot_cpu_data.x86_model <= 0x05 &&
	    boot_cpu_data.x86_stepping < 0x0A)
		return 1;
	else if (boot_cpu_has(X86_BUG_AMD_APIC_C1E))
		return 1;
	else
		return max_cstate;
}
```

## 我靠，原来 widndows 也是可以修改，但是也没用
- https://superuser.com/questions/1640355/setting-max-c-state-in-windows-10

- 完全不可用啊，但是也许是因为没有使用 sudo 执行

https://superuser.com/questions/121883/any-way-to-disable-specific-cpu-idle-cx-states
