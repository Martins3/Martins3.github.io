# QEMU 中的锁
<!-- vim-markdown-toc GitLab -->

- [Big QEMU Lock](#big-qemu-lock)
- [BQL Advanced Topic](#bql-advanced-topic)
  - [migration](#migration)
  - [main loop](#main-loop)
  - [rcu](#rcu)
  - [interrupt_request](#interrupt_request)
  - [qemu_mutex_iothread_locked](#qemu_mutex_iothread_locked)
- [resources shared between vCPU thread](#resources-shared-between-vcpu-thread)
- [tcg vCPU thread](#tcg-vcpu-thread)
    - [lifecycle of vCPU thread](#lifecycle-of-vcpu-thread)
    - [exit_request](#exit_request)
    - [halt](#halt)
- [locks between vCPU](#locks-between-vcpu)
      - [sync between main loop and vCPU](#sync-between-main-loop-and-vcpu)
    - [current_cpu](#current_cpu)
    - [setlongjmp](#setlongjmp)
  - [queue_work_on_cpu](#queue_work_on_cpu)
  - [exclusive context](#exclusive-context)
- [misc](#misc)
  - [mmap_lock](#mmap_lock)

<!-- vim-markdown-toc -->
之所以使用 lock ，是因为存在共享的资源。

## Big QEMU Lock
使用 Big QEMU Lock (下面简称为 BQL) 是因为设备的模拟是串行的。
比如 pic 中断控制器在 QEMU 中描述在 `hw/intc/i8259.c` 中, pic 的状态保存在 `PICCommonState` 中，多个 vCPU thread 访问 pic 的时候，
那就需要靠 BQL 来实现互斥，只能逐个调用 pic 的模拟函数，也就是 pic_ioport_read /  pic_ioport_write 。
如果一个 vCPU 在执行 pic_ioport_write，另一个 vCPU 在 pic_ioport_read 的时候，其获取的状态可能错误的中间状态。

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
    - migration/block.c
    - migration/colo.c
    - migration/migration.c
    - migration/ram.c
    - migration/savevm.c
    - migration/block-dirty-bitmap.c
    - hw/vfio/migration.c

实际上，和 migration 相关的还有 qemu_mutex_lock_ramlist，从原则上将当持有了 BQL 的时候，就屏蔽了所有的 lock 的。
但是在 ram_init_bitmaps 首先上锁 BQL，然后是 ramlist 的。
具体可以从 `b2a8658ef5dc57ea` 分析，有待进一步跟进。

### main loop
main loop 中上锁位置非常的早，在 `pc_init1 => qemu_init_subsystems` 中几乎是 BQL 初始化之后就会获取。

创建的 vCPU 例如 `mttcg_cpu_thread_fn` 因为无法获取 BQL 而无法进一步执行，一切都需要等待 main loop 初始化好。

如果 cpu realize 失败，会调用 `x86_cpu_unrealizefn => cpu_remove_sync` 来清理资源包括释放 vCPU 的，为了让 vCPU 进一步执行，所以 cpu_remove_sync 中需要短暂的释放 BQL

在 [QEMU 中的线程和事件循环](https://martins3.github.io/qemu/threads.html)中，我们分析了 main loop 如何实现事件监听。当 vCPU thread 需要模拟设备操作，比如 DMA 的时候，最后会调用
具体设备的 callback 函数，但是 vCPU thread 不会等待下去，而是将其中 callback 函数让 main loop 执行。而 main loop 就是靠事件监听来知道有 vCPU 提交任务给他了。当 main loop 执行完成之后，
只需要向 vCPU 发送一个中断，也即是最后调用到 `tcg_handle_interrupt`, 向 CPUState::interrupt_request 插入一个中断，而 tcg 执行的时候，每一个 tb 都会检查这个，如果插入了中断，就会退出，最后在 `cpu_handle_interrupt` 地方处理。

```c
/*
#0  apic_send_msi (msi=0x7fffffffd110) at ../hw/intc/apic.c:726
#1  0x0000555555c6ab4c in apic_mem_write (opaque=<optimized out>, addr=4100, val=48, size=<optimized out>) at ../hw/intc/apic.c:757
#2  0x0000555555cd2711 in memory_region_write_accessor (mr=mr@entry=0x55555698bc90, addr=4100, value=value@entry=0x7fffffffd298, size=size@entry=4, shift=<optimized out
>, mask=mask@entry=4294967295, attrs=...) at ../softmmu/memory.c:492
#3  0x0000555555cceb9e in access_with_adjusted_size (addr=addr@entry=4100, value=value@entry=0x7fffffffd298, size=size@entry=4, access_size_min=<optimized out>, access_
size_max=<optimized out>, access_fn=access_fn@entry=0x555555cd2680 <memory_region_write_accessor>, mr=0x55555698bc90, attrs=...) at ../softmmu/memory.c:554
#4  0x0000555555cd1c47 in memory_region_dispatch_write (mr=mr@entry=0x55555698bc90, addr=4100, data=<optimized out>, data@entry=48, op=op@entry=MO_32, attrs=attrs@entry
=...) at ../softmmu/memory.c:1504
#5  0x0000555555ca12ed in address_space_stl_internal (endian=DEVICE_LITTLE_ENDIAN, result=0x0, attrs=..., val=48, addr=<optimized out>, as=0x0) at /home/maritns3/core/k
vmqemu/include/exec/memory.h:2868
#6  address_space_stl_le (as=as@entry=0x555556606820 <address_space_memory>, addr=<optimized out>, val=48, attrs=attrs@entry=..., result=result@entry=0x0) at /home/mari
tns3/core/kvmqemu/memory_ldst.c.inc:357
#7  0x0000555555cee124 in stl_le_phys (val=<optimized out>, addr=<optimized out>, as=0x555556606820 <address_space_memory>) at /home/maritns3/core/kvmqemu/include/exec/
memory_ldst_phys.h.inc:121
#8  ioapic_service (s=s@entry=0x555556a42360) at ../hw/intc/ioapic.c:138
#9  0x0000555555cee3ff in ioapic_set_irq (opaque=0x555556a42360, vector=<optimized out>, level=1) at ../hw/intc/ioapic.c:186
#10 0x0000555555b92664 in gsi_handler (opaque=0x555556af6ff0, n=0, level=1) at ../hw/i386/x86.c:600
#11 0x0000555555b42a9e in qemu_irq_pulse (irq=0x555556ab3b50) at /home/maritns3/core/kvmqemu/include/hw/irq.h:22
#12 update_irq (timer=<optimized out>, set=<optimized out>) at ../hw/timer/hpet.c:219
#13 0x0000555555e6fe88 in timerlist_run_timers (timer_list=0x555556707120) at ../util/qemu-timer.c:573
#14 timerlist_run_timers (timer_list=0x555556707120) at ../util/qemu-timer.c:498
#15 0x0000555555e70097 in qemu_clock_run_all_timers () at ../util/qemu-timer.c:669
#16 0x0000555555e4ced9 in main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:542
#17 0x0000555555c58231 in qemu_main_loop () at ../softmmu/runstate.c:726
#18 0x0000555555940c92 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:50
```

### rcu
在 call_rcu_thread 中，需要持有 lock 才可以释放资源，这很奇怪。既然都是可以开始来执行 hook 函数了，说明这些资源已经是没有人使用的，那么为什么还需要使用 BQL 保护。
其原因在: https://lists.gnu.org/archive/html/qemu-devel/2015-02/msg03170.html

### interrupt_request
因为一个 CPU 利用 ipi 机制给另一个 vCPU 发送中断，所以 interrupt_request 需要被 BQL 保护，其调用位置为:

- cpu_check_watchpoint => tcg_handle_interrupt
- cpu_handle_halt => apic_poll_irq / cpu_reset_interrupt
- cpu_handle_exception
- edu_fact_thread => edu_raise_irq => msi_notify / pci_set_irq
- helper_write_crN => cpu_set_apic_tpr

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

## resources shared between vCPU thread
在这里，重新总结一下被 vCPU 共享的资源，以及建立起来的 lock
因为 vCPU 存在一些共享资源，所以需要也是需要互斥的，下面罗列一些:

- remote TLB flush : 一个 vCPU 需要 flush 另一个 vCPU 的 soft TLB，这个事情通过 [queue_work_on_cpu](#queue_work_on_cpu) 实现的
- ipi : 所以 [interrupt_request](#interrupt_request) 需要被 BQL 保护的
- tcg_region_state::lock : 因为 tb buffer 是划分为一个个的 region 的，对于这些 region 操作
- tcg_region_tree::lock : 在每一个 region 中存在一个 GTree 来记录从 retaddr 到 TranslationBlock 结构体的映射。看了下 Glib 的源码 ./glib/gtree.c，其不是 thread safe 的，所以需要外部上锁保护。
- PageDesc::lock : 用于保护一个 guest page 上翻译的所有 tb 。
- TBContext::htable : 根据物理地址找到 TranslationBlock 的映射。有的 vCPU 因为 SMC 可能在修改，而另一个 vCPU 在使用，所以需要考虑共享的问题。qht 的实现利用了 rcu 机制。

## tcg vCPU thread
- rr_cpu_thread_fn : 使用一个 thread 模拟所有的 vCPU
- mttcg_cpu_thread_fn : 每一个 thread 模拟一个 vCPU

QEMU 只有两种模式，不存在有的 thread 模拟多个，有的模拟一个的鬼畜情况，那只是徒增复杂了。
一个 thread 模拟一个 vCPU 当然是很自然的，但是多线程带来很多挑战，具体可以参考 [mttcg](./mttcg.md)

rr 和 mttcg 的执行的相似指出在于，都是调用 `tcg_cpus_exec` 执行的，其展开之后大致如下:
```c
    cpu_exec_start(cpu);
    cpu_exec_enter(cpu);
    while (!cpu_handle_exception(cpu, &ret)) {
        while (!cpu_handle_interrupt(cpu, &last_tb)) {
          tb_gen_code
          cpu_loop_exec_tb
        }
    }
    cpu_exec_exit(cpu);
    cpu_exec_end(cpu);
```
其中，cpu_exec_enter 和 cpu_exec_exit 调用 arch 相关的 hook

对比 rr_cpu_thread_fn 和 mttcg_cpu_thread_fn 的执行流程:

* rr_cpu_thread_fn

```c
while (1) {
  while (cpu && cpu_work_list_empty(cpu) && !cpu->exit_request) {
    qatomic_mb_set(&rr_current_cpu, cpu);
    current_cpu = cpu;

    tcg_cpus_exec()

    cpu = CPU_NEXT(cpu); // 在这里轮转需要使用的 cpu
  }

  rr_wait_io_event();
}
```

* mttcg_cpu_thread_fn

```c
while (!cpu->unplug || cpu_can_run(cpu)){
    if (cpu_can_run(cpu)) {
      tcg_cpus_exec()
    }

    qatomic_mb_set(&cpu->exit_request, 0);
    qemu_wait_io_event(cpu);
}
```

下面总结一下 rr 和 mttcg 的实现差异:
- 在 rr_cpu_thread_fn 中多出来的一个 while loop 在于要轮转 vCPU:
- 同时 rr_cpu_thread_fn 为了保证每一个 vCPU 都可以运行一段时间的，防止 starvation 的出现，还使用了 rr timer 机制，通过 rr_start_kick_timer 创建出来一个定时器，将会周期性的让 `rr_current_cpu` `cpu_exit` 出来。
- rr_kick_vcpu_thread : 因为不知道具体是哪一个 vCPU 在执行，需要向所有的 vCPU 退出，才可以达到 vCPU thread 从执行状态退出的目的
- rr_wait_io_event : 需要等待所有的 all_cpu_threads_idle 才会进入 idle 状态，否则会去执行下一个 vCPU 的
- rr_current_cpu : rr 需要记录当前真正使用的 vCPU，然后在 rr_kick_thread => rr_kick_next_cpu 中就可以调用 kick 来进行操作了

#### lifecycle of vCPU thread
- thread_kicked : 在 qemu_cpu_kick 中，因为 AccelOpsClass::kick_vcpu_thread 注册过，不会调用 cpus_kick_thread 上
- stop / stopped : 这是用于处理 vmstate 的之类的将 cpu 停下来的操作。实际上，runstate 之类的也是处理这个事情。

#### exit_request
- exit_request : cpu_exit 让 vCPU 从执行流逐步退出
  - 在 cpu_exit 中做两个事情，设置 icount_decr_ptr 让 vCPU 在 tb 结束的位置退出，其次是设置 exit_request = 1
  - cpu_handle_interrupt : 检测到 exit_request 之后会设置 `cpu->exception_index = EXCP_INTERRUPT`，这导致执行流进入到 `cpu_handle_exception` 中
  - cpu_handle_exception 中因为检测到 `cpu->exception_index >= EXCP_INTERRUPT`，将会重置 `cpu->exception_index = -1` 并且进一步导致退出 `cpu_exec`
```c
void cpu_exit(CPUState *cpu)
{
    qatomic_set(&cpu->exit_request, 1);
    /* Ensure cpu_exec will see the exit request after TCG has exited.  */
    smp_wmb();
    qatomic_set(&cpu->icount_decr_ptr->u16.high, -1);
}
```

#### halt
x86 halt 指令会让 CPU 进入低功耗的状态，当外界有中断到来的时候，CPU 才会继续运行。
显然，当 guest 执行 halt 指令之后，host 对应的 vCPU thread 也是需要进入到 idle 的状态。

- cpu_x86_load_seg_cache_sipi : 机器启动的位置，将 CPUState::halted 从 1 设置为 0
- 当遇到 halt 指令，会调用 helper do_hlt
- do_hlt 设置 CPUState::halted 并且通过 siglongjmp 跳转到 cpu_exec 中
- cpu_handle_interrupt : 将 CPUState::interrupt_request 中插入的 CPU_INTERRUPT_HALT 装换为在 CPUState::exception_index 上插入 EXCP_HLT 从而进一步退出到 qemu_tcg_rr_cpu_thread_fn
- qemu_tcg_rr_cpu_thread_fn 会调用到 qemu_tcg_rr_wait_io_event
- 在 qemu_tcg_rr_wait_io_event 中调用 all_cpu_threads_idle 来分析 CPU 是否进入到 idle 中
- 如果是，vCPU 将会等待到 `qemu_cond_wait(first_cpu->halt_cond, &qemu_global_mutex)`
- 如果有中断到来了，例如在 tcg_handle_interrupt 中，调用 cpu_exit 可以会 broadcast halt_cond 从而让 vCPU 继续执行

## locks between vCPU
因为 kvm vCPU thread 的比较简单，就不分析了。下面只是关注 tcg 的 vCPU thread，在没有 explicit 的指出的情况下，vCPU thread 指的是 tcg vCPU thread。


```c
static QemuMutex qemu_cpu_list_lock;   // 这个就是 cpu 的 lock，一旦持有，其他的 cpu 都不可以动弹的，也是用于实现下面的各种 cond
static QemuCond exclusive_cond;        // 用于实现 start_exclusive 中 wait，在 cpu_exec_end 中 notify 的。
static QemuCond exclusive_resume;      // 在 inclusive_idle 的调用
static QemuCond qemu_work_cond;        // 用于实现 do_run_on_cpu 的，在 process_queued_cpu_work 中 qemu_cond_broadcast(&qemu_work_cond);

static QemuMutex qemu_global_mutex;    // 这个居然就是 bql 啊
struct QemuCond * CPUState::halt_cond; // 当整个 cpu 处于 stop 的状态，那么会卡到这里去

static QemuCond qemu_pause_cond;       // pause_all_vcpus 中用于等待所有的 vCPU 进入 stop 的状态
```

##### sync between main loop and vCPU
在 x86_cpu_realizefn => qemu_init_vcpu 中，会创建并且执行 vCPU thread
如果是 -thread=single 的 tcg 的 vCPU 执行的位置从 qemu_tcg_rr_cpu_thread_fn 开始

* **main loop** 等待

- main loop 会 wait 在  qemu_init_vcpu 中 qemu_cpu_cond 上，同时释放 BQL
- 因为 BQL 释放，所以 vCPU 从 qemu_mutex_lock_iothread() 上继续运行，进行简单的初始化之后，使用 cpu_thread_signal_created 来告诉 main loop 可以继续运行
- 最后 vCPU 在 qemu_wait_io_event 中因为等待到 CPUState::halt_cond 上才真正的释放 BQL，main loop 才可以继续的。

* **child 的等待**


```c
  /* wait for initial kick-off after machine start */
  while (first_cpu->stopped) {
    qemu_cond_wait(first_cpu->halt_cond, &qemu_global_mutex);

    /* process any pending work */
    CPU_FOREACH(cpu) {
      current_cpu = cpu;
      qemu_wait_io_event_common(cpu);
    }
  }
```

- 因为 main loop 几乎总是持有 BQL，这个导致 vCPU 始终从  qemu_cond_wait 无法离开
- 在 do_run_on_cpu 中，因为 main loop 需要等待 qemu_work_cond，所以会暂时释放 BQL
  - 让 vCPU 现在可以执行到 qemu_wait_io_event 并且处理掉一些任务
- 当 main loop 调用 resume_all_vcpus => `qemu_cond_broadcast(cpu->halt_cond)` 之后，并且在 main_loop 在 os_host_main_loop_wait 中释放 lock 之后，那个时候 vCPU 才可以真正的离开


通过上面的流程，终于可以理解为什么 tlb_flush_by_mmuidx 需要增加一个对于 CPUState::created 的判断了。
```c
void tlb_flush_by_mmuidx(CPUState *cpu, uint16_t idxmap)
{
    tlb_debug("mmu_idx: 0x%" PRIx16 "\n", idxmap);

    if (cpu->created && !qemu_cpu_is_self(cpu)) {
        async_run_on_cpu(cpu, tlb_flush_by_mmuidx_async_work,
                         RUN_ON_CPU_HOST_INT(idxmap));
    } else {
        tlb_flush_by_mmuidx_async_work(cpu, RUN_ON_CPU_HOST_INT(idxmap));
    }
}
```

因为 tlb_flush_by_mmuidx 的调用比 qemu_init_vcpu 早，此时 vCPU 还被挡在 BQL 上了，是不可能有去执行 qemu_wait_io_event 来执行
async_run_on_cpu 挂载上的任务的。

```c
/*
#0  tlb_flush_by_mmuidx (cpu=0x555556b09970, idxmap=7) at ../accel/tcg/cputlb.c:384
#1  0x0000555555c5e3e8 in listener_add_address_space (as=<optimized out>, listener=0x555556a08508) at ../softmmu/memory.c:2839
#2  memory_listener_register (listener=0x555556a08508, as=<optimized out>) at ../softmmu/memory.c:2902
#3  0x0000555555c9362a in cpu_address_space_init (cpu=cpu@entry=0x555556b09970, asidx=asidx@entry=0, prefix=prefix@entry=0x555555f871a7 "cpu-memory", mr=<optimized out>) at ../softmmu/physmem.c:759
#4  0x0000555555b772bd in tcg_cpu_realizefn (cs=0x555556b09970, errp=<optimized out>) at ../target/i386/tcg/sysemu/tcg-cpu.c:76
#5  0x0000555555cf1e6b in cpu_exec_realizefn (cpu=cpu@entry=0x555556b09970, errp=errp@entry=0x7fffffffcd70) at ../cpu.c:137
#6  0x0000555555be220e in x86_cpu_realizefn (dev=0x555556b09970, errp=0x7fffffffcdd0) at ../target/i386/cpu.c:6156
```


#### current_cpu
- 赋值位置: cpu_exec / rr_cpu_thread_fn / mttcg_cpu_thread_fn
  - :duck: 实际上 cpu_exec 中间并没有必要进行对于 cpu_exec 的赋值操作，和两个 thread_fn 重叠了
```c
  /* replay_interrupt may need current_cpu */
  current_cpu = cpu;
```
- 使用位置
  - tb_invalidate_phys_page_range__locked : 为了实现 precise SMC，如果当前 cpu 正在 invalidate 自己运行的 tb 需要做一些特殊操作
  - 其他的位置暂时不讨论了

#### setlongjmp
保存上下文: sigsetjmp
- cpu_exec_step_atomic
- cpu_exec

跳转回去的位置: siglongjmp
- cpu_loop_exit

采用 sigsetjmp 可以快速上下文(regs and stack)，通过 siglongjmp 可以快速回到 sigsetjmp 的位置，如果函数调用层次很深，可以避免逐个返回。

### queue_work_on_cpu
queue_work_on_cpu 存在三个调用者:
- run_on_cpu : 需要等待该 cpu 完成任务之后才可以继续。
- async_run_on_cpu : 提交任务给 vCPU 然后就可以离开了。
- async_safe_run_on_cpu : 要求任务在 [exclusive context](#exclusive-context) 下执行的。

分析一下 async_run_on_cpu 的执行流程:

- vCPU thread A:
  - async_run_on_cpu
    - 初始化 qemu_work_item
    - queue_work_on_cpu : 将 qemu_work_item 挂到队列上
      - qemu_cpu_kick
        - `qemu_cond_broadcast(cpu->halt_cond)` : 如果 vCPU B 处于 idle 的状态，那么将其醒过来
        - cpu_exit : 将正在执行的 vCPU 线程停止执行。

- vCPU thread B:
  - rr_wait_io_event : 因为 cpu_exit 退出到此处 (rr 是这个，mttcg 是 qemu_wait_io_event)
    - 如果 all_cpu_threads_idle 那么将会等待在 CPUState::halt_cond，通过 qemu_cpu_kick 可以让其继续运行
    - qemu_wait_io_event_common
      - process_queued_cpu_work
        - 将挂载上去的任务逐个执行，
        - qemu_cond_broadcast(&qemu_work_cond) : 用于通知 run_on_cpu 任务已经结束了

### exclusive context
使用 rr 作为例子，mttcg 差不多类似:

- rr_cpu_thread_fn
  - tcg_cpus_exec
    - cpu_exec_start :star:
    - cpu_exec
    - cpu_exec_end :star:
  - cpu_exec_step_atomic
    - start_exclusive :star:
    - end_exclusive :star:
  - rr_wait_io_event
    - qemu_wait_io_event_common
      - start_exclusive() :star:
      - `wi->func(cpu, wi->data)`
      - end_exclusive() :star:

需要 exclusive context 下执行主要是两个点:
- tb_flush : 因为释放所有的 tb，防止一个 vCPU 正在释放，而其他的还在使用 tb 的
- cpu_exec_step_atomic : 模拟 atomic 访存，比如 cmpxch 之类的

考虑一下这个上锁的需求，多个 cpu_exec 可以同时进行，但是一旦一个进入 exclusive 的状态，其他的都不可以使用:
- cpu_exec_start 中间上 reader lock
- start_exclusive 中间上 writer lock

qemu 通过下面的变量组合起来实现的:
- CPUState::running CPUState::has_waiter
- pending_cpus : 用于统计当前还有多少个 reader 没有离开。
- exclusive_resume : 用于阻止新的 reader 进入。
- qemu_cpu_list_lock : 用于阻止新的 writer 进入。
- exclusive_cond : 当 pending_cpus == 0 的时候，开始等待最后的一个 reader 离开

其好处在于:
- 使用 naive 的 reader-writer lock 会 starve writer 的，这显然不合理。
- reader 默认需要上锁的，在快路径上，cpu_exec_start 完全不需要上锁。

## misc

### mmap_lock
```c
static pthread_mutex_t mmap_mutex = PTHREAD_MUTEX_INITIALIZER;
static __thread int mmap_lock_count;

void mmap_lock(void)
{
    if (mmap_lock_count++ == 0) {
        pthread_mutex_lock(&mmap_mutex);
    }
}
```
利用 mmap_lock_count 一个 thread 可以反复上锁，但是可以防止其他 thread 并发访问。

参考两个资料，可以知道 mmap_lock 是只有用户态翻译才需要的:
1. https://qemu.readthedocs.io/en/latest/devel/multi-thread-tcg.html
2. tcg_region_init 上面的注释

用户态的线程数量可能很大，所以创建多个 region 是不合适的，所以只创建一个，
而且用户进程的代码大多数都是相同，所以 tb 相关串行也问题不大。

[^1]: [Live Migrating QEMU-KVM Virtual Machines](https://developers.redhat.com/blog/2015/03/24/live-migrating-qemu-kvm-virtual-machines#)
[^2]: [为什么 conditional variable 需要一个 mutex](https://stackoverflow.com/questions/14924469/does-pthread-cond-waitcond-t-mutex-unlock-and-then-lock-the-mutex)
[^3]: https://martins3.github.io/qemu/map.html

<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
