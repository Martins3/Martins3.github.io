# bpftool

## 基本使用

### sudo bpftool prog show

虚拟机中 ./bpftool.guest.prog 物理机中 ./bpftool.guest.prog

### bpftool feature

./bpftool.feature

## bpftool 的使用

bpftool prog help

## 从 bpftool feature 的实现说起

- [ ] 似乎不同的 eBPF program types 会有不同的 helper

eBPF program_type cgroup_sockopt is available

Scanning eBPF program types...

为什么会有这么多的 eBPF program types ?

## TODO

- 看完这个操作吧
  https://gist.github.com/navarrothiago/e1f7228610a0bd07aea4928f6381e61d


## `bpftool.feature` 输出解读

<!-- 35d83232-d67a-4a71-b237-581c28a91556 -->

### 总结

这份文件是一次完整的 `bpftool feature probe`
文本输出。它描述的是“运行命令时，当前内核在当前权限和启动配置下实际探测到的能力”，而不只是内核源码里定义了哪些枚举值。

从结果看，这台机器的 eBPF 支持很完整：

- `bpf()` 系统调用可用，但普通用户被禁止直接使用；管理员可以修改这个策略。
- eBPF JIT 已启用，并且内核配置为始终使用 JIT；JIT hardening 没有启用。
- 内核和内核模块都带 BTF，适合使用 CO-RE、fentry/fexit、BPF LSM 等依赖 BTF
  的能力。
- 本次 `bpftool` 认识的 32 种 program type 和 33 种 map type 全部探测为
  available。
- 支持大程序限制、有限循环以及 eBPF ISA v2、v3、v4。
- 唯一明显缺少的是 `bpf_override_return()` 所需的内核配置。这不影响普通的
  kprobe、tracepoint 或 fentry/fexit 观测，只影响用 kprobe
  修改被探测函数返回值的错误注入场景。

不过，`available` 只代表 `bpftool` 的最小探测成功，不代表所有 attach
方式、所有参数组合和所有硬件 offload 都可用。

### 输出分为哪几部分

`bpftool feature probe` 按下面的顺序探测：

1. 系统运行参数和内核配置。
2. `bpf()` 系统调用是否存在。
3. 各种 eBPF program type 是否可加载。
4. 各种 eBPF map type 是否可创建。
5. 每一种 program type 可以调用哪些 helper。
6. 大程序、有限循环和 eBPF 指令集扩展等杂项能力。

源码入口是 `tools/bpf/bpftool/feature.c` 中的 `do_feature()` 和
`do_probe()`。`do_probe()` 依次调用
`section_system_config()`、`section_syscall_config()`、`section_program_types()`、`section_map_types()`、`section_helpers()`
和 `section_misc()`，所以输出顺序与实现是一一对应的。

### 系统配置

开头几行来自运行时 sysctl，而不是内核 `.config`：

| 输出                                                              | 含义                                                                                                                 |
| ----------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------- |
| `bpf() syscall restricted to privileged users (admin can change)` | `/proc/sys/kernel/unprivileged_bpf_disabled` 为 2。非特权用户不能直接使用 `bpf()`，但管理员仍可把它改回 0。          |
| `JIT compiler is enabled`                                         | `/proc/sys/net/core/bpf_jit_enable` 已打开，eBPF 字节码会编译为本机指令。                                            |
| `JIT compiler hardening is disabled`                              | 没有启用 JIT constant blinding 等加固。由于非特权 BPF 已禁用，主要风险面已经被权限策略隔离，但这两项不是同一个机制。 |
| `JIT compiler kallsyms exports are enabled for root`              | root 可以在 kallsyms 中看到 JIT 后的 BPF 符号，方便 perf、崩溃分析和调试。                                           |
| `Global memory limit ... 528482304 bytes`                         | JIT 代码使用的全局内存上限约为 504 MiB，不是单个 BPF 程序的大小上限。                                                |

内核配置里最关键的几组是：

- 基础能力：`CONFIG_BPF`、`CONFIG_BPF_SYSCALL`、`CONFIG_HAVE_EBPF_JIT`、`CONFIG_BPF_JIT`
  和 `CONFIG_BPF_JIT_ALWAYS_ON` 都是 `y`。
