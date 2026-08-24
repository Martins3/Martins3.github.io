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

## 结合当前源码重新理解 BQL

本节基于 QEMU `v11.1.0-rc2-10-g1c69bcc8047e` 重写前面的分析。BQL 的准确定位是：
它是 QEMU system emulator
中最粗粒度的进程内互斥锁，用来保护全局状态以及尚未实现线程安全的设备模型代码。它提供了一个传统的串行执行域，但“所有设备模拟必须串行”不是
QEMU 的执行模型；能够自行处理并发的代码可以在 BQL 外运行。


### API 名称修改
接口对应关系是：

```txt
qemu_mutex_iothread_locked()  -> bql_locked()
qemu_mutex_lock_iothread()    -> bql_lock()
qemu_mutex_unlock_iothread()  -> bql_unlock()
```

当前声明位于 include/qemu/main-loop.h:259，实现位于 system/cpus.c:555。

所以移植旧代码时，通常直接替换即可：

```txt
if (bql_locked()) {
    ...
}
```

其判断语义没有改变：返回当前执行上下文是否持有 BQL。
对于 block layer，仍不建议调用它，应按照注释使用 qemu_in_main_thread()。

### BQL 本身

`system/cpus.c` 中的 `bql` 是一个全局 `QemuMutex`，由 `qemu_init_cpu_loop()`
初始化。`qemu_init_subsystems()` 随即获取它，所以机器创建、设备 realize 和 vCPU
创建的主要初始化阶段处在 BQL 临界区内；新建的 vCPU 线程会先在自己的线程入口获取
BQL，因此在初始化线程释放锁以前不能继续。

当前公开接口是 `include/qemu/main-loop.h` 中的 `bql_lock()`、`bql_unlock()` 和
`bql_locked()`，旧代码中的 `qemu_mutex_lock_iothread()`、`qemu_mutex_unlock_iothread()` 和
`qemu_mutex_iothread_locked()` 已经不再是当前接口。`bql_lock_impl()`
会断言当前执行上下文没有持锁，所以 BQL 不能递归获取。`BQL_LOCK_GUARD()`
则只在调用者尚未持锁时获取 BQL，适合既可能从 BQL 内、也可能从 BQL 外进入的函数。

`bql_locked()` 的状态使用 coroutine-local TLS
记录，不应该用线程身份猜测。对锁的基本约束是：BQL
是最外层的粗粒度锁，若还需要别的锁，通常应当先取 BQL；
但持有 BQL 并不会令 RAMList、block layer、AioContext 或设备私有锁失去作用。

### 各线程何时持有 BQL

main loop、vCPU thread 和 `IOThread` 的 BQL 关系并不相同：

- main loop 是 BQL 的默认执行域。`system/main.c` 中的 `qemu_default_main()` 获取
  BQL 后进入 `qemu_main_loop()`。`util/main-loop.c` 中的
  `os_host_main_loop_wait()` 只在 `qemu_poll_ns()` 阻塞等待外部事件时释放
  BQL，并在分发 GLib callback 之前重新获取；`main_loop_wait()` 随后的 notifier
  和 timer callback 也在 BQL 内执行。
- KVM vCPU thread 在 `kvm_vcpu_thread_fn()` 中获取
  BQL，用它处理停止、热拔除和排队工作等控制面事件。`kvm_cpu_exec()` 在进入
  `KVM_RUN` 前释放 BQL，并且处理 `KVM_EXIT_IO`、`KVM_EXIT_MMIO`
  时仍然没有重新获取；具体的 I/O 访问路径按目标 `MemoryRegion`
  的要求决定是否获取 BQL。退出本轮 vCPU 执行后，`kvm_cpu_exec()` 才重新获取
  BQL。
- MTTCG 的每个 vCPU thread 在 `mttcg_cpu_thread_fn()` 中持有 BQL
  处理控制面事件，在调用 `tcg_cpu_exec()` 执行 guest
  代码前释放它。`rr_cpu_thread_fn()` 使用一个 host thread 轮流执行多个
  vCPU，但执行 guest 代码时同样释放 BQL。
