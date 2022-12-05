# 14

## IX: A Protected Dataplane Operating System for High Throughput and Low Latency
- [pdf](http://www.abelay.me/data/ix_osdi14.pdf)
- 项目地址: https://github.com/ix-project/ix

While kernel bypass eliminates context switch overheads,
on its own it does not eliminate the difficult tradeoffs between high packet rates and low latency (see §5.2)
- [ ] 为什么，我感觉只是 CPU 消耗过高的问题?

IX separates the control plane, which is responsible for system configuration and coarse-grain
resource provisioning between applications, from the dataplanes, which run the networking stack and application
logic.
- [ ] 提出 data plane 和 control plane 是从这里开始的吗?

- 两端优雅对于网络栈的总结

Unfortunately, commodity operating systems have been designed under very different hardware assumptions.
Kernel schedulers, networking APIs, and network stacks have been designed under the assumptions of multiple
applications sharing a single processing core and packet inter-arrival times being many times higher than the latency of interrupts and system calls. As a result, such
operating systems trade off both latency and throughput in favor of fine-grain resource scheduling.

Interrupt coalescing (used to reduce processing overheads), queuing latency due to device driver processing intervals, the use
of intermediate buffering, and CPU scheduling delays frequently add up to several hundred µs of latency to remote
requests.
The overheads of buffering and synchronization needed to support flexible, fine-grain scheduling of applications to cores increases CPU and memory system overheads, which limits throughput.
As requests between service tiers of datacenter applications often consist of small packets, common NIC hardware optimizations, such as TCP segmentation and receive side coalescing, have a
marginal impact on packet rate

:( 看不下去了。

# 18
- [ ]  splitkernel


# 22

## XRP: In-Kernel Storage Functions with eBPF
- 使用 ebpf 来在内核中 by-pass 存储栈。
- https://www.usenix.org/conference/osdi22/presentation/zhong
