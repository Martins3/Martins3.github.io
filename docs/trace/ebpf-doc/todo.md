## 这里我不太懂的

似乎有印象，tcpdump 就是用的 ebpf 吧，为什么这个也是 https://github.com/mozillazg/ptcpdump

而且这里的: make build-bpf 是做啥的?

## 这个做什么的，为什么感觉和 bptool 的功能类似
https://ebpf-go.dev/guides/getting-started/#iteration-workflow

## 这个文件是做什么的?
net/ipv4/tcp_bpf.c

## 这个和 bpf nvme 是一个东西吗?
```txt
  │ CONFIG_NETKIT:                                                                                                                                                                                               │
  │                                                                                                                                                                                                              │
  │ The netkit device is a virtual networking device where BPF programs                                                                                                                                          │
  │ can be attached to the device(s) transmission routine in order to                                                                                                                                            │
  │ implement the driver's internal logic. The device can be configured                                                                                                                                          │
  │ to operate in L3 or L2 mode. If unsure, say N.                                                                                                                                                               │
  │                                                                                                                                                                                                              │
  │ Symbol: NETKIT [=n]                                                                                                                                                                                          │
  │ Type  : bool                                                                                                                                                                                                 │
  │ Defined at drivers/net/Kconfig:465                                                                                                                                                                           │
  │   Prompt: BPF-programmable network device                                                                                                                                                                    │
  │   Depends on: NETDEVICES [=y] && NET_CORE [=y] && BPF_SYSCALL [=y]                                                                                                                                           │
  │   Location:                                                                                                                                                                                                  │
  │     -> Device Drivers                                                                                                                                                                                        │
  │       -> Network device support (NETDEVICES [=y])                                                                                                                                                            │
  │         -> Network core driver support (NET_CORE [=y])                                                                                                                                                       │
  │           -> BPF-programmable network device (NETKIT [=n])                                                                                                                                                   │
  │
```

## 作为补充文档看看吧
https://news.ycombinator.com/item?id=41062838

## sched 分析
https://github.com/qais-yousef/sched-analyzer

https://news.ycombinator.com/item?id=41513860

## 在 4.19 中，没办法使用 bpftrace 的这个功能
这个依赖的底层功能是什么，从 sysfs 中获取 vmlinux 就可以了吗?
```txt
🧀  sudo bpftrace -v -e "kprobe:new_id_store { @[kstack] = count(); }"
BTF: failed to find BTF data for vmlinux, errno 3
BTF: failed to find BTF data
BTF: failed to find BTF data for vmlinux, errno 3
BTF: failed to find BTF data
node count: 7
Attaching 1 probe...
load kprobe:new_id_store, with BTF, with func_infos: BTF load failed
load kprobe:new_id_store, with BTF: BTF load failed
load kprobe:new_id_store, version: 4.19.90 = -22: Invalid argument
load kprobe:new_id_store, version: 4.19.90 = -22: Invalid argument
load kprobe:new_id_store = -22: Invalid argument

Error log:
back-edge from insn 69 to 49

ERROR: Error loading program: kprobe:new_id_store
```

所以，可以让 bcc 不去依赖 kernel-devel 吗?

为什么 bcc 依赖不是
```txt
🧀  sudo trace kprobe:new_id_store
modprobe: FATAL: Module kheaders not found in directory /lib/modules/martins3-4.19.x86_64
Unable to find kernel headers. Try rebuilding kernel with CONFIG_IKHEADERS=m (module) or installing the kernel development package for your running kernel version.
chdir(/run/booted-system/kernel-modules/lib/modules/martins3-4.19.x86_64/build): No such file or directory
Failed to compile BPF module <text>
```
想不到还有这种 hacking 办法:

```sh
sudo ln -sf /usr/src/kernels/martins3-4.19.x86_64 /run/booted-system/kernel-modules/lib/modules/martins3-4.19.x86_64/build
```

## 用户态 trace
https://news.ycombinator.com/item?id=38268958
https://blogs.oracle.com/linux/post/intro-to-bcc-3

## 这里的 xdp 仔细看看，其他的查漏补缺
  - https://github.com/eunomia-bpf/bpf-developer-tutorial
    - 现在，这个教程可以刮目相看了

## ebpf 定义新 bpf program types
https://docs.ebpf.io/linux/program-type/

或者说，为什么需要定义 program 的类型，是因为不同类型的函数
可以使用不同的函数吗?

例如为什么要定义出来这个: BPF_PROG_TYPE_SYSCALL

- [ ] 可以这里的 program types 继续看看
```txt
Tracing program types
These program types are triggered by tracing events from the kernel or userspace

BPF_PROG_TYPE_KPROBE
BPF_PROG_TYPE_TRACEPOINT
BPF_PROG_TYPE_PERF_EVENT
BPF_PROG_TYPE_RAW_TRACEPOINT
BPF_PROG_TYPE_RAW_TRACEPOINT_WRITABLE
BPF_PROG_TYPE_TRACING
```

似乎这个精确的回答了问题
https://arthurchiao.art/blog/bpf-advanced-notes-5-zh/

__bpf_kfunc 是什么含义?
```txt
__bpf_kfunc s32 scx_bpf_dsq_nr_queued(u64 dsq_id)
```

## libbpf 提供的 header
```txt
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
```
例如提供了 : scx_bpf_create_dsq