- `iothread.c` 中的 `iothread_run()` 直接运行自己的 `AioContext`，没有获取
  BQL。设备放入 `IOThread` 的目的正是让数据面 callback 在 BQL
  外并行执行；共享状态必须使用设备私有锁、原子操作、RCU 或 event-loop ownership
  等机制保护。main loop 和 `IOThread` 都是事件循环，但不能把二者统称为“持有 BQL
  的 iothread”。
- worker thread、migration thread 和 QMP OOB handler 通常也不天然持有
  BQL。需要访问 BQL 保护的全局状态时，它们必须在明确的边界内获取
  BQL，或者把工作调度到 main loop。

因此，更准确的总体执行图是：

```txt
main loop:  [持有 BQL，分发 callback/timer] -> [释放 BQL，poll 等待] -> [重新持有]
vCPU:       [持有 BQL，处理控制事件]       -> [释放 BQL，执行 guest] -> [按需进入 BQL]
IOThread:   [不持有 BQL，运行自己的 AioContext 和设备数据面 callback]
```

### vCPU 访问 RAM 和设备

不能把所有 pio/mmio 访问都概括成“vCPU 先获取 BQL”：

- 普通 RAM 是 direct `MemoryRegion`。地址转换和对象生命周期主要依赖 RCU，读写
  RAM 的快路径不需要 BQL。
- 对 MTTCG 的设备访问，`accel/tcg/cputlb.c` 中的
  `do_ld_mmio_beN()`、`do_st_mmio_leN()` 等当前使用 `BQL_LOCK_GUARD()` 包住
  `MemoryRegionOps` callback。
- KVM exit 以及 `address_space_read()`、`address_space_write()`
  一类通用访问会进入 `system/physmem.c` 中的
  `prepare_mmio_access()`。调用者未持有 BQL 且 `MemoryRegion::lockless_io` 为
  false 时，它只在本次 `memory_region_dispatch_read()` 或
  `memory_region_dispatch_write()` 周围获取 BQL，随后立即释放。
- 设备可通过 `memory_region_enable_lockless_io()` 声明该 region 的 I/O callback
  能够自行处理并发。此时走 `prepare_mmio_access()` 的路径不替它获取
  BQL，设备必须提供细粒度锁或 lock-free 方案。
- ioeventfd 数据面还可以绕过普通的 `MemoryRegionOps` callback，由 `IOThread`
  中的 event notifier callback 处理。例如 virtio-blk 要求 transport 支持
  notifier，数据面依靠自己的队列和 block-layer 同步，而不是依靠 BQL 串行化全部
  I/O。

BQL 因而是设备访问的默认兼容方案，而不是不可绕过的 I/O 总锁。判断一个 callback
是否可并发，必须同时检查它运行在哪个 `AioContext`、对应 `MemoryRegion` 是否允许
lockless I/O，以及设备自身的同步约束。

### 一个必须依赖 BQL 的具体例子：NVMe queue 与 controller reset

当前 NVMe 设备是说明 BQL 作用的好例子。`hw/nvme/ctrl.c` 在 realize
阶段通过 `memory_region_init_io()` 创建 `NvmeCtrl::iomem`，其 callback 是
`nvme_mmio_read()` 和 `nvme_mmio_write()`，但没有对这个 region 调用
`memory_region_enable_lockless_io()`。因此，从 KVM vCPU 发起的普通 NVMe BAR
访问会经过如下路径：

```txt
KVM vCPU thread（原本没有 BQL）
  -> kvm_cpu_exec()
    -> KVM_EXIT_MMIO
      -> address_space_rw()
        -> prepare_mmio_access()
          -> bql_lock()
            -> nvme_mmio_write()
              -> nvme_write_bar() 或 nvme_process_db()
          -> bql_unlock()
```

考虑下面两个可以来自不同执行上下文的操作：

