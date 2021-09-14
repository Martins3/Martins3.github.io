# QEMU 中的锁

<!-- vim-markdown-toc GitLab -->

- [Big QEMU Lock](#big-qemu-lock)
- [BQL Advanced Topic](#bql-advanced-topic)
  - [migration](#migration)
  - [main loop](#main-loop)
  - [rcu](#rcu)
  - [interrupt_request](#interrupt_request)
- [vCPU thread 之间的交互](#vcpu-thread-之间的交互)
- [vCPU 和 io thread 的交互](#vcpu-和-io-thread-的交互)
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
但是实际上，BQL 的使用位置要上面多一点，这些是高级话题，可以暂时跳过：
- [ ] cpu_exec_step_atomic
- pause_all_vcpus

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

### rcu
在 call_rcu_thread 中，需要持有 lock 才可以释放资源，这很奇怪。既然都是可以开始来执行 hook 函数了，说明这些资源已经是没有人使用的，那么为什么还需要使用 BQL 保护。
其原因在: https://lists.gnu.org/archive/html/qemu-devel/2015-02/msg03170.html

### interrupt_request
interrupt_request 需要被 BQL 保护，其调用位置为:

- cpu_check_watchpoint => tcg_handle_interrupt
- cpu_handle_halt => apic_poll_irq / cpu_reset_interrupt
- cpu_handle_exception => ??
- edu_fact_thread => edu_raise_irq => msi_notify / pci_set_irq
- helper_write_crN => cpu_set_apic_tpr

- 注入位置
  - tcg_handle_interrupt :  将 mask 插入到 CPUState::interrupt_request
  - cpu_reset_interrupt : 将 mask 从 CPUState::interrupt_request 中清理
- 使用位置 : cpu_handle_interrupt => TCGCPUOps::cpu_exec_interrupt => x86_cpu_exec_interrupt 的

因为中断的注入可能来自于 main loop 或者是其他的 vCPU thread，所以同样这个需要 BQL 的保护

## vCPU thread 之间的交互
- 为什么 vCPU 需要交互?
  - 模拟 remote TLB flush, 一个 vCPU 的
  - ipi ?
  - [因为 tb buffer 是共享的](https://martins3.github.io/qemu/map.html#%E6%A0%B9%E6%8D%AE-guest-physical-address-%E6%89%BE%E5%88%B0-translation-block)
  - [page_lock](https://martins3.github.io/qemu/map.html#%E6%A0%B9%E6%8D%AE-ram-addr-%E6%89%BE%E8%AF%A5-guest-page-%E4%B8%8A%E5%85%B3%E8%81%94%E7%9A%84%E6%89%80%E6%9C%89%E7%9A%84-tb)
  - memory model : 不能出现一个 cpu 在修改，另一个 cpu 在使用的情况吧

## vCPU 和 io thread 的交互

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

那么只有用户态才需要啊 ?

参考两个资料:
1. https://qemu.readthedocs.io/en/latest/devel/multi-thread-tcg.html
2. tcg_region_init 上面的注释

用户态的线程数量可能很大，所以创建多个 region 是不合适的，所以只创建一个，
而且用户进程的代码大多数都是相同，所以 tb 相关串行也问题不大。

[^1]: [Live Migrating QEMU-KVM Virtual Machines](https://developers.redhat.com/blog/2015/03/24/live-migrating-qemu-kvm-virtual-machines#)

<script src="https://utteranc.es/client.js" repo="Martins3/Martins3.github.io" issue-term="url" theme="github-light" crossorigin="anonymous" async> </script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
