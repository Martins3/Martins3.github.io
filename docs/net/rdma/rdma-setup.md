# RDMA 网卡配置指南

> [!NOTE]
> 参考神奇海螺的意见，有待验证

## 硬件信息

| 项目 | 值 |
|------|-----|
| 设备名 | mlx5_0 |
| 网卡接口 | enp0s6np0 |
| 型号 | Mellanox ConnectX-5 (MT4119) |
| 驱动 | mlx5_core |
| 固件 | 16.31.1014 |
| MAC | 8c:2a:8e:40:a3:7c |
| 协议 | RoCE (RDMA over Converged Ethernet) |
| IP 地址 | 10.0.3.1/30 |

## 设备状态检查

```bash
# 查看 RDMA 设备列表
rdma dev

# 查看 RDMA 链路状态
rdma link

# 详细设备信息
ibstat

# 或使用 ibv_devinfo
ibv_devinfo
```

预期输出：
```
$ rdma dev
0: mlx5_0: node_type ca protocol roce fw 16.31.1014

$ rdma link
link mlx5_0/1 state ACTIVE physical_state LINK_UP netdev enp0s6np0

$ ibstat
CA 'mlx5_0'
    CA type: MT4119
    Number of ports: 1
    Firmware version: 16.31.1014
    Port 1:
        State: Active
        Physical state: LinkUp
        Rate: 10
        Link layer: Ethernet
```

## 网络配置

### 当前配置

| 地址 | 用途 |
|------|------|
| 10.0.3.0 | 网络地址 |
| 10.0.3.1 | 本机 IP |
| 10.0.3.2 | 对端可用 IP |
| 10.0.3.3 | 广播地址 |

### 修改 IP 配置

```bash
# 使用 NetworkManager 修改
sudo nmcli connection modify "Wired connection 1" ipv4.method manual ipv4.addresses 10.0.3.1/30
sudo nmcli connection up "Wired connection 1"

# 或使用 ip 命令（临时）
sudo ip addr flush dev enp0s6np0
sudo ip addr add 10.0.3.1/30 dev enp0s6np0
sudo ip link set enp0s6np0 up
```

### 验证配置

```bash
ip addr show enp0s6np0
ip route | grep enp0s6np0
```

## RDMA 测试

### 基础诊断

```bash
# 设备信息
ibstat
ibstatus
ibv_devinfo

# 链路信息
rdma dev show mlx5_0 -dd
rdma link show mlx5_0/1

# 查看 GID (用于 RoCE)
cat /sys/class/infiniband/mlx5_0/ports/1/gids/0
```

### 带宽测试

**服务端（本机 10.0.3.1）：**
```bash
ib_write_bw -d mlx5_0
```

**客户端（对端 10.0.3.2）：**
```bash
ib_write_bw -d mlx5_0 10.0.3.1
```

**测试结果示例：**
```
---------------------------------------------------------------------------------------
                    RDMA_Write BW Test
 Dual-port       : OFF		Device         : mlx5_0
 Number of qps   : 1		Transport type : IB
 Connection type : RC		Using SRQ      : OFF
 PCIe relax order: ON		Lock-free      : OFF
 ibv_wr* API     : ON
 TX depth        : 128
 CQ Moderation   : 1
 Mtu             : 1024[B]
 Link type       : Ethernet
 GID index       : 5
 Max inline data : 0[B]
 rdma_cm QPs	 : OFF
 Data ex. method : Ethernet
---------------------------------------------------------------------------------------
 local address: LID 0000 QPN 0x0088 PSN 0x39fdf1 RKey 0x184f00 VAddr 0x00ffff8e39d000
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:10:00:03:01
 remote address: LID 0000 QPN 0x0087 PSN 0x24875a RKey 0x184e00 VAddr 0x00ffffa664d000
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:10:00:03:01
---------------------------------------------------------------------------------------
 #bytes     #iterations    BW peak[MiB/sec]    BW average[MiB/sec]   MsgRate[Mpps]
 65536      5000             5946.61            5945.70		     0.095131
```

**其他带宽测试命令：**
```bash
# 读操作带宽测试
ib_read_bw -d mlx5_0          # 服务端
ib_read_bw -d mlx5_0 10.0.3.1 # 客户端

# 发送操作带宽测试
ib_send_bw -d mlx5_0          # 服务端
ib_send_bw -d mlx5_0 10.0.3.1 # 客户端

# 原子操作带宽测试
ib_atomic_bw -d mlx5_0          # 服务端
ib_atomic_bw -d mlx5_0 10.0.3.1 # 客户端
```

