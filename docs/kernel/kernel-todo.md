- 内核模块的冲突规则是什么?
- 什么是 autofs 的哇:
  - https://web.mit.edu/rhel-doc/5/RHEL-5-manual/Deployment_Guide-en-US/s1-nfs-client-config-autofs.html
  - https://www.kernel.org/doc/html/latest/filesystems/autofs.html
- echo b > /sys/sys-trigger 来重启的原理是什么？
- 调查一下 fs/iomap
  - https://patchwork.kernel.org/project/linux-fsdevel/patch/1464792297-13185-3-git-send-email-hch@lst.de/
- mark_oom_victim -> `__thaw_task`
  - 什么 uninterruptable sleep 之类的哇
- memory policy, cgroup cpuset, cgroup memory 三个位置同时可以限制内存使用

- vmstat.c 是做什么的? vmstat_shepherd
- 为什么切换一下内核，disk 的名称就会发生变化，这个问题让 alpine.sh 的 root= 很麻烦

- [ ] ksm
- [ ] shmem
- [ ] /home/maritns3/core/vn/kernel/plka/syn/mm/memory.md 整理一下 pgfault 的过程
- [ ] gup 机制 : FOLL_GET 之类的 flags 烦死人了

- 问一个问题: pg_data_t 中间 pg 是什么？ ask the stackoverflow
- 为什么将 ext4 从模块修改 ext4，然后整个项目几乎重新编译

- kobj_to_hstate
  - 当内核中写 /sys/devices/system/node/node0/hugepages/hugepages-1048576kB/nr_hugepages 的时候，通过这个可以知道当前的 node 是什么

- hugetlbfs_fallocate : 根本看不懂这个哇
```c
        cond_resched();

        /*
         * fallocate(2) manpage permits EINTR; we may have been
         * interrupted because we are using up too much memory.
         */
        if (signal_pending(current)) {
            error = -EINTR;
            break;
        }
```
- 热升级如何实现?

- 我发现了一个这个问题 backtrace, 那么有什么办法通过 bpftrace 知道谁在使用 io_uring 吗?
```txt
io_write+280
io_issue_sqe+1182
io_wq_submit_work+129
io_worker_handle_work+615
io_wqe_worker+766
ret_from_fork+34
```

- 折磨
```c
/*
 * On an anonymous page mapped into a user virtual memory area,
 * page->mapping points to its anon_vma, not to a struct address_space;
 * with the PAGE_MAPPING_ANON bit set to distinguish it.  See rmap.h.
 *
 * On an anonymous page in a VM_MERGEABLE area, if CONFIG_KSM is enabled,
 * the PAGE_MAPPING_MOVABLE bit may be set along with the PAGE_MAPPING_ANON
 * bit; and then page->mapping points, not to an anon_vma, but to a private
 * structure which KSM associates with that merged page.  See ksm.h.
 *
 * PAGE_MAPPING_KSM without PAGE_MAPPING_ANON is used for non-lru movable
 * page and then page->mapping points a struct address_space.
 *
 * Please note that, confusingly, "page_mapping" refers to the inode
 * address_space which maps the page from disk; whereas "page_mapped"
 * refers to user virtual address space into which the page is mapped.
 */
#define PAGE_MAPPING_ANON   0x1
#define PAGE_MAPPING_MOVABLE    0x2
#define PAGE_MAPPING_KSM    (PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
#define PAGE_MAPPING_FLAGS  (PAGE_MAPPING_ANON | PAGE_MAPPING_MOVABLE)
```

- make menuconfig 下的所有 memory 选项都是应该分析一下的。
- zap_page_range 为什么会去调用 lru_add_drain，我的一生之敌啊

- fs/cachefiles : 如果使用了 disk 文件系统来缓存网络，那么 sshfs 为什么性能还是这么差?
  - 还是说，其实是使用上了的
  - [ ] 其实利用 nfs 来理解一下分布式
- online_pages 和内核启动过程中，应该是存在非常多相似之处

- 调度系统处理大核和小核

## TODO
- driver/base 下的代码需要分析一下

- mkfifo 和 mknod 's relation ?

- meminfo 是如何实现的？


