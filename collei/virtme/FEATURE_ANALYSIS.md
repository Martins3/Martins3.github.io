# virtme-ng 全面功能分析

## 一、架构概览

```
┌─────────────────────────────────────────────────────────────────┐
│                     virtme-ng (vng)                              │
│                   Python 前端 (virtme_ng/run.py)                  │
│  - 参数解析 | 内核下载/构建 | 远程编译 | 生成 virtme-run 命令      │
└─────────────────────────────────────────────────────────────────┘
                                ↓
┌─────────────────────────────────────────────────────────────────┐
│                   virtme-run                                    │
│              Python 后端 (virtme/commands/run.py)                │
│  - QEMU 参数组装 | initramfs 生成 | virtiofs 管理 | 启动 QEMU      │
└─────────────────────────────────────────────────────────────────┘
                                ↓
┌─────────────────────────────────────────────────────────────────┐
│                    QEMU                                         │
│  - microvm/q35 | virtio-fs/9p | KVM | 各种设备                    │
└─────────────────────────────────────────────────────────────────┘
                                ↓
┌─────────────────────────────────────────────────────────────────┐
│              virtme-init (guest 中的 PID 1)                      │
│         bash 脚本 (virtme/guest/virtme-init)                      │
│  - 挂载 rootfs | overlay 设置 | 网络 | sshd | 用户会话             │
└─────────────────────────────────────────────────────────────────┘
```

---

## 二、核心功能分类

### 2.1 内核管理

| 功能 | 说明 | 对应选项 |
|------|------|----------|
| **已安装内核启动** | 使用系统已安装的内核版本 | `--kimg v6.6.0` |
| **本地内核目录** | 使用编译好的内核源码目录 | `--kdir /path/to/linux` |
| **内核自动下载** | 从 Ubuntu mainline 下载预编译内核 | `--run v6.6.17` |
| **内核自动构建** | 自动生成最小配置并编译 | `--build` |
| **远程编译** | 在远程服务器编译后传回 | `--build-host ssh://...` |
| **模块管理** | 自动检测/刷新内核模块 | `--mods none/use/auto` |
| **模块依赖** | 自动解析模块依赖关系 | 自动 |
| **跨架构编译** | 支持交叉编译 | `--arch aarch64` |

**最小内核配置策略**:
- 使用 `virtme-configkernel` 生成最小化 .config
- 保留基本功能：virtio, networking, debug, 必要文件系统
- 跳过大量驱动以加速编译

### 2.2 文件系统共享

| 功能 | 说明 | 技术实现 |
|------|------|----------|
| **virtio-fs (默认)** | 高性能共享 host rootfs | virtiofsd + vhost-user-fs |
| **9p (fallback)** | 兼容性回退 | 9pnet_virtio |
| **只读模式** | 默认安全模式，不修改 host | overlay upper=tmpfs |
| **读写模式** | 直接修改 host (危险!) | `--rw` |
| **overlay 可写** | 写时复制，修改不持久 | `--overlay-rwdir /etc` |
| **额外只读目录** | 共享额外目录 (只读) | `--rodir host=guest` |
| **额外读写目录** | 共享额外目录 (读写) | `--rwdir host=guest` |

**overlay 挂载点 (默认)**:
```bash
/etc /lib /home /opt /srv /usr /var /tmp
```

### 2.3 QEMU 机器类型

| 类型 | 适用场景 | 特点 |
|------|----------|------|
| **microvm** (x86_64 默认) | 快速启动测试 | 无 BIOS, MMIO, 启动快 |
| **q35** | 兼容性测试 | 完整 PCI, 传统启动 |
| **virt** (ARM) | ARM 测试 | 标准 ARM virt 平台 |

**microvm 特点**:
- `-M microvm,accel=kvm,pcie=on,rtc=on`
- 使用 `virtio-*-device` 而非 `virtio-*-pci`
- 需要 `virtio-mmio.force-legacy=false`
- 不支持 NUMA (使用 NUMA 时自动禁用)

### 2.4 网络支持

| 模式 | 说明 | 选项 |
|------|------|------|
| **无网络** | 默认安全模式 | 无 |
| **user 模式** | NAT + 端口转发 | `--network user` |
| **bridge 模式** | 桥接到 host 网桥 | `--network bridge=br0` |
| **loop 模式** | VM 内部回环 | `--network loop` |
| **DHCP** | 自动获取 IP | 默认启用，可禁用 `--no-dhcp` |

### 2.5 远程控制台

| 功能 | 说明 | 选项 |
|------|------|------|
| **VSOCK Console** | 通过 VSOCK 连接 VM console | `--console` |
| **VSOCK SSH** | 通过 VSOCK 的 SSH 服务 | `--ssh` |
| **TCP SSH** | 通过 TCP 的 SSH 服务 | `--ssh --ssh-tcp` |
| **客户端连接** | 连接到已启动的服务 | `--console-client`, `--ssh-client` |
| **空密码** | 允许无密码 SSH (vsock only) | `--empty-passwords` |