1. vCPU 0 写 Submission Queue Tail Doorbell。`nvme_process_db()` 取得
   `n->sq[qid]`，更新 `NvmeSQueue::tail`，再调度该 SQ 的 BH。main loop 随后在
   `nvme_process_sq()` 中读取 guest command，递增 `sq->head`，从
   `sq->req_list` 取出 `NvmeRequest`，把它移入 `sq->out_req_list`，并通过
   `n->cq[sq->cqid]` 找到 completion queue。
2. vCPU 1 向 `CC` 写入 `EN=0` 关闭 controller。`nvme_write_bar()` 会调用
   `nvme_ctrl_reset()`；后者遍历并释放全部 SQ/CQ。`nvme_free_sq()` 不仅把
   `n->sq[sqid]` 置为 `NULL`，还删除 `sq->bh`、释放 `sq->io_req`，最后释放动态分配的
   `NvmeSQueue`。

如果这两个路径之间没有 BQL，可能出现如下交错：

```txt
main loop / vCPU 0                       vCPU 1
------------------------------------    --------------------------------
sq = n->sq[qid]
req = QTAILQ_FIRST(&sq->req_list)
                                        nvme_ctrl_reset()
                                          nvme_free_sq(sq, n)
                                            n->sq[qid] = NULL
                                            qemu_bh_delete(sq->bh)
                                            g_free(sq->io_req)
                                            g_free(sq)
继续访问 sq、req 或 n->cq[sq->cqid]
```

最后一步会成为 use-after-free，或者破坏 request 的 `QTAILQ`。这里不是仅给
`sq->tail` 使用原子变量就能解决，因为 reset 改变的是整组对象的生命周期：SQ、CQ、BH、
request array 及链表必须作为一个一致的设备状态被观察。

当前实现让这两个路径共享 BQL：NVMe MMIO region 的 callback 默认由
`prepare_mmio_access()` 加锁，main loop 分发 SQ BH 时也持有
BQL。`nvme_process_sq()` 甚至直接使用 `assert(bql_locked())`
声明这一前提。因此 controller reset 要么发生在一次 queue processing
之前，要么发生在其完整结束之后，不会在中途释放它正在使用的对象。

migration 提供了同一设计的另一个佐证。`nvme_ctrl_pre_save()` 需要取消所有 SQ
BH、drain I/O 并遍历 request 状态；它和 `nvme_process_sq()` 都明确注释由 BQL
避免彼此竞态，并且都断言已经持有 BQL。仅仅停止 vCPU
仍不足以替代这里的锁，因为已调度的 main-loop BH 和 migration thread 也可能访问同一个
`NvmeCtrl`。

即使启用 NVMe 的 `ioeventfd` 属性，这个实现也没有取消上述前提。
`nvme_init_sq_ioeventfd()` 使用 `event_notifier_set_handler()` 把
`nvme_sq_notifier()` 注册到 main loop；notifier 最终仍调用带有 BQL 断言的
`nvme_process_sq()`。ioeventfd 可以避免每次 doorbell 都产生普通的 KVM MMIO
exit，但不等于 NVMe queue state 已经变成 lockless 或已迁移到独立 `IOThread`。

严格地说，NVMe 需要的是“对 queue processing、reset 和 migration
状态建立共同的同步与生命周期协议”，并非理论上只能使用 BQL。如果为每个 queue
增加私有锁和引用计数，设计好 reset/drain 顺序，并把 migration
与所有数据面线程同步，也可以把这些路径移出 BQL；但在当前源码中并不存在这样一套替代协议，
所以 BQL 是该设备正确性的一部分，而不只是性能上的保守选择。

### vCPU thread 与 main loop thread 为什么需要互斥

vCPU thread 和 main loop thread 并不是始终互斥。vCPU 执行 guest 指令、访问普通
RAM 时不持有 BQL；main loop 阻塞在 `qemu_poll_ns()` 时也不持有 BQL。只有它们准备
进入同一个 legacy 全局状态或设备模型临界区时，才通过 BQL 汇合：

```txt
vCPU thread:     guest code -> MMIO/PIO exit -> 获取 BQL -> 设备 callback -> 释放 BQL
main loop:       poll 返回  -> 重新获取 BQL  -> timer/BH/fd/QMP callback -> 再次 poll
```

