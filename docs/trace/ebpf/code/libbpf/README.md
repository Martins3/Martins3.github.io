# ebpf demo

参考:
- https://nakryiko.com/posts/libbpf-bootstrap/
- https://github.com/libbpf/libbpf-bootstrap

类似的例子在 kernel 仓库的 samples/bpf/ 下也有很多

## NixOS 中编译 BPF

Nix 的 Clang wrapper 面向 host target，会为 `-target bpf` 注入不适用的
`--gcc-toolchain` 参数。`default.nix` 因此通过 `CLANG` 为 BPF 编译导出
`llvmPackages.clang-unwrapped`，用户态程序仍使用 `clangStdenv` 的 host
compiler。

当前内核 BTF 可能包含带 tag 的匿名 struct/union 成员。BPF 编译参数使用
`-fms-extensions -Wno-microsoft-anon-tag` 保留这些成员的语义，并启用
`-Wall -Werror` 检查项目源码。

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
BARE_BPF_OBJ = single iter_uds iter_slub bpf_cubic
```
BARE_BPF_OBJ 需要对应类似 single.bpf.c


3. bpf_cubic.bpf.c 和 bpf_tracing_net.h 直接从 kernel source tree 中拷贝过来的
```sh
sudo bpftool struct_ops register .output/bpf_cubic.bpf.o
# 检查系统中现在的 controller
sysctl net.ipv4.tcp_congestion_control
# 修改系统中的 controller 为 bpf_cubic
sysctl -w net.ipv4.tcp_congestion_control=bpf_cubic
```
对应内核的支持为 net/ipv4/bpf_tcp_ca.c

## test case 使用说明

`make all` 构建两类目标：`APPS` 同时生成 BPF object、skeleton 和用户态
`.out`；`BARE_BPF_OBJ` 只生成 `.output/*.bpf.o`，需要脚本或 `bpftool`
完成加载和触发。

| 目标                  | Hook / 数据通路                                                 | 测试目的                                                                                                                               | 运行和观察方式                                                                                                                                     |
| --------------------- | --------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------- |
| `tc`                  | `SEC("tc")`，loopback ingress                                   | 验证 libbpf TC hook 的创建、挂载和卸载，以及对 Ethernet/IPv4 报文的边界检查和解析                                                      | `sudo ./tc.out` 后产生 loopback IPv4 流量，在 `trace_pipe` 查看总长度和 TTL                                                                        |
| `minimal`             | `sys_enter_write` tracepoint；`fmod_ret/update_socket_protocol` | 验证 skeleton 的 open/load/attach 流程、通过 BSS 向 BPF 传入 PID，以及 fmod_ret 修改内核函数返回值                                     | `sudo ./minimal.out`；tracepoint 只处理自身写系统调用，输出在 `trace_pipe`。fmod_ret 部分要求内核暴露对应 BTF 函数并允许修改返回值                 |
| `mapwriter`           | classic socket filter + `BPF_MAP_TYPE_ARRAY`                    | 验证用 `SO_ATTACH_BPF` 把程序挂到 UNIX datagram socket，并比较用户态 `bpf_map_update_elem()`、BPF helper 和 `mmap()` 三种 map 访问方式 | `sudo ./mapwriter.out`；程序周期性写 socket 触发 BPF，终端显示 mmap 后的 map 值，`trace_pipe` 显示触发信息                                         |
| `cg`                  | `cgroup/getsockopt`、`cgroup/setsockopt`                        | 验证从 object 中选择 cgroup 程序、附加到指定 cgroup，并 pin `bpf_link`；观察该 cgroup 内进程的 socket option 调用                      | 使用 `cg.sh` 或 `sudo ./cg.out <cgroup-path> .output/cg.bpf.o [program-id]`，在 `trace_pipe` 查看进程名。pin 路径为 `/sys/fs/bpf/test_cgrp2_sock2` |
| `ds`                  | `sys_enter_write` tracepoint + array map                        | 用于尝试 tracepoint 中的 map lookup 和原子更新，并保留直接构造 `union bpf_attr` 创建 map 的实验入口                                    | `sudo ./ds.out` 后查看 `trace_pipe`。当前 BPF 以 PID 作为仅有 10 项的 array key，常规 PID 会越界，因此原子更新分支通常不会执行，属于未完成实验     |
| `task_iter`           | `iter/task` + `bpf_seq_write()`                                 | 验证 task iterator、per-CPU array 作为临时缓冲区、`bpf_get_task_stack()` 获取内核栈，以及 iterator 向用户态传输二进制定长记录          | `sudo ./task_iter.out`，终端列出 PID、进程名、任务状态和内核栈长度                                                                                 |
| `bootstrap`           | 进程 exec/exit tracepoint + hash map + ring buffer              | 演示完整的 libbpf-bootstrap 工作流：关联进程启动时间、通过 CO-RE 读取 task 信息，并用 ring buffer 上报进程生命周期                     | `sudo ./bootstrap.out`；可用 `-d <毫秒>` 过滤短进程，终端显示 EXEC/EXIT、PID/PPID、文件名、退出码和存活时间                                        |
| `test_map_in_map`     | `ksyscall/connect` + map-in-map                                 | 验证 `ARRAY_OF_MAPS`、`HASH_OF_MAPS` 的 inner map ID，并比较动态 inner-map lookup 与 verifier 内联 lookup 的结果                       | `sudo ./test_map_in_map.out`；程序构造特殊 IPv6 地址触发 connect kprobe，预期 `Array of Array`、`Hash of Array`、`Hash of Hash` 均显示 `Pass`      |
| `single`              | `sys_enter_write` tracepoint                                    | 最小的 bare BPF object，用于练习不依赖 skeleton 的编译和 `bpftool` 手工加载                                                            | `single.sh` 当前只执行 `bpftool prog load`，没有把 tracepoint 程序 attach 到 hook，因此仅能验证加载，不能在 `trace_pipe` 看到触发输出              |
| `iter_uds`            | `iter/unix` + seq output                                        | 验证 UNIX socket iterator 和 `BPF_SEQ_PRINTF()`，输出 socket UID、文件系统路径及 `@` 开头的 abstract socket 名称                       | `sudo ./iter.sh`，脚本构建、临时 pin、读取 iterator，并在退出时删除 pin                                                                            |
| `iter_slub`           | `iter/kmem_cache` + array/hash map                              | 验证 kmem cache iterator 的 seq 输出，并把 slab 名称和对象大小保存到 map；`task_struct` 还会写入索引 hash                              | `iter.sh` 已定义 `slub()`，但默认执行 `uds()`；切换脚本末尾调用后运行，输出应与 `/proc/slabinfo` 中的 cache 名称和对象大小对应                     |
| `bpf_cubic`           | `struct_ops` TCP congestion control                             | 验证用 BPF struct_ops 实现并注册 CUBIC 拥塞控制，包括 kfunc、kconfig extern 和内核 TCP 回调                                            | `sudo bpftool struct_ops register .output/bpf_cubic.bpf.o`，再用 `sysctl net.ipv4.tcp_congestion_control` 检查或切换到 `bpf_cubic`                 |
| `map_communication`  | raw tracepoint + hash/array/ring buffer                         | 在同一次请求中比较 hash 的 `bpf()` 更新、mmap array 的双向共享，以及 ring buffer 的 BPF 到用户态事件                                    | `sudo ./map_communication.out`；程序会验证 hash 不能 mmap、共享数组得到 `40 + 2 = 42`，并打印 ring buffer 记录                                   |
| `arena`              | `BPF_MAP_TYPE_ARENA` + raw tracepoint                           | 验证 arena 全局区和 BPF 通过 `bpf_arena_alloc_pages()` 按需分配的页面都能被用户态直接读写                                               | `sudo ./arena.out`；第一次显示 BPF 分配页，第二次由用户态直接改共享值并验证 BPF 结果。要求 Linux 6.9+ 和支持 arena 的近期 Clang/libbpf             |
| `ext4_trace`         | `fentry/filemap_fault` + hash map                               | 按文件名统计 `filemap_fault()` 次数，并周期性写入共享内存文件                                                                           | `sudo ./ext4_trace.out`；按 Ctrl-C 退出后读取 `/dev/shm/ext4_trace_buffer`                                                                        |
| `readahead`          | `kprobe/page_cache_ra_unbounded` + hash map                     | 统计每种 `nr_to_read` 页数对应的 readahead 调用次数                                                                                     | `sudo ./readahead.out`；按 Ctrl-C 后在终端查看精确分布                                                                                            |

### mapwriter : SEC("socket") 何时调用

这是 BPF_PROG_TYPE_SOCKET_FILTER，仅加载不会运行。
必须通过 SO_ATTACH_BPF 附加到具体 socket。

mapwriter.c 的流程是：

socketpair(A, B)
    |
SO_ATTACH_BPF 到 B
    |
write(A)
    |
数据准备进入 B 的接收队列
    |
调用 my_socket_prog(skb)

它是接收过滤器：

- socket 创建时不调用。
- attach 时不调用。
- 附加 socket 收到 skb 时调用。
- attached socket 主动发送数据通常不会直接触发。
- 返回 0 表示丢弃数据。
- 返回 skb->len 表示保留完整数据。
- 返回较小正数表示截断后保留。

当前 my_socket_prog() 返回 0，所以每次 write(sockets[0]) 都会触发 BPF、更新 map、打印日志，然后把发往 sockets[1] 的 datagram 丢弃。
实测已经看到 BPF triggered。

## BPF 与用户态共享数据：hash、mmap array、ring buffer、arena

`map_communication` 把前三种机制放在一个请求中，数据路径如下：

```text
userspace -- bpf(BPF_MAP_UPDATE_ELEM) --> hash map -- lookup --> BPF
userspace <----------- shared mmap -----------> mmapable array <-> BPF
userspace <------------- poll/read ---------------- ring buffer <-- BPF
```

- Hash map 适合稀疏、动态 key。用户态每次 lookup/update 都要经过 `bpf()`；
  demo 还会尝试 mmap hash map，并确认内核返回不支持。
- `BPF_F_MMAPABLE` array 在创建时预留全部元素对应的不可分页内核内存。完成一次
  `mmap()` 后，用户态读写元素不再需要逐次调用 `bpf()`，但固定容量的空闲元素也占资源。
- Ring buffer 是变长事件流。BPF 用 reserve/submit 生产，用户态用 libbpf
  消费；它不提供用户态向 BPF 写入数据的反向通道，等待事件通常仍需要 poll 类系统调用。
- Arena 从 Linux 6.9 开始提供共享虚拟地址空间。创建 map 时只确定最多页数，页面由
  用户态缺页或 BPF 的 `bpf_arena_alloc_pages()` 按需分配，因此更适合指针、链表、树等
  大小动态的数据结构。它不是普通 key/value map，也没有并发一致性协议；双方并发访问
  时仍需自行使用原子操作、锁或明确的所有权/序列号。

这里的 array mmap 行为对应本地内核源码 `kernel/bpf/arraymap.c` 中
`array_map_mmap()`；arena 的 mmap、缺页、分配和释放实现位于
`kernel/bpf/arena.c` 的 `arena_map_mmap()`、`arena_vm_fault()`、
`bpf_arena_alloc_pages()` 和 `bpf_arena_free_pages()`。上游用法可参考
`tools/testing/selftests/bpf` 中 `arena_list`、`arena_htab` 和 `arena_atomics`
相关程序。

只构建并运行这组实验：

```sh
nix-shell --run 'make map_communication arena'
sudo ./map_communication.out
sudo ./arena.out
```

延伸资料：

- [Reading from an eBPF map without paying for kernel-call](https://stackoverflow.com/questions/78582877/reading-from-an-ebpf-map-without-paying-for-kernel-call)
- [bpf-playground arena 示例](https://github.com/jfernandez/bpf-playground/blob/main/arena.bpf.c)
- [Linux `bpf: Introduce bpf_arena` commit](https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=317460317a02a1af512697e6e964298dedd8a163)


## ebpf 的数据结构
<!-- 2816316d-6e35-48f8-85d8-eaae1fd63963 -->

### 通用 Map 容器

```txt
 类型                          语义                             典型用途
━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 PERCPU_ARRAY/HASH             每 CPU 独立副本                  无锁计数、直方图
────────────────────────────  ───────────────────────────────  ─────────────────────────────────────────────────
 LRU_HASH/LRU_PERCPU_HASH      容量满后自动淘汰                 flow cache、热点跟踪
────────────────────────────  ───────────────────────────────  ─────────────────────────────────────────────────
 QUEUE/STACK                   FIFO/LIFO，支持 push/pop/peek    BPF 内部任务队列、对象池
────────────────────────────  ───────────────────────────────  ─────────────────────────────────────────────────
 LPM_TRIE                      最长前缀匹配                     IPv4/IPv6 路由、ACL
────────────────────────────  ───────────────────────────────  ─────────────────────────────────────────────────
 BLOOM_FILTER                  概率集合，可能误报               在 hash lookup 前快速过滤
────────────────────────────  ───────────────────────────────  ─────────────────────────────────────────────────
 ARRAY_OF_MAPS/HASH_OF_MAPS    Map 嵌套                         配置快照、租户隔离；现有 test_map_in_map 已覆盖
────────────────────────────  ───────────────────────────────  ─────────────────────────────────────────────────
 PROG_ARRAY                    保存 BPF 程序并执行 tail call    拆分大型 BPF 程序
────────────────────────────  ───────────────────────────────  ─────────────────────────────────────────────────
 STACK_TRACE                   保存栈地址序列                   profiling、off-CPU 分析
```

完整列表位于本地 include/uapi/linux/bpf.h 的 enum bpf_map_type。

生命周期绑定存储

- TASK_STORAGE：数据绑定到 task_struct。
- SK_STORAGE：绑定 socket。
- INODE_STORAGE：绑定 inode。
- CGRP_STORAGE：绑定 cgroup。

对象销毁时数据自动释放，很适合保存安全状态、连接统计和跟踪上下文，不需要自己用 PID/指针维护 hash key。

### BPF 内部动态数据结构

- bpf_obj_new() / bpf_obj_drop()：分配、释放带 BTF 类型的内核对象。
- bpf_list_head：侵入式双向链表。
- bpf_rb_root：红黑树。
- bpf_refcount：引用计数。
- kptr：在 map 或对象中保存受 verifier 跟踪的内核指针。
- bpf_percpu_obj_new()：动态分配 per-CPU 对象。

Verifier 会检查所有权转移、锁、引用释放以及对象是否同时存在于多个容器中。本地可参考：

- ./tools/testing/selftests/bpf/progs/linked_list.c 中 list_push_pop()
- ./tools/testing/selftests/bpf/progs/rbtree.c 中 __add_three()
- ./tools/testing/selftests/bpf/progs/bpf_qdisc_fq.c 中链表、红黑树、引用计数的组合应用

Arena 则允许自己在共享地址空间中实现链表、哈希表、树和 allocator；它比 bpf_obj_new() 更自由，但内存布局和并发协议也更需要自行负责。

### 专用索引结构

- DEVMAP/CPUMAP：XDP 重定向到网卡或 CPU。
- XSKMAP：重定向到 AF_XDP socket。
- SOCKMAP/SOCKHASH：socket 查找、SK_MSG/SK_SKB 重定向。
- REUSEPORT_SOCKARRAY：选择 SO_REUSEPORT socket。
- STRUCT_OPS：保存并注册一组内核回调，不是普通数据 map。

另外，原始描述需要补充：BPF_MAP_TYPE_USER_RINGBUF 支持用户态生产、BPF 通过 bpf_user_ringbuf_drain() 消费。普通 RINGBUF 和
USER_RINGBUF 组合后可以形成双向消息通道。

## TODO

后续最值得增加的实验顺序是：

1. USER_RINGBUF，补齐真正的 user → BPF 事件通道。
2. QUEUE + STACK + BLOOM_FILTER + LPM_TRIE，展示专用 map 语义。
3. bpf_obj_new + linked list + rbtree，展示 verifier 管理的动态对象和所有权。
4. PERCPU + LRU_HASH + TASK_STORAGE，展示并发、淘汰和生命周期绑定。

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
