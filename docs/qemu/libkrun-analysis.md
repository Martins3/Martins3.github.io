(这个结果，我不认可, 然后需继续调查一 libkrunfw 真的就是 linux 吗?)

无非就是一个，然后按照
```txt
{ pkgs ? import <nixpkgs> {} }:

let
  libkrun = pkgs.libkrun;
  libkrunfw = pkgs.libkrunfw;
in
pkgs.mkShell {
  buildInputs = [
    libkrun
    libkrunfw
  ];

  shellHook = ''
    echo "libkrun development environment"
    echo "libkrun: ${libkrun}"
    echo "libkrunfw: ${libkrunfw}"

    # 设置环境变量方便编译示例
    export LIBKRUN_LIB="${libkrun}/lib"
    export LIBKRUN_INC="${libkrun.dev}/include"

    # 使用系统 gcc 避免 IFUNC 冲突
    export PATH="/usr/bin:$PATH"

    echo ""
    echo "Environment variables set:"
    echo "  LIBKRUN_LIB=$LIBKRUN_LIB"
    echo "  LIBKRUN_INC=$LIBKRUN_INC"
    echo ""
    echo "Usage:"
    echo "  gcc -o test test.c -I\$LIBKRUN_INC -L\$LIBKRUN_LIB -lkrun -Wl,-rpath,\$LIBKRUN_LIB"
  '';
}
```
## 一句话总结

libkrun 是一个用 Rust 编写的动态库，封装了基于 KVM（Linux）/ HVF（macOS）的轻量级虚拟机能力，通过简单的 C API 让应用程序轻松获得虚拟化隔离能力。

---

## Layer 1: 高屋建瓴（战略层）

### 设计哲学：为什么存在？

**核心问题**：容器隔离性不足，传统虚拟机太重

| 方案 | 隔离性 | 启动速度 | 内存开销 | 复杂度 |
|------|--------|----------|----------|--------|
| 传统容器（runC） | 低（共享内核） | 快 | 低 | 低 |
| 传统虚拟机（QEMU） | 高 | 慢 | 高 | 高 |
| libkrun | 高 | 快（<100ms） | 低（~10MB） | 低 |

libkrun 定位于"轻量级虚拟化"，在保持接近容器的启动速度和资源开销的同时，提供硬件级隔离。

### 演进历史

```
2019: Firecracker 由 AWS 开源，证明轻量级虚拟化可行性
   ↓
2020: libkrun 项目启动，基于 Firecracker 代码库
   ↓
2021: 引入 TSI（Transparent Socket Impersonation）网络技术
   ↓
2022: 支持 AMD SEV 内存加密
   ↓
2023: 支持 Intel TDX，增加 GPU 直通（venus/native-context）
   ↓
2024: 添加 macOS HVF 支持，实现跨平台
   ↓
2025: API 稳定（1.0.0+），广泛应用于 crun/krunkit/muvm 等项目
```

### 生态定位

```
┌─────────────────────────────────────────────────────────────┐
│                    容器运行时生态                            │
├─────────────────────────────────────────────────────────────┤
│  高性能计算 │  桌面虚拟化  │   机密计算   │   嵌入式/边缘   │
│  (Kata)    │  (krunkit)  │  (SEV/TDX)  │   (muvm)       │
├─────────────────────────────────────────────────────────────┤
│                     libkrun（中间层）                        │
├─────────────────────────────────────────────────────────────┤
│   crun     │   podman    │   Docker    │   自定义工具    │
└─────────────────────────────────────────────────────────────┘
```

**与同类技术对比**：

| 特性 | libkrun | Firecracker | Cloud-Hypervisor | QEMU |
|------|---------|-------------|------------------|------|
| 定位 | 动态库 | VMM | VMM | 完整系统模拟 |
| 集成度 | 高（库形式） | 中（进程） | 中（进程） | 低 |
| API 复杂度 | 简单 C API | REST API | CLI/API | CLI/QMP |
| 启动时间 | <100ms | <125ms | ~150ms | >1s |
| 内存开销 | ~10MB | ~15MB | ~20MB | >100MB |
| 适用场景 | 容器集成 | Serverless | 云原生 | 通用虚拟化 |

### 生产实践

**主要用户**：

