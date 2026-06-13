# 分析那些 CPU flags 会操作系统操作

- 那些地方修改了这些玩意儿？

- show_cpuinfo : 这里
```c
	for (i = 0; i < 32*NCAPINTS; i++)
		if (cpu_has(c, i) && x86_cap_flags[i] != NULL)
			seq_printf(m, " %s", x86_cap_flags[i]);
```

set_cpu_cap

- get_cpu_cap 总的初始化位置
  - 三个位置上调用？而是是 AMD 机器？

- [ ]

- 如何理解什么是 cpuid_leafs

## 关键文档
### /home/martins3/core/linux/Documentation/x86/cpuinfo.rst

If the expected flag does not appear in /proc/cpuinfo, things are murkier.
Users need to find out the reason why the flag is missing and find the way
how to enable it, which is not always easy. There are several factors that
can explain missing flags:
1. the expected feature failed to enable
2. the feature is missing in hardware
3. platform firmware did not enable it
5. the feature is disabled at build or run time
6. an old kernel is in use, or the kernel does not support the feature and thus has not enabled it.


## /home/martins3/core/linux/Documentation/virt/kvm/x86/cpuid.rst

## tools/arch/x86/kcpuid

来调用 cpuid 指令来获取 cpuid 的:

shows features which the kernel supports. For a full list of CPUID flags
which the CPU supports, use tools/arch/x86/kcpuid.

- [ ] 但是为什么 /proc/cpuinfo 获取到的 flags 反而更多啊
参考 ./kcpuid-flags.txt ./cpuinfo-flags.txt 中的内容