openeuler 总结的关于 5.10 内核的提升:
1. 支持调度器优化：优化 CFS Task 的公平性，新增
NUMA-Aware 异步调用机制，在 NVDIMM 初始
化方面有明显的提升；优化 SCHED_IDLE 的调度
策略，可以显著改善高优先级任务的调度延迟，
降低对其他任务的干扰。优化 NUMA balancing
机制，带来更好的亲和性、更高的使用率和更少
的无效迁移。
2. CPU 隔离机制增强：支持中断隔离，支持
unbound kthreads 隔离，增强 CPU 核的隔离性，
可以更好的避免业务间的相互干扰。
3. 进程间通信优化：pipe_wait、epoll_wait 唤醒机
制优化，解决唤醒多个等待线程的性能问题。
4. 内存管理增强：优化内存初始化、内存控制、统
计、异构内存、热插拔等功能，并提供更有效的
用户控制接口。热点锁及信号量优化，激进内存
和碎片整理，优化 VMAP、vmalloc 机制，显著
提升内存申请效率。KASAN、kmemleak、slub_
debug、OOM 等内存维测特性增强，提升定位和
解决内存问题的效率。
5. cgroup 优化单线程迁移性能：消除对 Thread
Group 读写信号量的依赖；引入 Time
Namespace 方便容器迁移。
6. 系统容器支持对容器内使用文件句柄数进行限制：
文件句柄包括普通文件句柄和网络套接字。启动
容器时，可以通过指定 --files-limit 参数限制容器
内打开的最大句柄数。
7. 支持 PSI ：提供了一种评估系统资源 CPU、内存、
数据读写压力的方法。准确的检测方法可以帮资
源使用者确定合适的工作量，帮助系统制定高效
的资源调度策略，最大化利用系统资源，改善用
户体验。
8. TCP 发包切换到了 Early Departure Time 模型：
解决原来 TCP 框架的限制，根据调度策略给数据
包设置 Early Departure Time 时间戳，避免大的
队列缓存带来的时延，同时大幅提升 TCP 性能。
9. 支持 MultiPath TCP 可在移动与数据场景提升性
能和可靠性：支持在负载均衡场景多条子流并行
传输。
10. Ext4 引入一种新的、更轻量级的日志方法：- fast
commit，可以大大减少 fsync 等耗时操作，带来
更好的性能。
11. dm-writecache 特性：提升 SSD 大块顺序写性能，
提高 DDR 持久性内存的性能。
13. IMA 商用增强：在开源 IMA 方案基础上，增强安
全性、提升性能、提高易用性，助力商用落地。
14. 支持 per-task 栈检查：增强对 ROP 攻击的防护
能力。
16. MPAM 资源管控：支持 ARM64 架构 Cache QoS
以及内存带宽控制技术。
17. 支持基于 SEDI 和 PMU 的 NMI 机制：使能 hard
lockup 检测。使能 perf nmi，能更精确的进行性
能分析。
18. 支持虚拟机热插拔：ARM64 支持虚拟机 CPU 热
插拔，提高资源配置的灵活性。

## 写一个内核依赖图
> 先收集起来

提前准备: C，深入理解计算机系统，组成原理

- memory
  - folio
- 虚拟化


## virtiofs
- [ ] https://libvirt.org/kbase/virtiofs.html
  - [ ] ali 的人建议 : https://virtio-fs.gitlab.io/ : /home/maritns3/core/54-linux/fs/fuse/virtio_fs.c : 只有 1000 多行，9pfs 也很小，这些东西有什么特殊的地方吗 ?

首先将 virtiofs 用起来: https://www.tauceti.blog/post/qemu-kvm-share-host-directory-with-vm-with-virtio/
> virtio-fs on the other side is designed to offer local file system semantics and performance. virtio-fs takes advantage of the virtual machine’s co-location with the hypervisor to avoid overheads associated with network file systems. virtio-fs uses FUSE as the foundation. Unlike traditional FUSE where the file system daemon runs in userspace, the virtio-fs daemon runs on the host. A VIRTIO device carries FUSE messages and provides extensions for advanced features not available in traditional FUSE.

似乎需要 Qemu 5.0 才可以。

## 整理一个 blog ，作为一个导航的存在

1. percpu 如何实现，amd64 中间使用 为什么使用 fs(也许是 gs 寄存器实现) 来支持 percpu
3. 多核为锁, 调度器带来何种挑战
4. 为什么会出现从一个 CPU 中被调度出去，从另一个恢复，会出现什么特殊的情况。
5. 多核让 PIC 升级成为了 APIC，我们开始需要分析如何正确负载
6. 多核出现形成了一个新的学科，memory consistency and cache coherency