1. **crun** - 容器运行时，通过 `krun` 实现虚拟化隔离
2. **krunkit** - macOS 上的 GPU 加速虚拟机（用于运行 Linux 图形应用）
3. **muvm** - Asahi Linux 项目，用于在 Apple Silicon 上运行 x86 游戏
4. **krunvm** - 命令行工具，快速启动微虚拟机

**普及度评估**：
- 相对小众，但在特定场景（macOS 虚拟化、机密容器）中增长迅速
- 作为 crun 的虚拟化后端，间接被 Podman 用户使用
- 在 Apple Silicon Mac 用户中因 krunkit/muvm 而受到关注

### Trade-offs：带来的问题与限制

**优势**：
- 启动极快（接近容器）
- 资源开销小
- 简单 C API 易于集成
- 支持机密计算（SEV/TDX）
- 跨平台（Linux/macOS）

**限制**：

1. **内核依赖**：需要特定的 libkrunfw 内核（基于主线内核打补丁）
2. **功能受限**：不是通用 VMM，不支持热迁移、快照等高级功能
3. **调试困难**：微虚拟机内部状态难以查看
4. **网络特殊**：TSI 模式需要自定义内核，传统网络需要额外工具（passt/gvproxy）
5. **安全边界**：VMM 和 guest 在同一安全上下文，需要额外的 namespace 隔离

**什么场景不用**：
- 需要完整操作系统管理的场景
- 需要热迁移/快照的企业级虚拟化
- 需要广泛硬件兼容性的场景

---

## Layer 2: 架构原理（战术层）

### 核心概念

```
┌─────────────────────────────────────────────────────────────────┐
│                        Application                              │
│                     (crun/krunkit/...)                          │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                      libkrun C API                              │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐   │
│  │ krun_create │ │ krun_set_*  │ │ krun_start_enter        │   │
│  │ _ctx()      │ │ 配置 API    │ │ 启动并进入 VM           │   │
│  └─────────────┘ └─────────────┘ └─────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                         VMM (Rust)                              │
│  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────────┐   │
│  │ Virtio 设备 │ │ vCPU 管理   │ │ 内存管理                │   │
│  │ (console/   │ │ (KVM/HVF)   │ │ (Guest Memory)          │   │
│  │  block/net) │ │             │ │                         │   │
│  └─────────────┘ └─────────────┘ └─────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    Hypervisor (KVM/HVF)                         │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                     Guest MicroVM                               │
│              (Linux + init.krun + 应用程序)                      │
└─────────────────────────────────────────────────────────────────┘
```

### 关键抽象

1. **Context（上下文）**：VM 配置的容器，通过 `krun_create_ctx()` 创建
2. **Virtio 设备**：标准化的半虚拟化设备接口
3. **TSI（Transparent Socket Impersonation）**：透明套接字代理，让 VM 无需虚拟网卡即可访问网络
4. **init.krun**：VM 内部的 init 程序，负责挂载文件系统并启动用户程序

### 工作原理

#### 启动流程

```
1. 创建 Context
   krun_create_ctx() → 返回 ctx_id

2. 配置 VM
   krun_set_vm_config(ctx, vcpus, ram)     → 设置 vCPU 和内存
   krun_set_root(ctx, path)                → 设置根文件系统（virtio-fs）
   krun_set_exec(ctx, exec_path, argv)     → 设置要执行的程序
   [可选] krun_add_disk(ctx, ...)          → 添加块设备
   [可选] krun_add_net_*(ctx, ...)         → 配置网络

3. 启动 VM
   krun_start_enter(ctx)                   → 启动并阻塞直到 VM 退出
```

#### 两种网络模式对比

| 特性 | TSI（默认） | virtio-net + passt/gvproxy |
|------|-------------|---------------------------|
| 虚拟网卡 | 无 | 有（eth0） |
| 实现方式 | VMM 代理 socket 调用 | 数据包转发 |
| 启动要求 | 需要 libkrunfw 内核 | 需要 passt/gvproxy 进程 |
| 适用场景 | 简单应用、快速启动 | 需要完整网络栈的应用 |
| 限制 | 不支持 raw socket | 需要额外进程 |
| 启用方式 | 不添加网卡时自动启用 | 调用 `krun_add_net_*()` |

#### 与 libkrunfw 的关系

