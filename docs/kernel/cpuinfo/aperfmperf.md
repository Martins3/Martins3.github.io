# aperfmperf

```c
#define X86_FEATURE_APERFMPERF		( 3*32+28) /* P-State hardware coordination feedback capability (APERF/MPERF MSRs) */
```

通过这个 feature 来获取 CPU 的频率的:

相关的初始化位置: arch/x86/kernel/cpu/aperfmperf.c
