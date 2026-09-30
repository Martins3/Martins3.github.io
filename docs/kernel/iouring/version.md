# io_uring 版本迭代

本文按 Linux 正式版本记录 io_uring 的主要用户可见变化。版本边界通过本地
Linux 主线仓库的 tag 对比确认，重点检查：

- `include/uapi/linux/io_uring.h` 中新增的 opcode、setup flag、feature bit 和
  register opcode；
- `io_uring/opdef.c` 中 opcode 到 `prep`、`issue`、`cleanup`、`fail` 回调的映射；
- `io_uring/` 下对应实现，以及引入 UAPI 的 commit。

不能只根据运行内核的版本号判断功能是否存在：发行版可能回移特性，也可能只
回移修复。应用应优先探测实际 ABI。

## 如何判断当前内核支持什么

### 普通 opcode

使用 liburing 的 `io_uring_get_probe_ring()` 和 `io_uring_opcode_supported()`。
它们最终通过 `IORING_REGISTER_PROBE` 取得 `struct io_uring_probe`。内核侧入口是
`io_uring/register.c` 中 `io_probe()`，支持情况来自 `io_uring/opdef.c` 的
`io_issue_defs[]`。

### feature bit

调用 `io_uring_queue_init_params()` 后检查 `struct io_uring_params.features`，例如：

- `IORING_FEAT_REG_REG_RING`：`io_uring_register()` 可以使用已注册的 ring；
- `IORING_FEAT_RECVSEND_BUNDLE`：支持 send/recv bundle；
- `IORING_FEAT_MIN_TIMEOUT`：支持最小等待时间；
- `IORING_FEAT_RW_ATTR`：读写请求支持 attribute。

内核当前公布的 feature 集合在 `io_uring/io_uring.h` 中 `IORING_FEAT_FLAGS`。

### setup flag 和 register opcode

这两类没有统一的 probe 接口。通常先尝试请求该能力，并在 `-EINVAL`、
`-EOPNOTSUPP` 等返回值下退回旧路径。不要把 liburing 头文件中存在某个宏，
误认为运行内核一定已经实现它；liburing 版本与内核版本是两个独立维度。

## 版本总览