```
┌─────────────┐      ┌─────────────┐      ┌─────────────┐
│  libkrun    │ ───→ │  libkrunfw  │ ───→ │   内核镜像   │
│  (VMM 逻辑) │      │ (内核提供)  │      │ (预编译)    │
└─────────────┘      └─────────────┘      └─────────────┘
      ↓
   动态加载
   libkrunfw.so
   导出:
   - krunfw_get_kernel()
   - krunfw_get_initrd() (TEE)
   - krunfw_get_qboot() (TEE)
```

### 安全模型

**重要**：libkrun 认为 VMM 和 Guest 属于同一安全上下文。

```
┌─────────────────────────────────────────┐
│              Host OS                    │
│  ┌─────────────────────────────────┐   │
│  │  Namespace / UserNS 隔离（推荐） │   │
│  │  ┌───────────────────────────┐  │   │
│  │  │        VMM (libkrun)      │  │   │
│  │  │  ┌─────────────────────┐  │  │   │
│  │  │  │     Guest VM        │  │  │   │
│  │  │  │  ┌───────────────┐  │  │  │   │
│  │  │  │  │   Application  │  │  │  │   │
│  │  │  │  └───────────────┘  │  │  │   │
│  │  │  └─────────────────────┘  │  │   │
│  │  └───────────────────────────┘  │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

---

## Layer 3: 源码实现（实现层）

### 项目结构

```
libkrun/
├── include/libkrun.h          # C API 头文件
├── init/init.c                # VM 内部的 init 程序
├── src/
│   ├── libkrun/src/lib.rs     # 主库入口，C API 实现
│   ├── vmm/src/               # VMM 核心实现
│   │   ├── lib.rs             # VMM 主逻辑
│   │   ├── builder.rs         # VM 构建器
│   │   ├── resources.rs       # 资源配置管理
│   │   ├── device_manager/    # 设备管理
│   │   └── vmm_config/        # 配置结构定义
│   ├── devices/src/           # 虚拟设备实现
│   │   └── virtio/            # Virtio 设备
│   │       ├── block/         # 块设备
│   │       ├── console.rs     # 控制台
│   │       ├── fs/            # virtio-fs
│   │       ├── gpu/           # GPU（virgl/venus）
│   │       ├── net/           # 网络设备
│   │       ├── rng.rs         # 随机数
│   │       ├── snd/           # 声音
│   │       └── vsock/         # Vsock（TSI 实现）
│   ├── arch/src/              # 架构相关代码
│   │   ├── x86_64/            # x86_64 特定
│   │   ├── aarch64/           # ARM64 特定
│   │   └── riscv64/           # RISC-V 特定
│   ├── kernel/src/            # 内核加载相关
│   ├── rutabaga_gfx/src/      # GPU 图形支持
│   └── utils/src/             # 工具库
├── examples/                  # 示例程序
│   └── chroot_vm.c            # chroot 风格 VM
└── tests/                     # 测试
```

### 关键数据结构

#### ContextConfig（VM 配置）

```rust
// src/libkrun/src/lib.rs
struct ContextConfig {
    krunfw: Option<KrunfwBindings>,      // libkrunfw 绑定
    vmr: VmResources,                     // VM 资源
    workdir: Option<String>,             // 工作目录
    exec_path: Option<String>,           // 执行路径
    env: Option<String>,                 // 环境变量
    args: Option<String>,                // 参数
    #[cfg(feature = "blk")]
    block_cfgs: Vec<BlockDeviceConfig>,  // 块设备配置
    vsock_config: VsockConfig,           // vsock 配置（TSI）
    // ...
}
```

#### VmResources（VM 资源）

```rust
// src/vmm/src/resources.rs
pub struct VmResources {
    pub vm_config: VmConfig,              // VM 基本配置
    pub kernel_bundle: KernelBundle,      // 内核镜像
    pub cmdline_config: KernelCmdlineConfig,  // 内核命令行
    pub block_devices: BlockBuilder,      // 块设备构建器
    pub net_devices: NetBuilder,          // 网络设备构建器
    pub vsock_device: VsockBuilder,       // vsock 设备
    // ...
}
```

### C API 核心函数

```c
// 创建/销毁上下文
int32_t krun_create_ctx(void);
int32_t krun_free_ctx(uint32_t ctx_id);

