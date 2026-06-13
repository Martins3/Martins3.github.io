## cpuid leaf : CPUID_LEAF_MWAIT
<!-- d86177c9-5d60-4741-84f8-96124ad1b5c9 -->

```txt
drivers/acpi/acpi_pad.c
```

```txt
#define CPUID_LEAF_MWAIT	0x05
```

cpuid(CPUID_MWAIT_LEAF, &eax, &ebx, &ecx, &edx);


   寄存器   值 (十六进制)   含义
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   EAX      0x40 (64)       MWAIT 最小轮询周期 (TSC 计数值)
   EBX      0x40 (64)       MWAIT 最大轮询周期 (TSC 计数值)
   ECX      0x3             支持的扩展特性
   EDX      0x11            支持的 C-state 子状态信息

如果在经典 hygon 机器上测试:

> • eax = 0x40
> • ebx = 0x40
> • ecx = 0x3
> • edx = 0x11

这里的 edx 说明就是只有 C0 是支持的

使用 cpuid -1 -r -l 5 的结果
```txt
   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00000011
```
cpuid -1 -l 5
```txt
   MONITOR/MWAIT (5):
      smallest monitor-line size (bytes)       = 0x40 (64)
      largest monitor-line size (bytes)        = 0x40 (64)
      enum of Monitor-MWAIT exts supported     = true
      supports intrs as break-event for MWAIT  = true
      monitorless MWAIT supported              = false
      number of C0 sub C-states using MWAIT    = 0x1 (1)
      number of C1 sub C-states using MWAIT    = 0x1 (1)
      number of C2 sub C-states using MWAIT    = 0x0 (0)
      number of C3 sub C-states using MWAIT    = 0x0 (0)
      number of C4 sub C-states using MWAIT    = 0x0 (0)
      number of C5 sub C-states using MWAIT    = 0x0 (0)
      number of C6 sub C-states using MWAIT    = 0x0 (0)
      number of C7 sub C-states using MWAIT    = 0x0 (0)
```


13900k
```txt
CPU:
   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x10102020

CPU:
   MONITOR/MWAIT (5):
      smallest monitor-line size (bytes)       = 0x40 (64)
      largest monitor-line size (bytes)        = 0x40 (64)
      enum of Monitor-MWAIT exts supported     = true
      supports intrs as break-event for MWAIT  = true
      monitorless MWAIT supported              = false
      number of C0 sub C-states using MWAIT    = 0x0 (0)
      number of C1 sub C-states using MWAIT    = 0x2 (2)
      number of C2 sub C-states using MWAIT    = 0x0 (0)
      number of C3 sub C-states using MWAIT    = 0x2 (2)
      number of C4 sub C-states using MWAIT    = 0x0 (0)
      number of C5 sub C-states using MWAIT    = 0x1 (1)
      number of C6 sub C-states using MWAIT    = 0x0 (0)
      number of C7 sub C-states using MWAIT    = 0x1 (1)
```

## intel sdm
17.7 MWAIT EXTENSIONS FOR ADVANCED POWER MANAGEMENT

## 这个检查结果很全过程

结果发现，有的加载，有的不加载，不是说一定加载的:
```txt
lsmod_node-67-77.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-67-76.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-67-75.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-129-196.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002120

lsmod_node-128-104.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-129-193.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002120

lsmod_node-128-102.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00000011

lsmod_node-132-141.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-67-58.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-128-73.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00000011

lsmod_node-128-101.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00000011

lsmod_node-67-57.txt
2:   0x00000005 0x00: eax=0x00000000 ebx=0x00000000 ecx=0x00000000 edx=0x00002020

lsmod_node-132-143.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-128-106.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-132-144.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-128-115.txt
2:   0x00000005 0x00: eax=0x00000000 ebx=0x00000000 ecx=0x00000000 edx=0x00000000

lsmod_node-128-110.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-128-107.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-129-194.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002120

lsmod_node-129-195.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002120

lsmod_node-128-83.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002120

lsmod_node-128-103.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00000011

lsmod_node-128-109.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-128-105.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-128-81.txt
2:   0x00000005 0x00: eax=0x00000000 ebx=0x00000000 ecx=0x00000000 edx=0x00002020

lsmod_node-128-82.txt
2:   0x00000005 0x00: eax=0x00000000 ebx=0x00000000 ecx=0x00000000 edx=0x00002020

lsmod_node-128-108.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020

lsmod_node-67-74.txt
2:   0x00000005 0x00: eax=0x00000040 ebx=0x00000040 ecx=0x00000003 edx=0x00002020
```

其实未必可以加载的:
```txt
$ lsmod | grep acpi
acpi_power_meter       20480  0
acpi_ipmi              20480  0
ipmi_msghandler       126976  3 ipmi_devintf,ipmi_si,acpi_ipmi
$ sudo modprobe acpi_pad
$ lsmod | grep acpi
acpi_pad              184320  0
acpi_power_meter       20480  0
acpi_ipmi              20480  0
ipmi_msghandler       126976  3 ipmi_devintf,ipmi_si,acpi_ipmi
```

具体代码分析为:

power_saving_mwait_init 中的内容，如果只有最低两位有，那么就跳过:
```c
	edx >>= MWAIT_SUBSTATE_SIZE;
	for (i = 0; i < 7 && edx; i++, edx >>= MWAIT_SUBSTATE_SIZE) {
		if (edx & MWAIT_SUBSTATE_MASK) {
			highest_cstate = i;
			highest_subcstate = edx & MWAIT_SUBSTATE_MASK;
		}
	}
	power_saving_mwait_eax = (highest_cstate << MWAIT_SUBSTATE_SIZE) |
		(highest_subcstate - 1);
```

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
