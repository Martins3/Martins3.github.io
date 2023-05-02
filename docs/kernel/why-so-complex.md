# 为什么 Linux 6.0 相比于 Linux 0.1 复杂那么多

> There are so many features in the Linux kernel it sometimes blows my mind. eventfd, signalfd, timerfd, memfd, pidfd. The whole fricking tc/qdisc featureset (OMG). netlink. io_uring. criu. SO_REUSEPORT. Teaming. Namespaces. veths. vsocks. Dpdk/netmap/af_packet. XDP ! Seccomp.
>
> [Hacker News Reader](https://news.ycombinator.com/item?id=27328285)

> 简述一下 https://lwn.net/Articles/872321/

复杂性可以划分为:
- essential complexity
- accidental complexity


## 横向扩展

### 更多的架构支持

### 更多的驱动支持
- PCIe
- scsi
- ata
- HBA (Host Bus Adapter)
- virtio

## 纵向扩展
1. 内存管理
1. io_uring
2. iommu
3. kvm
4. cgroup
5. 虚拟化
6. kvm
7. kdump / kexec
8. perf ftrace
9. vfio
10. RDMA

很长一段时间，Linux 内核只能使用 gcc 编译。

memory 中 Transparent Huge Page 和
## accidental complexity

- incomplete transitions
  - cgroup 中同时存在 v1 和 v2 机制
  - x86 kvm 中同时存在
  - x86 各种模式的兼容
- 重复逻辑以及缺乏抽象
  - 暂时没有发现

## 参考
- https://www.zhihu.com/question/35484429/answer/62964898

## 和其他各种建议操作系统的对比
- https://github.com/vvaltchev/tilck

## 内核学习三个阶段
1. 地址空间: 用户态为什么不能访问内核的地址空间，为什么内核可以通过
2. 如何访问设备，如 PCIe 的工作方式
3. 中断和抢占
4. 锁机制

> 代码流程分析的头头是道，但是如果我问题，现在虚拟地址是什么，物理地址是什么，stack 在哪里，
当前代码的位置能够发生中断，当前的位置能够发生抢占，当前位置持有了什么锁，如果发生了中断或者抢占，
接下来的执行流程是什么，如果答不出来，我只能给你零分。

反而没有想象的那么重要的部分：
- 内存页面分配：
- 调度算法：

一些，看似没关系，实际上，无处不在：
- cgroup