| 内核 | 主要新增 ABI | 作用 |
| --- | --- | --- |
| 6.0 | `IORING_OP_SEND_ZC`、`IORING_RECV_MULTISHOT`、`IORING_SETUP_SINGLE_ISSUER` | 网络零拷贝发送、单个 SQE 持续接收、单提交者优化 |
| 6.1 | `IORING_OP_SENDMSG_ZC`、`IORING_SETUP_DEFER_TASKRUN`、`IORING_URING_CMD_FIXED` | 零拷贝 `sendmsg`、显式批量运行 task_work、uring_cmd 固定缓冲区 |
| 6.2 | `IORING_SEND_ZC_REPORT_USAGE` | 报告零拷贝请求是否退化为复制；同时强化 multishot 完成批处理 |
| 6.3 | `IORING_FEAT_REG_REG_RING`、`IORING_MSG_RING_FLAGS_PASS` | 用注册 ring 调用 `io_uring_register()`，向目标 ring 传递 CQE flags |
| 6.4 | `IORING_TIMEOUT_MULTISHOT`、`IOU_PBUF_RING_MMAP` | 周期性 timeout、由内核分配并 mmap 的 provided-buffer ring |
| 6.5 | `IORING_SETUP_NO_MMAP`、`IORING_SETUP_REGISTERED_FD_ONLY` | 用户提供 ring/SQE 内存；创建时直接注册 ring 而不安装普通 fd |
| 6.6 | `IORING_SETUP_NO_SQARRAY`、`IORING_ASYNC_CANCEL_OP`、socket uring_cmd | 去掉 SQ array 间接层、按 opcode 取消、异步查询 socket 队列长度 |
| 6.7 | multishot read、waitid、futex、socket option、可取消 uring_cmd | 扩展到非 socket 流式读取、进程等待、同步原语和完整 direct socket 配置 |
| 6.8 | `IORING_OP_FIXED_FD_INSTALL`、`IORING_REGISTER_PBUF_STATUS` | 把 direct descriptor 安装为普通 fd；查询 buffer group 状态 |
| 6.9 | `IORING_OP_FTRUNCATE`、`IORING_REGISTER_NAPI` | 异步截断文件；为网络请求配置 NAPI busy polling |
| 6.10 | send/recv bundle、`IORING_FEAT_RECVSEND_BUNDLE` | 一次请求连续消费多个 provided buffers |
| 6.11 | `IORING_OP_BIND`、`IORING_OP_LISTEN` | socket 创建、配置、bind、listen 可以留在 io_uring/direct-fd 路径 |
| 6.12 | incremental provided buffers、registered clock、clone buffers | 大 buffer 可分段消费；指定等待时钟；ring 间复用已注册 buffer |
| 6.13 | hybrid IOPOLL、ring resize、registered wait region | 降低纯 busy poll 成本；运行中调整 ring；复用等待参数内存 |
| 6.14 | `IORING_FEAT_RW_ATTR` | 读写 attribute，首个使用者是 block PI 元数据 |
| 6.15 | zero-copy receive、`IORING_OP_EPOLL_WAIT`、fixed readv/writev | 网络接收零拷贝、异步 epoll wait、固定 iovec |
| 6.16 | `IORING_OP_PIPE` | 异步创建 pipe/direct descriptor；ZCRX 支持 DMA-BUF area |
| 6.17 | vectorized send、socket TX timestamp | 单次 send 使用 `io_vec`，异步取得软/硬件发送时间戳 |
| 6.18 | `IORING_SETUP_CQE_MIXED`、multishot uring_cmd、query | 同一 CQ 混用 16/32 字节 CQE，并加入通用能力查询入口 |
| 6.19 | `IORING_SETUP_SQE_MIXED`、128-byte opcode | 同一 SQ 混用 64/128 字节 SQE，加入 `NOP128`、`URING_CMD128` |
| 7.0 | `IORING_SETUP_SQ_REWIND`、`IORING_REGISTER_BPF_FILTER` | 每次从 SQE 0 开始提交；为 ring 注册 BPF 过滤程序 |
| 7.1 | `IORING_TIMEOUT_IMMEDIATE_ARG` | timeout 值可直接放在 SQE 中，避免再读取用户态 `timespec` |

后续版本的表项是 ABI 速查，以下分节详细展开本文原先关注的 6.0 到 6.7。

## Linux 6.0

### 零拷贝发送

`IORING_OP_SEND_ZC` 为 TCP/UDP、IPv4/IPv6 引入零拷贝发送。一次发送通常涉及
两类完成：发送请求本身的 CQE，以及表示用户缓冲区可以安全复用的 notification
CQE。因此，看到发送 CQE 并不等于可以立刻改写缓冲区。

源码入口：

- `include/uapi/linux/io_uring.h` 中 `IORING_OP_SEND_ZC` 和
  `IORING_CQE_F_NOTIF`；
- `io_uring/opdef.c` 中 `[IORING_OP_SEND_ZC]`；
- v6.0 `io_uring/net.c` 中 `io_sendzc_prep()`、`io_sendzc()` 和
  `io_sendzc_cleanup()`；这些函数从 6.1 起改为 `io_send_zc_*` 命名。