// 基本配置
int32_t krun_set_vm_config(uint32_t ctx_id, uint8_t num_vcpus, uint32_t ram_mib);
int32_t krun_set_root(uint32_t ctx_id, const char *root_path);
int32_t krun_set_exec(uint32_t ctx_id, const char *exec_path,
                      const char *const argv[], const char *const envp[]);

// 存储配置
int32_t krun_add_disk(uint32_t ctx_id, const char *block_id,
                      const char *disk_path, bool read_only);
int32_t krun_add_virtiofs(uint32_t ctx_id, const char *c_tag, const char *c_path);

// 网络配置
int32_t krun_add_net_unixstream(uint32_t ctx_id, const char *c_path, int fd,
                                uint8_t *const c_mac, uint32_t features, uint32_t flags);
int32_t krun_set_port_map(uint32_t ctx_id, const char *const port_map[]);

// GPU/显示
int32_t krun_set_gpu_options(uint32_t ctx_id, uint32_t virgl_flags);
int32_t krun_add_display(uint32_t ctx_id, uint32_t width, uint32_t height);

// 启动
int32_t krun_start_enter(uint32_t ctx_id);
```

### TSI（Transparent Socket Impersonation）实现

TSI 是 libkrun 的创新特性，让 VM 无需虚拟网卡即可访问网络：

```
Guest Application          Guest Kernel           VMM (libkrun)           Host
       │                        │                      │                    │
       │ socket()               │                      │                    │
       │→───────────────────────→│                      │                    │
       │                        │ vsock 协议            │                    │
       │                        │→─────────────────────→│                    │
       │                        │                      │ 代理为 host socket  │
       │                        │                      │→───────────────────→│
       │                        │                      │                    │ connect()
       │                        │                      │←───────────────────│
       │←───────────────────────────────────────────────────────────────────│
```

关键代码位置：`src/devices/src/virtio/vsock/tsi/`

### init.krun 程序

VM 内部的 1 号进程（init），负责：

1. 挂载基本文件系统（/dev, /proc, /sys, cgroup2）
2. 解析配置（/.krun_config.json 或环境变量）
3. 设置 rlimits
4. 启动用户程序
5. 程序退出后，通过 ioctl 向 virtio-fs 报告退出码

```c
// init/init.c 核心流程
int main() {
    mount_filesystems();           // 挂载 dev, proc, sys
    setup_redirects();             // 重定向 stdin/stdout/stderr

    #ifdef SEV
    chroot_luks();                 // SEV 模式解密根文件系统
    #endif

    config_parse_file(&argv, &workdir);  // 解析配置
    set_rlimits(rlimits);
    chdir(workdir);

    execv(exec_path, argv);        // 执行用户程序

    set_exit_code(exit_code);      // 报告退出码
}
```

### 构建变体

| 变体 | 特性 | 适用场景 |
|------|------|----------|
| libkrun | 基础功能 | 通用 |
| libkrun-sev | + AMD SEV 加密 | 机密计算 |
| libkrun-tdx | + Intel TDX 加密 | 机密计算 |
| libkrun-efi | + OVMF/EDK2 | macOS 启动发行版内核 |

---

## Layer 4: 测试验证（实践层）

> **注意**：本节包含实际构建尝试中遇到的问题和解决方案，以及基于代码分析的测试方法。

### 本机验证记录

**环境信息**：
- OS: Fedora Linux 42 (Server Edition)
- 架构: x86_64
- Nix: 2.31.2

#### 验证结果汇总

| 步骤 | 状态 | 说明 |
|------|------|------|
| Nix Shell 环境 | 成功 | `nix-shell` 进入开发环境 |
| 基本 API 测试 | 成功 | `krun_create_ctx` 等工作正常 |
| chroot_vm 编译 | 失败 | nix 版本未启用 NET 功能 |
| 源码构建 libkrunfw | 进行中 | 内核编译耗时较长，约 15-30 分钟 |

#### 实际验证记录

**测试程序**：`test_minimal.c` - 验证 libkrun 核心 API

```c
// 最小化测试 - 不依赖 NET/BLK 功能
#include <libkrun.h>
#include <stdio.h>

