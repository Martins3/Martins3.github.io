# CPU hotplug

关键参考

<p align="center">
  <img src="https://www.dingmos.com/usr/uploads/2022/12/2930619187.png" alt="drawing" align="center"/>
</p>
<p align="center">
[Linux 内核 | CPU 热插拔（Hotplug）](https://www.dingmos.com/index.php/archives/117/)
</p>


通过 /sys/devices/system/cpu/hotplug/states 来查看所有注册的回调函数

## 主要文件
- kernel/cpu.c

## 关键函数分析

```txt
#0  _cpu_down (cpu=cpu@entry=1, tasks_frozen=tasks_frozen@entry=0, target=target@entry=CPUHP_OFFLINE) at kernel/cpu.c:1154
#1  0xffffffff81144914 in cpu_down_maps_locked (target=CPUHP_OFFLINE, cpu=1) at kernel/cpu.c:1228
#2  cpu_down (target=CPUHP_OFFLINE, cpu=1) at kernel/cpu.c:1236
#3  cpu_device_down (dev=<optimized out>) at kernel/cpu.c:1253
#4  0xffffffff81b3b455 in device_offline (dev=0xffff88833365ba48) at drivers/base/core.c:4088
#5  device_offline (dev=0xffff88833365ba48) at drivers/base/core.c:4072
#6  0xffffffff81b3b585 in online_store (dev=0xffff88833365ba48, attr=<optimized out>, buf=<optimized out>, count=2) at drivers/base/core.c:2651
#7  0xffffffff814d312c in kernfs_fop_write_iter (iocb=0xffffc90001ff3ea0, iter=<optimized out>) at fs/kernfs/file.c:334
#8  0xffffffff81423055 in call_write_iter (iter=0xffffc90001ff3e78, kio=0xffffc90001ff3ea0, file=0xffff88810dc17500) at ./include/linux/fs.h:1851
#9  new_sync_write (ppos=0xffffc90001ff3f08, len=2, buf=0x55b7744652a0 "0\n", filp=0xffff88810dc17500) at fs/read_write.c:491
#10 vfs_write (file=file@entry=0xffff88810dc17500, buf=buf@entry=0x55b7744652a0 "0\n", count=count@entry=2, pos=pos@entry=0xffffc90001ff3f08) at fs/read_write.c:584
#11 0xffffffff814234c3 in ksys_write (fd=<optimized out>, buf=0x55b7744652a0 "0\n", count=2) at fs/read_write.c:637
#12 0xffffffff8227db9f in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001ff3f58) at arch/x86/entry/common.c:50
#13 do_syscall_64 (regs=0xffffc90001ff3f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#14 0xffffffff824000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```
- `_cpu_down`
  - cpuhp_kick_ap_work
    - cpuhp_kick_ap
      - 目标函数运行: cpuhp_thread_fun
  - cpuhp_down_callbacks
    - cpuhp_invoke_callback_range

## 问题
### unplug 一个 CPU，然后热迁移，QEMU 如何保证 CPU 的数量

### kvm 和 QEMU 分别需要何种配合
这都是随便搜搜得到，估计这个资料很多吧:
- https://wiki.qemu.org/Features/CPUHotplug
- https://www.qemu.org/docs/master/system/cpu-hotplug.html
- http://events17.linuxfoundation.org/sites/events/files/slides/CPU%20Hot-plug%20support%20in%20QEMU.pdf

### kernel/code/sched-hotplug.sh 执行之后，在 htop 中还是可以看到，只是标志为 offline，lscpu 就真的看不到了

### 为什么需要函数 `cpuhp_threads_init`
online 的时候，会存在调用:
```txt
#0  cpuhp_thread_fun (cpu=1) at kernel/cpu.c:772
#1  0xffffffff8117974f in smpboot_thread_fn (data=0xffff88810027e140) at kernel/smpboot.c:164
#2  0xffffffff8116e96c in kthread (_create=0xffff88810082f840) at kernel/kthread.c:376
#3  0xffffffff810029bc in ret_from_fork () at arch/x86/entry/entry_64.S:308
#4  0x0000000000000000 in ?? ()
```

### 如果当时 CPU 很忙，被要求 offline

```c
static struct syscore_ops kvm_syscore_ops = {
	.suspend	= kvm_suspend,
	.resume		= kvm_resume,
};
```

## 当 online CPU 的时候，类似 boot CPU 启动
```txt

🧀  t tsx_dev_mode_disable
Attaching 1 probe...
^C
@[
    tsx_dev_mode_disable+1
    tsx_ap_init+15
    smp_store_cpu_info+73
    start_secondary+81
    secondary_startup_64_no_verify+224
]: 1
```

# hotplug : 从 acpi 的视角

# [kernel doc](https://www.kernel.org/doc/html/latest/firmware-guide/acpi/index.html)

- 使用 make menuconfig 大致分析一下一共都存在什么功能吧

## acpi 中断的过程

- 在 memory hotplug 总是出现如下的 backtrace ，所以中断哪里是:

```txt
#15 0xffffffff816cfa05 in acpi_hotplug_work_fn (work=0xffff8880093142c0) at drivers/acpi/osl.c:1162
#16 0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff888003ab83c0, work=0xffff8880093142c0) at kernel/workqueue.c:2289
#17 0xffffffff811232c8 in worker_thread (__worker=0xffff888003ab83c0) at kernel/workqueue.c:2436
#18 0xffffffff81129c73 in kthread (_create=0xffff888003aaa400) at kernel/kthread.c:376
#19 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
```

cat /proc/interrupt 中可以看到这一行:
```txt
9:          2   IO-APIC   9-fasteoi   acpi
```

进一步的检查 docs/qemu/interrupt.md 的文档，找到 acpi 调用 `acpi_os_install_interrupt_handler` 将 `acpi_irq` 注册为中断。

真的是对于 workqueue 使用的行云流水啊:
```txt
#0  acpi_hotplug_schedule (adev=adev@entry=0xffff888003b64800, src=src@entry=1) at include/linux/slab.h:600
#1  0xffffffff816d5c9f in acpi_bus_notify (handle=0xffff888003af0420, type=1, data=<optimized out>) at drivers/acpi/bus.c:531
#2  0xffffffff816edc4f in acpi_ev_notify_dispatch (context=0xffff8881001e8410) at drivers/acpi/acpica/evmisc.c:171
#3  0xffffffff816cf941 in acpi_os_execute_deferred (work=0xffff88810016e750) at drivers/acpi/osl.c:850
#4  0xffffffff81122d37 in process_one_work (worker=worker@entry=0xffff888008151cc0, work=0xffff88810016e750) at kernel/workqueue.c:2289
#5  0xffffffff811232c8 in worker_thread (__worker=0xffff888008151cc0) at kernel/workqueue.c:2436
#6  0xffffffff81129c73 in kthread (_create=0xffff88810016e640) at kernel/kthread.c:376
#7  0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#8  0x0000000000000000 in ?? ()
```

```txt
#0  acpi_os_execute (type=type@entry=OSL_GPE_HANDLER, function=function@entry=0xffffffff816ec615 <acpi_ev_asynch_execute_gpe_method>, context=context@entry=0xffff888003afb048) at drivers/acpi/osl.c:1074
#1  0xffffffff816ec7fd in acpi_ev_gpe_dispatch (gpe_device=gpe_device@entry=0xffff8880039170c0, gpe_event_info=gpe_event_info@entry=0xffff888003afb048, gpe_number=gpe_number@entry=3) at drivers/acpi/acpica/evgpe.c:823
#2  0xffffffff816ec96d in acpi_ev_detect_gpe (gpe_device=gpe_device@entry=0xffff8880039170c0, gpe_event_info=gpe_event_info@entry=0xffff888003afb048, gpe_number=gpe_number@entry=3) at drivers/acpi/acpica/evgpe.c:723
#3  0xffffffff816eca49 in acpi_ev_gpe_detect (gpe_xrupt_list=gpe_xrupt_list@entry=0xffff888003aacac0) at drivers/acpi/acpica/evgpe.c:424
#4  0xffffffff816eebb9 in acpi_ev_sci_xrupt_handler (context=0xffff888003aacac0) at drivers/acpi/acpica/evsci.c:98
#5  0xffffffff816cfa23 in acpi_irq (irq=<optimized out>, dev_id=<optimized out>) at drivers/acpi/osl.c:549
#6  0xffffffff811658d1 in __handle_irq_event_percpu (desc=desc@entry=0xffff888003918a00) at kernel/irq/handle.c:158
#7  0xffffffff81165a7f in handle_irq_event_percpu (desc=0xffff888003918a00) at kernel/irq/handle.c:193
#8  handle_irq_event (desc=desc@entry=0xffff888003918a00) at kernel/irq/handle.c:210
#9  0xffffffff81169c1b in handle_fasteoi_irq (desc=0xffff888003918a00) at kernel/irq/chip.c:714
#10 0xffffffff810b9ad4 in generic_handle_irq_desc (desc=0xffff888003918a00) at include/linux/irqdesc.h:158
#11 handle_irq (regs=<optimized out>, desc=0xffff888003918a00) at arch/x86/kernel/irq.c:231
#12 __common_interrupt (regs=<optimized out>, vector=33) at arch/x86/kernel/irq.c:250
#13 0xffffffff81ef93f3 in common_interrupt (regs=0xffffffff82a03de8, error_code=<optimized out>) at arch/x86/kernel/irq.c:240
Backtrace stopped: Cannot access memory at address 0xffffc90000004018
```

我不是很懂这里的设计，在 acpi_os_execute 中使用 workqueue 一直执行到 acpi_hotplug_schedule ，然后再次使用
执行到 acpi_hotplug_work_fn ，也许是因为不在乎效率吧。

## 其他
- [ ] https://unix.stackexchange.com/questions/242013/disable-gpe-acpi-interrupts-on-boot


## [ ] 在物理机上 hotplug usb 的 backtrace 是什么样子的

## http://127.0.0.1:3434/translations/zh_CN/core-api/cpu_hotplug.html

## http://events17.linuxfoundation.org/sites/events/files/slides/CPU%20Hot-plug%20support%20in%20QEMU.pdf

## 从 qmeu 中找到了些文档
- qemu/docs/specs/acpi_cpu_hotplug.rst
- docs/specs/acpi_mem_hotplug.rst

## 真的不敢想，hotplug + 嵌套的有多刺激

- L0 hotplug 一个 cpu 到 L1 中

- L1 hotplug 一个 cpu 到 L2 中，似乎没啥区别

## 先用起来再说
- https://www.qemu.org/docs/master/system/cpu-hotplug.html

参考这个来写命令:
- https://qemu-project.gitlab.io/qemu/interop/qemu-qmp-ref.html#qapidoc-2497

```txt
-> { "execute": "device_add",
     "arguments": { "driver": "e1000", "id": "net1",
                    "bus": "pci.0",
                    "mac": "52:54:00:12:34:56" } }
<- { "return": {} }
```

## 自己构建的内核无法实现 CPU 热插拔
echo 1 > /sys/devices/system/cpu/cpu4/online

## qemu 常用命令
hotpluggable-cpus

## 有趣的 ARM CPU 热插拔
https://mp.weixin.qq.com/s/L6T3vdFhfuFvc8FA09zqsw

先到 mac 中测试下。

## 意识到，maxcpus 会影响 guest os 的识别的，这个到底是如何实现的?

RCU ，或者 percpu 之类似乎都是可以看到的

## top 中观察到这个，这个是做什么的?
```txt
  13 root      20   0       0      0      0 S   0.0  0.0   0:00.00 cpuhp/0
  14 root      20   0       0      0      0 S   0.0  0.0   0:00.00 cpuhp/1
```

似乎是一个 cpu 一个:
```txt
🧀  ps -elf | grep cpuhp
1 S root          20       2  0  80   0 -     0 -      Aug29 ?        00:00:00 [cpuhp/0]
1 S root          21       2  0  80   0 -     0 -      Aug29 ?        00:00:00 [cpuhp/1]
1 S root          26       2  0  80   0 -     0 -      Aug29 ?        00:00:00 [cpuhp/2]
1 S root          31       2  0  80   0 -     0 -      Aug29 ?        00:00:00 [cpuhp/3]
1 S root          36       2  0  80   0 -     0 -      Aug29 ?        00:00:00 [cpuhp/4]
1 S root          41       2  0  80   0 -     0 -      Aug29 ?        00:00:00 [cpuhp/5]
1 S root          46       2  0  80   0 -     0 -      Aug29 ?        00:00:00 [cpuhp/6]
1 S root          51       2  0  80   0 -     0 -      Aug29 ?        00:00:00 [cpuhp/7]
```

## 热插的时候的有趣日志:
```txt
[ 1249.343862] ACPI: CPU59 has been hot-added
[ 1249.360313] smpboot: Booting Node 1 Processor 59 APIC 0x4b
[ 1249.381232] clocksource: [martins3:clocks_calc_mult_shift:63] 1000000000 374400000 600
[ 1249.381488] clocksource: [martins3:clocks_calc_mult_shift:88] 12562779 25
```
1. ACPI ?
2. smpboot 为什么知道是哪一个 Processor ?
3. clocks_calc_mult_shift 为什么需要被调用 ?

## 为什么有时候热插不上
```txt
[Tue Feb 18 22:48:02 2025] ACPI: Unable to map lapic to logical cpu number
```

## 发现 htop 的一个 bug
```txt
🧀  ls /sys/devices/system/cpu
 cpu0    cpufreq         enabled    kernel_max   online     present   umwait_control
 cpu1    cpuidle         hotplug    modalias     possible   smt       vulnerabilities
 cpu48   crash_hotplug   isolated   offline      power      uevent
```
这个时候，htop 会把前面的 cpu1 到 cpu48 都显示出来
## 原来 cpu0 的 online 是不可以 echo 的
/sys/devices/system/cpu0/online

## qemu 这个警告

qemu-system-x86_64: warning: Number of hotpluggable cpus requested (128) exceeds the recommended cpus supported by KVM (32)

```c
/* Find number of supported CPUs using the recommended
 * procedure from the kernel API documentation to cope with
 * older kernels that may be missing capabilities.
 */
static int kvm_recommended_vcpus(KVMState *s)
{
    int ret = kvm_vm_check_extension(s, KVM_CAP_NR_VCPUS);
    return (ret) ? ret : 4;
}
```


## ARM CPU 热插为什么不能工作来着?
<!-- b890a930-ac0f-4d08-a187-d048fcd08a9a -->

https://docs.google.com/document/d/1KBqDYz11xLR8MsGCb2WSkpcvukKWZDFBjNECH7z6s64/edit?tab=t.0#heading=h.rxadnjfqdyav

从 Documentation/arch/arm64/cpu-hotplug.rst ARM 从内核的角度如何支持

```txt
CONFIG_MEMORY
CONFIG_MEMORY_HOTPLUG
CONFIG_MEMORY_HOTPLUG_DEFAULT_ONLINE
CONFIG_ACPI_HOTPLUG_MEMORY
```

的确是 CPU 热插的时候，会遇到问题的:
```json
{
  "error": {
    "class": "GenericError",
    "desc": "machine does not support hot-plugging CPUs"
  }
}
```

https://www.youtube.com/watch?v=VdMkf06Pc6w&ab_channel=KVMForum

## cpu 热插之后，cpu 立刻就可以看到，不需要手动 online
echo 1 > /sys/devices/system/cpu/cpu7/online

这是如何做的，是 udev 的工作吗?

```sh
#!/usr/bin/env bash

set -E -e -u -o pipefail
cd "$(dirname "$0")"

total_number=$(grep -c ^processor /proc/cpuinfo)
total_number=32
for ((i = 1; i < total_number; i = i + 1)); do
	echo 0 >/sys/devices/system/cpu/"cpu$i"/online
done
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
