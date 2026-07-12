# VM 迁移系统 V2 设计稿
<!-- 84841e0e-cbcf-4309-b709-db082227e839 -->

> 状态：草案（WIP）
> 创建时间：2026-03-12
> 目标：用 NFS + RPC 替换 NBD，实现分布式 QEMU 集群

(
差不多是这个意思了，但是我发现我逐渐就制作了类似
libvirt 的东西
1. 首先，启动一个 daemon ，可以接受 rpc
2. daemon 可以接受要启动的命令
3. 如果是之前的本地启动，那么就发送 rpc ，让 daemon 启动这个服务，监听这个服务
4. 所以，相当于，我们要启动一个 pueue 的功能

(嗯，似乎就是这么简单了)

也没那么简单，nfs 存在这个 flock 丢失的问题:

[30757.904485] NFS: 10.0.0.2: lost 4 locks
[30819.343931] NFS: 10.0.0.2: lost 4 locks
[30880.778548] NFS: 10.0.0.2: lost 4 locks

)

## 1. 架构概览

```
┌─────────────┐                      ┌─────────────┐
│  Source Host │                      │ Target Host │
│  (正在运行VM) │  ──1. RPC 调用────>  │  (等待接收) │
└──────┬──────┘                      └──────┬──────┘
       │                                    │
       │    ┌─────────────────────────┐     │
       └───>│      NFS Server         │<────┘
            │  (/shared/vm-disks/)    │
            └─────────────────────────┘
```

### 核心变化
- **存储层**：NFS 共享存储，所有节点看到完全相同的磁盘镜像
- **计算层**：RPC 远程启动 QEMU，参数完全一致
- **迁移流程**：源主机通知目标主机"启动并等待"，然后开始热迁移

---

## 2. 组件划分

| 组件 | 部署位置 | 职责 |
|------|----------|------|
| **VM Agent** | 每台主机后台运行 | 接收 RPC 指令，本地启动/停止 QEMU |
| **Migration Controller** | 源主机（或独立服务） | 协调迁移流程，调用目标主机 Agent |
| **NFS** | 独立存储服务器 | 共享磁盘镜像、配置文件 |

---

## 3. RPC 接口设计（gRPC）

```protobuf
syntax = "proto3";

service VmAgent {
  // 准备接收迁移：启动 QEMU 并进入暂停等待状态
  rpc PrepareMigration(PrepareRequest) returns (PrepareResponse);

  // 取消准备（如果迁移失败回滚）
  rpc CancelMigration(CancelRequest) returns (Status);

  // 查询 VM 状态
  rpc QueryVmStatus(QueryRequest) returns (VmStatus);

  // 普通启动 VM（非迁移场景）
  rpc StartVm(StartRequest) returns (Status);

  // 停止 VM
  rpc StopVm(StopRequest) returns (Status);
}

message PrepareRequest {
  string vm_name = 1;           // VM 名称
  string uuid = 2;              // VM UUID（确保一致性）
  QemuConfig config = 3;        // QEMU 完整配置
  MigrationCapability caps = 4; // 迁移能力协商
}

message QemuConfig {
  string qemu_binary = 1;       // qemu-system-x86_64
  uint32 memory_mb = 2;         // -m
  uint32 smp = 3;               // -smp
  repeated Disk disks = 4;      // -drive / -blockdev
  repeated Nic nics = 5;        // -netdev / -device
  string machine_type = 6;      // -machine
  string cpu_model = 7;         // -cpu
  repeated string extra_args = 8; // 其他参数
  bool wait_for_gdb = 9;        // -s （暂停等待）
  uint32 gdb_port = 10;         // -gdb tcp::PORT
}

message Disk {
  string node_name = 1;
  string file_path = 2;         // NFS 路径，如 /shared/vms/vm1/disk.qcow2
  string format = 3;            // qcow2, raw
  string if_type = 4;           // virtio, scsi
  bool readonly = 5;
}

message PrepareResponse {
  bool success = 1;
  string error_msg = 2;
  uint32 monitor_port = 3;      // QEMU monitor 端口
  uint32 migration_port = 4;    // 用于接收迁移的端口
  uint32 gdb_port = 5;          // GDB 端口（如果启用 -s）
  uint32 pid = 6;               // QEMU 进程 ID
}

message MigrationCapability {
  bool compress = 1;
  bool multifd = 2;
  bool rdma = 3;
  string version = 4;           // 兼容性检查
}
```

---

## 4. 迁移流程时序

```
源主机                              目标主机
  │                                    │
  │ ───────── 1. PrepareMigration ──> │
  │                                    │  启动 QEMU -s -incoming tcp:0:4444
  │ <──────── 2. Ready (port info) ── │
  │                                    │
  │ ──────── 3. 开始热迁移 (QMP) ────> │
  │   migrate -d tcp:target:4444       │  接收内存状态
  │                                    │
  │ ───────── 4. 查询迁移进度 ───────> │
  │   直到 completed                   │
  │                                    │
  │ ───────── 5. Cancel（如失败）────> │  或继续运行
```

---

## 5. 技术栈（Rust）