**SSH 配置**:
- 自动生成 host key
- 使用当前用户的 SSH key
- 配置写入 `~/.config/virtme-ng/ssh.config`

### 2.6 Init 系统支持

| Init | 说明 | 选项 |
|------|------|------|
| **virtme-init (默认)** | 自定义轻量级 init | 无 |
| **virtme-ng-init** | Rust 实现的更快 init | 默认 (如果可用) |
| **systemd** | 使用 systemd | `--systemd` |

**virtme-init 功能**:
- 挂载 kernel fs (proc, sysfs, devtmpfs)
- 设置 overlayfs
- 启动 udevd
- 配置网络 (udhcpc)
- 可选启动 sshd
- 可选启动 snapd
- 设置用户环境
- 运行脚本或交互 shell

### 2.7 脚本执行模式

| 功能 | 说明 | 选项 |
|------|------|------|
| **执行命令** | 在 VM 中执行命令并退出 | `--exec "cmd"` |
| **执行脚本** | 执行 shell 脚本 | `--script-sh "script"` |
| **图形应用** | 启动图形程序 | `--graphics xeyes` |
| **返回码传递** | 将 guest 返回码传回 host | 自动 |
| **I/O 重定向** | stdin/stdout/stderr 透传 | 自动 |

**脚本执行架构**:
```
Host stdin → virtio-serial → Guest /dev/virtio-ports/virtme.stdin
Host stdout ← virtio-serial ← Guest /dev/virtio-ports/virtme.stdout
Host stderr ← virtio-serial ← Guest /dev/virtio-ports/virtme.stderr
```

### 2.8 存储

| 功能 | 说明 | 选项 |
|------|------|------|
| **virtio-blk 磁盘** | 添加块设备 | `--blk-disk name=path` |
| **virtio-scsi 磁盘** | 添加 SCSI 磁盘 | `--disk path` |

### 2.9 调试功能

| 功能 | 说明 | 选项 |
|------|------|------|
| **GDB 调试** | 连接 GDB 到 VM | `--gdb` (需配合 `--debug`) |
| **内存转储** | 生成 crash dump | `--dump` |
| **显示命令** | 打印 QEMU 命令不执行 | `--dry-run --show-command` |
| **详细日志** | 显示启动信息 | `--verbose` |
| **保存 initramfs** | 保存生成的 initramfs | `--save-initramfs path` |

### 2.10 其他功能

| 功能 | 说明 | 选项 |
|------|------|------|
| **balloon** | 内存气球 | `--balloon` |
| **声音** | 音频支持 | `--sound` |
| **vmcoreinfo** | 崩溃信息设备 | `--vmcoreinfo` |
| **NVIDIA GPU** | GPU 直通 | `--nvgpu` |
| **snaps** | snapd 支持 | `--snaps` |
| **空密码** | 所有用户空密码 | `--empty-passwords` |
| **工作目录** | 设置 guest 工作目录 | `--cwd`, `--pwd` |
| **用户名** | 以指定用户运行 | `--user` |

---

## 三、关键技术实现

### 3.1 virtio-fs 启动流程

```bash
# 1. 启动 virtiofsd (host)
virtiofsd \
    --socket-path /tmp/virtmeXXX \
    --shared-dir / \
    --sandbox none \
    --cache always

# 2. QEMU 参数
-object memory-backend-memfd,id=mem,size=1G,share=on
-numa node,memdev=mem
-chardev socket,id=charvirtfs,path=/tmp/virtmeXXX
-device vhost-user-fs-pci,chardev=charvirtfs,tag=ROOTFS

# 3. Kernel cmdline
rootfstype=virtiofs root=ROOTFS

# 4. Guest init 挂载
mount -t virtiofs -o ro ROOTFS /newroot
```

### 3.2 overlayfs 设置流程

```bash
# 在 initramfs 中
mkdir -p /run/tmpfs
mount -t tmpfs none /run/tmpfs

# 为每个目录创建 overlay
for dir in /etc /home /var; do
    mkdir -p /run/tmpfs/$dir/{upper,work}
    mount -t overlay overlay \
        -o lowerdir=/newroot$dir,upperdir=/run/tmpfs/$dir/upper,workdir=/run/tmpfs/$dir/work \
        /newroot$dir
done
```

### 3.3 microvm vs q35

| 特性 | microvm | q35 |
|------|---------|-----|
| Machine | `-M microvm,accel=kvm,pcie=on` | `-M q35` |
| BIOS | 无 | OVMF/SeaBIOS |
| 启动时间 | < 1s | 2-3s |
| 设备类型 | `virtio-*-device` | `virtio-*-pci` |
| MMIO | virtio-mmio | PCI MMIO |
| NUMA | 不支持 | 支持 |

### 3.4 initramfs 内容

