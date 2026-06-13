# NUMA
1. 创建的内存最好是就是在附近 ：buddy 和 slub 分配器的策略，这些策略被整理成为 mempolicy.c
2. 运行过程中间发生变化 : migrate.c

首先分析一波 numa 的基础知识 [^6]

用户层次:
Available policies are
1. **page interleaving** (i.e., allocate in a round-robin fashion from all, or a subset, of the nodes on the system), inorder to overload the initial boot node with boot-time allocations.
2. **preferred node allocation** (i.e., preferably allocate on a particular node),
3. **local allocation** (i.e., allocate on the node on which the task is currently executing), or
4. **allocation only on specific nodes** (i.e., allocate on some subset of the available nodes).  It is also possible to bind tasks to specific nodes.

分析 syscall :
1. get_mempolicy
2. mbind
3. migrate_page



## 问题
3. 每一个 node 主要管理什么信息，每一个 zone 中间放置什么内容?
4. 哪里涉及到了 nodemask 的，主要作用是什么 ?

- [ ] 如何理解这个？
```c
/*
 * Array of node states.
 */
nodemask_t node_states[NR_NODE_STATES] __read_mostly = {
	[N_POSSIBLE] = NODE_MASK_ALL,
	[N_ONLINE] = { { [0] = 1UL } },
#ifndef CONFIG_NUMA
	[N_NORMAL_MEMORY] = { { [0] = 1UL } },
#ifdef CONFIG_HIGHMEM
	[N_HIGH_MEMORY] = { { [0] = 1UL } },
#endif
	[N_MEMORY] = { { [0] = 1UL } },
	[N_CPU] = { { [0] = 1UL } },
#endif	/* NUMA */
};
EXPORT_SYMBOL(node_states);
```
- [ ] 到底 memory policy 是一个进程的行为还是直接影响所有的程序的

## numad
- https://pagure.io/numad/tree/master


## 这个下面的接口都可以看看
- /sys/devices/system/node/node0

## 看看这个工具
https://github.com/intel/numatop

https://frankdenneman.nl/2016/07/06/introduction-2016-numa-deep-dive-series/

## 阅读一下
- https://frankdenneman.nl/2016/07/06/introduction-2016-numa-deep-dive-series/

[^6]: [NUMA (Non-Uniform Memory Access): An Overview](https://queue.acm.org/detail.cfm?id=2513149)
[^7]: [kernel doc : numa memory policy](https://www.kernel.org/doc/html/latest/admin-guide/mm/numa_memory_policy.html)

## 对比一下这两个
```c
#define first_online_node	first_node(node_states[N_ONLINE])
#define first_memory_node	first_node(node_states[N_MEMORY])
static __always_inline unsigned int next_online_node(int nid)
{
	return next_node(nid, node_states[N_ONLINE]);
}
static __always_inline unsigned int next_memory_node(int nid)
{
	return next_node(nid, node_states[N_MEMORY]);
}
```

## cluster 到底是什么?


cluster id 的意思是看

看这个: https://lists.gnu.org/archive/html/qemu-arm/2021-03/msg01031.html

> A cluster means a group of cores that share some resources (e.g. cache)
> among them under the LLC. For example, ARM64 server chip Kunpeng 920 has
> 6 or 8 clusters in each NUMA, and each cluster has 4 cores. All clusters
> share L3 cache data while cores within each cluster share the L2 cache.

检查一下 Yanan Wang ，对应的时间段 kernel 中也提交了很多相关的代码。

看来 cluster 是共享 L2 的含义

https://www.hikunpeng.com/doc_center/source/zh/kunpengcpfs/systuningguide/systemtg/kunpengcluster_05_0007.html

这两个应该关注下:
```txt
cat /sys/devices/system/cpu/cpu0/topology/cluster_cpus_list
echo 1 > /proc/sys/kernel/sched_cluster
```


### 一个具体的问题
```txt
qemu-system-aarch64: warning: CPU-9 and CPU-10 in socket-0-cluster-0 have been associated with node-0 and node-1 respectively. It can cause OSes like Linux to misbehave
```

配置在 qemu 中看到: validate_cpu_cluster_to_numa_boundary

```diff
History:        #0
Commit:         a494fdb715832000ee9047a549a35aacfea8175e
Author:         Gavin Shan <gshan@redhat.com>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Tue 09 May 2023 08:27:37 AM CST
Committer Date: Mon 26 Jun 2023 04:23:01 PM CST

numa: Validate cluster and NUMA node boundary if required

For some architectures like ARM64, multiple CPUs in one cluster can be
associated with different NUMA nodes, which is irregular configuration
because we shouldn't have this in baremetal environment. The irregular
configuration causes Linux guest to misbehave, as the following warning
messages indicate.

  -smp 6,maxcpus=6,sockets=2,clusters=1,cores=3,threads=1 \
  -numa node,nodeid=0,cpus=0-1,memdev=ram0                \
  -numa node,nodeid=1,cpus=2-3,memdev=ram1                \
  -numa node,nodeid=2,cpus=4-5,memdev=ram2                \
```


## 看看这个图，太棒了
https://unix.stackexchange.com/questions/468766/understanding-output-of-lscpu


## 从其他的角度看看

- https://sqltouch.blogspot.com/2023/08/numa-and-soft-numa-in-sql-server-to-get.html
- https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2012-r2-and-2012/dn282282(v=ws.11)
- https://blogs.vmware.com/performance/2017/03/virtual-machine-vcpu-and-vnuma-rightsizing-rules-of-thumb.html


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