- BTF：`CONFIG_DEBUG_INFO_BTF` 和 `CONFIG_DEBUG_INFO_BTF_MODULES` 都是
  `y`。前者提供 vmlinux BTF，后者让模块也能提供 BTF。
- tracing：`CONFIG_BPF_EVENTS`、`CONFIG_KPROBE_EVENTS`、`CONFIG_UPROBE_EVENTS`、`CONFIG_TRACING`
  和 `CONFIG_FTRACE_SYSCALLS` 都是 `y`。
- cgroup：`CONFIG_CGROUPS`、`CONFIG_CGROUP_BPF` 等配置已经打开。
- network：XDP、LWT、TC、socket map、SRv6 BPF 等相关配置都存在；其中部分
  TC/netfilter/test 配置是 `m`，表示以模块形式提供，而不是功能缺失。

下面两项没有设置：

```txt
CONFIG_FUNCTION_ERROR_INJECTION is not set
CONFIG_BPF_KPROBE_OVERRIDE is not set
```

二者共同影响 `bpf_override_return()`。因此文档前面 `bpftrace --info` 中的
`override_return: no` 与这里是相互印证的。

`CONFIG_HZ=1000` 不是 BPF 开关，它表示每秒 1000 个 jiffy。`bpftool`
输出它是因为使用 `bpf_jiffies64()` 时，用户需要知道 jiffy 与时间的换算关系。

### `bpf()` 系统调用可用意味着什么

`bpf() syscall is available`
只是在确认系统调用号存在。`tools/bpf/bpftool/feature.c` 中 `probe_bpf_syscall()`
会故意发起一次参数无效的 `BPF_PROG_LOAD`，只要返回值不是
`ENOSYS`，就认为系统调用存在。

所以这行不表示当前用户一定有权限加载程序，也不表示某一种具体 BPF
功能可用。权限和具体能力由后面的探测继续判断。

### 为什么有这么多 program type

program type 不是同一种程序的用途标签，而是 verifier 与内核 hook
之间的契约。它至少确定四件事：

1. 程序从哪里被触发、如何 attach。
2. `R1` 指向的 context 是什么类型，程序可以访问 context 的哪些字段。
3. 程序允许调用哪些 helper 或 kfunc。
4. 程序返回值的语义，例如放行、丢包、重定向或选择 socket。

因此网络包、socket、cgroup、tracing 和安全策略不能共用一个 program
type。它们看到的数据结构、生命周期、执行上下文和返回动作都不同。verifier
必须在加载时知道这些约束，才能证明程序安全。

这 32 种类型可以按用途理解：

| 类别           | 主要 program type                                                                                         | 用途                                                                                           |
| -------------- | --------------------------------------------------------------------------------------------------------- | ---------------------------------------------------------------------------------------------- |
| tracing/perf   | `kprobe`、`tracepoint`、`raw_tracepoint`、`raw_tracepoint_writable`、`perf_event`、`tracing`              | 观察内核函数、静态事件和性能事件；`tracing` 还承载 fentry/fexit、BPF iterator 等 attach type。 |
| 报文处理       | `socket_filter`、`sched_cls`、`sched_act`、`xdp`、`lwt_*`、`lwt_seg6local`、`flow_dissector`、`netfilter` | 在协议栈的不同层次分类、修改、丢弃或重定向报文。                                               |
| socket         | `sock_ops`、`sk_skb`、`sk_msg`、`sk_reuseport`、`sk_lookup`                                               | 控制 TCP/socket 行为、处理 socket map 中的数据流或选择监听 socket。                            |
| cgroup 策略    | `cgroup_skb`、`cgroup_sock`、`cgroup_device`、`cgroup_sock_addr`、`cgroup_sysctl`、`cgroup_sockopt`       | 把网络、设备、sysctl 和 socket 策略绑定到 cgroup。                                             |
| 内核扩展与安全 | `struct_ops`、`ext`、`lsm`                                                                                | 实现内核结构体回调、替换/扩展 BPF 或内核函数、执行 LSM 安全策略。                              |
| 特殊用途       | `lirc_mode2`、`syscall`                                                                                   | 红外接收以及由用户态显式触发的 syscall 类型 BPF 程序。                                         |

