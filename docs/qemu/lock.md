# QEMU 中的锁

<!-- vim-markdown-toc GitLab -->

- [Big QEMU Lock](#big-qemu-lock)
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
  - store_helper / load_helper 会做出判断
  - prepare_mmio_access 中

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
