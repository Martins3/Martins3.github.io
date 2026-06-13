## Big QEMU Lock

使用 Big QEMU Lock (下面简称为 BQL) 是因为设备的模拟是串行的。
比如 pic 中断控制器在 QEMU 中描述在 `hw/intc/i8259.c` 中, pic 的状态保存在 `PICCommonState` 中，多个 vCPU thread 访问 pic 的时候，
那就需要靠 BQL 来实现互斥，只能逐个调用 pic 的模拟函数，也就是 pic_ioport_read /  pic_ioport_write 。
如果一个 vCPU 在执行 `pic_ioport_write`，另一个 vCPU 在 pic_ioport_read 的时候，其获取的状态可能错误的中间状态。

回忆一下，[QEMU 中的线程和事件循环](https://martins3.github.io/qemu/threads.html) 中 QEMU 的执行模型:
- vCPU 在执行过程中，通过 pio / mmio 访问设备, 其模拟最后是通过调用 MemoryRegionOps 实现的
- vCPU 可以将其耗时操作 offload 到 iothread (main loop 或者 IOThread)上，所以 iothread 做的事情就是在进行设备访问。

按照这种指导思想可以很容易确认下面的位置的 BQL 的使用:
- vCPU 执行的时候无需上锁
  - kvm_cpu_exec : 在 `kvm_vcpu_ioctl(cpu, KVM_RUN, 0)` 之前 unlock，之后 lock 上
  - mttcg_cpu_thread_fn : 同上，原理类似，只是 accel 是 tcg，每一个 thread 模拟一个 vCPU
  - rr_cpu_thread_fn :  同上, 原理类似，accel 是 tcg，一个 thread 模拟多个 vCPU
- vCPU 进行 IO 之前需要上锁的，回忆[QEMU softmmu 访存 helper 整理](https://martins3.github.io/qemu/softmmu-functions.html) 中分析的访问设备的路径:
  - store_helper / load_helper => io_readx / io_writex 中会直接调用 qemu_mutex_iothread_locked 来做出判断来，如果没有上锁会进行上锁的
  - flatview_write_continue / flatview_read_continue / memory_ldst.inc.c 会调用 prepare_mmio_access 来保证接下来的执行是持有锁的
- iothread 调用的 callback 全部的需要在有锁的条件下进行的，请看 `os_host_main_loop_wait` 的实现

## BQL Advanced Topic
但是实际上，BQL 的使用位置要上面多一点，这些是高级话题，可以暂时跳过。

### migration
- migration[^1] 相关的。因为需要保存所有的 cpu 的状态，所以自然需要持有 BQL 的，其关联的文件为：

实际上，和 migration 相关的还有 `qemu_mutex_lock_ramlist`，从原则上将当持有了 BQL 的时候，就屏蔽了所有的 lock 的。
但是在 `ram_init_bitmaps` 首先上锁 BQL，然后是 ramlist 的。
具体可以从 `b2a8658ef5dc57ea` 分析，有待进一步跟进。

### main loop
main loop 中上锁位置非常的早，在 `pc_init1 => qemu_init_subsystems` 中几乎是 BQL 初始化之后就会获取。

创建的 vCPU 例如 `mttcg_cpu_thread_fn` 因为无法获取 BQL 而无法进一步执行，一切都需要等待 main loop 初始化好。

如果 cpu realize 失败，会调用 `x86_cpu_unrealizefn => cpu_remove_sync` 来清理资源包括释放 vCPU 的，为了让 vCPU 进一步执行，所以 cpu_remove_sync 中需要短暂的释放 BQL

在 [QEMU 中的线程和事件循环](https://martins3.github.io/qemu/threads.html)中，我们分析了 main loop 如何实现事件监听。
当 vCPU thread 需要模拟设备操作，比如 DMA 的时候，最后会调用具体设备的 callback 函数，
但是 vCPU thread 不会等待下去，而是将其中 callback 函数让 main loop 执行。
而 main loop 就是靠事件监听来知道有 vCPU 提交任务给他了。
当 main loop 执行完成之后， 只需要向 vCPU 发送一个中断，也即是最后调用到 `tcg_handle_interrupt`,
向 CPUState::interrupt_request 插入一个中断，而 tcg 执行的时候，每一个 tb 都会检查这个，如果插入了中断，就会退出 ，
最后在 `cpu_handle_interrupt` 地方处理。

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

### interrupt_request

因为一个 CPU 利用 ipi 机制给另一个 vCPU 发送中断，所以 interrupt_request 需要被 BQL 保护，其调用位置为:

- `cpu_check_watchpoint` => tcg_handle_interrupt
- `cpu_handle_halt` => apic_poll_irq / cpu_reset_interrupt
- `cpu_handle_exception`
- `edu_fact_thread` => edu_raise_irq => msi_notify / pci_set_irq
- `helper_write_crN` => cpu_set_apic_tpr

- 注入位置
  - tcg_handle_interrupt :  将 mask 插入到 CPUState::interrupt_request
  - cpu_reset_interrupt : 将 mask 从 CPUState::interrupt_request 中清理
- 使用位置 : cpu_handle_interrupt => TCGCPUOps::cpu_exec_interrupt => x86_cpu_exec_interrupt 的

因为中断的注入可能来自于 main loop 或者是其他的 vCPU thread，所以同样这个需要 BQL 的保护

### qemu_mutex_iothread_locked
下面来讨论一下一些持有 BQL 的位置

- process_queued_cpu_work : 是持有 BQL 的，所以在 start_exclusive 的时候首先需要释放 BQL
  - 所以 async_run_on_cpu 的 hook 执行的时候也是有 BQL 的

- cputlb.c 中 io_readx 和 io_writex 中会检测，当没有 locked 时候，然后一定上锁
  - io_readx 和 io_writex 只是被 load_helper 和 store_helper 调用的
  - 但是 store_helper 和 load_helper 的调用来源有两个位置，一个是执行流中，一个中通过 cpu 访问虚拟地址的 helper，例如 `target/i386/tcg/seg_helper.h` 中定义的函数访问的，后者可能是在有 BQL 的环境中调用的

- memory_region_transaction_commit : 这个可以保证不存在多个 thread 同时修改 memory mapping ，但是可以一个在修改，另一个还在访问, 这是因为 AddressSpace::current_map 的访问是通过 rcu 的。


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
