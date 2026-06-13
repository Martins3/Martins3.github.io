# 基本使用方法
1. 需要 enable default.nix ，不然无法编译，有类似这种错误:
```txt
tc.bpf.c:4:10: fatal error: 'bpf/bpf_endian.h' file not found
    4 | #include <dian.h>c.bpf.o] Error 1
      |          ^~~~~~~~~~~~~~~~~~
1 error generated.
make: *** [Makefile:80: .output/tbpf/bpf_en
```
2. Makefile 中定义两种程序:
```txt
APPS = tc minimal mapwriter cg ds task_iter bootstrap test_map_in_map
BARE_BPF_OBJ = single iter bpf_cubic
```
BARE_BPF_OBJ 需要对应类似 single.bpf.c


3. bpf_cubic.c 和 bpf_tracing_net.h 直接从 kernel source tree 中拷贝过来的
```sh
sudo bpftool struct_ops register .output/bpf_cubic.bpf.o
# 检查系统中现在的 controller
sysctl net.ipv4.tcp_congestion_control
# 修改系统中的 controller 为 bpf_cubic
sysctl -w net.ipv4.tcp_congestion_control=bpf_cubic
```
对应内核的支持为 net/ipv4/bpf_tcp_ca.c

## 研究下 CORE 机制

如何在 A 环境中构建，在 B 环境中测试

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
