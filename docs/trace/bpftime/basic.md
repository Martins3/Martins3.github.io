# bpftime

用户态 eBPF runtime：用二进制改写（frida-gum）在用户态跑 eBPF 程序，
支持 uprobe/uretprobe、syscall hook（zpoline）、XDP（AF_XDP/DPDK，实验性）、GPU kernel hook。
声称用户态 uprobe 比内核 uprobe 快约 10 倍。兼容现有 clang + libbpf 工具链，无需修改。

- https://github.com/eunomia-bpf/bpftime
- https://github.com/eunomia-bpf/eunomia-bpf
- LPC 23 slides: https://lpc.events/event/17/contributions/1639/attachments/1280/2585/userspace-ebpf-bpftime-lpc.pdf
- OSDI '25 paper: https://www.usenix.org/conference/osdi25/presentation/zheng-yusheng
- 介绍文章: https://mp.weixin.qq.com/s/cMjFKYKQHqoXKXC9ibhFRQ

两种模式：

- 纯用户态：不依赖内核 eBPF，可跑在老内核 / 无 root 环境，用用户态 verifier（PREVAIL）
- 与内核 eBPF 协作：从内核加载 eBPF，和内核侧程序共享 map

局限：部分内核 helper/kfunc 不可用，不能访问 task_struct 等内核数据结构。

继续观察吧

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
