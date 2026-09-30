# acpi 如何支持到 CPU 热插拔机制
[acpi](https://www.kernel.org/doc/html/latest/firmware-guide/acpi/index.html)

## acpi 中断的过程 : 以 hotplug 为例子

热添加的流程大致是：

```txt
QEMU device_add 创建 vCPU
    ↓
设置 CPU 插入状态，触发 SCI（ACPI 中断）
    ↓
guest 执行 ACPI AML，发出 Notify：检查这个设备
    ↓
Linux ACPI 重新扫描，查询 _STA 等设备信息
    ↓
建立 CPU 编号映射，注册 CPU 设备
    ↓
打印 “CPU2 has been hot-added”
    ↓
后续上线，CPU 开始执行任务
```

QEMU 在这里模拟了支持 CPU 热插拔的硬件平台，并提供相应 ACPI 描述和方法。 QEMU
ACPI CPU 热插拔接口
(https://www.qemu.org/docs/master/specs/acpi_cpu_hotplug.html)

当前源码的 drivers/acpi/acpi_processor.c 中，acpi_processor_hotadd_init()
的关键顺序就是：

```txt
acpi_map_cpu(...);       /* 建立 CPU 编号映射 */
arch_register_cpu(...); /* 注册 CPU 设备 */
pr_info("CPU%d has been hot-added\n", pr->id);
```

接到 ACPI 通知后，热插拔工作队列中的流程（不是硬中断调用栈）：

1. 中断
```text
131.257837  irq/9-acpi-42
acpi_os_execute(kind=2, callback=acpi_ev_asynch_execute_gpe_method)
  acpi_ev_gpe_dispatch
  acpi_ev_detect_gpe
  acpi_ev_gpe_detect
  acpi_ev_sci_xrupt_handler
  acpi_irq
  irq_thread_fn
  irq_thread
  kthread
  ret_from_fork
  ret_from_fork_asm
```

2. 在 workqueue 中执行 aml 函数来 notify
```text
131.258546  kworker/0:2-104
acpi_ev_queue_notify_request(node=C002, value=1)
  acpi_ex_opcode_2A_0T_0R
  acpi_ds_exec_end_op
  acpi_ps_parse_loop
  acpi_ps_parse_aml
  acpi_ps_execute_method
  acpi_ns_evaluate
  acpi_ev_asynch_execute_gpe_method
  acpi_os_execute_deferred
  process_one_work
  worker_thread
  kthread
  ret_from_fork
  ret_from_fork_asm
```

3. 接受到 notify 后，来执行真正的任务，在 kacpi_hotplug_wq 中
- ret_from_fork_asm
  - ret_from_fork
    - kthread
      - worker_thread
        - process_scheduled_works
          - process_one_work
            - acpi_hotplug_work_fn
              - acpi_device_hotplug
                - acpi_generic_hotplug_event
                  - acpi_scan_device_check
                    - acpi_scan_rescan_bus
                      - acpi_bus_scan
                        - acpi_bus_attach
                          - acpi_dev_for_each_child
                            - device_for_each_child
                              - acpi_bus_attach
                                - acpi_scan_attach_handler
                                  - acpi_processor_add
                                    - acpi_processor_get_info
                                      - acpi_processor_hotadd_init

这里的确有三个 backtrace
1. SCI: 从中断到执行 GPE AML 方法。
2. kacpid_wq : AML 的执行希望是 async ，所以放到 kacpid_wq 中去
3. kacpi_hotplug_wq : 真的完成任务

让 aml 来执行 notify 真的有点奇怪，但是大致就是如此的:
```txt
  Linux 初始化：
    acpi_install_notify_handler(..., acpi_bus_notify, ...)
                                    ↑ 保存这个函数指针

  AML 执行：
    Notify(C002, 1)
      → AML_NOTIFY_OP 的 C 实现
      → acpi_ev_queue_notify_request
      → acpi_os_execute
          └─ Linux 实现：queue_work(kacpi_notify_wq)
               ↓
         acpi_ev_notify_dispatch
           → 调用注册的函数指针：acpi_bus_notify
           → acpi_hotplug_schedule
```

## AML 方法总结

如果你指的是 QEMU device_add 热添加 CPU，这次实验确实执行了 AML。
热添加和热拔出都需要，只是执行的方法不同。

 操作            执行的主要 AML                          作用
━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 热添加          _E02 → CSCN → CTFY → Notify(C002, 1)    通知 Linux 检查新增 CPU
──────────────  ──────────────────────────────────────  ─────────────────────────────────────
 添加后的枚举    _STA → CSTA 等                          查询 CPU 是否存在、可用
──────────────  ──────────────────────────────────────  ─────────────────────────────────────
 热拔出请求      _E02 → CSCN → CTFY → Notify(C002, 3)    请求 Linux 准备移除
──────────────  ──────────────────────────────────────  ─────────────────────────────────────
 真正拔出        _EJ0 → CEJ0                             Linux 下线 CPU 后，通知 QEMU 删除它

热添加时，QEMU 已经创建了 vCPU；AML 帮助 Linux 发现它、查询状态并注册设备，因此不需要一个与 _EJ0 对称的“插入方法”。

如果只是 guest 内写 cpuN/online=0/1，则不会走上面的 ACPI 设备添加/移除通知链。本次实验也验证了这一点。


## QEMU 模拟

QEMU 提供 AML，同时实现 AML 访问的寄存器
- `hw/i386/acpi-build.c` 的 `build_dsdt()` 调用 `build_cpus_aml()`，后者位于
  `hw/acpi/cpu.c`，生成 `_E02`、`CSCN`、`CTFY`、`_STA`、`_EJ0` 等 AML。
- `hw/acpi/cpu.c` 的 `cpu_hotplug_hw_init()` 注册 `cpu_hotplug_ops`，其中
  `.read = cpu_hotplug_rd`、`.write = cpu_hotplug_wr`，实现 CPU 热插拔寄存器。
  本机这些寄存器映射在 I/O 地址空间的 `0xaf00` 开始处。

```c
static const MemoryRegionOps cpu_hotplug_ops = {
    .read = cpu_hotplug_rd,
    .write = cpu_hotplug_wr,
    .endianness = DEVICE_LITTLE_ENDIAN,
    .valid = {
        .min_access_size = 1,
        .max_access_size = 4,
    },
};
```

### unplug

qmp 发送请求，Guest 接受到中断，之后 Guest 执行 AML 方法，
最后写入到 MMIO 空间，qemu 接受到后，真正的卸载

```txt
- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - kvm_handle_io
            - address_space_write
              - flatview_write
                - flatview_write_continue
                  - flatview_write_continue_step
                    - memory_region_dispatch_write
                      - access_with_adjusted_size
                        - memory_region_write_accessor
                          - cpu_hotplug_wr
                            - object_unparent
                              - object_property_del_child
                                - object_unref
                                  - object_finalize
                                    - object_deinit
                                      - device_finalize
                                        - qapi_event_send_device_deleted
```

此时对应的 qemu 的代码:
```text
acpi_os_write_port(port=0xaf04, value=0x08, width=8)
  acpi_ex_system_io_space_handler
  acpi_ev_address_space_dispatch
  acpi_ex_access_region
  acpi_ex_field_datum_io
  acpi_ex_write_with_update_rule
  acpi_ex_insert_into_field
  acpi_ex_write_data_to_field
  acpi_ex_store_object_to_node
  acpi_ex_store
  acpi_ex_opcode_1A_1T_1R
  acpi_ds_exec_end_op
  acpi_ps_parse_loop
  acpi_ps_parse_aml
  acpi_ps_execute_method
  acpi_ns_evaluate
  acpi_evaluate_object
  acpi_evaluate_ej0
  acpi_device_hotplug
  acpi_hotplug_work_fn
  process_one_work
  worker_thread
  kthread
  ret_from_fork
  ret_from_fork_asm
```


### plug

```text
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - aio_ctx_dispatch
                  - aio_dispatch
                    - aio_bh_poll
                      - aio_bh_call
                        - do_qmp_dispatch_bh
                          - qmp_device_add
                            - qdev_device_add_from_qdict
                              - qdev_realize
                                - object_property_set_bool
                                  - object_property_set_qobject
                                    - object_property_set
                                      - property_set_bool
                                        - device_set_realized
                                          - x86_cpu_plug
                                            - acpi_cpu_plug_cb
```

这里的 `acpi_cpu_plug_cb()` 找到对应 `AcpiCpuStatus` 槽位，
保存 `cdev->cpu`。 当 `dev->hotplugged` 为真时，执行：

```c
void acpi_cpu_plug_cb(HotplugHandler *hotplug_dev,
                      CPUHotplugState *cpu_st, DeviceState *dev, Error **errp)
{
    AcpiCpuStatus *cdev;

    cdev = get_cpu_status(cpu_st, dev);
    if (!cdev) {
        return;
    }

    cdev->cpu = CPU(dev);
    if (dev->hotplugged) {
        cdev->is_inserting = true;
        acpi_send_event(DEVICE(hotplug_dev), ACPI_CPU_HOTPLUG_STATUS);
    }
}
```

## 问题回答

写了这么多，都是之前发现了热插拔的时候总是有这些日志:
```txt
[ 1249.343862] ACPI: CPU59 has been hot-added
[ 1249.360313] smpboot: Booting Node 1 Processor 59 APIC 0x4b
```

smpboot 为什么知道是哪一个 Processor ?

## TODO
- 使用 make menuconfig 大致分析一下一共都存在什么功能吧

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
