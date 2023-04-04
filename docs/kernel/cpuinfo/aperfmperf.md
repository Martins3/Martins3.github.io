# aperfmperf

```c
#define X86_FEATURE_APERFMPERF		( 3*32+28) /* P-State hardware coordination feedback capability (APERF/MPERF MSRs) */
```

通过这个 feature 来获取 CPU 的频率的:

相关的初始化位置: arch/x86/kernel/cpu/aperfmperf.c

## 其实现在的也不准

## aperfmperf 是无法透传给 vCPU 的
从 guest 中获取的 /proc/cpuinfo 完全是错的
```txt
➜  ~ cat /proc/cpuinfo | grep MHz
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
cpu MHz         : 2995.200
```
