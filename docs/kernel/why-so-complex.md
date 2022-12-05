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