```
initramfs/
├── init              # virtme-init
├── bin/
│   └── busybox       # 静态链接
├── dev/
│   ├── null
│   ├── kmsg
│   └── console
├── lib/modules/
│   └── (必要的 .ko 文件)
└── modules/
    └── load_all.sh   # 模块加载脚本
```

**必要内核模块**:
- `virtio_fs.ko` (virtio-fs 文件系统)
- `overlay.ko` (overlayfs)
- `9pnet.ko`, `9pnet_virtio.ko` (9p fallback)
- `virtio_mmio.ko` (microvm)
- `virtio_pci.ko` (q35)
- 各种 virtio 驱动

---

## 四、Kernel Cmdline 参数参考

| 参数 | 说明 | 示例 |
|------|------|------|
| `rootfstype=virtiofs` | rootfs 类型 | virtiofs/9p |
| `root=ROOTFS` | rootfs 标签 | ROOTFS |
| `virtme_hostname=name` | 设置主机名 | virtme-ng |
| `virtme_console=ttyS0` | 控制台设备 | ttyS0 |
| `virtme_user=username` | 运行用户 | root |
| `virtme_root_user=1` | 表示 host 以 root 运行 | 1 |
| `virtme_chdir=path` | 工作目录 | /home/user |
| `virtme_rw_overlayN=path` | 可写 overlay 目录 | /etc |
| `virtme_link_mods=path` | 模块链接路径 | /lib/modules/... |
| `virtme_root_mods=1` | 使用 root 的模块 | 1 |
| `virtme_initmountN=path` | 额外挂载点 | /mnt |
| `virtme.exec=base64` | 执行脚本 (base64) | `dW5hbWU...` |
| `virtme_graphics=1` | 启用图形 | 1 |
| `virtme.sound` | 启用声音 | |
| `virtme.ssh` | 启用 SSH | |
| `virtme_ssh_channel=vsock` | SSH 通道类型 | vsock/tcp |
| `virtme_ssh_user=user` | SSH 用户 | username |
| `virtme.dhcp` | 启用 DHCP | |
| `virtme.snapd` | 启用 snapd | |
| `virtme_empty_passwords=1` | 空密码 | 1 |
| `virtme_stty_con=...` | 终端设置 | rows 42 cols 175 |

---

## 五、与 collei 框架的对比

| 功能 | virtme-ng | collei 框架 |
|------|-----------|-------------|
| **目标** | 快速内核测试 | 通用 VM 管理 |
| **rootfs** | 共享 host (COW) | 独立磁盘镜像 |
| **启动速度** | 极快 (< 2s) | 较慢 (需 boot) |
| **数据持久化** | 不持久 (默认) | 持久化到磁盘 |
| **网络** | 可选 | 默认启用 |
| **设备** | 精简 | 丰富 (VFIO, NVMe 等) |
| **initramfs** | 自定义极简 | 发行版默认 |
| **使用场景** | 内核开发测试 | 完整系统测试 |

---

## 六、功能优先级建议

### P0 (核心功能，必须实现)
1. ✅ virtio-fs 共享 host rootfs
2. ✅ overlayfs 可写层 (写时复制)
3. ✅ 自定义 initramfs (virtme-init)
4. ✅ microvm 机器类型支持
5. ✅ 与 collei 框架集成 (通过 `opt/virtme` 启用)

### P1 (重要功能，建议实现)
1. 🔄 脚本执行模式 (`--exec`)
2. 🔄 网络支持 (user/bridge)
3. 🔄 额外目录共享 (`--rodir`, `--rwdir`)
4. 🔄 用户/工作目录设置
5. 🔄 详细日志/调试

### P2 (增强功能，可选实现)
1. ⭕ SSH/VSOCK 远程控制台
2. ⭕ 内核模块自动处理
3. ⭕ 图形应用支持
4. ⭕ 磁盘设备添加
5. ⭕ GDB 调试支持

### P3 (高级功能，暂不实现)
1. ⭕ 远程编译
2. ⭕ 内核自动构建
3. ⭕ 跨架构支持
4. ⭕ snapd 支持
5. ⭕ GPU 直通

---

## 七、关键设计决策建议

### 1. 文件系统共享
- **只用 virtio-fs**，去掉 9p (如你所说)
- 依赖 host 的 virtiofsd (系统已安装)
- 复用 collei 现有的 `setup_fs_share` 逻辑

### 2. initramfs
- 使用精简的 init (类似 virtme-init)
- 支持 overlayfs 设置
- 从 kernel_dir 自动提取必要模块

### 3. 启动模式
- 默认使用 microvm (如果 x86_64)
- 可配置使用 q35 (如果需要 NUMA 等)
- 通过 `opt/machine` 配置

### 4. 与 collei 集成
- 不破坏现有功能
- 通过 `opt/virtme` 文件启用新模式
- 复用现有网络、串口、显示配置

### 5. 用户接口
- 保持 collei 的配置风格 (文件配置)
- 支持 `opt/virtme_rw` 启用可写模式
- 支持 `opt/share_root` 指定共享目录

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