## copy_to_user 实现机制

# 操作性试验
一个有意思的实践
https://stackoverflow.com/questions/36346835/active-inactive-list-in-linux-kernel?rq=1

重写类似的模块玩一下:
https://stackoverflow.com/questions/56097946/get-cpu-var-put-cpu-var-not-updating-the-per-cpu-variable

## vfs 的上锁规则是什么

```txt
#0  instance_mkdir (name=0xffff8881613b8520 "g") at kernel/trace/trace.c:9364
#1  0xffffffff815bc0da in tracefs_syscall_mkdir (mnt_userns=<optimized out>, inode=0xffff888161aec750, dentry=<optimized out>, mode=<optimized out>) at fs/tracefs/inode.c:87
#2  0xffffffff81374a0f in vfs_mkdir (mnt_userns=0xffffffff82a627c0 <init_user_ns>, dir=0xffff888161aec750, dentry=dentry@entry=0xffff888127f4f240, mode=<optimized out>, mode@entry=511) at fs/namei.c:4035
#3  0xffffffff813797e1 in do_mkdirat (dfd=dfd@entry=-100, name=0xffff88812165c000, mode=mode@entry=511) at fs/namei.c:4060
#4  0xffffffff813799d3 in __do_sys_mkdir (mode=<optimized out>, pathname=<optimized out>) at fs/namei.c:4080
#5  __se_sys_mkdir (mode=<optimized out>, pathname=<optimized out>) at fs/namei.c:4078
#6  __x64_sys_mkdir (regs=<optimized out>) at fs/namei.c:4078
#7  0xffffffff81fa4bcb in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001cc3f58) at arch/x86/entry/common.c:50
#8  do_syscall_64 (regs=0xffffc90001cc3f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#9  0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#10 0x0000000000000000 in ?? ()
```

为什么最后上锁是在这里的，如果修改目录的时候，上锁规则是什么?
```c
static int instance_mkdir(const char *name)
{
	struct trace_array *tr;
	int ret;

	mutex_lock(&event_mutex);
	mutex_lock(&trace_types_lock);

	ret = -EEXIST;
	if (trace_array_find(name))
		goto out_unlock;

	tr = trace_array_create(name);

	ret = PTR_ERR_OR_ZERO(tr);

out_unlock:
	mutex_unlock(&trace_types_lock);
	mutex_unlock(&event_mutex);
	return ret;
}
```

- [ ] /sys/devices/system/node/node0/ 是如何创建出来的?


## 这几个选项都是做什么的
```plain
CONFIG_GENERIC_IRQ_INJECTION=y
CONFIG_HAVE_FUNCTION_ERROR_INJECTION=y
# CONFIG_BLK_DEV_NULL_BLK_FAULT_INJECTION is not set
# CONFIG_EDAC_AMD64_ERROR_INJECTION is not set
# CONFIG_NFSD_FAULT_INJECTION is not set
# CONFIG_NOTIFIER_ERROR_INJECTION is not set
CONFIG_FUNCTION_ERROR_INJECTION=y
CONFIG_FAULT_INJECTION=y
CONFIG_FAULT_INJECTION_DEBUG_FS=y
CONFIG_FAIL_MAKE_REQUEST=y
```

## x86-64 的环境中，可以使用 kvm 运行 32 位内核吗

## 如果就是在 interrupt thread 中睡眠，会如何

## bus 的 match

- 现在才知道 virtio 设备同时挂到 PCI 和 virtio bus 上的


- [ ] struct bus_type 的 match 和 probe 是什么关系?
```c
static inline int driver_match_device(struct device_driver *drv,
              struct device *dev)
{
  return drv->bus->match ? drv->bus->match(dev, drv) : 1;
}
```

`device_attach` 是提供给外部的一个常用的函数，会调用 `bus->probe`，在当前的上下文就是 pci_device_probe 了。

- [ ] `pci_device_probe` 的内容是很简单, 根据设备找驱动的地方在哪里？
  - 根据参数 struct device 获取 pc_driver 和 pci_device
  - 分配 irq number
  - 回调 `pci_driver->probe`, 使用 virtio_pci_probe 为例子
      - 初始化 pci 设备
      - 回调 `virtio_driver->probe`, 使用 virtnet_probe 作为例子

- [ ] 什么 file operation 的 direct IO 是什么情况 ?