如果 main loop 的 poll 已经返回，但某个 vCPU 正在设备 callback 中持有 BQL，main
loop 会阻塞在重新获取 BQL 的位置；反过来，如果 main loop 正在执行 timer/BH，vCPU
会阻塞在 `prepare_mmio_access()` 或 `BQL_LOCK_GUARD()`。锁竞争只推迟共享状态的
callback，不会要求其他仍在执行普通 guest code 的 vCPU 全部停下。

这样既允许 guest CPU 执行与 main loop 并行，又保证来自 guest 和 host event loop
的两个入口不会同时破坏同一份模拟状态。典型场景如下：

| vCPU thread 的入口 | main loop thread 的入口 | 共享状态 | 没有互斥的后果 |
| --- | --- | --- | --- |
| MMIO/PIO register callback | timer callback | 计数器配置、timer deadline、IRQ level | 丢失重编程、错误超时、伪中断 |
| doorbell/register callback | BH 或 I/O completion callback | queue head/tail、request 链表、CQ/SQ | 重复消费、链表破坏、use-after-free |
| 设备 MMIO callback | reset、hot-unplug、in-band QMP command | `DeviceState`、`MemoryRegion` 和设备私有对象 | callback 访问已重置或释放的对象 |
| 中断确认或设备寄存器访问 | timer/fd callback 注入中断 | interrupt controller 和 CPU interrupt state | 丢中断、重复中断或错误优先级 |

#### 场景一：vCPU 的设备访问与 main loop 的 timer/BH

前面的 NVMe 例子本质上已经是 vCPU 与 main loop 的互斥：vCPU 在
`nvme_process_db()` 或 `nvme_write_bar()` 中修改 controller，main loop 则在 SQ BH
的 `nvme_process_sq()` 中消费同一组 queue 和 request。BQL 不仅防止两个 vCPU
同时改 NVMe，也防止 main loop 正在使用 `NvmeSQueue` 时 vCPU 通过 `CC.EN=0`
释放它。

8254 PIT 展示了另一种更常见的 timer 竞态。`hw/timer/i8254.c` 中：

- vCPU 的 PIO write 进入 `pit_ioport_write()`。写入过程会分多步更新
  `write_state`、`write_latch`、`mode`、`count` 和 `count_load_time`，
  `pit_load_count()` 还会重新计算并修改 IRQ timer。
- main loop 从 `main_loop_wait()` 返回后，在 BQL 内运行 timer。`pit_irq_timer()`
  通过 `pit_irq_timer_update()` 读取上述字段，计算输出电平与下一次
  `expire_time`，然后调用 `qemu_set_irq()` 和 `timer_mod()`。

假设没有 BQL，可能发生下面的交错：

```txt
main loop:  根据旧的 count/mode 计算出旧 expire_time
vCPU:       写入新的 count、count_load_time，并 timer_mod(new_expire_time)
main loop:  继续用旧 expire_time 调用 timer_mod(old_expire_time)
```

guest 的新定时器配置就被旧 timer callback 覆盖了。另一个可能是 callback 读到新的
`count` 却配上旧的 `count_load_time` 或 `mode`，从而计算出一个从未真实存在过的
PIT 状态。BQL 把整个 `pit_ioport_write()` 与整个 `pit_irq_timer()` 排成先后顺序，保护的
是多字段不变量，而不只是某一个整数的原子读写。

在传统 PC machine 中，PIT 的 timer 最终通过 `qemu_set_irq()` 进入 8259 PIC 的
`pic_set_irq()`，修改 `PICCommonState::irr` 和 `last_irr`；与此同时，vCPU 可能通过
`pic_ioport_write()` 修改 `imr`、`isr` 和 `priority_add`，例如发送 EOI 或改变 mask。
这些操作同样必须按一个确定顺序发生，否则一次 IRQ edge 可能与 EOI/mask 更新互相覆盖。

#### 场景二：vCPU 的设备访问与 main loop 的 reset/hot-unplug