| 组件 | 推荐库 |
|------|--------|
| RPC 框架 | tonic (gRPC) |
| 异步运行时 | tokio |
| 序列化 | prost (protobuf) |
| 配置管理 | config + serde |
| 命令行 | clap |
| 进程管理 | tokio::process |
| 错误处理 | thiserror + anyhow |
| 日志 | tracing + tracing-subscriber |

---

## 6. 项目结构

```
nbd-rs/                    # 或改名为 vm-agent/
├── Cargo.toml
├── proto/
│   ├── migration.proto
│   ├── nbd.proto          # 保留兼容或删除
│   └── common.proto
├── src/
│   ├── main.rs            // 服务入口
│   ├── config.rs          // 配置管理
│   ├── server.rs          // gRPC 服务启动
│   ├── services/          // 业务逻辑
│   │   ├── mod.rs
│   │   ├── migration.rs   // VM 迁移服务
│   │   └── vm.rs          // VM 生命周期管理
│   ├── manager/           // 核心管理器
│   │   ├── mod.rs
│   │   ├── process.rs     // QEMU 进程管理
│   │   ├── port.rs        // 端口分配器
│   │   └── nfs.rs         // NFS 路径检查
│   └── cli.rs             // 客户端命令行
└── tests/
```

---

## 7. 关键实现要点

### 7.1 VM Agent 核心逻辑

```rust
pub struct VmAgent {
    nfs_mount: PathBuf,          // /shared/vms
    qemu_binary: PathBuf,
    vms: RwLock<HashMap<String, VmInstance>>,
}

pub struct VmInstance {
    process: Child,
    monitor_port: u16,
    migration_port: u16,
    status: VmState,
}

#[tonic::async_trait]
impl vm_agent_server::VmAgent for VmAgent {
    async fn prepare_migration(&self, request: Request<PrepareRequest>)
        -> Result<Response<PrepareResponse>, Status>
    {
        // 1. 检查 NFS 路径可访问
        // 2. 动态分配端口
        // 3. 构建 QEMU 命令行（与源主机一致）
        // 4. 启动进程
        // 5. 等待 QEMU ready
    }
}
```

### 7.2 QEMU 启动参数

关键参数保持一致：
- `-S` : 暂停 CPU
- `-gdb tcp::PORT,server,nowait` : 调试端口
- `-incoming tcp:0:MIGRATION_PORT` : 接收迁移
- `-monitor tcp:127.0.0.1:MONITOR_PORT,server,nowait`
- 磁盘路径使用 NFS 挂载点

### 7.3 与现有 collei 系统集成

```rust
// 读取 collei 风格的 VM 配置
fn load_vm_config(vm_dir: &Path) -> Result<QemuConfig> {
    let opt_dir = vm_dir.join("opt");

    // 读取 ram, smp, uuid 等文件
    // 转换为 QemuConfig
    // 磁盘路径映射到 NFS
}
```

---

## 8. 待解决问题（TODO）

### 8.1 NFS 相关
- [ ] NFS 挂载选项（async/noac/actimeo=0）
- [ ] 性能测试（IOPS、延迟）
- [ ] 是否需要集群文件系统（OCFS2/GFS2）

### 8.2 网络配置
- [ ] MAC 地址生成策略（基于 UUID 保持一致）
- [ ] 目标主机网桥名称可能不同
- [ ] IP 地址冲突处理（同子网/跨子网）

### 8.3 兼容性
- [ ] QEMU 版本检查
- [ ] machine type 兼容性
- [ ] CPU 特性一致性

### 8.4 调试支持
- [ ] GDB 远程连接自动化
- [ ] 断点设置工作流
- [ ] 调试与迁移的结合

### 8.5 高可用
- [ ] Agent 进程保活
- [ ] 迁移失败回滚机制
- [ ] 多目标主机选择策略

---

## 9. 简化版备选（HTTP/json）

如果不引入 gRPC，可用 axum 快速实现：

```rust
#[derive(Deserialize)]
struct StartVmRequest {
    vm_name: String,
    qemu_args: Vec<String>,  // 直接传递构建好的参数
    incoming_port: u16,
}

async fn handle_start(Json(req): Json<StartVmRequest>) -> Json<StartVmResponse> {
    // 启动 QEMU，返回 monitor_port 和 pid
}
```

---

## 10. 与旧架构对比

| 特性 | NBD 旧架构 (当前) | NFS + RPC 新架构 |
|------|------------------|-----------------|
| 存储后端 | NBD 导出/导入 | NFS 共享 |
| 磁盘迁移 | 需要拷贝 | 零拷贝（共享存储）|
| 启动命令 | 不同（源启动，目标 NBD） | **完全一致** |
| 远程控制 | 仅磁盘管理 | **完整 VM 生命周期** |
| 调试能力 | 有限 | **完整（-s + GDB）** |
| 复杂度 | 中等（需管理 NBD 进程） | **低（无额外进程）** |
| 单点故障 | NBD server | NFS server |

---

## 附录：原有设计参考

- 旧实现：`nbd/s.py`（服务端）、`nbd/c.py`（客户端）
- 使用裸 socket + JSON 通信
- 混合传输 JSON 头和二进制文件
- 需要管理 qemu-nbd 进程生命周期

---

*最后更新：2026-03-12*
*下一步：等待进一步细化或原型实现*



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
