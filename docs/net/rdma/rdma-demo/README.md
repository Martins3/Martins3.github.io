# RDMA Programming Demo

基于 rdma-core 的 RDMA 编程示例，包含 polling 和 interrupt 两种 completion 模式，以及配套的内核观察脚本。

## 项目结构

```
.
├── rdma_pingpong.c          # SEND/RECV 示例 (双端操作)
├── rdma_pingpong_poll.out   # 轮询模式程序
├── rdma_pingpong_event.out  # 中断模式程序
├── rdma_write_read.c        # WRITE/READ 示例 (单端操作)
├── rdma_write_read.out      # RDMA 真读写操作演示
├── rdma_atomic.c            # Atomic 操作示例 (Fetch-and-Add/CAS)
├── rdma_atomic.out          # 原子操作演示
├── run.sh                   # 一键测试脚本
├── Makefile                 # 编译配置
├── shell.nix                # Nix 开发环境
├── docs/                    # 文档目录
│   └── RDMA操作类型.md       # WRITE/READ 详解
├── demo_trace.sh            # ftrace 追踪演示
├── trace_cq.sh              # ftrace CQ 追踪
├── trace_irq.sh             # 中断分析脚本
├── trace_latency.sh         # 延迟对比测试
├── trace_bpf.bt             # bpftrace 延迟追踪
└── README.md                # 本文档
```

## 快速开始

### 1. 进入开发环境

```bash
nix-shell
# 或直接使用系统环境（需安装 rdma-core-devel）
```

### 2. 编译

```bash
make
```

生成两个程序：
- `rdma_pingpong_poll.out` - 轮询模式（Polling）
- `rdma_pingpong_event.out` - 中断模式（Interrupt/Event）

### 3. 运行测试

#### SEND/RECV 测试 (双端操作)

**一键测试（推荐）:**
```bash
sudo ./run.sh [iterations] [server_ip]
```

**手动测试:**

服务端（对端）:
```bash
./rdma_pingpong_poll.out -s -d mlx5_0
```

客户端（本端）:
```bash
./rdma_pingpong_poll.out -d rocep0s6 10.0.3.2
```

#### WRITE/READ 测试 (真正的 RDMA 单端操作)

**服务端（对端）:**
```bash
./rdma_write_read.out -s -d mlx5_0
```

**客户端（本端）:**
```bash
./rdma_write_read.out -d rocep0s6 10.0.3.2
```

**WRITE/READ 演示内容：**
1. 客户端通过 **RDMA READ** 直接读取服务端内存
2. 客户端通过 **RDMA WRITE** 直接写入服务端内存
3. 再次 **RDMA READ** 验证写入的数据

**关键区别：** 数据传输过程中服务端 CPU **完全不参与**！

#### Atomic 操作测试 (原子操作)

**服务端：**
```bash
./rdma_atomic.out -s -d mlx5_0
```

**客户端：**
```bash
./rdma_atomic.out -d rocep0s6 10.0.3.2
```

**Atomic 演示内容：**
1. **Fetch-and-Add**：原子读取并增加远端计数器
2. **Compare-and-Swap**：原子比较并交换，实现分布式锁
3. 多次原子操作演示

**关键区别：** 原子操作由硬件保证，服务端 CPU **完全不参与**！

## 操作类型对比

| 操作类型 | 代码文件 | 对端CPU参与 | 适用场景 |
|---------|---------|-----------|---------|
| **SEND/RECV** | `rdma_pingpong.c` | **需要**两端参与 | 控制信息交换、连接建立 |
| **WRITE** | `rdma_write_read.c` | **不需要** | 大量数据写入远端 |
| **READ** | `rdma_write_read.c` | **不需要** | 大量数据从远端读取 |
| **Atomic F&A** | `rdma_atomic.c` | **不需要** | 原子计数器、统计 |
| **Atomic CAS** | `rdma_atomic.c` | **不需要** | 分布式锁、同步 |

### SEND/RECV vs WRITE/READ