### 延迟测试

**服务端（本机 10.0.3.1）：**
```bash
ib_write_lat -d mlx5_0
```

**客户端（对端 10.0.3.2）：**
```bash
ib_write_lat -d mlx5_0 10.0.3.1
```

**测试结果示例：**
```
---------------------------------------------------------------------------------------
                    RDMA_Write Latency Test
 Dual-port       : OFF		Device         : mlx5_0
 Number of qps   : 1		Transport type : IB
 Connection type : RC		Using SRQ      : OFF
 PCIe relax order: OFF		Lock-free      : OFF
 ibv_wr* API     : ON
 TX depth        : 1
 Mtu             : 1024[B]
 Link type       : Ethernet
 GID index       : 5
 Max inline data : 220[B]
 rdma_cm QPs	 : OFF
 Data ex. method : Ethernet
---------------------------------------------------------------------------------------
 local address: LID 0000 QPN 0x008a PSN 0x8ddbc2 RKey 0x180ae8 VAddr 0x00aaaac935b000
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:10:00:03:01
 remote address: LID 0000 QPN 0x0089 PSN 0xb11212 RKey 0x1807e5 VAddr 0x00aaaadb449000
 GID: 00:00:00:00:00:00:00:00:00:00:255:255:10:00:03:01
---------------------------------------------------------------------------------------
 #bytes #iterations    t_min[usec]    t_max[usec]  t_typical[usec]    t_avg[usec]
 2       1000          1.89           4.53         1.92      	       1.92
```

**其他延迟测试命令：**
```bash
ib_read_lat -d mlx5_0          # 服务端
ib_read_lat -d mlx5_0 10.0.3.1 # 客户端

ib_send_lat -d mlx5_0          # 服务端
ib_send_lat -d mlx5_0 10.0.3.1 # 客户端

ib_atomic_lat -d mlx5_0          # 服务端
ib_atomic_lat -d mlx5_0 10.0.3.1 # 客户端
```

### 高级选项

```bash
# 指定端口和队列深度
ib_write_bw -d mlx5_0 -p 18515 -q 16

# 指定消息大小
ib_write_bw -d mlx5_0 -s 1048576  # 1MB 消息

# 使用特定 GID 索引（RoCEv2）
ib_write_bw -d mlx5_0 -x 3 10.0.3.1
```

## RoCE 配置

### 查看 RoCE 模式

通过 GID 表查看支持的 RoCE 版本：

```bash
# 查看 GID 0-3 的类型（RoCE v1 或 v2）
for i in 0 1 2 3; do
    echo -n "GID ${i} type: "
    cat /sys/class/infiniband/mlx5_0/ports/1/gid_attrs/types/${i}
done

# 查看 GID 对应的网络设备
for i in 0 1 2 3; do
    echo -n "GID ${i} ndev: "
    cat /sys/class/infiniband/mlx5_0/ports/1/gid_attrs/ndevs/${i}
done

# 查看 GID 值
cat /sys/class/infiniband/mlx5_0/ports/1/gids/0
cat /sys/class/infiniband/mlx5_0/ports/1/gids/3
```

**示例输出：**
```
GID 0 type: IB/RoCE v1
GID 1 type: RoCE v2
GID 2 type: IB/RoCE v1
GID 3 type: RoCE v2
GID 0 ndev: enp0s6np0
GID 1 ndev: enp0s6np0
```

### 选择 RoCE 版本

在测试工具中通过 `-x` 参数指定 GID 索引：

```bash
# 使用 GID 0 (RoCE v1)
ib_write_bw -d mlx5_0 -x 0

# 使用 GID 1 (RoCE v2)
ib_write_bw -d mlx5_0 -x 1

# 使用 GID 3 (RoCE v2，IPv6 格式)
ib_write_bw -d mlx5_0 -x 3
```

默认情况下，测试工具会自动选择合适的 GID。

## 故障排查

### 检查驱动加载

```bash
lsmod | grep mlx5
# 应显示 mlx5_core, mlx5_ib

cat /sys/class/infiniband/mlx5_0/node_type
# 应显示 "1: CA"
```

