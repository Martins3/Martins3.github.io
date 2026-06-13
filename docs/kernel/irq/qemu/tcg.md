### tcg pic

- pc_i8259_create
  - `i8259_init(isa_bus, x86_allocate_cpu_irq())`

x86_allocate_cpu_irq 会创建出来一个 qemu_irq 出来，其 handler 为 pic_irq_request

在 pic_irq_request 中，会首先判断是否需要发送给 lapic，如果是，那么调用 apic_deliver_pic_intr
如果不是，那么使用 cpu_interrupt 直接发送给 cpu 了。

- gsi_handler : irq routing
  - pic_set_irq : pic 的入口
    - pic_update_irq
      - qemu_irq_raise
        - pic_irq_request : 这个位置就是注册的 pic 中断的操作
          - pic_irq_request
            - apic_local_deliver
              - apic_set_irq
                - apic_update_irq
                  - cpu_interrupt

### tcg ioapic

- ioapic_realize
  - qdev_init_gpio_in(dev, ioapic_set_irq, IOAPIC_NUM_PINS);

发送给 ioapic 的中断的入口是 ioapic_set_irq，而经过 ioapic 通过调用 ioapic_service 将中断转发到 ioapic 上。

- ioapic_service
  - ioapic_entry_parse : 从 ioredtbl 中解析填充数据，填充 ioapic_entry_info
    - 在 ioapic_mem_write 可以修改 IOAPICCommonState::ioredtbl 来修改 ioapic 的中断路由。
  - stl_le_phys(ioapic_as, info.addr, info.data);
      - 实际上，这就是将中断发送到 apic 的过程，上面的注释也分析过，采用类似 msi 的过程
      - stl_le_phys 经过 QEMU 构建的 memory model
        - apic_mem_write
          - apic_send_msi : 最终选择正确的 CPU 进行中断的发送

```txt
- main 
  - qemu_main_loop 
    - main_loop_wait 
      - qemu_clock_run_all_timers 
        - timerlist_run_timers 
          - timerlist_run_timers 
            - update_irq 
              - qemu_irq_pulse 
                - gsi_handler 
                  - ioapic_set_irq 
                    - ioapic_service 
                      - stl_le_phys 
                        - address_space_stl_le 
                          - address_space_stl_internal 
                            - memory_region_dispatch_write 
                              - access_with_adjusted_size 
                                - memory_region_write_accessor 
                                  - apic_mem_write 
                                    - apic_send_msi 
```



## 整理一下这个东西
首先来回忆一下整个 vCPU 的执行流程

- `sigsetjmp(cpu->jmp_env, 0)`
-  while (!cpu_handle_exception(cpu))
    -  while (!cpu_handle_interrupt(cpu))
      - tb = tb_find()
      - cpu_loop_exec_tb(tb)

当需要插入中断的时候，最后调用到 cpu_interrupt 上，
```c
    cpu->interrupt_request |= mask;
```
在 cpu_handle_interrupt 中检测到 interrupt_request 上之后，会调用和 arch 相关的实现函数来处理:

- x86_cpu_exec_interrupt : 调用 x86 注册
  - x86_cpu_pending_interrupt : 获取当前到来的是什么中断
  - cpu_get_pic_interrupt
    - apic_get_interrupt : 获取 into
  - do_interrupt_x86_hardirq
    - do_interrupt_all
      - do_interrupt64 : 64 bit 的模式
      - do_interrupt_protected : 32 bit 的处理模式
        - 在其中解析 guest idt 的内容，然后让 guest 的执行跳转到该位置

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