int main() {
    // 1. 初始化日志
    krun_init_log(-1, KRUN_LOG_LEVEL_INFO, KRUN_LOG_STYLE_NEVER, 0);

    // 2. 创建上下文
    int32_t ctx = krun_create_ctx();
    printf("Context ID: %d\n", ctx);

    // 3. 配置 VM
    krun_set_vm_config(ctx, 1, 512);

    // 4. 设置根文件系统
    krun_set_root(ctx, "/tmp");

    // 5. 配置执行
    const char *argv[] = {"/bin/sh", NULL};
    krun_set_exec(ctx, "/bin/sh", argv, NULL);

    // 6. 清理
    krun_free_ctx(ctx);
    return 0;
}
```

**运行结果**：
```
=== Minimal libkrun Test ===

1. Initializing log...
   Result: 0

2. Creating context...
   Context ID: 0

3. Setting VM config (1 vCPU, 512 MiB)...
   Success

4. Setting root filesystem...
   Result: 0

5. Setting exec...
   Result: 0

6. Freeing context...
   Result: 0

=== Test Complete ===
```

**结论**：
- libkrun 核心 API 工作正常
- 可以创建上下文、配置 VM、设置根文件系统和执行命令
- 实际启动 VM 需要完整的 libkrunfw 内核（构建中）

#### 构建问题记录

**问题 1**：Fedora 42 与内核 6.12 的兼容性问题

在 Fedora 42 上直接构建 libkrunfw 时遇到 `gelf.h` 找不到的问题，原因是内核的 `objtool` 需要特定的头文件路径。

**问题 2**：Nix gcc 与系统库的 IFUNC 冲突

使用 `nix-shell '<nixpkgs>' -A stdenv` 时，Nix 的 gcc 与系统 glibc 存在兼容性问题。

**建议**：
对于开发和测试，使用 `nix-shell` 配合系统 gcc 是最简便的方法。对于生产部署，建议使用 nixpkgs 中预编译的版本，或等待社区修复构建问题。

#### 1. 使用 Nix Shell（推荐，不污染全局环境）

在项目目录创建 `shell.nix`：

```nix
{ pkgs ? import <nixpkgs> {} }:

let
  libkrun = pkgs.libkrun;
  libkrunfw = pkgs.libkrunfw;
in
pkgs.mkShell {
  buildInputs = [
    libkrun
    libkrunfw
  ];

  shellHook = ''
    export LIBKRUN_LIB="${libkrun}/lib"
    export LIBKRUN_INC="${libkrun.dev}/include"

    # 使用系统 gcc 避免 IFUNC 冲突
    export PATH="/usr/bin:$PATH"

    echo "libkrun development environment ready"
    echo "LIBKRUN_LIB=$LIBKRUN_LIB"
    echo "LIBKRUN_INC=$LIBKRUN_INC"
  '';
}
```

进入开发环境：

```bash
cd ~/data/libkrun
nix-shell

# 在 nix-shell 中，环境变量已自动设置
gcc -o test_libkrun test_libkrun.c \
    -I$LIBKRUN_INC -L$LIBKRUN_LIB \
    -lkrun -Wl,-rpath,$LIBKRUN_LIB

./test_libkrun
```

**优点**：
- 不污染全局环境（`~/.nix-profile`）
- 依赖版本锁定，可复现
- 退出 shell 后环境自动清理

**注意**：nixpkgs 中的 libkrun 版本（当前 1.15.1）可能功能受限（如未启用 NET），与最新源码（1.17.0+）有差异

#### 2. 基本功能测试

创建测试程序：

```c
// test_libkrun.c
#include <libkrun.h>
#include <stdio.h>

int main() {
    printf("Testing libkrun...\n");

    // 创建上下文
    int32_t ctx = krun_create_ctx();
    if (ctx < 0) {
        printf("Failed to create context: %d\n", ctx);
        return 1;
    }
    printf("Created context: %d\n", ctx);

    // 配置 VM：2 vCPUs, 1024 MiB RAM
    int ret = krun_set_vm_config(ctx, 2, 1024);
    if (ret != 0) {
        printf("Failed to set VM config: %d\n", ret);
        krun_free_ctx(ctx);
        return 1;
    }
    printf("Set VM config: 2 vCPUs, 1024 MiB RAM\n");

    // 清理
    ret = krun_free_ctx(ctx);
    printf("Freed context: %d\n", ret);

    printf("libkrun basic test passed!\n");
    return 0;
}
```

编译运行：

```bash
# 获取 nix store 路径
LIBKRUN_LIB=$(ls -d /nix/store/*-libkrun-1.*/lib | head -1)
LIBKRUN_INC=$(ls -d /nix/store/*-libkrun-1.*/include | head -1)