普通 in-band QMP handler 在 main thread 中持有 BQL。例如 `qmp_device_del()` 会进入
`qdev_unplug()`，同步 unplug 最终可能 unrealize 并解除设备对象的 parent；设备的
unrealize callback 会删除 `MemoryRegion`、timer、BH、notifier 和设备私有内存。

与此同时，vCPU 可能刚因 MMIO/PIO exit 进入该设备的 `MemoryRegionOps` callback。
如果 hot-unplug 能与 callback 并行，仅仅把 region 从新的 memory topology 中移除并不
能保证安全：已经完成地址翻译的 vCPU 仍可能握有旧 region 或 opaque device pointer。
BQL 让 legacy device callback 与 main-loop teardown 互斥，使 teardown 必须等待当前
callback 返回。

这里还会配合 RCU 和引用计数，而不是只靠 BQL：`AddressSpace::current_map` 的 reader
可以在旧 `FlatView` 上完成访问，BQL 外保存 `MemoryRegion` 时还需要
`memory_region_ref()`。RCU 解决旧 mapping 何时可回收，引用计数解决 owner
生命周期，BQL 则解决设备内部 callback 与控制面修改不能并发的问题。

#### 场景三：main loop 注入事件与 vCPU 消费事件

main loop 的 timer、fd callback 或 BH 经常需要向 vCPU 注入中断或改变设备 IRQ
level；vCPU 则会确认中断、执行 EOI、读取设备状态并继续运行。这不是单个 pending bit
的问题：`interrupt_request` 的 bit 可以用原子操作传递，但 APIC/PIC、设备 IRQ
状态、CPU halted/reset 状态及相关 side effect 往往涉及多个对象。

所以当前设计是“原子通知加 BQL 状态转换”：生产者用原子 bit 和 kick 让正在 BQL
外执行的 vCPU 尽快退出，vCPU 在 `cpu_handle_interrupt()` 中取得 BQL 后处理复杂状态。
BQL 提供状态转换的全序，原子操作负责 BQL 外的跨线程可见性和唤醒；两者解决的问题不同。

#### 哪些时候不需要二者互斥

- vCPU 执行 guest 指令或访问 direct RAM 时，main loop 可以同时处理 callback。
- main loop 在 `qemu_poll_ns()` 中等待事件时会释放 BQL，vCPU 可以进入设备 callback。
- 只通过原子变量传递的简单通知可以不持有 BQL，但后续复杂状态处理仍可能需要它。
- 标记为 lockless I/O 的 `MemoryRegion` 不依赖 BQL，其设备必须提供自己的同步协议。
- 独立 `IOThread` 的 callback 默认不持有 BQL，它与 vCPU/main loop 的共享状态也必须用
  设备私有锁、AioContext ownership、RCU、drain 或消息传递保护。

因此，判断 vCPU thread 与 main loop thread 是否需要互斥，不能只看“一个执行 CPU、
一个处理 I/O”。真正的判断标准是：两个入口是否会同时访问同一份可变状态，是否需要维持
跨字段不变量，以及其中一个入口是否可能销毁另一个入口正在使用的对象。

### 中断、vCPU work 和 exclusive section

`CPUState::interrupt_request` 也不是简单地“由 BQL 保护”。当前
`cpu_set_interrupt()` 使用 `qatomic_or()` 注入 bit，`cpu_reset_interrupt()` 使用
`qatomic_and()` 清除 bit，`cpu_test_interrupt()` 使用 acquire load
读取。`cpu_interrupt()` 这个高层入口仍要求持有 BQL，MTTCG 的
`cpu_handle_interrupt()` 在修改更多 CPU/设备状态前也会获取
BQL。这里实际组合了两层机制：原子操作保证 pending bit 的跨线程通信，BQL
串行化中断处理引发的复杂模拟状态变化。

`async_run_on_cpu()` 的普通 callback 由 vCPU 在持有 BQL 的
`process_queued_cpu_work()` 中执行。`async_safe_run_on_cpu()` 的语义不同：其
callback 要在所有其他 vCPU 都退出 `cpu_exec` 的 exclusive section
中执行。`process_queued_cpu_work()` 会先释放 BQL，再调用
`start_exclusive()`，避免其他 vCPU 为进入 BQL 而睡眠、当前 vCPU
又等待它退出所形成的死锁。

