# acpi pad
<!-- 0cc86a87-7eba-4b71-905a-2df04ccd73b7 -->

## 基本原理
> [!NOTE]
> 参考神奇海螺的意见，有待验证

1. ACPI Processor Aggregator 驱动：实现了 ACPI 规范中的 Processor Aggregator 设备（ACPI000C HID）
2. CPU 电源管理：
  • 可以指定数量的 CPU 进入"空闲"状态（通过创建 power saving 内核线程）
  • 使用 MWAIT 指令让 CPU 进入低功耗状态
  • 支持轮询（round-robin）在多个 CPU 之间分配任务
3. sysfs 接口：
  • /sys/devices/platform/processor_aggregator/rrtime：轮询时间（秒）
  • /sys/devices/platform/processor_aggregator/idlepct：空闲百分比
  • /sys/devices/platform/processor_aggregator/idlecpus：空闲 CPU 数量
4. ACPI 接口：
  • 支持 _PUR（Processor Utilization Request）方法查询固件请求的空闲 CPU 数量
  • 支持 _OST（OS Status Indication）向固件报告状态
  • 处理 ACPI 通知事件（0x80）

这个驱动在虚拟化场景中特别重要，比如在 VMware ESXi 环境中，它允许 hypervisor 通过 ACPI 通知客户机将某些 CPU 置于空闲状态，从而实现更高效的电源管
理。

驱动在初始化时会检查是否支持 MWAIT 指令以及 CPUID 中的 C-state 信息。
然后注册一个平台驱动来处理 ACPI000C 设备，通过创建专门的节能线程来执行电源管理策略。

drivers/acpi/acpi_pad.c 是 ACPI Processor Aggregator Driver（ACPI 处理器聚合器驱动），主要用于虚拟化环境下的 CPU 电源管理。

## 什么时候启动?
<!-- 27abf3c4-33c8-4db4-bd69-91c67b8d48f9 -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

1. ACPI 固件通知（自动启动）

当 ACPI 固件发送 Processor Aggregator Device 通知（0x80） 时：

acpi_pad_handle_notify()  →  acpi_pad_idle_cpus()  →  set_power_saving_task_num()  →  create_power_saving_task()

典型场景：Hypervisor（如 VMware ESXi）检测到宿主机资源紧张，通过 ACPI 通知客户机将某些 CPU 置于空闲状态。

2. 手动通过 Sysfs 写入

echo N > /sys/devices/platform/processor_aggregator/idlecpus

触发路径：

idlecpus_store()  →  acpi_pad_idle_cpus(num)  →  set_power_saving_task_num(num)

线程创建细节

static int create_power_saving_task(void)
{
    ps_tsks[ps_tsk_num] = kthread_run(power_saving_thread,
        (void *)(unsigned long)ps_tsk_num,
        "acpi_pad/%d", ps_tsk_num);  // 线程名如 acpi_pad/0, acpi_pad/1
    ...
}

启动条件限制

线程启动前必须满足：

1. CPU 支持 MWAIT：power_saving_mwait_init() 检测到 X86_FEATURE_MWAIT
2. 支持中断打断 MWAIT：CPUID5_ECX_EXTENSIONS_SUPPORTED 和 CPUID5_ECX_INTERRUPT_BREAK
3. 非 Xen Dom0：如果是 Xen 初始域，驱动会直接退出

线程功能

启动后，power_saving_thread 会：

1. 轮询绑定：定期（默认 1 秒）在不同 CPU 之间轮询（避免固定某个 CPU 长期空闲）
2. MWAIT 空闲：让 CPU 进入最深的 C-State 休眠（通过 mwait_idle_with_hints()）
3. 空闲比例控制：默认 95% 时间 MWAIT，5% 时间睡眠（可配置 idlepct）

## 如何解决高 cpu 占用问题
去掉这个模块

这个未必有点太超前了吧:
https://www.intel.com/content/dam/support/us/en/documents/network/omni-adptr/sb/Intel_OP_Performance_Tuning_UG_H93143_v1_0.pdf
https://askubuntu.com/questions/1535191/what-is-acpi-pad-and-how-do-i-stop-it-taking-up-so-much-cpu#:~:text=1%20Answer,Xeon%20processors%2C%20not%20my%20own.

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