```c
// SEND/RECV (双端操作，类似 socket)
struct ibv_send_wr wr = {
    .opcode = IBV_WR_SEND,  // 发送数据到对端接收队列
};
// 对端必须提前 post_recv，CPU 需要参与

// WRITE (单端操作，真正的 RDMA)
struct ibv_send_wr wr = {
    .opcode = IBV_WR_RDMA_WRITE,  // 直接写入对端内存
    .wr.rdma.remote_addr = remote_addr,  // 对端内存地址
    .wr.rdma.rkey = rkey,                // 对端内存密钥
};
// 对端 CPU 不参与，像访问本地内存一样

// READ (单端操作，真正的 RDMA)
struct ibv_send_wr wr = {
    .opcode = IBV_WR_RDMA_READ,  // 直接从对端内存读取
    .wr.rdma.remote_addr = remote_addr,
    .wr.rdma.rkey = rkey,
};
// 对端 CPU 不参与

// Atomic Fetch-and-Add (硬件原子操作)
struct ibv_send_wr wr = {
    .opcode = IBV_WR_ATOMIC_FETCH_AND_ADD,
    .wr.atomic.remote_addr = remote_addr,
    .wr.atomic.rkey = rkey,
    .wr.atomic.compare_add = 10,  // 增加 10
};
// 返回操作前的旧值，硬件保证原子性，对端 CPU 不参与

// Atomic Compare-and-Swap (硬件原子操作)
struct ibv_send_wr wr = {
    .opcode = IBV_WR_ATOMIC_CMP_AND_SWP,
    .wr.atomic.remote_addr = remote_addr,
    .wr.atomic.rkey = rkey,
    .wr.atomic.compare_add = expected,  // 期望值
    .wr.atomic.swap = new_val,          // 新值
};
// 如果原值等于期望值则替换，返回旧值，用于实现分布式锁
```

## 两种 Completion 模式对比

### Polling 模式（轮询）

```c
// 用户态循环查询 CQ
while (ibv_poll_cq(cq, 1, &wc) == 0) {
    // spin wait
}
```

**特点:**
- 延迟最低（无上下文切换）
- CPU 占用高（100% 一个核）
- 适合低延迟敏感型应用

### Interrupt 模式（中断）

```c
// 通过 completion channel 等待中断
ibv_get_cq_event(comp_channel, &ev_cq, &ev_ctx);
ibv_ack_cq_events(cq, 1);
ibv_req_notify_cq(cq, 0);
```

**特点:**
- CPU 占用低（阻塞等待中断）
- 延迟较高（中断开销）
- 适合吞吐型应用

## 内核观察脚本

### 1. ftrace CQ 追踪

```bash
sudo ./trace_cq.sh 10
```

```txt
# tracer: function
#
# entries-in-buffer/entries-written: 10/10   #P:8
#
#                                _-----=> irqs-off/BH-disabled
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| / _-=> migrate-disable
#                              |||| /     delay
#           TASK-PID     CPU#  |||||  TIMESTAMP  FUNCTION
#              | |         |   |||||     |         |
 rdma_pingpong_p-20895   [000] ..... 15890.844800: rdma_user_mmap_io <-mlx5_ib_mmap
 rdma_pingpong_p-20895   [000] ..... 15890.846450: rdma_user_mmap_io <-mlx5_ib_mmap
          <idle>-0       [006] ..s1. 15890.847030: mlx5_ib_poll_cq <-__ib_process_cq
          <idle>-0       [006] ..s1. 15890.852570: mlx5_ib_poll_cq <-__ib_process_cq
 rdma_pingpong_e-20911   [007] ..... 15895.371787: rdma_user_mmap_io <-mlx5_ib_mmap
 rdma_pingpong_e-20911   [007] ..... 15895.373425: rdma_user_mmap_io <-mlx5_ib_mmap
          <idle>-0       [006] ..s1. 15895.373972: mlx5_ib_poll_cq <-__ib_process_cq
          <idle>-0       [006] ..s1. 15895.378158: ib_uverbs_comp_handler <-mlx5_ib_cq_comp
          <idle>-0       [006] ..s1. 15895.380244: ib_uverbs_comp_handler <-mlx5_ib_cq_comp
          <idle>-0       [006] ..s1. 15895.381383: mlx5_ib_poll_cq <-__ib_process_cq
```



### 3. bpftrace 延迟追踪

```bash
sudo bpftrace trace_bpf.bt
```

实时追踪各种延迟分布：
- User polling latency
- Interrupt handling latency
- mlx5 driver poll latency
- NAPI poll latency
- NET_RX softirq latency

### 4. 自动化对比测试

```bash
sudo ./trace_latency.sh 1000 rocep0s6 10.0.3.2
```

自动运行两种模式的测试并对比结果。

## 内核源码相关

内核源码路径: `/home/martins3/data/kernel/linux-build`

关键文件：

