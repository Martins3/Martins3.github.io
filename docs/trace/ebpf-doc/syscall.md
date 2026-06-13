# bpf syscall 的基本观察
<!-- 8d725b6d-31b6-467c-880e-737f65a69441 -->

## 概述

`bpf()` 系统调用是 Linux 内核中用于加载和管理 eBPF 程序及映射的核心接口。更多详细信息可参考:
- [bpf(2) man page](https://man7.org/linux/man-pages/man2/bpf.2.html)
- 内核文档: `Documentation/userspace-api/ebpf/syscall.rst`

## bpf() 系统调用函数原型

```c
#include <linux/bpf.h>
#include <linux/bpf_common.h>

int bpf(int cmd, union bpf_attr *attr, unsigned int size);
```

- `cmd`: 指定要执行的操作（见 `enum bpf_cmd`）
- `attr`: 指向 `bpf_attr` union 的指针，包含操作所需的参数
- `size`: `attr` 的大小

## bpf_cmd 枚举

bpf() 系统调用支持以下命令：

```c
enum bpf_cmd {
    BPF_MAP_CREATE,                    /* 创建映射 */
    BPF_MAP_LOOKUP_ELEM,               /* 查找映射元素 */
    BPF_MAP_UPDATE_ELEM,               /* 更新映射元素 */
    BPF_MAP_DELETE_ELEM,               /* 删除映射元素 */
    BPF_MAP_GET_NEXT_KEY,              /* 获取下一个 key */
    BPF_PROG_LOAD,                     /* 加载 eBPF 程序 */
    BPF_OBJ_PIN,                       /* 将对象固定到 BPF 文件系统 */
    BPF_OBJ_GET,                       /* 获取固定对象 */
    BPF_PROG_ATTACH,                   /* 附加程序到 hook */
    BPF_PROG_DETACH,                   /* 从 hook 分离程序 */
    BPF_PROG_TEST_RUN,                 /* 测试运行程序 */
    BPF_PROG_RUN = BPF_PROG_TEST_RUN,  /* 别名 */
    BPF_PROG_GET_NEXT_ID,              /* 获取下一个程序 ID */
    BPF_MAP_GET_NEXT_ID,               /* 获取下一个映射 ID */
    BPF_PROG_GET_FD_BY_ID,             /* 通过 ID 获取程序 fd */
    BPF_MAP_GET_FD_BY_ID,              /* 通过 ID 获取映射 fd */
    BPF_OBJ_GET_INFO_BY_FD,            /* 获取对象信息 */
    BPF_PROG_QUERY,                    /* 查询已附加的程序 */
    BPF_RAW_TRACEPOINT_OPEN,           /* 打开原始 tracepoint */
    BPF_BTF_LOAD,                      /* 加载 BTF 数据 */
    BPF_BTF_GET_FD_BY_ID,              /* 通过 ID 获取 BTF fd */
    BPF_TASK_FD_QUERY,                 /* 查询任务 fd */
    BPF_MAP_LOOKUP_AND_DELETE_ELEM,    /* 查找并删除元素 */
    BPF_MAP_FREEZE,                    /* 冻结映射 */
    BPF_BTF_GET_NEXT_ID,               /* 获取下一个 BTF ID */
    BPF_MAP_LOOKUP_BATCH,              /* 批量查找 */
    BPF_MAP_LOOKUP_AND_DELETE_BATCH,   /* 批量查找并删除 */
    BPF_MAP_UPDATE_BATCH,              /* 批量更新 */
    BPF_MAP_DELETE_BATCH,              /* 批量删除 */
    BPF_LINK_CREATE,                   /* 创建 link */
    BPF_LINK_UPDATE,                   /* 更新 link */
    BPF_LINK_GET_FD_BY_ID,             /* 通过 ID 获取 link fd */
    BPF_LINK_GET_NEXT_ID,              /* 获取下一个 link ID */
    BPF_ENABLE_STATS,                  /* 启用统计 */
    BPF_ITER_CREATE,                   /* 创建迭代器 */
    BPF_LINK_DETACH,                   /* 分离 link */
    BPF_PROG_BIND_MAP,                 /* 绑定映射到程序 */
    BPF_TOKEN_CREATE,                  /* 创建 BPF token */
    BPF_PROG_STREAM_READ_BY_FD,        /* 流式读取 */
    __MAX_BPF_CMD,
};
```

## bpf_prog_type 程序类型

eBPF 程序按类型分类，每种类型对应特定的执行上下文和可用 helper 函数：

```c
enum bpf_prog_type {
    BPF_PROG_TYPE_UNSPEC,
    BPF_PROG_TYPE_SOCKET_FILTER,       /* 套接字过滤（经典 BPF 兼容） */
    BPF_PROG_TYPE_KPROBE,              /* kprobe 动态追踪 */
    BPF_PROG_TYPE_SCHED_CLS,           /* 流量分类（tc） */
    BPF_PROG_TYPE_SCHED_ACT,           /* 流量动作（tc） */
    BPF_PROG_TYPE_TRACEPOINT,          /* 静态 tracepoint */
    BPF_PROG_TYPE_XDP,                 /* XDP（eXpress Data Path） */
    BPF_PROG_TYPE_PERF_EVENT,          /* perf 事件 */
    BPF_PROG_TYPE_CGROUP_SKB,          /* cgroup 套接字缓冲区 */
    BPF_PROG_TYPE_CGROUP_SOCK,         /* cgroup 套接字 */
    BPF_PROG_TYPE_LWT_IN,              /* 轻量级隧道入站 */
    BPF_PROG_TYPE_LWT_OUT,             /* 轻量级隧道出站 */
    BPF_PROG_TYPE_LWT_XMIT,            /* 轻量级隧道传输 */
    BPF_PROG_TYPE_SOCK_OPS,            /* 套接字操作 */
    BPF_PROG_TYPE_SK_SKB,              /* 套接字缓冲区（sockmap） */
    BPF_PROG_TYPE_CGROUP_DEVICE,       /* cgroup 设备控制 */
    BPF_PROG_TYPE_SK_MSG,              /* 套接字消息 */
    BPF_PROG_TYPE_RAW_TRACEPOINT,      /* 原始 tracepoint */
    BPF_PROG_TYPE_CGROUP_SOCK_ADDR,    /* cgroup 套接字地址 */
    BPF_PROG_TYPE_LWT_SEG6LOCAL,       /* SRv6 本地处理 */
    BPF_PROG_TYPE_LIRC_MODE2,          /* LIRC 红外遥控 */
    BPF_PROG_TYPE_SK_REUSEPORT,        /* 套接字复用端口选择 */
    BPF_PROG_TYPE_FLOW_DISSECTOR,      /* 流解析器 */
    BPF_PROG_TYPE_CGROUP_SYSCTL,       /* cgroup sysctl */
    BPF_PROG_TYPE_RAW_TRACEPOINT_WRITABLE, /* 可写原始 tracepoint */
    BPF_PROG_TYPE_CGROUP_SOCKOPT,      /* cgroup 套接字选项 */
    BPF_PROG_TYPE_TRACING,             /* 追踪（fentry/fexit） */
    BPF_PROG_TYPE_STRUCT_OPS,          /* 结构体操作 */
    BPF_PROG_TYPE_EXT,                 /* BPF 程序扩展 */
    BPF_PROG_TYPE_LSM,                 /* Linux 安全模块 */
    BPF_PROG_TYPE_SK_LOOKUP,           /* 套接字查找 */
    BPF_PROG_TYPE_SYSCALL,             /* 可执行系统调用的程序 */
    BPF_PROG_TYPE_NETFILTER,           /* netfilter 钩子 */
    __MAX_BPF_PROG_TYPE
};
```

### 程序类型说明

| 程序类型 | 用途 | 典型场景 |
|---------|------|---------|
| `SOCKET_FILTER` | 网络数据包过滤 | tcpdump、原始套接字 |
| `KPROBE` | 内核函数动态追踪 | 调试、性能分析 |
| `TRACEPOINT` | 内核静态追踪点 | 内核事件追踪 |
| `XDP` | 高性能网络包处理 | DDoS 防护、负载均衡 |
| `SCHED_CLS/SCHED_ACT` | 流量控制 | QoS、流量整形 |
| `CGROUP_*` | cgroup 相关控制 | 资源限制、安全策略 |
| `TRACING` | 内核追踪（BTF 基础）| fentry/fexit/modify_return |
| `LSM` | 安全策略 | MAC、沙箱 |
| `SYSCALL` | 执行系统调用 | 用户空间 BPF 程序 |

### BPF_PROG_TYPE_PERF_EVENT vs BPF_PROG_TYPE_KPROBE

这两个程序类型都用于内核追踪，但使用方式和触发机制有重要区别：

#### BPF_PROG_TYPE_PERF_EVENT

**用途**：附加到 perf 事件（PMU - Performance Monitoring Unit），基于硬件/软件性能计数器采样触发。

**触发方式**：
- 基于**采样**（sampling）触发，不是每次事件都触发
- 由 perf 事件子系统驱动，支持硬件性能计数器（CPU 周期、缓存未命中等）和软件事件
- 使用 `perf_event_open()` 系统调用创建 perf 事件，然后通过 `BPF_PROG_ATTACH` 或 `bpf_link_create()` 附加 BPF 程序

**典型场景**：
- CPU 性能分析（周期采样）
- 获取调用栈（`bpf_get_stackid()` / `bpf_get_stack()`）
- 硬件性能监控（PMC - Performance Monitoring Counters）

**代码示例**：
```c
// BPF 程序
SEC("perf_event")
int oncpu(void *ctx)
{
    // 当 perf 事件触发时执行
    // ctx 是 pt_regs 指针
    bpf_get_stackid(ctx, &stackmap, 0);
    return 0;
}

// 用户空间创建 perf 事件并附加
struct perf_event_attr attr = {
    .type = PERF_TYPE_HARDWARE,
    .config = PERF_COUNT_HW_CPU_CYCLES,  // 每 N 个 CPU 周期触发一次
    .sample_period = 1000000,            // 采样周期
};
int pmu_fd = perf_event_open(&attr, -1, 0, -1, 0);
bpf_program__attach_perf_event(prog, pmu_fd);
```

##### 经典使用场景

可以想象，就是 perf_event 就是 trace 点，然后在哪些点可以加载各种任务。
```c
// BPF 程序 - 采样指令指针
SEC("perf_event")
int do_sample(struct bpf_perf_event_data *ctx)
{
    u64 ip = PT_REGS_IP(&ctx->regs);  // 获取当前指令指针
    u32 *value, init_val = 1;

    // 统计每个 IP 出现的频率
    value = bpf_map_lookup_elem(&ip_map, &ip);
    if (value)
        *value += 1;
    else
        bpf_map_update_elem(&ip_map, &ip, &init_val, BPF_NOEXIST);
    return 0;
}

// 用户空间 - 创建 perf 事件（CPU 周期采样）
struct perf_event_attr attr = {
    .type = PERF_TYPE_SOFTWARE,
    .config = PERF_COUNT_SW_CPU_CLOCK,  // 基于 CPU 时钟
    .freq = 1,                          // 使用频率模式
    .sample_period = 99,                // 每秒采样 99 次
};
```

```c
SEC("perf_event")
int oncpu(void *ctx)
{
    u32 key = 0;

    // 获取内核栈 ID
    int kernel_stack_id = bpf_get_stackid(ctx, &stack_map, 0);

    // 获取用户栈 ID
    int user_stack_id = bpf_get_stackid(ctx, &stack_map, BPF_F_USER_STACK);

    // 组合栈 ID 作为 key，统计出现次数
    struct key_t k = {
        .pid = bpf_get_current_pid_tgid() >> 32,
        .kernel_stack_id = kernel_stack_id,
        .user_stack_id = user_stack_id,
    };

    u64 *count = bpf_map_lookup_elem(&counts, &k);
    if (count)
        (*count)++;
    return 0;
}
```

```c
SEC("perf_event")
int trace_cpu_time(struct bpf_perf_event_data *ctx)
{
    u64 pid_tgid = bpf_get_current_pid_tgid();
    u32 pid = pid_tgid >> 32;

    // 累加当前任务的 CPU 时间
    u64 *time = bpf_map_lookup_elem(&cpu_time, &pid);
    if (time)
        (*time) += sampling_period;
    return 0;
}
```

#### BPF_PROG_TYPE_KPROBE

**用途**：动态追踪内核函数，在指定内核函数入口（kprobe）或返回（kretprobe）处插入探针。

**触发方式**：
- 基于**事件**触发，每次目标函数被调用都会触发
- 通过 kprobe 子系统实现，修改内核代码路径（INT3 或跳转指令）
- 支持 `kprobe`（入口）、`kretprobe`（返回）、`kprobe.multi`（批量函数匹配）

**典型场景**：
- 内核函数调用追踪
- 参数检查和修改
- 函数返回值捕获
- `bpf_override_return()`（修改函数返回值，需 CONFIG_BPF_KPROBE_OVERRIDE）

**代码示例**：
```c
// BPF 程序
SEC("kprobe/do_nanosleep")
int trace_do_nanosleep(struct pt_regs *ctx)
{
    // 每次 do_nanosleep 被调用时执行
    bpf_printk("do_nanosleep called");
    return 0;
}

SEC("kretprobe/do_nanosleep")
int trace_do_nanosleep_ret(struct pt_regs *ctx)
{
    // 每次 do_nanosleep 返回时执行
    bpf_printk("do_nanosleep returned");
    return 0;
}
```


## bpf_map_type 映射类型

BPF 映射用于在内核和用户空间之间，以及 eBPF 程序之间共享数据：

```c
enum bpf_map_type {
    BPF_MAP_TYPE_UNSPEC,
    BPF_MAP_TYPE_HASH,                 /* 哈希表 */
    BPF_MAP_TYPE_ARRAY,                /* 数组 */
    BPF_MAP_TYPE_PROG_ARRAY,           /* 程序数组（尾调用） */
    BPF_MAP_TYPE_PERF_EVENT_ARRAY,     /* perf 事件数组 */
    BPF_MAP_TYPE_PERCPU_HASH,          /* 每 CPU 哈希表 */
    BPF_MAP_TYPE_PERCPU_ARRAY,         /* 每 CPU 数组 */
    BPF_MAP_TYPE_STACK_TRACE,          /* 栈追踪 */
    BPF_MAP_TYPE_CGROUP_ARRAY,         /* cgroup 数组 */
    BPF_MAP_TYPE_LRU_HASH,             /* LRU 哈希表 */
    BPF_MAP_TYPE_LRU_PERCPU_HASH,      /* 每 CPU LRU 哈希表 */
    BPF_MAP_TYPE_LPM_TRIE,             /* 最长前缀匹配 Trie */
    BPF_MAP_TYPE_ARRAY_OF_MAPS,        /* 映射的数组 */
    BPF_MAP_TYPE_HASH_OF_MAPS,         /* 映射的哈希表 */
    BPF_MAP_TYPE_DEVMAP,               /* 设备映射（XDP） */
    BPF_MAP_TYPE_SOCKMAP,              /* 套接字映射 */
    BPF_MAP_TYPE_CPUMAP,               /* CPU 映射（XDP） */
    BPF_MAP_TYPE_XSKMAP,               /* XDP 套接字映射 */
    BPF_MAP_TYPE_SOCKHASH,             /* 套接字哈希表 */
    BPF_MAP_TYPE_CGROUP_STORAGE,       /* cgroup 存储 */
    BPF_MAP_TYPE_REUSEPORT_SOCKARRAY,  /* 复用端口套接字数组 */
    BPF_MAP_TYPE_PERCPU_CGROUP_STORAGE, /* 每 CPU cgroup 存储 */
    BPF_MAP_TYPE_QUEUE,                /* 队列 */
    BPF_MAP_TYPE_STACK,                /* 栈 */
    BPF_MAP_TYPE_SK_STORAGE,           /* 套接字本地存储 */
    BPF_MAP_TYPE_DEVMAP_HASH,          /* 设备哈希映射 */
    BPF_MAP_TYPE_STRUCT_OPS,           /* 结构体操作 */
    BPF_MAP_TYPE_RINGBUF,              /* 环形缓冲区 */
    BPF_MAP_TYPE_INODE_STORAGE,        /* inode 本地存储 */
    BPF_MAP_TYPE_TASK_STORAGE,         /* 任务本地存储 */
    BPF_MAP_TYPE_BLOOM_FILTER,         /* 布隆过滤器 */
    BPF_MAP_TYPE_USER_RINGBUF,         /* 用户空间环形缓冲区 */
    BPF_MAP_TYPE_CGRP_STORAGE,         /* cgroup 存储（v2） */
    BPF_MAP_TYPE_ARENA,                /* 内存竞技场 */
    BPF_MAP_TYPE_INSN_ARRAY,           /* 指令数组 */
    __MAX_BPF_MAP_TYPE
};
```

## bpf_attach_type 附加类型

定义 eBPF 程序可以附加到的 hook 点：

```c
enum bpf_attach_type {
    BPF_CGROUP_INET_INGRESS,
    BPF_CGROUP_INET_EGRESS,
    BPF_CGROUP_INET_SOCK_CREATE,
    BPF_CGROUP_SOCK_OPS,
    BPF_SK_SKB_STREAM_PARSER,
    BPF_SK_SKB_STREAM_VERDICT,
    BPF_CGROUP_DEVICE,
    BPF_SK_MSG_VERDICT,
    BPF_CGROUP_INET4_BIND,
    BPF_CGROUP_INET6_BIND,
    BPF_CGROUP_INET4_CONNECT,
    BPF_CGROUP_INET6_CONNECT,
    BPF_CGROUP_INET4_POST_BIND,
    BPF_CGROUP_INET6_POST_BIND,
    BPF_CGROUP_UDP4_SENDMSG,
    BPF_CGROUP_UDP6_SENDMSG,
    BPF_LIRC_MODE2,
    BPF_FLOW_DISSECTOR,
    BPF_CGROUP_SYSCTL,
    BPF_CGROUP_UDP4_RECVMSG,
    BPF_CGROUP_UDP6_RECVMSG,
    BPF_CGROUP_GETSOCKOPT,
    BPF_CGROUP_SETSOCKOPT,
    BPF_TRACE_RAW_TP,
    BPF_TRACE_FENTRY,              /* 函数入口追踪 */
    BPF_TRACE_FEXIT,               /* 函数退出追踪 */
    BPF_MODIFY_RETURN,             /* 修改函数返回值 */
    BPF_LSM_MAC,
    BPF_TRACE_ITER,                /* 迭代器追踪 */
    BPF_CGROUP_INET4_GETPEERNAME,
    BPF_CGROUP_INET6_GETPEERNAME,
    BPF_CGROUP_INET4_GETSOCKNAME,
    BPF_CGROUP_INET6_GETSOCKNAME,
    BPF_XDP_DEVMAP,
    BPF_CGROUP_INET_SOCK_RELEASE,
    BPF_XDP_CPUMAP,
    BPF_SK_LOOKUP,
    BPF_XDP,
    BPF_SK_SKB_VERDICT,
    BPF_SK_REUSEPORT_SELECT,
    BPF_SK_REUSEPORT_SELECT_OR_MIGRATE,
    BPF_PERF_EVENT,
    BPF_TRACE_KPROBE_MULTI,        /* 多 kprobe */
    BPF_LSM_CGROUP,
    BPF_STRUCT_OPS,
    BPF_NETFILTER,
    BPF_TCX_INGRESS,               /* TCX 入站 */
    BPF_TCX_EGRESS,                /* TCX 出站 */
    BPF_TRACE_UPROBE_MULTI,        /* 多 uprobe */
    BPF_CGROUP_UNIX_CONNECT,
    BPF_CGROUP_UNIX_SENDMSG,
    BPF_CGROUP_UNIX_RECVMSG,
    BPF_CGROUP_UNIX_GETPEERNAME,
    BPF_CGROUP_UNIX_GETSOCKNAME,
    BPF_NETKIT_PRIMARY,
    BPF_NETKIT_PEER,
    BPF_TRACE_KPROBE_SESSION,
    BPF_TRACE_UPROBE_SESSION,
    __MAX_BPF_ATTACH_TYPE
};
```

## bpf_link_type 链接类型

BPF link 提供了一种统一的方式来管理 eBPF 程序的附加：

```c
enum bpf_link_type {
    BPF_LINK_TYPE_UNSPEC = 0,
    BPF_LINK_TYPE_RAW_TRACEPOINT = 1,
    BPF_LINK_TYPE_TRACING = 2,
    BPF_LINK_TYPE_CGROUP = 3,
    BPF_LINK_TYPE_ITER = 4,
    BPF_LINK_TYPE_NETNS = 5,
    BPF_LINK_TYPE_XDP = 6,
    BPF_LINK_TYPE_PERF_EVENT = 7,
    BPF_LINK_TYPE_KPROBE_MULTI = 8,
    BPF_LINK_TYPE_STRUCT_OPS = 9,
    BPF_LINK_TYPE_NETFILTER = 10,
    BPF_LINK_TYPE_TCX = 11,
    BPF_LINK_TYPE_UPROBE_MULTI = 12,
    BPF_LINK_TYPE_NETKIT = 13,
    BPF_LINK_TYPE_SOCKMAP = 14,
    __MAX_BPF_LINK_TYPE,
};
```

## 重要说明

### 追踪程序稳定性

> 注意：与追踪相关的程序类型（`BPF_PROG_TYPE_KPROBE`、`BPF_PROG_TYPE_TRACEPOINT`、
> `BPF_PROG_TYPE_PERF_EVENT`、`BPF_PROG_TYPE_RAW_TRACEPOINT`）**不受稳定 API 约束**。
> 因为内核内部数据结构可能在不同版本间发生变化，可能破坏现有的追踪 BPF 程序。
>
> 追踪 BPF 程序对应于**特定的内核版本**，而非所有未来版本。

### cgroup BPF 附加标志

当使用 `BPF_PROG_ATTACH` 将程序附加到 cgroup 时，可使用以下标志：

- **NONE**（默认）: 子树中不允许其他 BPF 程序
- **BPF_F_ALLOW_OVERRIDE**: 子 cgroup 的程序可以覆盖当前程序
- **BPF_F_ALLOW_MULTI**: 允许多个程序同时附加（FIFO 顺序执行）
- **BPF_F_REPLACE**: 替换指定位置的程序

### 程序加载标志

- **BPF_F_STRICT_ALIGNMENT**: 强制严格的指针对齐检查
- **BPF_F_ANY_ALIGNMENT**: 放宽指针对齐要求
- **BPF_F_TEST_RND_HI32**: 测试时随机化高 32 位
- **BPF_F_TOKEN_FD**: 使用 BPF token

## 参考文档

- [bpf(2) man page](https://man7.org/linux/man-pages/man2/bpf.2.html)
- 内核: `Documentation/userspace-api/ebpf/syscall.rst`
- 内核: `Documentation/bpf/syscall_api.rst`
- 头文件: `include/uapi/linux/bpf.h`

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