BQL 与 exclusive section 不能互换：持有 BQL 不代表 vCPU 已停止，因为 vCPU
正常执行 guest 代码时本来就在 BQL 外；exclusive section 解决的是“其他 vCPU 不在
`cpu_exec_start()` 与 `cpu_exec_end()` 之间执行”的问题。

### 内存拓扑和 migration

`memory_region_transaction_commit()` 明确要求持有 BQL，因此内存拓扑的 writer
被串行化；`AddressSpace::current_map` 通过 RCU 发布新的 `FlatView`，reader
可以继续使用旧 view 完成访问。若代码在 BQL 外长期保存可能被 hot-unplug 的
`MemoryRegion`，还必须使用 `memory_region_ref()` 保证其 owner
的生命周期。原文关于“更新可与旧映射上的访问并行”的方向是对的，但 RCU
只解决对象发布和回收，不自动保护设备内部状态。

migration 也不是“全程持有 BQL 保存所有 CPU 状态”。`migration_thread()` 的主体在
BQL 外运行，只在 setup、精确 dirty-bitmap 同步、switchover
和最终设备状态等临界阶段获取 BQL。最终切换还会通过 `migration_stop_vm()` 停止
VM，并配合 dirty tracking、RCU、RAMList mutex、bitmap mutex、multifd
同步和各设备自己的迁移约束取得一致状态。

当前 `ram_init_bitmaps()` 自己获取 RAMList mutex，而它的 setup 调用链又处在
migration thread 显式获取的 BQL
内。两把锁保护的是不同对象和不同的并发来源；“持有 BQL 后就屏蔽了所有其他
lock”这一推论不成立。

### 对前文的勘误

- “使用 BQL 是因为设备模拟是串行的”因果倒置。BQL 是让大量 legacy
  全局状态和设备模型获得串行语义的兼容机制；QEMU 正在用
  `IOThread`、设备私有锁、原子操作、RCU 和 lockless I/O 缩小这个串行域。
- “iothread 调用的 callback 全部需要持锁”不正确。main loop callback 通常持有
  BQL，真正的 `IOThread` callback 则默认不持有 BQL。
- “`KVM_RUN` 返回后马上重新获取 BQL”不符合当前代码。`kvm_cpu_exec()` 在 BQL
  外处理大多数 KVM exit，普通设备 region 由 `prepare_mmio_access()` 在 dispatch
  周围按需加锁。
- `store_helper/load_helper -> io_readx/io_writex -> qemu_mutex_iothread_locked()`
  是旧版本路径和旧 API。当前 MTTCG MMIO helper 使用 `BQL_LOCK_GUARD()`，通用
  AddressSpace 路径使用 `prepare_mmio_access()`。
- “pio/mmio 访问一定持有 BQL”过于绝对。direct RAM 不需要 BQL，允许 lockless I/O
  的 region 以及 `IOThread` 数据面也可以在 BQL 外执行。
- “持有 BQL 就屏蔽了所有 lock”错误。BQL 只是最外层的粗粒度锁，RAMList、block
  layer、AioContext、RCU 和设备私有锁仍各自承担同步或生命周期职责。
- “`interrupt_request` 由 BQL 保护”不完整。pending bit
  本身使用原子读改写；高层注入接口和复杂中断状态处理才使用 BQL。
- “`process_queued_cpu_work()` 中的 hook 都持有 BQL”只适用于普通 work item。由
  `async_safe_run_on_cpu()` 提交的 exclusive work item 会在 BQL 外执行。
- migration 不能只靠 BQL 获得一致快照。BQL 不会停止正在 BQL 外执行 guest 的
  vCPU，最终一致性还依赖停止 VM、dirty tracking 和各子系统同步。

## 我有一个问题，设备模拟的代码，主要在那个 thread 中执行的?

**似乎**

非常看有没有 ioeventfd

如果有，那么工作就在 main loop 中，

如果没有，那么就是 vCPU thread 中完成了

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