### 检查设备文件

```bash
ls -la /dev/infiniband/
# 应包含 uverbs0 (用户态 verbs 接口), umad0 (管理接口), rdma_cm 等

ls -la /sys/class/infiniband/mlx5_0/
```

### 检查网卡状态

```bash
# 驱动信息
ethtool -i enp0s6np0

# 链路状态
ethtool enp0s6np0 | grep -E "Speed|Duplex|Link"

# 统计信息
cat /sys/class/infiniband/mlx5_0/ports/1/counters/
```

### 网络连通性测试

```bash
# 测试对端连通性
ping 10.0.3.2

# 检查路由
ip route get 10.0.3.2
```

### 常见问题

1. **端口状态不是 Active**
   ```bash
   # 检查物理连接
   cat /sys/class/infiniband/mlx5_0/ports/1/phys_state
   # 应显示 "5: LinkUp"
   ```

2. **RoCE 连接失败**
   ```bash
   # 检查 GID 配置
   cat /sys/class/infiniband/mlx5_0/ports/1/gids/0
   # 确保有有效的 IPv4/IPv6 GID
   ```

3. **性能测试连接失败**
   - 确认防火墙允许端口 18515（默认）
   - 检查对端是否已启动服务端
   - 验证 IP 地址和路由可达

## 开发参考

### libibverbs 编程

```c
#include <infiniband/verbs.h>

// 获取设备列表
struct ibv_device **dev_list;
int num_devices;
dev_list = ibv_get_device_list(&num_devices);

// 打开设备
struct ibv_context *ctx = ibv_open_device(dev_list[0]);

// 查询端口
struct ibv_port_attr port_attr;
ibv_query_port(ctx, 1, &port_attr);

// 清理
ibv_free_device_list(dev_list);
```

编译：`gcc -o test test.c -libverbs`

## 工具参考

| 工具 | 用途 |
|------|------|
| ibstat | 显示 InfiniBand 设备状态 |
| ibstatus | 显示端口状态摘要 |
| ibv_devinfo | 详细设备信息 |
| rdma | RDMA 子系统管理 |
| ib_write_bw | 写带宽测试 |
| ib_read_bw | 读带宽测试 |
| ib_send_bw | 发送带宽测试 |
| ib_write_lat | 写延迟测试 |
| ib_read_lat | 读延迟测试 |
| ib_send_lat | 发送延迟测试 |
| ibnetdiscover | 发现 IB 网络拓扑（仅 InfiniBand，RoCE 不可用） |

## 实际测试性能

本地回环测试（同设备 QP 间）：

### 写带宽 vs 消息大小

| 消息大小 | 平均带宽 | 消息速率 |
|---------|---------|---------|
| 256 bytes | 1160 MiB/sec | 4.75 Mpps |
| 1024 bytes | 3641 MiB/sec | 3.73 Mpps |
| 4096 bytes | 5693 MiB/sec | 1.46 Mpps |
| 65536 bytes | 5942 MiB/sec | 0.095 Mpps |
| 1 MiB | 5921 MiB/sec | 0.006 Mpps |

**峰值带宽**: ~5946 MiB/sec (~48 Gbps)

### 写延迟

| 消息大小 | 平均延迟 | 最小延迟 | 99% 分位 |
|---------|---------|---------|---------|
| 2 bytes | 1.92 usec | 1.89 usec | 2.15 usec |

### 测试结论

- **带宽**: 接近理论峰值 48Gbps (ConnectX-5 10G 速率 x4 PCIe 通道)
- **延迟**: 单跳 RDMA 写延迟约 1.9 微秒
- **最佳消息大小**: 64KB 以上可达到峰值带宽
- **小包性能**: 256 bytes 小包仍可达到 4.75 Mpps

## 当前状态

| 项目 | 状态 |
|------|------|
| 驱动 (mlx5_core) | [OK] 已加载 |
| 设备 (mlx5_0) | [OK] 已识别 |
| 端口状态 | [OK] Active / LinkUp |
| 协议 | [OK] RoCE |
| IP 地址 | [OK] 10.0.3.1/30 |
| 对端连通性 | [OK] 10.0.3.2 可达 |
| 带宽测试 | [OK] ~48 Gbps (本地) |
| 延迟测试 | [OK] ~1.92 us (本地) |

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
