# CPUX86State reset
https://www.qemu.org/docs/master/devel/reset.html

使用 x86_cpu_reset 进行初始化

- [ ] 关于 reset 其实会进行两次, 真的有必要吗 ?
- [ ] 为什么两次路径不相同啊

每一个 CPU 都会进行一次

- `x86_cpu_realizefn`
  - `cpu_reset`
    - `device_cold_reset`
      - `resettable_reset`
        - `resettable_assert_reset`
          - `resettable_phase_hold`
            - `x86_cpu_reset`

- `qemu_init`
  - `qmp_x_exit_preconfig`
    - `qemu_machine_creation_done` : 我们的老朋友啊
      - `qdev_machine_creation_done`
        - `qemu_system_reset`
          - `pc_machine_reset`
            - `qemu_devices_reset`
              - `x86_cpu_machine_reset_cb`
                - `cpu_reset`
                  - `device_cold_reset`
                    - `resettable_assert_reset`
                      - `resettable_phase_hold`
                        - `device_transitional_reset`
                          - `x86_cpu_reset`

在 v4.2 的时候，reset 更加的容易:
```txt
cpu_reset (cpu=0x5555565e1800) at /home/maritns3/core/kvmqemu/hw/core/cpu.c:243
0x00005555559f6685 in qemu_devices_reset () at /home/maritns3/core/kvmqemu/hw/core/reset.c:69
0x00005555558dc37f in pc_machine_reset (machine=<optimized out>) at /home/maritns3/core/kvmqemu/hw/i386/pc.c:2140
0x000055555598a1a1 in qemu_system_reset (reason=SHUTDOWN_CAUSE_NONE) at /home/maritns3/core/kvmqemu/vl.c:1551
0x00005555557f92b2 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at /home/maritns3/core/kvmqemu/vl.c:4436
```

在 v6.0 的时候, `ps2_kbd_reset`
```txt
#0  ps2_kbd_reset (opaque=0x555556a5e260) at ../hw/input/ps2.c:956
#1  0x0000555555e7ee3a in qemu_devices_reset () at ../hw/core/reset.c:69
#2  0x0000555555b9b6b9 in pc_machine_reset (machine=0x555556975de0) at ../hw/i386/pc.c:1644
#3  0x0000555555c71748 in qemu_system_reset (reason=SHUTDOWN_CAUSE_NONE) at ../softmmu/runstate.c:442
#4  0x0000555555ae669b in qdev_machine_creation_done () at ../hw/core/machine.c:1299
#5  0x0000555555c226b5 in qemu_machine_creation_done () at ../softmmu/vl.c:2579
#6  0x0000555555c22788 in qmp_x_exit_preconfig (errp=0x5555567aa610 <error_fatal>) at ../softmmu/vl.c:2602
#7  0x0000555555c24e56 in qemu_init (argc=28, argv=0x7fffffffd7d8, envp=0x7fffffffd8c0) at ../softmmu/vl.c:3635
#8  0x000055555582e575 in main (argc=28, argv=0x7fffffffd7d8, envp=0x7fffffffd8c0) at ../softmmu/main.c:49
```

- [ ] 至少需要回答一個問題，為什麼單獨是 cpu 的初始化需要是，两次 reset, 其他人都是一次

- [ ] 找到 devices 需要多个阶段 reset 的
- [ ] `device_class_init`

## reset
reset 的官方文档 [^2]

- [ ] 可以进一步的分析一下 interrupt/qemu-irq.md 中关于 `kvm_ioapic_put` 和 `kvm_pic_put` 的调用路径
    - 系统是如何初始化一个 ioapic 和两个 pic 的状态的

## hotplug
- [ ] 有必要分析一下 `device_set_realized` 一般都做了什么东西啊

- [ ] `x86_cpu_pre_plug` : 原来一般系统的初始化也需要处理这个东西
```txt
#0  x86_cpu_pre_plug (hotplug_dev=0x55555686bd60, dev=0x55555688f490, errp=0x555555d8d0e3 <type_table_lookup+39>) at ../hw/i386/x86.c:264
#1  0x0000555555b9ac03 in pc_machine_device_pre_plug_cb (hotplug_dev=0x555556975de0, dev=0x555556d5ef50, errp=0x7fffffffd120) at ../hw/i386/pc.c:1380
#2  0x0000555555e795a5 in hotplug_handler_pre_plug (plug_handler=0x555556975de0, plugged_dev=0x555556d5ef50, errp=0x7fffffffd120) at ../hw/core/hotplug.c:23
#3  0x0000555555e7dfc3 in device_set_realized (obj=0x555556d5ef50, value=true, errp=0x7fffffffd228) at ../hw/core/qdev.c:754
#4  0x0000555555d920d5 in property_set_bool (obj=0x555556d5ef50, v=0x555556bb1bc0, name=0x555556128b99 "realized", opaque=0x555556899b40, errp=0x7fffffffd228) at ../qom/object.c:2257
#5  0x0000555555d900f6 in object_property_set (obj=0x555556d5ef50, name=0x555556128b99 "realized", v=0x555556bb1bc0, errp=0x5555567aa610 <error_fatal>) at ../qom/object.c:1402
#6  0x0000555555d8ba93 in object_property_set_qobject (obj=0x555556d5ef50, name=0x555556128b99 "realized", value=0x555556b8ef70, errp=0x5555567aa610 <error_fatal>) at ../qom/qom-qobject.c:28
#7  0x0000555555d9046e in object_property_set_bool (obj=0x555556d5ef50, name=0x555556128b99 "realized", value=true, errp=0x5555567aa610 <error_fatal>) at ../qom/object.c:1472
#8  0x0000555555e7d01b in qdev_realize (dev=0x555556d5ef50, bus=0x0, errp=0x5555567aa610 <error_fatal>) at ../hw/core/qdev.c:389
#9  0x0000555555b896dc in x86_cpu_new (x86ms=0x555556975de0, apic_id=0, errp=0x5555567aa610 <error_fatal>) at ../hw/i386/x86.c:111
#10 0x0000555555b897af in x86_cpus_init (x86ms=0x555556975de0, default_cpu_version=1) at ../hw/i386/x86.c:138
```

