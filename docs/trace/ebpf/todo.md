https://nakryiko.com/posts/libbpf-bootstrap/

https://github.com/libbpf/libbpf-bootstrap


libbpf 是 kernel 中一部分，按道理，可以直接利用这个:
```txt
make -C tools/lib/bpf/
```
1. 但是索引无法正常构建:
2. 让这个函数直接索引到 : bpf_get_current_pid_tgid

## 问题
- [ ] 不知道为什么，minimal 在 host 中 sudo cat /sys/kernel/debug/tracing/trace_pipe 没有输出
- [ ] guest 中 tc.bpf.c 不可以正常运行

对于 socket 还是不太了解，什么时候调用这个 hook
```txt
SEC("socket")
int my_socket_prog(struct __sk_buff *skb)
```

## libbpf 到底提供了那些功能

## 这个也去看看吧
https://www.ebpf.top/en/post/top_and_tricks_for_bpf_libbpf/

## 从 bpftool feature 的实现说起

- [ ] 似乎不同的 eBPF program types 会有不同的 helper

- [ ] 不同的类型，是不是只是 attach 的位置不同
```txt
Scanning eBPF program types...
eBPF program_type socket_filter is available
eBPF program_type kprobe is available
eBPF program_type sched_cls is available
eBPF program_type sched_act is available
eBPF program_type tracepoint is available
eBPF program_type xdp is available
eBPF program_type perf_event is available
eBPF program_type cgroup_skb is available
eBPF program_type cgroup_sock is available
eBPF program_type lwt_in is available
eBPF program_type lwt_out is available
eBPF program_type lwt_xmit is available
eBPF program_type sock_ops is available
eBPF program_type sk_skb is available
eBPF program_type cgroup_device is available
eBPF program_type sk_msg is available
eBPF program_type raw_tracepoint is available
eBPF program_type cgroup_sock_addr is available
eBPF program_type lwt_seg6local is available
eBPF program_type lirc_mode2 is NOT available
eBPF program_type sk_reuseport is available
eBPF program_type flow_dissector is available
eBPF program_type cgroup_sysctl is available
eBPF program_type raw_tracepoint_writable is available
eBPF program_type cgroup_sockopt is available
eBPF program_type tracing is available
eBPF program_type struct_ops is available
eBPF program_type ext is available
eBPF program_type lsm is NOT available
eBPF program_type sk_lookup is available
eBPF program_type syscall is available
```

## 尝试下这个: https://github.com/libbpf/libbpf-rs


## 这几个数据结构尝试下

> BPF 程序已经有几种与用户空间通信的方式，包括环形缓冲区、哈希映射和数组映射。然而，每种方法都存在一些问题。
> 1. 环形缓冲区（ring buffer）可以用于将性能测量或跟踪事件发送到用户空间进程，但不能从用户空间接收数据。
> 2. 哈希映射（hash map）可以用于此目的，但从用户空间访问它们需要进行bpf()系统调用。
> 3. 数组映射（array map）可以使用mmap()将它们映射到用户空间进程的地址空间中，但 Starovoitov 指出它们的“缺点是数组的整个内存在开始时就被预留下来”。数组映射（以及新的 arena）存储在不可分页的内核内存中，所以未使用的页面会产生明显的资源利用效率上的浪费。

当然 arena 也需要尝试下

https://github.com/jfernandez/bpf-playground/blob/main/arena.bpf.c

尝试下，

BPF_MAP_TYPE_ARRAY

https://stackoverflow.com/questions/78582877/reading-from-an-ebpf-map-without-paying-for-kernel-call

## socketpair 的使用
```c
    if(socketpair(AF_UNIX, SOCK_DGRAM, 0, sockets)) {
        printf("failed to create socket pair '%s'\n", strerror(errno));
        goto cleanup;
    }
```

## 为什么 nixos 中用的也是 bcc ，如何让 bcc 用上 libbpf-tools

```txt
🤒  profile
Sampling at 49 Hertz of all threads by user + kernel stack... Hit Ctrl-C to end.
modprobe: ERROR: could not insert 'kheaders': Operation not permitted
Unable to find kernel headers. Try rebuilding kernel with CONFIG_IKHEADERS=m (module) or installing the kernel development package for your running kernel version.
chdir(/run/booted-system/kernel-modules/lib/modules/6.9.7/build): No such file or directory
Traceback (most recent call last):
  File "/nix/store/xy6yvm8rpakicln8hj046lgzjrb0ib7z-bcc-0.30.0/share/bcc/tools/.profile-wrapped", line 309, in <module>
    b = BPF(text=bpf_text)
        ^^^^^^^^^^^^^^^^^^
  File "/nix/store/xy6yvm8rpakicln8hj046lgzjrb0ib7z-bcc-0.30.0/lib/python3.11/site-packages/bcc-0.30.0-py3.11.egg/bcc/__init__.py", line 479, in __init__
Exception: Failed to compile BPF module <text>
```

## 使用 minimal.out 的时候，启动的过程中有这个问题

```txt
libbpf: object 'minimal_bpf': failed (-95) to create BPF token from '/sys/fs/bpf', skipping optional step...
```

## 常见 api 记录

## 参考这个 ？
https://github.com/jfernandez/bpf-playground

## test case 使用说明

| task_iter | 来自于 libbpf-bootstrap | array |
| bootstrap | 来自于 libbpf-bootstrap | ring buffer |
| test_map_in_map.bpf.c | kernel source  samples/bpf | 测试嵌套数据结构的 BPF_MAP_TYPE_ARRAY_OF_MAPS，而且似乎没有使用 libbpf 提供的自动生成 skel 的|


## 似乎 nixos 自动插入了一些参数进去了

```txt
clang: warning: argument unused during compilation: '--gcc-toolchain=/nix/store/6g5fhxv0bdm7236ixdwq1izzawbc7grm-gcc-13.2.0' [-Wunused-command-line-argument]
```

2025-01-27 还是不知道如何解决:
```txt
clang: warning: argument unused during compilation: '--gcc-toolchain=/nix/store/4krab2h0hd4wvxxmscxrw21
pl77j4i7j-gcc-13.3.0' [-Wunused-command-line-argument]
```

## 测试一下这个技术
b4 am -o- https://lore.kernel.org/lkml/20230810081319.65668-5-zhouchuyi@bytedance.com/\#r > a.diff

bpf_oom_evaluate_task

写一个内核模块看看吧
```txt
+BTF_SET8_START(oom_bpf_fmodret_ids)
+BTF_ID_FLAGS(func, bpf_oom_evaluate_task)
+BTF_SET8_END(oom_bpf_fmodret_ids)
```

```txt
+BTF_SET8_START(bpf_oom_policy_kfunc_ids)
+BTF_ID_FLAGS(func, set_oom_policy_name)
+BTF_SET8_END(bpf_oom_policy_kfunc_ids)
```

### 类似的

参考 net/ipv4/bpf_tcp_ca.c 来实现一个类似，然后在 ebpf 写一个对应的测试

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
