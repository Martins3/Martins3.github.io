## mmap 的观测

```txt
@[
        rdma_user_mmap_io+0
        ib_uverbs_mmap+140
        __mmap_new_vma+212
        __mmap_region+996
        mmap_region+204
        do_mmap+984
        vm_mmap_pgoff+324
        ksys_mmap_pgoff+356
        __arm64_sys_mmap+52
        invoke_syscall.constprop.0+100
        el0_svc_common.constprop.0+68
        do_el0_svc+36
        el0_svc+60
        el0t_64_sync_handler+268
        el0t_64_sync+432
]: 4
```

```txt
@[
        mlx5_ib_mmap+0
        __mmap_new_vma+212
        __mmap_region+996
        mmap_region+204
        do_mmap+984
        vm_mmap_pgoff+324
        ksys_mmap_pgoff+356
        __arm64_sys_mmap+52
        invoke_syscall.constprop.0+100
        el0_svc_common.constprop.0+68
        do_el0_svc+36
        el0_svc+60
        el0t_64_sync_handler+268
        el0t_64_sync+432
]: 6
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

“是如何 mmap 的”可以分成两步理解：

1. 驱动决定“映射什么”

- mlx5_ib_mmap() 解析 vma->vm_pgoff 编码出来的 command/index。
- 对 UAR、clock page、VAR、MEMIC 等不同对象，算出对应物理页 PFN 和缓存属性。
- 例如 UAR 场景会走 uar_mmap()，用 uar_index2pfn() 算 PFN，并选择 pgprot_writecombine() 或 pgprot_noncached()。

2. RDMA core 负责“怎么映射”

- rdma_user_mmap_io() 校验 VM_SHARED 和长度。
- 设置 vma->vm_page_prot = prot。
- 调 io_remap_pfn_range()，把 PFN 映射进用户 VMA。
- 然后挂上 rdma_umap_priv，便于后续 hot-unplug/disassociate 时 zap 这些用户映射。

也就是说：

- mlx5_ib_mmap 决策对象和属性
- rdma_user_mmap_io 执行实际 remap
- ib_uverbs_mmap 只是上层分发口

## 观察到 zero copy 才可以的
可以观察到没有系统调用就可以了吧


## 中断到 event 的通知过程

总体来说，就是这个机制了，ib_uverbs_comp_handler 中可以找到的
```txt
[
        ib_uverbs_comp_handler+0
        mlx5_cq_tasklet_cb+272
        tasklet_action_common+340
        tasklet_action+56
        handle_softirqs+300
        __do_softirq+28
        ____do_softirq+24
        call_on_irq_stack+36
        do_softirq_own_stack+36
        __irq_exit_rcu+316
        irq_exit_rcu+24
        el1_interrupt+72
        el1h_64_irq_handler+24
        el1h_64_irq+132
        default_idle_call+56
        cpuidle_idle_call+376
        do_idle+156
        cpu_startup_entry+56
        secondary_start_kernel+224
        __secondary_switched+192
]: 2
```

## 为什么 demo 中需要使用 tcp ?

## 为什么 ping 可以直接用?

rdma-core 提供了 rping ，基本的使用方法是:

server 运行 : rping -s

client 运行 : rping -c -a 10.0.3.2 -v

想想也的确是这样的，就是通过 rdma 来链接的

## 最多支持多个用户态程序使用 poll 模式?

估计和 QP 和 CQ 有关的

• QP 是 Queue Pair。

  它是 RDMA 最核心的通信对象，由一对队列组成：

  - SQ：Send Queue
  - RQ：Receive Queue

  应用往 QP 上发 WR，网卡从 QP 取请求执行，完成后把结果写到 CQ。

  可以粗略理解为：

  - QP 决定“和谁通信、用什么语义通信”
  - CQ 决定“完成结果到哪里收”
  - MR 决定“哪块内存允许 RDMA 访问”

  常见 QP 类型有：

  - RC：Reliable Connected
  - UC：Unreliable Connected
  - UD：Unreliable Datagram
  - 以及一些扩展类型如 Raw Packet

  QP 还会经历状态机：

  - RESET -> INIT -> RTR -> RTS
  - 出错后可能进 ERR

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