## VMSD
https://lists.gnu.org/archive/html/qemu-devel/2014-06/msg05689.html

> The JSON-format output can then be used to compare the vmstate info for different QEMU versions, specifically to test whether live migration would break due to changes in the vmstate data.

保证迁移的两个 QEMU 中的内容完全相同的要求实在是太高了, 所以设置为仅仅为 VMSD 相同即可，也就是 dump 出来的文件相同 `-dump-vmstate state.json`

## 可以阅读的资料
https://blog.kernel.love/qemu-device-hotplug.html

[^1]: http://events17.linuxfoundation.org/sites/events/files/slides/CPU%20Hot-plug%20support%20in%20QEMU.pdf
[^2]: https://qemu.readthedocs.io/en/latest/devel/reset.html

## 如何实现强制重启?

通过 virsh qemu-monitor-command --hmp 7 "system_reset"

但是其实现的原理非常的有趣

那么谁来 kick 一下这些 vCPU 呢?

## 看来的确是这个问题了
qemu 45ed68a1a3a19754ade954d75a3c9

但是问题是，为什么 qemu 需要


就是这个 patch 了，

至少说，公版 qemu 没有出现无法 reset 的问题

## 升级内核

qemu=/home/martins3/data/qemu-e93ded1bf6c9/build/qemu-system-$ARCH

## 这种 reset 初始化回去调用两次
- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_init_vcpu
          - kvm_arch_init_vcpu
            - kvm_init_nested_state

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_init_board
        - machine_run_board_init
          - pc_init1
            - x86_cpus_init
              - x86_cpu_new
                - qdev_realize
                  - object_property_set_bool
                    - object_property_set_qobject
                      - object_property_set
                        - property_set_bool
                          - device_set_realized
                            - x86_cpu_realizefn
                              - cpu_reset
                                - device_cold_reset
                                  - resettable_reset
                                    - resettable_assert_reset
                                      - resettable_phase_hold
                                        - x86_cpu_reset_hold
                                          - kvm_arch_reset_vcpu
                                            - kvm_init_nested_state_reset

那 KVM_SET_NESTED_STATE 为什么也会调用两次?

从这里会有一次:
- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_machine_creation_done
        - qdev_machine_creation_done
          - cpu_synchronize_all_post_init
            - cpu_synchronize_post_init
              - kvm_cpu_synchronize_post_init (函数指针，手动添加的)
                - do_run_on_cpu
                  - queue_work_on_cpu

这里也有一次:
- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_machine_creation_done
        - qdev_machine_creation_done
          - qemu_system_reset
            - cpu_synchronize_all_states


都是通过 queue work 中执行上的，没办法的:
- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - qemu_wait_io_event
          - process_queued_cpu_work
            - do_kvm_cpu_synchronize_post_init
              - kvm_arch_put_registers
                - kvm_put_nested_state


所以，问题是，开机之后，为什么这里的代码需要执行两次:
```c
void cpu_synchronize_all_states(void)
{
    CPUState *cpu;

    CPU_FOREACH(cpu) {
        cpu_synchronize_state(cpu);
    }
}

void cpu_synchronize_all_post_init(void)
{
    CPUState *cpu;

    CPU_FOREACH(cpu) {
        cpu_synchronize_post_init(cpu);
    }
}
```

当执行 system_reset 的时候，其结果为:
- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_should_exit
        - qemu_system_reset
          - cpu_synchronize_all_post_reset
```c
void cpu_synchronize_all_post_reset(void)
{
    CPUState *cpu;

    CPU_FOREACH(cpu) {
        cpu_synchronize_post_reset(cpu);
    }
}
```

他们都是会去调用 kvm_arch_reset_vcpu 的。

**这个分析也是错误，实际上一共调用 6 次**

## 看看这个 backtrace
- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_init_vcpu
          - kvm_arch_init_vcpu
            - kvm_init_nested_state

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_init_board
        - machine_run_board_init
          - pc_init1
            - x86_cpus_init
              - x86_cpu_new
                - qdev_realize
                  - object_property_set_bool
                    - object_property_set_qobject
                      - object_property_set
                        - property_set_bool
                          - device_set_realized
                            - x86_cpu_realizefn
                              - cpu_reset
                                - device_cold_reset
                                  - resettable_reset
                                    - resettable_assert_reset
                                      - resettable_phase_hold
                                        - x86_cpu_reset_hold
                                          - kvm_arch_reset_vcpu
                                            - kvm_init_nested_state_reset

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