也就是在这个函数中：
tools/lib/bpf/bpf_helpers.h



但是实际上这个也是 kernel/sched/ext.c 中间的函数:
```c
/**
 * scx_bpf_create_dsq - Create a custom DSQ
 * @dsq_id: DSQ to create
 * @node: NUMA node to allocate from
 *
 * Create a custom DSQ identified by @dsq_id. Can be called from any sleepable
 * scx callback, and any BPF_PROG_TYPE_SYSCALL prog.
 */
__bpf_kfunc s32 scx_bpf_create_dsq(u64 dsq_id, s32 node)
{
	if (unlikely(node >= (int)nr_node_ids ||
		     (node < 0 && node != NUMA_NO_NODE)))
		return -EINVAL;
	return PTR_ERR_OR_ZERO(create_dsq(dsq_id, node));
}
```

## 显然这个书更加合适的
https://cilium.isovalent.com/hubfs/Learning-eBPF%20-%20Full%20book.pdf

## 构建内核的时候，最后可以看到这个，这个是做什么的?
```txt
  BTF [M] drivers/vfio/vfio_iommu_type1.ko
  BTF [M] drivers/target/iscsi/iscsi_target_mod.ko
  BTF [M] drivers/vfio/pci/mlx5/mlx5-vfio-pci.ko
  BTF [M] drivers/vfio/pci/nvgrace-gpu/nvgrace-gpu-vfio-pci.ko
  BTF [M] drivers/vfio/pci/vfio-pci.ko
  BTF [M] drivers/vfio/pci/virtio/virtio-vfio-pci.ko
  BTF [M] drivers/vfio/mdev/mdev.ko
  BTF [M] drivers/edac/edac_core.ko
  BTF [M] drivers/edac/edac_mce_amd.ko
  BTF [M] drivers/vhost/vhost_net.ko
```

## inode 在 bpf 是做什么用的
- kernel/bpf/bpf_inode_storage.c


## nettrace 这个项目很好
https://github.com/OpenCloudOS/nettrace

- 包括他的 readme 都是很好的，分析了各种 trace 工具的不足
- 为什么 kernel 不支持 BTF 需要重新编译?
- 看看他这个项目的结构

其实我们需要一个类似的项目来分析 block 系统，blocktrace 实在是太老了


## 似乎都搞到第三届了
https://space.bilibili.com/518970180?spm_id_from=333.337.0.0

https://github.com/linuxkerneltravel/ebpf-conference/blob/master/ebpf-conference-second/PPT/%E4%B8%BB%E4%BC%9A%E5%9C%BA/%E8%B0%A2%E5%AE%9D%E5%8F%8B--eBPF%E5%92%8C%E5%86%85%E6%A0%B8%E6%A8%A1%E5%9D%97%E5%9C%A8Linux%E8%AF%8A%E6%96%AD%E4%B8%AD%E7%9A%84%E5%BA%94%E7%94%A8.pdf


## 用户态的 trace 也可以使用 libbpf 吗?
https://github.com/josefbacik/systing

## 官方文档就已经写了很多了
Documentation/bpf/

## 看看这个东西效果
https://github.com/adgaultier/caracal

## 有趣
https://mp.weixin.qq.com/s/TH6dgGo7IfOrhy-_V-KMaw

## 看看这个工具
https://github.com/bpfsnoop/bpfsnoop

对于 bpftrace 有什么优势吗?

不过他是 go 写的

## 我们构建驱动的时候，有一个警告，这个是预期的吗?

Skipping BTF generation for martins3.ko due to unavailability of vmlinux

```txt
make -C /lib/modules/6.15.4/build M=/home/martins3/data/vn/code/src/m modules
make[1]: Entering directory '/usr/src/kernels/6.15.4'
make[2]: Entering directory '/home/martins3/data/vn/code/src/m'
call this in kernel environment
  CC [M]  main.o
  LD [M]  martins3.o
martins3.o: warning: objtool: test_suberror+0x1d: unreachable instruction
call this in kernel environment
  MODPOST Module.symvers
  CC [M]  martins3.mod.o
  LD [M]  martins3.ko
  BTF [M] martins3.ko
Skipping BTF generation for martins3.ko due to unavailability of vmlinux
make[2]: Leaving directory '/home/martins3/data/vn/code/src/m'
make[1]: Leaving directory '/usr/src/kernels/6.15.4'
```

## Monitoring Process/Thread exit using BPF
https://mp.weixin.qq.com/s/pAMa_EocY89o3KZpYsmBOw

https://blogs.oracle.com/linux/post/monitoring-processthread-exit-using-bpf

> 文章提出了一种基于 BPF 的解决方案：加载 BPF 程序到 `sched:sched_process_exit` 追踪点，
利用 环形缓冲区 (ring buffer) 和 哈希映射 (hashmap) 进行用户空间与内核空间通信。应用程序的进程/线程在启动时将自身注册到哈希映射，正常退出时取消注册。BPF 程序通过检查哈希映射来识别并仅报告异常退出的进程/线程给监控程序。这种方法更高效且定制化程度高。

https://mp.weixin.qq.com/s/OwaaxntiZdSqfW39OQXipg


BPF-DB: A Kernel-Embedded Transactional Database Management System For eBPF Applications
https://db.cs.cmu.edu/papers/2025/butrovich-sigmod2025.pdf

## 看看这个
https://github.com/daeuniverse/dae

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