`tools/lib/bpf/libbpf_probes.c` 中 `libbpf_probe_bpf_prog_type()`
通常加载一个只执行 `return 0` 的最小程序。对于必须提供 attach type、BTF ID
或其他上下文的类型，`probe_prog_load()`
会补充特殊参数，或者根据内核返回的预期错误判断该类型已经存在。

因此 `eBPF program_type kprobe is available`
的准确含义是“内核识别并接受这种程序类型的最小加载探测”，不是“任意 kprobe attach
方式都已验证”。例如它不能顺便证明 `kprobe_multi`、特定目标函数或某个内核模块上的
attach 一定成功。

### map type 结果怎么读

33 种 map type 也可以按数据模型分组：

| 类别                | 主要 map type                                                                                            |
| ------------------- | -------------------------------------------------------------------------------------------------------- |
| 通用键值/数组       | `hash`、`array`、`percpu_*`、`lru_*`、`lpm_trie`                                                         |
| 程序调度与事件      | `prog_array`、`perf_event_array`、`stack_trace`                                                          |
| map in map          | `array_of_maps`、`hash_of_maps`                                                                          |
| 网络重定向与 socket | `devmap`、`devmap_hash`、`cpumap`、`xskmap`、`sockmap`、`sockhash`、`reuseport_sockarray`                |
| 容器与算法          | `queue`、`stack`、`bloom_filter`                                                                         |
| 对象本地存储        | `cgroup_storage`、`percpu_cgroup_storage`、`cgrp_storage`、`sk_storage`、`inode_storage`、`task_storage` |
| 用户态通信          | `ringbuf`、`user_ringbuf`                                                                                |
| 内核扩展/共享内存   | `struct_ops`、`arena`                                                                                    |

`tools/lib/bpf/libbpf_probes.c` 中 `libbpf_probe_bpf_map_type()` 会为每种 map
构造一组最小合法参数，再调用 `BPF_MAP_CREATE`。例如 ring buffer 需要页大小的
`max_entries`，map in map 需要先创建 inner map，本地存储 map 需要临时 BTF。

所以 `map_type arena is available` 表示内核至少能创建最小 arena
map。这是一个很强的“内核较新”信号，但仍不等于任意 map flag、容量、NUMA 配置或
mmap 用法都测试通过。

### 为什么不同 program type 的 helper 不同
<!-- c9ce84d8-95f8-4f56-99c5-a491d83d500d -->

helper 能否使用取决于程序的执行上下文和安全约束。例如：

- `kprobe` 可以读取 tracing 上下文、获取调用栈，但不能随意使用只对
  `struct __sk_buff` 有意义的报文修改 helper。
- `xdp` 操作的是 `xdp_md`，可以调整 packet head/tail、访问 XDP
  元数据和重定向报文。
- `sched_cls`/`sched_act` 操作
  `struct __sk_buff`，位于更完整的网络栈上下文，所以文件中探测到的 helper 数量比
  XDP 更多。
- `lirc_mode2` 的上下文和任务非常窄，因此只探测到 13 个 helper。

这种限制不是文档层面的约定，而是在 verifier 中按 program type
执行的。加载程序时，即使 helper ID 本身存在，如果当前 program type
不允许它，verifier 仍会拒绝程序。

helper 探测的实现也值得注意。`libbpf_probe_bpf_helper()` 构造一条
`BPF_CALL helper_id` 加一条 `BPF_EXIT`，然后分析 verifier 日志：

- `invalid func`：内核根本不认识这个 helper ID。
- `unknown func` 或 `program of this type cannot use helper`：内核认识它，但当前
  program type 不允许。
- 如果只是参数类型不对，反而能说明 helper 已被识别，因此仍会报告为 available。

默认探测会跳过 `bpf_trace_printk()`、`bpf_trace_vprintk()` 和
`bpf_probe_write_user()`，因为探测它们可能向 dmesg 写警告。需要包含这些 helper
时使用：

```sh
sudo bpftool feature probe kernel full
```

这份列表只覆盖 `enum bpf_func_id` 定义的传统 helper，不覆盖 BTF kfunc。kfunc
的可用性还取决于具体内核/模块 BTF、kfunc set、program type 和 attach
上下文，不能从这里的 helper 列表推导出来。

### 四个空 helper 列表不是“不支持 helper”

下面四段没有列出任何 helper：

