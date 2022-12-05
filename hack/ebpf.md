# eBPF
https://github.com/mikeroyal/eBPF-Guide

- 这里的资料很不错的: https://www.ebpf.top/page/6/

- [ ] profiling 和 tracing 有什么区别，bpf 是如何分别实现两者的功能的
- [ ] networking 相关的工作
- [ ] bpf 如何实现安全?

似乎 eBPF 可以让 package 的 filter 提前。

两个很新的 talk:
- https://www.youtube.com/watch?v=_5Z2AU7QTH4
- https://www.youtube.com/watch?v=AV8xY318rtc
- https://www.circonus.com/2018/05/linux-system-monitoring-with-ebpf/ ：里面的图是如何被绘制出来的

## xdp
- https://www.spinics.net/lists/xdp-newbies/msg00185.html
- https://www.tigera.io/learn/guides/ebpf/ebpf-xdp/
- https://docs.cilium.io/en/latest/bpf/

## links

- [ ] [Dive into BPF: a list of reading material](https://qmonnet.github.io/whirl-offload/2016/09/01/dive-into-bpf/) : may be read all articles of the author

https://github.com/netblue30/firejail
https://filipnikolovski.com/posts/ebpf/
https://facebookmicrosites.github.io/bpf/
https://github.com/zoidbergwill/awesome-ebpf

## overview

为什么使用eBPF ?[^1]
3. 为了向内核中间添加功能，如果修改kernel source code，需要等到用户更新内核。如果使用kernel module，每次内核升级，都需要发布对应的kernel module.
1. eBPF 是 100% modular and composable 的
2. eBPF 可以实现 hotpatching

1. safety & security : verifier 保证内核中间发
2. continuous delivery : 程序可以动态修改
3. performance : JIT compiler ensures native execution performance (为什么JIT可以实现 native f)

bpf 可以加入的位置 : kprobe uprobe syscall fentry/fexit, some network related stuff

eBPF map 的作用，eBPF helper ，function calls

eBPF 的升级内容:[^2]
1. 64bit 的寄存器
2. 寄存器数量从2个到10个
3. `BPF_CALL` : Plus, a new `BPF_CALL` instruction made it possible to call in-kernel functions cheaply.
4. The ease of mapping eBPF to native instructions lends itself to just-in-time compilation, yielding improved performance.

eBPF 的作用不仅仅限于 packet filter 的功能，其实可以动态的 debug 内核:
eBPF is also useful for debugging the kernel and carrying out performance analysis;
programs can be attached to tracepoints, kprobes, and perf events.
Because eBPF programs can access kernel data structures, developers can write and test new debugging code without having to recompile the kernel. The implications are obvious for busy engineers debugging issues on live, running systems. It's even possible to use eBPF to debug user-space programs by using Userland Statically Defined Tracepoints.


eBPF verifier :
1. 对于CFG进行DFS，保证其中不会出现递归，死循环，以及不可执行的代码
2. 对于每条指令都进行模拟执行，保证程序的执行总是正常的
3. secure mode 下，不可以使用指针运算
4. Registers with uninitialized contents (those that have never been written to) cannot be read;
5. *The contents of registers R0-R5 are marked as unreadable across functions calls by storing a special value to catch any reads of an uninitialized register.* 什么叫做，R0 R5 之间
6. Similar checks are done for reading variables on the stack and to make sure that no instructions write to the read-only frame-pointer register.

Lastly, the verifier uses the eBPF program type (covered later) to restrict which kernel functions can be called from eBPF programs and which data structures can be accessed.


```c
int bpf(int cmd, union bpf_attr *attr, unsigned int size);
```
1. The `bpf_attr` union allows data to be passed between the kernel and user space;
2. the exact format depends on the `cmd` argument.
3. The `size` argument gives the size of the `bpf_attr` union object in bytes.

cmd 类型包括:
1. 修改用于eBPF程序和kernel或者user space 沟通的 eBPF map
2. 将 eBPF 附着于特定的位置(socket file descriptor)

Though there appear to be many different commands, they can be broken down into three categories:
1. commands for working with eBPF programs,
2. working with eBPF maps,
3. or commands for working with both programs and maps (collectively known as objects).

eBPF map : Each map is defined by four values: a type, a maximum number of elements, a value size in bytes, and a key size in bytes.

如何使用 BPF:
1. 使用 Clang -march=bpf 编译 或者手动写汇编代码
2. samples/bpf/ 提供了很多测试程序
3. libpf 库 For example, the high-level flow of an eBPF program and user program using libbpf might go something like:
  - Read the eBPF bytecode into a buffer in your user application and pass it to bpf_load_program().
  - The eBPF program, when run by the kernel, will call bpf_map_lookup_elem() to find an element in a map and store a new value in it.
  - The user application calls bpf_map_lookup_elem() to read out the value stored by the eBPF program in the kernel.
这些测试程序的问题在于，需要使用 bpf 程序需要在 kernel source tree 中间编译，BCC 处理掉这个问题。

eBPF 的关键工具 : BCC 介绍了基本使用规则 [^3]
The project consists of a toolchain for writing, compiling, and loading eBPF programs, along with example programs and battle-hardened tools for debugging and diagnosing performance issues.

eBPF 更多的使用 : BCC 介绍处理用户层的代码 [^4]
// TODO 上面的代码操作一下

// TODO 关于 BPF 的问题在于，还是无法理解为什么实现监控

// TODO 一些高级话题 : [^6]

brendangregg 写的关于 eBPF 的内容: [^12]
1. eBPF 的消息来源:
kprobes: kernel dynamic tracing.
uprobes: user level dynamic tracing.
tracepoints: kernel static tracing.
perf_events: timed sampling and PMCs.

2. eBPF 将获取到的消息导出用户层的方法:
The BPF program has two ways to pass measured data back to user space: either per-event details, or via a BPF map. BPF maps can implement arrays, associative arrays, and histograms, and are suited for passing summary statistics.

3. 如何利用 bcc 进行编程:
// TODO 挺有意思的东西
http://www.brendangregg.com/ebpf.html#frontends

## BPF Performance Tools
也许分析其中的目录就是非常足够的吧，知道一共存在什么东西!
https://search.safaribooksonline.com/book/operating-systems-and-server-administration/linux/9780136588870

## bpftrace
建立在 bcc 上方的易用工具，手动编译真简单呀!
https://github.com/iovisor/bpftrace

bcc 也提供了各种工具，包括 trace argdist 以及 funccount 等等

到底可以实现什么 ?


## bcc


#### tutorial

Before using bcc, you should start with the Linux basics. One reference is the [Linux Performance Analysis in 60s](http://techblog.netflix.com/2015/11/linux-performance-analysis-in-60s.html) post, which covers these commands:

1. uptime
1. dmesg | tail
1. vmstat 1
1. mpstat -P ALL 1
1. pidstat 1
1. iostat -xz 1
1. sar -n DEV 1
1. sar -n TCP,ETCP 1
1. top

> TODO 传统的分析工具都是从 /proc 中间读取数据的

分析各种 domain specific 工具:
1. runqlat
2. profile 暂时不知道如何使用

然后分析三个 generic 的工具:
argdist
trace :
funccount

> TODO


#### tutorial bcc python developers
kprobe uprobe USDT SDT perf trace 等等，但是其实过于强调其中的

https://github.com/iovisor/bcc/blob/master/docs/tutorial_bcc_python_developer.md
      |
     \|/
记录一下问题:
1. 为什么 bpf 不能直接访问，而是需要这种封装函数
```c
    data.pid = bpf_get_current_pid_tgid();
    data.ts = bpf_ktime_get_ns();
    bpf_get_current_comm(&data.comm, sizeof(data.comm));
```
2. 这些 macro 的含义是什么，一共存在多少种这种东西。
```c
BPF_HASH(last);
BPF_PERF_OUTPUT(events); // 感觉非常神奇，似乎是 perf 提供特地的通道
```

3. uprobe 的例子非常玄乎，一个测试完全合乎例子，一个没有测试

## 基本的代码分析
verifier.c                             1051           1739           7396
btf.c                                   656            532           3429
syscall.c                               529            177           2765
core.c                                  281            276           1672
hashtab.c                               289            164           1327
cgroup.c                                208            225           1131
arraymap.c                              173             99            815
offload.c                               129             22            561
devmap.c                                140            103            541
inode.c                                 127             32            522
bpf_lru_list.c                          131             61            502
stackmap.c                               97             53            482
bpf_struct_ops.c                        105             58            470
local_storage.c                         113             29            458
lpm_trie.c                              109            215            422
cpumap.c                                119            149            404
helpers.c                                86             21            394
trampoline.c                             56             41            329
reuseport_array.c                        60             69            231
disasm.c                                 22              7            229
xskmap.c                                 43              4            218
queue_stack_maps.c                       56             31            202
tnum.c                                   28             25            143
dispatcher.c                             28             17            113
percpu_freelist.c                        17              7             94
map_in_map.c                             19             13             88
bpf_lru_list.h                           13              7             62
sysfs_btf.c                               9              5             32
Makefile                                  1              1             30
disasm.h                                  7              4             29
percpu_freelist.h                         4              5             23
map_in_map.h                              4              3             14
bpf_struct_ops_types.h                    1              2              6


## 资源和总结
1. 为什么 package filter 和 IO, monitoring , tracing 的功能呢在一起？
2. 为什么需要在内核中间插入代码，而且必须使用llvm 的 ?

https://lwn.net/Articles/740157/  : 分析 bpf 的内核工作模式(感觉主要是 checker)

internal sandbox virtual machine

https://netflixtechblog.com/linux-performance-analysis-in-60-000-milliseconds-accc10403c55  : perf 常规知识补充


持有不少资源 : https://www.iovisor.org/technology/ebpf
https://github.com/iovisor/bcc : BCC - Tools for BPF-based Linux IO analysis, networking, monitoring
https://github.com/brendangregg : BCC 的作者还有其他的神奇的项目

https://docs.cilium.io/en/stable/bpf/ : cilium 提供的 bpf 文档


#### reference guide
C 语言总是需要 lua, cpp 或者 python 的辅助，将 eBPF 程序插入到其中其中，划分为 BPF C 和 bcc python

1. 插入的位置
2. 输出数据
3. 从内核中间获取数据
4. MAPS 为什么发要定义这么多的类型

python : 各种 attach 函数， 分析 map 以及输出

## paper
- https://www.usenix.org/conference/nsdi21/presentation/ghigoff : 使用 eBPF 加速 memcahced ，无需修改内核。

## todo
- [ ] https://github.com/dippynark/bpf-sockmap
- [ ]  bpftrace -e 'BEGIN { printf("Hello, World!\n"); }' BEGIN 是什么意思，是否存在类似的工具
- [ ] bpftrace -e 'tracepoint:syscalls:sys_enter_nanosleep { printf("%s is sleeping.\n", comm); }'
    - [ ]  参数 comm 是什么指定的 ?
    - [ ]  能不能直接 `sys_enter_nanosleep` 不要前面的前缀
    - [ ]  `sudo bpftrace -e 'tracepoint:syscalls:sys_enter_nanosleep { printf("%s is sleeping ==> %d.\n", comm, __syscall_nr); }'` 居然不知道参数 `__syscall_nr`，但是
- [ ] https://css.csail.mit.edu/jitk/ : BPF 的文章
- https://www.graplsecurity.com/post/kernel-pwning-with-ebpf-a-love-story
  - 从安全工程师的角度分析 eBPF
- https://blog.px.dev/ebpf-openssl-tracing/ : 讲解使用  eBPF 调试的案例
- [ ] cilium 有开源的一个工具: https://icloudnative.io/posts/tetragon/
- [ ] 使用 go 实现一个基于 bpf 的 tracer
- [ ] https://buoyant.io/blog/ebpf-sidecars-and-the-future-of-the-service-mesh

## verifier
https://twitter.com/shunghsiyu/status/1553592644219318272?s=20&t=Tn6g5qhu7pNHCr0-DnKVng

## security
- https://www.zerodayinitiative.com/blog/2020/4/8/cve-2020-8835-linux-kernel-privilege-escalation-via-improper-ebpf-program-verification
- https://www.graplsecurity.com/post/kernel-pwning-with-ebpf-a-love-story
- https://blog.hexrabbit.io/2021/11/03/CVE-2021-34866-writeup/

## libbpf
这里提供了一些有用的链接的:
https://github.com/iovisor/bcc/tree/master/libbpf-tools

应该是从这里开始:
https://github.com/libbpf/libbpf

## paper
- 使用 eBPF 来加速存储栈
- https://www.usenix.org/conference/osdi22/presentation/zhong

[^1]: [Outlook : future of eBPF](https://docs.google.com/presentation/d/1AcB4x7JCWET0ysDr0gsX-EIdQSTyBtmi6OAW7bE0jm0/preview?slide=id.g704abb5039_2_106)
[^2]: [A thorough introduction to eBPF](https://lwn.net/Articles/740157/)
[^3]: [An introduction to the BPF Compiler Collection](https://lwn.net/Articles/742082/)
[^12]: [Linux Extended BPF (eBPF) Tracing Tools](http://www.brendangregg.com/ebpf.html)
[^6]: [Some advanced BCC topics](https://lwn.net/Articles/747640/)