| 路径 | 说明 |
|------|------|
| `drivers/infiniband/core/uverbs_cmd.c` | ib_uverbs_poll_cq |
| `drivers/infiniband/hw/mlx5/cq.c` | mlx5_ib_poll_cq |
| `drivers/infiniband/core/device.c` | RDMA 设备管理 |

## 命令行选项

### run.sh 脚本

```
./run.sh [iterations] [server_ip] [local_dev] [remote_dev]

Example:
  ./run.sh 100 10.0.3.2 rocep0s6 mlx5_0
```

### rdma_pingpong 程序

```
./rdma_pingpong -s -d <device> -i <iterations>

Options:
  -s, --server         Run as server
  -d, --device=NAME    IB device name (rocep0s6, mlx5_0, etc)
  -i, --iterations=N   Number of iterations (default: 1000)
  -e, --event          Force event mode (compile-time default can be overridden)
  -h, --help           Show help
```

## 预期输出

### Polling 模式

```
=== Results ===
Total time:     0.123 ms
Avg latency:    6.150 us
Total wait/CQ:  0.115 ms
Messages:       10 sent, 10 received
Throughput:     162601.63 msg/sec
```

### Interrupt 模式

```
=== Results ===
Total time:     20.760 ms
Avg latency:    1038.000 us
Total wait/CQ:  20.729 ms
Messages:       10 sent, 10 received
Throughput:     963.39 msg/sec
```

## 调试技巧

### 查看 RDMA 设备

```bash
ibstat          # 设备状态
ibv_devinfo     # 详细设备信息
rdma link show  # RDMA 链接状态
```

### 检查内核模块

```bash
lsmod | grep mlx      # mlx5 驱动
lsmod | grep ib       # InfiniBand 核心
```

### 查看 debugfs

```bash
ls /sys/kernel/debug/tracing/
cat /sys/kernel/debug/rdma/mlx5/
```

## 常见问题 (FAQ)

### Q: 为什么 RDMA 程序还需要 TCP？

**A**: TCP **只在连接建立阶段**使用，用于交换 RDMA 连接所需的元信息（lid, qpn, psn 等）。一旦 RDMA QP（Queue Pair）建立完成，TCP 连接就会关闭，后续**所有数据传输都是纯 RDMA**，不经过 TCP/IP 协议栈。

```c
// 1. TCP 连接建立
sockfd = tcp_connect(server_ip, 18515);

// 2. 交换 RDMA 连接信息
exchange_conn_info(sockfd, &local_info, &remote_info);

// 3. TCP 关闭！
close(sockfd);

// 4. 建立 RDMA QP
modify_qp_to_init();  // INIT
modify_qp_to_rtr();   // Ready To Receive
modify_qp_to_rts();   // Ready To Send

// 5. 纯 RDMA 数据传输！
ibv_post_send();  // 无 TCP 参与！
ibv_poll_cq();    // 无 TCP 参与！
```

详细解释见：[docs/WHY_TCP.md](docs/WHY_TCP.md)

### Q: SEND/RECV 和 WRITE/READ 有什么区别？

**A**: 
- **SEND/RECV**: 双端操作，类似 socket，两端 CPU 都需要参与
- **WRITE/READ**: 单端操作，真正的 RDMA，远端 CPU 不参与

详细解释见：[docs/RDMA操作类型.md](docs/RDMA操作类型.md)

### Q: 如何验证数据真的通过 RDMA 传输，没有经过 TCP？

**A**: 使用 `strace` 观察系统调用：

```bash
# RDMA 传输阶段不会有 send/write 系统调用
strace -e trace=send,write,read,recv ./rdma_pingpong ...

# 只能看到初始化的 ioctl，没有数据传输的 send/write
ioctl(fd, RDMA_VERBS_IOCTL, ...)
```

详细方法见：[ZERO_COPY_ANALYSIS.md](ZERO_COPY_ANALYSIS.md)

## 参考文档

### 项目内文档

- [RDMA操作类型详解](docs/RDMA操作类型.md) - 知乎 Mellanox 文章，详解 SEND/RECV/WRITE/READ 操作差异
- [RDMA Atomic 操作详解](docs/RDMA_ATOMIC.md) - Fetch-and-Add 和 Compare-and-Swap 操作详解
- [为什么需要TCP](docs/WHY_TCP.md) - 解释为什么 RDMA 程序需要 TCP 进行连接建立

### 外部资源

- [RDMAmojo - RDMA Programming](https://www.rdmamojo.com/)
- [rdma-core documentation](https://github.com/linux-rdma/rdma-core)
- InfiniBand Architecture Specification Vol. 1

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
