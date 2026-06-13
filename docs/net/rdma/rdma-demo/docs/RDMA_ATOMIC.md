# RDMA Atomic 操作详解

## 什么是 Atomic 操作？

RDMA Atomic（原子）操作允许在**远端内存**上执行原子操作，特点：
- **单端操作**：远端 CPU 不参与
- **硬件原子性**：网卡保证操作的原子性
- **返回旧值**：操作完成后返回操作前的值

## 两种 Atomic 操作

### 1. Fetch-and-Add (F&A)

**语义**：原子地读取远端内存值，并增加指定数值

```c
// 伪代码
old_val = *remote_addr;     // 读取旧值
*remote_addr += add_val;    // 增加
return old_val;             // 返回旧值
```

**使用场景**：
- 分布式计数器
- 统计计数
- 序列号生成
- 负载均衡（轮询）

**代码示例**：
```c
struct ibv_send_wr wr = {
    .opcode = IBV_WR_ATOMIC_FETCH_AND_ADD,
    .wr.atomic = {
        .remote_addr = remote_addr,   // 远端内存地址
        .rkey = rkey,                  // 远端内存密钥
        .compare_add = add_val,        // 要增加的值
    },
};
```

### 2. Compare-and-Swap (CAS)

**语义**：如果远端内存值等于期望值，则替换为新值

```c
// 伪代码
old_val = *remote_addr;
if (old_val == expected) {
    *remote_addr = new_val;
}
return old_val;
```

**使用场景**：
- 分布式锁
- 无锁数据结构
- 乐观并发控制
- 状态机更新

**代码示例**：
```c
struct ibv_send_wr wr = {
    .opcode = IBV_WR_ATOMIC_CMP_AND_SWP,
    .wr.atomic = {
        .remote_addr = remote_addr,
        .rkey = rkey,
        .compare_add = expected,    // 期望值
        .swap = new_val,            // 新值
    },
};
```

## 内存注册要求

Atomic 操作需要特殊的内存权限：

```c
mr = ibv_reg_mr(pd, buf, size,
    IBV_ACCESS_LOCAL_WRITE |
    IBV_ACCESS_REMOTE_READ |
    IBV_ACCESS_REMOTE_WRITE |
    IBV_ACCESS_REMOTE_ATOMIC);  // 必须包含此权限！
```

## 完整调用流程

### 1. 服务端（提供内存）

```c
// 1. 注册可原子访问的内存
struct ibv_mr *mr = ibv_reg_mr(pd, buf, size,
    IBV_ACCESS_REMOTE_ATOMIC);

// 2. 初始化数据
struct atomic_data {
    uint64_t counter;
    uint64_t lock;
} *data = buf;
data->counter = 100;
data->lock = 0;  // 未锁定

// 3. 将 mr->rkey 和 buf 地址发送给客户端
```

### 2. 客户端（执行原子操作）

```c
// 1. 获取服务端的 rkey 和地址
uint32_t rkey = remote_info.rkey;
uint64_t remote_addr = remote_info.addr;

// 2. 创建本地缓冲区接收旧值
uint64_t *result_buf = malloc(8);
struct ibv_mr *mr = ibv_reg_mr(pd, result_buf, 8, ...);

// 3. 执行 Fetch-and-Add
struct ibv_sge sge = {
    .addr = (uint64_t)result_buf,
    .length = 8,
    .lkey = mr->lkey,
};
struct ibv_send_wr wr = {
    .opcode = IBV_WR_ATOMIC_FETCH_AND_ADD,
    .sg_list = &sge,
    .num_sge = 1,
    .wr.atomic = {
        .remote_addr = remote_addr,
        .rkey = rkey,
        .compare_add = 10,  // 增加 10
    },
};
ibv_post_send(qp, &wr, &bad_wr);

// 4. 等待完成，result_buf 中包含旧值
poll_cq(cq);
printf("Old value: %lu\n", *result_buf);
```

## 实际应用示例

### 分布式锁（使用 CAS）

```c
// 尝试获取锁
uint64_t expected = 0;  // 期望未锁定
uint64_t new_val = 1;   // 设置为锁定
uint64_t old_val;

atomic_cmp_swap(remote_lock_addr, rkey, expected, new_val, &old_val);

if (old_val == 0) {
    // 获取锁成功！
    // 访问受保护的数据...
    
    // 释放锁
    atomic_cmp_swap(remote_lock_addr, rkey, 1, 0, &old_val);
} else {
    // 锁已被占用
}
```

### 分布式计数器（使用 F&A）

```c
// 增加计数器
uint64_t old_val;
atomic_fetch_add(remote_counter_addr, rkey, 1, &old_val);
printf("Counter incremented from %lu to %lu\n", old_val, old_val + 1);
```

## Atomic vs WRITE/READ

| 特性 | WRITE/READ | Atomic |
|-----|-----------|--------|
| **操作类型** | 数据传输 | 原子计算 |
| **返回值** | 无 | 返回旧值 |
| **硬件保证** | 数据到达 | 原子性 |
| **数据大小** | 任意（KB-MB） | 固定（8字节）|
| **使用场景** | 大数据传输 | 同步、计数 |

## 性能考虑

1. **数据大小**：Atomic 操作通常只支持 8 字节（64位）
2. **延迟**：Atomic 比 WRITE/READ 稍高，因为需要等待结果
3. **吞吐量**：Atomic 吞吐量较低，适合控制路径而非数据路径
4. **顺序性**：同一地址的 Atomic 操作是顺序的

## 注意事项

1. **对齐要求**：原子操作地址必须 8 字节对齐
2. **权限要求**：内存注册时必须包含 `IBV_ACCESS_REMOTE_ATOMIC`
3. **设备支持**：并非所有 RDMA 设备都支持 Atomic 操作
4. **网络要求**：某些网络（如 RoCEv1）可能不支持 Atomic

## 检查设备支持

```bash
# 查看设备是否支持 Atomic 操作
ibv_devinfo -d mlx5_0 | grep atomic

# 或者查看内核日志
dmesg | grep -i atomic
```

## 总结

RDMA Atomic 操作提供了**硬件级别的原子性保证**，使得在分布式系统中实现同步机制变得简单高效。虽然数据大小受限（8字节），但在实现分布式锁、计数器、无锁数据结构等场景下非常有用。

关键优势：
- **无需远端 CPU 参与**：真正的单端操作
- **硬件原子性**：无需软件锁
- **低延迟**：比传统网络锁快几个数量级

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
