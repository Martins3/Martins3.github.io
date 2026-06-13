## 如何快速知道 CPUID 的含义

intel :

Intel SDM Table 1-17. Information Returned by CPUID Instruction

amd : 似乎没有一个表格，文档也不是完全自洽的，例如
`Fn8000_0022_EBX[LbrStackSize]` ，其中的 LbrStackSize 的定义就是找不到的。

## TOPO 相关的
```txt
🧀  diff 13900k-core1.cpuid 13900k-core0.cpuid
3c3
<    0x00000001 0x00: eax=0x000b0671 ebx=0x01800800 ecx=0x7ffafbff edx=0xbfebfbff
---
>    0x00000001 0x00: eax=0x000b0671 ebx=0x00800800 ecx=0x7ffafbff edx=0xbfebfbff
19,21c19,21
<    0x0000000b 0x00: eax=0x00000001 ebx=0x00000002 ecx=0x00000100 edx=0x00000001
<    0x0000000b 0x01: eax=0x00000007 ebx=0x00000020 ecx=0x00000201 edx=0x00000001
<    0x0000000b 0x02: eax=0x00000000 ebx=0x00000000 ecx=0x00000002 edx=0x00000001
---
>    0x0000000b 0x00: eax=0x00000001 ebx=0x00000002 ecx=0x00000100 edx=0x00000000
>    0x0000000b 0x01: eax=0x00000007 ebx=0x00000020 ecx=0x00000201 edx=0x00000000
>    0x0000000b 0x02: eax=0x00000000 ebx=0x00000000 ecx=0x00000002 edx=0x00000000
61,63c61,63
<    0x0000001f 0x00: eax=0x00000001 ebx=0x00000002 ecx=0x00000100 edx=0x00000001
<    0x0000001f 0x01: eax=0x00000007 ebx=0x00000020 ecx=0x00000201 edx=0x00000001
<    0x0000001f 0x02: eax=0x00000000 ebx=0x00000000 ecx=0x00000002 edx=0x00000001
---
>    0x0000001f 0x00: eax=0x00000001 ebx=0x00000002 ecx=0x00000100 edx=0x00000000
>    0x0000001f 0x01: eax=0x00000007 ebx=0x00000020 ecx=0x00000201 edx=0x00000000
>    0x0000001f 0x02: eax=0x00000000 ebx=0x00000000 ecx=0x00000002 edx=0x00000000
```

第一个
logical process

参考 0x0b 和 0x1f 都是展示拓扑的: https://www.felixcloutier.com/x86/cpuid

所以，最后是执行一个来测试:
```txt
cgexec --sticky -g cpu,memory,cpuset:/ taskset -ac 1 cpuid -r -1
```

```diff
diff --git a/docs/kernel/cpuinfo/zhaoxin/182 b/docs/kernel/cpuinfo/zhaoxin/182
index d29be4b2b454..16d72f3e34cb 100644
--- a/docs/kernel/cpuinfo/zhaoxin/182
+++ b/docs/kernel/cpuinfo/zhaoxin/182
@@ -57,8 +57,8 @@
    0x80000008 0x00: eax=0x0000302e ebx=0x00000000 ecx=0x00000000 edx=0x00000000
    0x80860000 0x00: eax=0x00000000 ebx=0x00000000 ecx=0x00000000 edx=0x00000000
    0xc0000000 0x00: eax=0xc0000005 ebx=0x00000000 ecx=0x00000000 edx=0x00000000
-   0xc0000001 0x00: eax=0x000507bb ebx=0x00000000 ecx=0x00000000 edx=0x1ec33dff
+   0xc0000001 0x00: eax=0x000507bb ebx=0x00000000 ecx=0x00000000 edx=0x1ec13dff
    0xc0000002 0x00: eax=0x00000000 ebx=0x00000000 ecx=0x00000000 edx=0x00000000
    0xc0000003 0x00: eax=0x00000000 ebx=0x00000000 ecx=0x00000000 edx=0x00000000
-   0xc0000004 0x00: eax=0x0000002d ebx=0x100029c7 ecx=0x107832ce edx=0x00000000
+   0xc0000004 0x00: eax=0x00000044 ebx=0x100028af ecx=0x107832ce edx=0x00000000
    0xc0000005 0x00: eax=0x00000082 ebx=0x00000000 ecx=0x00000000 edx=0x00000000
```

## CPU 型号

** 67.88 服务器

cgexec --sticky -g cpu,memory,cpuset:/ -ac 1 cpuid -r -1

CPU:
   0x00000001 0x00: eax=0x00050657 ebx=0x12200800 ecx=0x7ffefbff edx=0xbfebfbff

```txt
🧀  cpuid -l 1 -1
CPU:
   version information (1/eax):
      processor type  = primary processor (0)
      family          = 0x6 (6)
      model           = 0x5 (5)
      stepping id     = 0x7 (7)
      extended family = 0x0 (0)
      extended model  = 0x5 (5)
      (family synth)  = 0x6 (6)
      (model synth)   = 0x55 (85)
```

```txt
Architecture:             x86_64
  CPU op-mode(s):         32-bit, 64-bit
  Address sizes:          46 bits physical, 48 bits virtual
  Byte Order:             Little Endian
CPU(s):                   64
  On-line CPU(s) list:    0-63
Vendor ID:                GenuineIntel
  Model name:             Intel(R) Xeon(R) Gold 5218 CPU @ 2.30GHz
    CPU family:           6
    Model:                85
    Thread(s) per core:   2
    Core(s) per socket:   16
    Socket(s):            2
    Stepping:             7
```

** 某一个机器

cpuid -r -1

CPU:
   0x00000001 0x00: eax=0x000806f8 ebx=0x48400800 ecx=0x7ffefbff edx=0xbfebfbff

也就是 6 8f 8

```txt
Architecture:          x86_64
CPU op-mode(s):        32-bit, 64-bit
Byte Order:            Little Endian
CPU(s):                48
On-line CPU(s) list:   0-47
Thread(s) per core:    2
Core(s) per socket:    12
Socket(s):             2
NUMA node(s):          2
Vendor ID:             GenuineIntel
CPU family:            6
Model:                 143
Model name:            INTEL(R) XEON(R) SILVER 4510
```

** 扩展内容

https://en.wikichip.org/wiki/intel/cpuid#Big_Cores_.28Server.29

从这里看
```txt
Emerald Rapids (2023)	     Golden Cove	0	0x6	0xC	0xF	Family 6 Model 207
Sapphire Rapids (2023)	Golden Cove SP	0	0x6	0x8	0xF	Family 6 Model 143
```

哦，原来 intel 的 4 代和 5 代是一个模糊的概念:
https://www.intel.com/content/www/us/en/products/sku/236637/intel-xeon-silver-4510-processor-30m-cache-2-40-ghz/specifications.html

```txt
Product   Collection 5th Gen Intel® Xeon® Scalable Processors
Code Name Products formerly Sapphire Rapids
```

从这里看，SPR 就是 4 代:
https://github.com/intel/Intel-Linux-Processor-Microcode-Data-Files/releases/tag/microcode-20250812

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