```txt
eBPF helpers supported for program type tracing:
eBPF helpers supported for program type struct_ops:
eBPF helpers supported for program type ext:
eBPF helpers supported for program type lsm:
```

不能把它们理解为这些 program type 不能调用
helper。`tools/lib/bpf/libbpf_probes.c` 中 `libbpf_probe_bpf_helper()`
明确对这四种类型返回 `EOPNOTSUPP`，因为无法脱离具体 BTF attach
target，使用同一份两指令程序可靠地探测所有 helper。

也就是说：program type 探测为 available，而 helper 矩阵在这里是 unknown，不是
empty。真正编写程序时，应以 verifier 的加载结果、对应 program type
文档以及目标内核的 BTF/kfunc 信息为准。

### 最后的 miscellaneous features

| 输出                                    | 实际含义                                                                                                              |
| --------------------------------------- | --------------------------------------------------------------------------------------------------------------------- |
| `Large program size limit is available` | 内核接受超过旧 `BPF_MAXINSNS` 4096 条指令的特权程序探测；这不等于程序可以无限大，仍受一百万级 verifier 复杂度等限制。 |
| `Bounded loop support is available`     | verifier 可以证明并接受有确定终止条件的循环，不再要求编译器把所有循环完全展开。                                       |
| `ISA extension v2/v3/v4 is available`   | 内核认识 bpftool 为各代 ISA 准备的探测指令。这里的 ISA 版本是 eBPF 指令集版本，不是 Linux 内核版本，也不是 BTF 版本。 |

这几项是通过真正加载包含对应指令的极小 `socket_filter` 程序探测的。实现位于
`tools/bpf/bpftool/feature.c` 的
`probe_large_insn_limit()`、`probe_bounded_loops()` 和
`probe_v*_isa_extension()`。

### 使用这份结果时的边界

这份输出适合回答“这台机器的基础 BPF 能力是否存在”，但不能单独回答下面的问题：

- 某个具体 hook 是否存在，例如某个 tracepoint、kfunc 或内核函数。
- 某种 attach 变体是否支持，例如 `kprobe_multi`、`uprobe_multi` 或 TCX。
- 某个 helper/kfunc 在给定参数、GPL license、sleepable 程序中是否可用。
- BPF token、硬件 offload、特定网卡驱动是否支持。
- 当前进程是否拥有 `CAP_BPF`、`CAP_PERFMON`、`CAP_NET_ADMIN` 等实际所需
  capability。

遇到这些问题，最终判断仍然应以目标程序在目标内核上的 `BPF_PROG_LOAD` 和 attach
结果为准，并保留 verifier log。`bpftool feature probe`
更像能力矩阵的第一层筛查。

为了让结果便于脚本处理或比较两台机器，可以使用 JSON 输出：

```sh
sudo bpftool -j feature probe kernel | jq
```

也可以只探测某个支持 BPF offload 的网卡：

```sh
sudo bpftool feature probe dev eth0
```

`kernel` 与 `dev eth0` 回答的是两个不同问题：前者探测主机内核能力，后者探测指定
netdev 的 BPF 硬件 offload 能力。

## sudo bpftrace --info

```txt
System
  OS: Linux 6.9.3 #1-NixOS SMP PREEMPT_DYNAMIC Thu May 30 07:45:04 UTC 2024
  Arch: x86_64

Build
  version: v0.20.4
  LLVM: 17.0.6
  unsafe probe: no
  bfd: yes
  libdw (DWARF support): yes

Kernel helpers
  probe_read: yes
  probe_read_str: yes
  probe_read_user: yes
  probe_read_user_str: yes
  probe_read_kernel: yes
  probe_read_kernel_str: yes
  get_current_cgroup_id: yes
  send_signal: yes
  override_return: no
  get_boot_ns: yes
  dpath: yes
  skboutput: yes
  get_tai_ns: yes
  get_func_ip: yes
  jiffies64: yes

Kernel features
  Instruction limit: 1000000
  Loop support: yes
  btf: yes
  module btf: yes
  map batch: yes
  uprobe refcount (depends on Build:bcc bpf_attach_uprobe refcount): yes

Map types
  hash: yes
  percpu hash: yes
  array: yes
  percpu array: yes
  stack_trace: yes
  perf_event_array: yes
  ringbuf: yes

Probe types
  kprobe: yes
  tracepoint: yes
  perf_event: yes
  kfunc: yes
  kprobe_multi: no
  uprobe_multi: yes
  raw_tp_special: yes
  iter: yes
```

