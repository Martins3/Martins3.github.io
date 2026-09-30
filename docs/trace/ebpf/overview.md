# ebpf 基础
<!-- c4af72ac-3687-4392-a2c2-a95300b95015 -->

## 需要搞的事情
- 到底 ebpf 是如何应用于网络包过滤的

- https://docs.cilium.io/en/latest/bpf/

## [x] kprobe 和 fentry 的差别是什么?

https://fuweid.com/post/2022-bpf-kprobe-fentry-poke/

kfunc 和 fentry 是一个东西，但是 bpftrace 喜欢使用 kfunc

## raw_tp 和 tp 区别
- https://mozillazg.com/2022/05/ebpf-libbpf-raw-tracepoint-common-questions-en.html

eBPF 的升级内容:[^2]
1. 64bit 的寄存器
2. 寄存器数量从 2 个到 10 个
3. `BPF_CALL` : Plus, a new `BPF_CALL` instruction made it possible to call in-kernel functions cheaply.
4. The ease of mapping eBPF to native instructions lends itself to just-in-time compilation, yielding improved performance.

eBPF 的作用不仅仅限于 packet filter 的功能，其实可以动态的 debug 内核:
eBPF is also useful for debugging the kernel and carrying out performance analysis;
programs can be attached to tracepoints, kprobes, and perf events.
Because eBPF programs can access kernel data structures, developers can write and test new debugging code without having to recompile the kernel. The implications are obvious for busy engineers debugging issues on live, running systems. It's even possible to use eBPF to debug user-space programs by using Userland Statically Defined Tracepoints.


eBPF verifier :
1. 对于 CFG 进行 DFS，保证其中不会出现递归，死循环，以及不可执行的代码
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
1. 修改用于 eBPF 程序和 kernel 或者 user space 沟通的 eBPF map
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


## 工具
- https://github.com/cloudflare/ebpf_exporter : 观测技术
- https://nakryiko.com/posts/retsnoop-intro/ : 保持关注

- https://blog.px.dev/ebpf-openssl-tracing/ : 讲解使用  eBPF 调试的案例
- [ ] cilium 有开源的一个工具: https://icloudnative.io/posts/tetragon/
  - https://github.com/cilium/tetragon
- [ ] 使用 go 实现一个基于 bpf 的 tracer
- [ ] https://buoyant.io/blog/ebpf-sidecars-and-the-future-of-the-service-mesh
- [ ] 编译内核的时候，似乎打包了大量的 BTF 文件，这些文件的内容都在哪里?
  - [ ] 似乎而且依赖 pathoe 的一个程序，但是具体是什么来着
- https://lwn.net/Articles/740157/  : 分析 bpf 的内核工作模式(感觉主要是 checker)
- https://netflixtechblog.com/linux-performance-analysis-in-60-000-milliseconds-accc10403c55  : perf 常规知识补充

- security
	- https://www.zerodayinitiative.com/blog/2020/4/8/cve-2020-8835-linux-kernel-privilege-escalation-via-improper-ebpf-program-verification
	- https://www.graplsecurity.com/post/kernel-pwning-with-ebpf-a-love-story
	- https://blog.hexrabbit.io/2021/11/03/CVE-2021-34866-writeup/

[^2]: [A thorough introduction to eBPF](https://lwn.net/Articles/740157/)
[^3]: [An introduction to the BPF Compiler Collection](https://lwn.net/Articles/742082/)
[^12]: [Linux Extended BPF (eBPF) Tracing Tools](http://www.brendangregg.com/ebpf.html)
[^6]: [Some advanced BCC topics](https://lwn.net/Articles/747640/)

- ebpf 来控制 oom : https://lwn.net/Articles/941614/

## TODO : cilium 提供的文档

- https://news.ycombinator.com/item?id=35989911 : Unit Testing eBPF Programs
- https://css.csail.mit.edu/jitk/ : BPF 的文章
- 持有不少资源 : https://www.iovisor.org/technology/ebpf
- https://docs.cilium.io/en/stable/bpf/ : cilium 提供的 bpf 文档
- https://www.ebpf.top/categories/network/
- https://github.com/netblue30/firejail : 没搞懂怎么使用，似乎 ebpf 关系也不大
- bpf internals : Brendan Gregg :  https://www.youtube.com/watch?v=_5Z2AU7QTH4
  - 使用 strace 分析了 bpf 的基本工作原理
- bpf Rethinking the Linux kernel : https://docs.google.com/presentation/d/1AcB4x7JCWET0ysDr0gsX-EIdQSTyBtmi6OAW7bE0jm0/preview?slide=id.g35f391192_00
- Documentation/bpf/
- [ebpf Documentary](https://news.ycombinator.com/item?id=39663135)
- https://www.grant.pizza/blog/libbpf-beginners-part-one/
  - https://www.grant.pizza/blog/tracing-go-functions-with-ebpf-part-1/
  - https://github.com/aquasecurity/tracee
  - https://github.com/aquasecurity/libbpfgo
  - 偶尔遇到的，到时候再看吧
- https://www.ebpf.top/en/post/top_and_tricks_for_bpf_libbpf/

## 高级话题: tcx mprog
- https://lore.kernel.org/all/20230719140858.13224-1-daniel@iogearbox.net/

从这里发现了两个单词，分别叫做 ingress 和 egress
- https://docs.cilium.io/en/stable/bpf/architecture/ ，然后又搜索到这里了

至于 ingress 和 egress 也不是这里专门有的吧

- https://blog.cloudflare.com/cloudflare-architecture-and-how-bpf-eats-the-world/

- 学术讨论
	- https://www.usenix.org/conference/osdi22/presentation/zhong : 使用 eBPF 来加速存储栈
	- https://www.usenix.org/conference/nsdi21/presentation/ghigoff : 使用 eBPF 加速 memcahced ，无需修改内核。
	- https://bpfman.io/v0.4.1/developer-guide/xdp-overview/
	- https://github.com/gojue/ecapture : Capture SSL/TLS text content without a CA certificate using eBPF. This tool is compatible with Linux/Android x86_64/Aarch64.

https://github.com/bpfman/bpfman

- [Dive into BPF: a list of reading material](https://qmonnet.github.io/whirl-offload/2016/09/01/dive-into-bpf/)

CONFIG_BPF_STREAM_PARSER is set to y 做什么的?

- [eBPF 介绍](https://news.ycombinator.com/item?id=22953730)

## 在 tools/include/uapi/linux/bpf.h

enum bpf_prog_type 和 enum bpf_attach_type 的关系是什么？

## ebpf
https://blog.gmem.cc/ebpf : 可以作为查漏补缺的参考
https://habr.com/ru/post/446312/
https://github.com/pythops/oryx 看看这个工具
https://www.zhihu.com/question/530525662/answer/3439777706 : 原来还有这个关系啊
https://github.com/acmel/dwarves : 原来 dwarves 的源码在这里
https://github.com/hengyoush/kyanos 似乎也没有什么的
https://github.com/lizrice/learning-ebpf

## bysyscall
https://github.com/oracle-samples/bysyscall

只能支持这些 syscall :
```txt
getpid()
gettid()
getuid()
getgid()
getrusage()
```

实现方法我理解是，先让内核在 fork 的时候，自动将
tid gid 写入到共享内存，之后，替换动态库中的 syscall ，然后去
直接去读共享空间就可以了。

## 核心文档

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