相关 commit：[`b48c312be05e`](https://git.kernel.org/linus/b48c312be05e)。

### multishot recv

`IORING_RECV_MULTISHOT` 让一个 recv SQE 连续产生多个 CQE。中间 CQE 带
`IORING_CQE_F_MORE`；不再带该标志的 CQE 终止请求。因为多个完成可能在应用
处理旧 CQE 前连续到达，数据请求必须配合 provided buffers 使用。

源码入口是 `io_uring/net.c` 中 `io_recvmsg_prep_multishot()`、
`io_recvmsg_multishot()` 和 `io_recv()`。

相关 commit：[`b3fdea6ecb55`](https://git.kernel.org/linus/b3fdea6ecb55)。

### single issuer

`IORING_SETUP_SINGLE_ISSUER` 声明只有一个 task 会提交请求，使内核能省掉部分
提交路径同步。通常这个 task 是创建 ring 的 task；若同时以
`IORING_SETUP_R_DISABLED` 创建，则执行 `IORING_REGISTER_ENABLE_RINGS` 的 task
成为 issuer。它约束的是 issuer，不是说 CQE 只能由一个线程消费。v6.0 的绑定
和检查逻辑位于 `io_uring/io_uring.c` 中 `io_uring_create()`、
`io_register_enable_rings()` 及提交路径。

相关 commit：[`97bbdc06a444`](https://git.kernel.org/linus/97bbdc06a444)。

参考：[Linux 6.0](https://kernelnewbies.org/Linux_6.0)。

## Linux 6.1

### defer task_work

`IORING_SETUP_DEFER_TASKRUN` 不再让 io_uring task_work 在每次无关的内核/用户态
切换时零散运行，而是主要在应用进入取完成事件的路径时批量处理。它要求
`IORING_SETUP_SINGLE_ISSUER`，常与 `IORING_SETUP_COOP_TASKRUN` 配合。

源码可从 `io_uring/io_uring.c` 中 `io_submit_sqes()`、`io_uring_enter()` 的
task_work 处理，以及 `io_uring/wait.c` 的等待路径观察。

相关 commit：[`c0e0d6ba25f1`](https://git.kernel.org/linus/c0e0d6ba25f1)。

### sendmsg zero-copy 和 uring_cmd fixed buffer

- `IORING_OP_SENDMSG_ZC` 把 scatter/gather 和控制消息场景纳入零拷贝发送；
- `IORING_URING_CMD_FIXED` 允许 `IORING_OP_URING_CMD` 使用已注册缓冲区，驱动
  通过 `io_uring/uring_cmd.c` 中 `io_uring_cmd_import_fixed()` 导入它。

参考：[Linux 6.1](https://kernelnewbies.org/Linux_6.1)。

## Linux 6.2

这一版主要是完成路径和并发语义的收敛，而不是增加大量 opcode：

- multishot 的辅助 CQE 支持延迟和批量提交，减少 CQ 锁操作；
- single-issuer ring 的完成工作更多地固定在所属 task context 中；
- `IORING_SEND_ZC_REPORT_USAGE` 允许 notification CQE 报告数据是否实际走了
  零拷贝，`IORING_NOTIF_USAGE_ZC_COPIED` 表示至少有一部分发生复制。

源码可从该版本 `io_uring/io_uring.c` 中 `io_fill_cqe_aux()`、
`io_req_task_complete()` 以及 `io_uring/net.c` 的 zero-copy notification 路径观察。

对应的两个 io_uring 合并节点：

- [`54e60e505d61`](https://git.kernel.org/linus/54e60e505d61)
- [`96f7e448b9f4`](https://git.kernel.org/linus/96f7e448b9f4)

参考：[Linux 6.2](https://kernelnewbies.org/Linux_6.2)。

## Linux 6.3

### registered ring fd

`IORING_FEAT_REG_REG_RING` 表示 `io_uring_register()` 的 ring 参数可以是 ring
内部注册表中的索引。调用时在 register opcode 上附加
`IORING_REGISTER_USE_REGISTERED_RING`。这样可绕过普通 fd table 查找，并能用于
`IORING_SETUP_REGISTERED_FD_ONLY` 后续建立的纯注册 ring。

v6.3 源码入口是 `io_uring/io_uring.c` 中
`SYSCALL_DEFINE4(io_uring_register)` 对
`IORING_REGISTER_USE_REGISTERED_RING` 的解析，以及
`__io_uring_register()` 的具体 register-op 分派。

### MSG_RING 传递 flags

设置 `IORING_MSG_RING_FLAGS_PASS` 后，`IORING_OP_MSG_RING` 可把指定 flags 传给
目标 CQE。v6.3 源码入口是 `io_uring/msg_ring.c` 中 `io_msg_ring_prep()` 和
`io_msg_ring_data()`。

参考：[Linux 6.3](https://kernelnewbies.org/Linux_6.3)。

## Linux 6.4

### multishot timeout

`IORING_TIMEOUT_MULTISHOT` 让一个 timeout SQE 周期性地产生 CQE，适合用作事件
循环的周期唤醒。中间完成同样使用 `IORING_CQE_F_MORE`；取消或错误会结束它。
源码入口是 `io_uring/timeout.c` 中 `io_timeout_prep()`、
`io_timeout_complete()` 和 `io_timeout()`。

### 内核分配 provided-buffer ring

注册 `IORING_REGISTER_PBUF_RING` 时设置 `IOU_PBUF_RING_MMAP`，内核分配 buffer
ring 内存，用户再以 `IORING_OFF_PBUF_RING | (bgid << IORING_OFF_PBUF_SHIFT)`
作为 offset 调用 `mmap()`。这和 6.5 的 `IORING_SETUP_NO_MMAP` 方向相反：前者
是内核为 provided-buffer ring 分配内存，后者是用户为主 SQ/CQ ring 提供内存。

源码入口是 `io_uring/kbuf.c` 中 `io_register_pbuf_ring()`。

参考：[Linux 6.4](https://kernelnewbies.org/Linux_6.4)。

## Linux 6.5

### 用户分配主 ring 内存

`IORING_SETUP_NO_MMAP` 允许应用通过 `sq_off.user_addr` 和 `cq_off.user_addr`
提供 SQ ring、CQ ring 和 SQE 内存，适合把多个 ring 紧凑放入 huge page。内核
仍会 pin 并校验这些页；该标志并不是“不共享内存”。

### 创建时只保留 registered fd

`IORING_SETUP_REGISTERED_FD_ONLY` 让 `io_uring_setup()` 返回注册索引，不把 ring
安装进进程 fd table。它要求 `IORING_SETUP_NO_MMAP`，后续 enter/register 调用
要分别使用 registered-ring 标志。

两项设置的参数检查和资源建立都在 v6.5 `io_uring/io_uring.c` 的
`io_uring_create()`；registered-ring 调用的解析在同一文件的
`SYSCALL_DEFINE4(io_uring_register)`。

参考：[Linux 6.5](https://kernelnewbies.org/Linux_6.5)。

## Linux 6.6

### 去掉 SQ array 间接层

普通提交通过 SQ array 把逻辑队列位置映射到 SQE index。
`IORING_SETUP_NO_SQARRAY` 让二者相同，减少一次内存读取。源码可从
`io_uring/io_uring.c` 中 `io_get_sqe()`、`io_submit_sqes()` 和 ring 内存布局
初始化观察。

### 按 opcode 取消

`IORING_ASYNC_CANCEL_OP` 使异步/同步 cancel 可按请求 opcode 匹配，而不只按
`user_data` 或 fd。`struct io_uring_sync_cancel_reg.opcode` 承载匹配值；核心匹配
在 `io_uring/cancel.c` 中 `io_cancel_req_match()`。

### socket uring_cmd 第一阶段

socket 文件的 `IORING_OP_URING_CMD` 开始支持 `SOCKET_URING_OP_SIOCINQ` 和
`SOCKET_URING_OP_SIOCOUTQ`，分别查询接收队列可读字节和发送队列未发送字节。
v6.6 的入口和 socket 分派都在 `io_uring/uring_cmd.c` 中
`io_uring_cmd()`、`io_uring_cmd_sock()`；较新的源码已把网络部分拆到
`io_uring/cmd_net.c`。

参考：[Linux 6.6](https://kernelnewbies.org/Linux_6.6)。

## Linux 6.7

### multishot read

`IORING_OP_READ_MULTISHOT` 类似普通 read，但一个 SQE 可以反复读取并产生 CQE：

- 只支持可 poll 的文件，例如 pipe；socket 通常仍应使用 multishot
  recv/recvmsg；
- 必须设置 buffer selection，并使用 provided buffers；
- 成功的中间 CQE 带 `IORING_CQE_F_MORE`；错误、取消或 CQ overflow 会终止
  multishot 请求。

源码入口是该版本 `io_uring/rw.c` 中 `io_read_mshot_prep()` 和
`io_read_mshot()`。

相关 commit：[`fc68fcda0491`](https://git.kernel.org/linus/fc68fcda0491)，分析见
[Multishot reads](https://lwn.net/Articles/944291/)。

### 异步 waitid

`IORING_OP_WAITID` 把等待子进程状态变化纳入 ring。若提交时没有事件，内核把
请求挂到 child wait queue；回调触发后通过 task_work 重试并发布 CQE，而不是
占住 io-wq worker 阻塞等待。该版本不支持 `rusage`，也不是 multishot。

源码入口是 `io_uring/waitid.c` 中 `io_waitid_prep()`、`io_waitid_wait()`、
`io_waitid_cb()` 和 `io_waitid()`。

相关 commit：[`f31ecf671ddc`](https://git.kernel.org/linus/f31ecf671ddc)。

### futex wait/wake/waitv

新增 `IORING_OP_FUTEX_WAIT`、`IORING_OP_FUTEX_WAKE` 和
`IORING_OP_FUTEX_WAITV`。wait 请求通过 futex wake callback 转成 io_uring
task_work 后完成，不需要阻塞 worker。该接口基于 futex2 语义，6.7 不支持 PI、
requeue 或 opcode 自带 timeout；超时应使用 io_uring linked timeout。

源码入口是 `io_uring/futex.c` 中 `io_futex_prep()`、`io_futex_wait()`、
`io_futex_wake()` 和 `io_futexv_wait()`。

相关 commit：[`194bb58c6090`](https://git.kernel.org/linus/194bb58c6090)。

### socket getsockopt/setsockopt

通过 socket fd 上的 `IORING_OP_URING_CMD` 提交
`SOCKET_URING_OP_GETSOCKOPT` 或 `SOCKET_URING_OP_SETSOCKOPT`，可直接配置
direct descriptor，不必先创建普通 fd。这里的 `optlen` 是 SQE 中的值，不是
像 `getsockopt(2)` 那样的用户指针；getsockopt 的实际长度由 `cqe->res` 返回。
用户缓冲区必须一直有效到 CQE 到达。

v6.7 源码入口是 `io_uring/uring_cmd.c` 中 `io_uring_cmd_sock()` 及其 get/set
sockopt 分支；较新的源码已把这部分拆到 `io_uring/cmd_net.c`。

相关 commit：[`a5d2f99aff6b`](https://git.kernel.org/linus/a5d2f99aff6b)。

### 可取消 uring_cmd

驱动可调用 `io_uring_cmd_mark_cancelable()` 把命令加入 ring 的可取消集合；收到
取消后，io_uring 再次调用驱动的 `file_operations.uring_cmd()`，并传入
`IO_URING_F_CANCEL`。框架负责请求归属和查找，驱动仍必须处理正常完成与取消
之间的竞争。

v6.7 源码入口是 `io_uring/uring_cmd.c` 中
`io_uring_cmd_mark_cancelable()`、`io_uring_cmd_done()`，以及
`io_uring/io_uring.c` 中 `io_uring_try_cancel_uring_cmd()`；较新的源码已将取消
查找也移入 `io_uring/uring_cmd.c`。

这也说明“传统 Linux AIO 完全无法取消”并不准确：Linux AIO 有 `io_cancel(2)`，
但具体文件系统/驱动经常无法取消已经发出的 I/O。io_uring 提供了按
`user_data`、fd、opcode 等更丰富的匹配方式，不过正在执行的操作最终能否安全
中止，仍取决于对应 opcode 或驱动实现。

参考：[Linux 6.7 io_uring improvements](https://kernelnewbies.org/LinuxChanges#Linux_6.7.io_uring_improvements)。

## 后续版本的源码入口

版本总览中 6.8 之后的能力可从以下位置继续展开：

- fixed-fd install、ftruncate、pipe：`io_uring/openclose.c`、
  `io_uring/truncate.c` 中相应 opcode 的 prep/issue 函数；
- NAPI busy poll：`io_uring/napi.c` 中 `io_register_napi()`、
  `io_napi_busy_loop()`；
- bind/listen、send/recv bundle、ZCRX、socket timestamp：
  `io_uring/net.c`、`io_uring/zcrx.c`、`io_uring/cmd_net.c`；
- incremental/clone provided buffers：`io_uring/kbuf.c` 和
  `io_uring/rsrc.c`；
- ring resize、registered memory/wait region：`io_uring/register.c`、
  `io_uring/memmap.c` 和 `io_uring/io_uring.c`；
- mixed SQE/CQE 和 opcode 分派：`include/uapi/linux/io_uring.h`、
  `io_uring/opdef.c` 和 `io_uring/io_uring.c`。

## 如何从源码复核版本差异

```bash
git diff v6.6..v6.7 -- include/uapi/linux/io_uring.h
git log --oneline v6.6..v6.7 -- io_uring include/uapi/linux/io_uring.h
git show v6.7:io_uring/opdef.c
```

`enum io_uring_op` 只追加、不重排，因此比较 UAPI 头文件能快速找到新增 opcode；
但纯性能优化、竞态修复和内部调度变化不会体现在 UAPI diff 中，还需要查看
`io_uring` merge commit 的说明和对应实现。

## 待补实验

- 写一个 `SOCKET_URING_OP_GETSOCKOPT` 最小测试，对比普通 `getsockopt(2)`，验证
  `optlen` 的传参差异以及 `cqe->res` 返回的实际长度；
- 对 `IORING_OP_READ_MULTISHOT` 使用 pipe 和 provided-buffer ring，观察
  `IORING_CQE_F_MORE`、buffer ID 以及 cancel 后的最后一个 CQE；
- 对 `IORING_OP_FUTEX_WAIT` 链接 timeout，分别复现 wake 赢、cancel 赢和 timeout
  赢三种竞争结果。

## 资料

- [LinuxVersions](https://kernelnewbies.org/LinuxVersions)
- [io_uring and networking in 2023](https://github.com/axboe/liburing/wiki/io_uring-and-networking-in-2023)
- [The rapid growth of io_uring](https://lwn.net/Articles/923369/)
- [On the way to io_uring networking](https://kernel-recipes.org/en/2023/schedule/on-the-way-to-io_uring-networking/)
- [io_uring reference](https://nick-black.com/dankwiki/index.php/Io_uring)

## io uring 版本迭代 : human

1. https://github.com/axboe/liburing/wiki/io_uring-and-networking-in-2023
2. https://lwn.net/Articles/923369/

- 2023
- https://kernel-recipes.org/en/2023/schedule/on-the-way-to-io_uring-networking/

- 具体的
- https://kernelnewbies.org/LinuxChanges#Linux_6.7.io_uring_improvements
- Multishot reads
https://lwn.net/Articles/944291/
- Cancelable uring_cmd
  - 忽然意识到 aio 是无法撤销的，只有超时
- Initial support for {s,g}etsockopt commands
 - 写一个 getsocket 的测试吧


参考 : https://kernelnewbies.org/LinuxVersions


- 6.0
- https://kernelnewbies.org/Linux_6.0
- https://kernelnewbies.org/Linux_6.1
- 6.2
	- https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=54e60e505d6144a22c787b5be1fdce996a27be1b
	- https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=96f7e448b9f4546ffd0356ffceb2b9586777f316
- https://kernelnewbies.org/Linux_6.3
- https://kernelnewbies.org/Linux_6.4
- https://kernelnewbies.org/Linux_6.5
- https://kernelnewbies.org/Linux_6.6
- https://nick-black.com/dankwiki/index.php/Io_uring

- https://github.com/axboe/liburing/wiki/io_uring-and-networking-in-2023
- https://kernel.dk/io_uring-whatsnew.pdf

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