## 问题解答
1. 到底有那些 bpf 的 helper

## SEC("") 到底含有什么内容

```txt
fentry/fexit
kprobe/kretprobe
perf_event

raw_tp <--- 三个的区别是什么
tp_btf
tracepoint

uprobe/uretprobe

usdt
```
这里的代码都是似乎分析了: libbpf-bootstrap/README.md

从 raw_tp 和 tp 在这里也是有点说明的:

https://manpages.ubuntu.com/manpages/lunar/man8/kvmexit-bpfcc.8.html
```txt
       The  impact  of  using this tool on the host should be negligible. While this tool is very
       efficient, it does affect the guest virtual machine itself, the average  test  results  on
       guest vm are as follows:
                      | cpu cycles
           no TP      |   1127
           regular TP |   1277 (13% downgrade)
           RAW TP     |   1187 (5% downgrade)
```

- [ ] 实际上，bpf 可以 hook 的位置很多，但是只是分析了其中很少的一部分
	-  kprobes
	-  uprobes
	-  syscalls
	-  fentry / fexit
	-  tracepoints
	-  network devices (tc / xdp)
	-  network routes
	-  TCP conjections algorithms
	-  sockets (data level)

### codex

SEC() 是 libbpf 提供的宏，用来把函数或变量放进 BPF ELF 文件的指定 section。它不是 C 关键字，也不是内核源码的普
通 section。

在 bpf_helpers.h 中，SEC() 基本等价于：

```txt
#define SEC(name) __attribute__((section(name), used))
```

例如 opensnoop.bpf.c 中：

SEC("tracepoint/syscalls/sys_enter_open")
int tracepoint__syscalls__sys_enter_open(struct syscall_trace_enter *ctx)
{
    ...
}

含义是：

1. Clang 把这个函数放入名为 tracepoint/syscalls/sys_enter_open 的 ELF section。
2. used 防止编译器认为函数未被 C 代码调用而将其删除。
3. libbpf 加载 .bpf.o 时解析 section 名。
4. tracepoint/ 告诉 libbpf，这是 BPF_PROG_TYPE_TRACEPOINT。
5. 后面的 syscalls/sys_enter_open 告诉 libbpf attach 到哪个 tracepoint。

实际流程为：

SEC("tracepoint/syscalls/sys_enter_open")
              |
              v
ELF section: tracepoint/syscalls/sys_enter_open
              |
              v
libbpf 的 section_defs[] 匹配 "tracepoint+"
              |
              v
程序类型 = BPF_PROG_TYPE_TRACEPOINT
attach 目标 = syscalls:sys_enter_open

在当前生成的 opensnoop.bpf.o 中，通过 readelf 确实能看到：

tracepoint/syscalls/sys_enter_open
tracepoint/syscalls/sys_enter_openat
tracepoint/syscalls/sys_enter_openat2
.maps
license

不同 SEC() 有不同含义：

SEC("kprobe/do_sys_open")       // kprobe
SEC("fentry/vfs_open")          // fentry
SEC("fexit/vfs_open")           // fexit
SEC("tracepoint/sched/sched_switch")
SEC("raw_tracepoint/sched_switch")
SEC("xdp")                      // XDP 程序
SEC("tc")                       // TC classifier
SEC(".maps")                    // BPF map 定义
SEC("license")                  // BPF 程序许可证

其中：

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    ...
} start SEC(".maps");

表示把 map 定义放进 .maps section，让 libbpf 根据 BTF 信息创建 map。

char LICENSE[] SEC("license") = "GPL";

表示把许可证放进 license section。内核加载 BPF 程序时会读取它；某些 GPL-only BPF helper 只有 GPL 兼容许可证才
能使用。

最关键的理解是：SEC() 的字符串是 BPF 程序与 libbpf 之间的一套约定。函数名主要用于生成 skeleton 字段和调试，而
真正决定程序类型及默认 attach 位置的是 section 名。对应实现可从 bpf_helpers.h 中 SEC()、libbpf.c 中
section_defs[]、find_sec_def() 和 attach_tp() 串起来看。

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
