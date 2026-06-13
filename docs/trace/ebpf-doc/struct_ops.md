## STRUCT_OPS

## [ ] LPC 2024
http://oldvger.kernel.org/bpfconf2024_material/struct_ops-lsfmmbpf-2024.pdf

这是什么高级话题？
- [x] Fuse-BPF 看来还不在主线
  - https://lwn.net/Articles/937433/
- [ ] BPF qdisc
  - http://oldvger.kernel.org/bpfconf2024_material/BPF-Qdisc.pdf

## LPC 2023
https://lpc.events/event/17/contributions/1607/attachments/1164/2407/lpc-struct_ops.pdf

> The BPF struct_ops is a kernel-side feature in Linux which allows user-defined methods to be called by subsystems.
> For example, it is now possible to define a congestion control algorithm in BPF and then proceed
> to register it with the TCP subsystem in order to effectively regulate traffic

## 这里的代码尝试下
- https://docs.ebpf.io/linux/program-type/BPF_PROG_TYPE_STRUCT_OPS/

## 原始 patch

- https://lwn.net/Articles/809092/

> The first use case included in this series is to implement
> TCP congestion control algorithm in BPF  (i.e. implement
> struct tcp_congestion_ops in BPF).

> There has been attempt to move the TCP CC to the user space
> (e.g. CCP in TCP).

> The recent BPF
> advancements (in particular BTF-aware verifier, BPF trampoline,
> BPF CO-RE...) made implementing kernel struct ops (e.g. tcp cc)
> possible in BPF.

> The idea is to allow implementing tcp_congestion_ops in bpf.
> It allows a faster turnaround for testing algorithm in the
> production while leveraging the existing (and continue growing) BPF
> feature/framework instead of building one specifically for
> userspace TCP CC.

信息量过大
1. BTF-aware verifier
2. BPF trampoline
3. 测试一下这里的内容 net/ipv4/bpf_tcp_ca.c
4. 和这里的 net/ipv4/tcp_dctcp.c

## 基本操作

sudo bpftool struct_ops show

bpftool struct_ops register bpf_cubic.o

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