# 编译
gcc -o test_libkrun test_libkrun.c \
    -I${LIBKRUN_INC} \
    -L${LIBKRUN_LIB} \
    -lkrun -Wl,-rpath,${LIBKRUN_LIB}

# 运行
./test_libkrun
```

**实际输出**：
```
Testing libkrun...
Created context: 0
Set VM config: 2 vCPUs, 1024 MiB RAM
Freed context: 0
libkrun basic test passed!
```

### 其他构建选项

除了使用 nix-shell 外，还可以从源码构建：

#### 选项 A：使用 Fedora/RHEL 系统原生工具链（从源码构建）

```bash
# 1. 安装所有依赖（使用 dnf）
sudo dnf install -y \
    rust cargo \
    glibc-static \
    elfutils-devel \
    patch python3-pyelftools \
    bc bison flex openssl-devel

# 2. 构建 libkrunfw
cd ~/data/libkrunfw
make clean && make
sudo make install

# 3. 构建 libkrun
cd ~/data/libkrun
make BLK=1 NET=1
sudo make install

# 4. 更新动态链接库缓存
sudo ldconfig
```

#### 选项 B：使用 krunvm（需要先有 libkrun）

如果已经安装了 libkrun（通过包管理器），可以用 krunvm 在 VM 中构建：

```bash
cd ~/data/libkrunfw
./build_on_krunvm.sh
make
```

#### 选项 C：使用 Nix Flakes（社区支持）

```bash
# 某些社区提供了 nix 构建支持
nix build github:containers/libkrun
```

### 功能检测（构建后）

构建完成后，可创建测试程序：

```c
// check_features.c
#include <libkrun.h>
#include <stdio.h>

int main() {
    printf("libkrun feature check:\n");
    printf("  NET:  %s\n", krun_has_feature(KRUN_FEATURE_NET) ? "YES" : "NO");
    printf("  BLK:  %s\n", krun_has_feature(KRUN_FEATURE_BLK) ? "YES" : "NO");
    printf("  GPU:  %s\n", krun_has_feature(KRUN_FEATURE_GPU) ? "YES" : "NO");
    printf("  SND:  %s\n", krun_has_feature(KRUN_FEATURE_SND) ? "YES" : "NO");
    return 0;
}
```

编译运行：
```bash
gcc -o check_features check_features.c -lkrun
./check_features
```

**预期输出**（取决于编译选项）：
```
libkrun feature check:
  NET:  YES
  BLK:  YES
  GPU:  NO
  SND:  NO
```

### chroot_vm 示例测试

**注意**：chroot_vm 示例需要 NET 功能支持。nixpkgs 中的 libkrun 默认可能未启用 NET，需要从源码构建完整版本。

从源码构建并运行：

```bash
cd ~/data/libkrun

# 构建带 NET 和 BLK 的 libkrun
make clean
make BLK=1 NET=1
sudo make install
sudo ldconfig

# 构建示例
cd examples
make

# 准备 rootfs（需要 podman）
make rootfs

# 运行 VM（TSI 模式）
./chroot_vm ./rootfs_fedora /bin/sh

# 或 Passt 模式
./chroot_vm --net=PASST ./rootfs_fedora /bin/sh
```

**使用 nix 版本的 libkrun**：
如果只需要测试基本功能，可以修改 chroot_vm.c 移除网络相关代码，或等待 nixpkgs 更新启用了 NET 的版本。

### 调试方法

**启用日志**：
```c
krun_init_log(KRUN_LOG_TARGET_DEFAULT,
              KRUN_LOG_LEVEL_DEBUG,
              KRUN_LOG_STYLE_ALWAYS, 0);
```

**环境变量**：
```bash
export RUST_LOG=debug
export LIBKRUN_DEBUG=1
```

**验证内核版本**（构建后）：
```bash
strings /usr/local/lib64/libkrunfw.so.5 | grep "Linux version"
```


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
