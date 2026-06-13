## pstore 如何使用
<!-- ab68ad3f-884b-46be-9942-0f0fc113d7c5 -->

- Documentation/admin-guide/ramoops.rst
- Documentation/admin-guide/pstore-blk.rst
- Documentation/devicetree/bindings/reserved-memory/ramoops.txt

https://blogs.oracle.com/linux/post/pstore-linux-kernel-persistent-storage-file-system
https://docs.kernel.org/admin-guide/pstore-blk.html

https://docs.kernel.org/admin-guide/pstore-blk.html

https://man7.org/linux/man-pages/man8/systemd-pstore.service.8.html

CONFIG_PSTORE=y

打开这几个的时候自动打开的:
```txt
CONFIG_ACPI_APEI=y
CONFIG_ACPI_APEI_GHES=y
CONFIG_ACPI_APEI_MEMORY_FAILURE=y
CONFIG_ACPI_APEI_EINJ=m
```

```txt
CONFIG_PSTORE=y
CONFIG_PSTORE_RAM=y
CONFIG_PSTORE_CONSOLE=y # 记录内核消息

CONFIG_PSTORE_PMSG=y # 记录用户态消息
```

不知道为什么，似乎没有机器使用这个东西。

	  For more information, see Documentation/admin-guide/pstore-blk.rst

	  For more information, see Documentation/admin-guide/ramoops.rst.

怎么看内核文档，反而感觉不需要使用 non volatile memory

## 说什么，来什么
https://mp.weixin.qq.com/s/w2_cfXQUq1yxM1JgzpTQNA

### 看看有办法搭建一个 pstore 的测试

看看:
- fs/pstore/
- Documentation/admin-guide/pstore-blk.rst

## 有模块是 efi_pstore 和 pstore

## 每次 find 的时候，都遇到这个
```txt
🧀  find /sys -name "dma_ops" -exec cat {} +
find: ‘/sys/kernel/debug’: Permission denied
find: ‘/sys/fs/pstore’: Permission denied
find: ‘/sys/fs/bpf’: Permission denied
```

## 物理机中观察到了这个
```txt
[  556.195087][ T3839] Call Trace:
[  556.198622][ T3839]  <TASK>
[  556.201775][ T3839]  dump_stack_lvl+0x32/0x50
[  556.206688][ T3839]  panic+0x340/0x360
[  556.210915][ T3839]  ? _printk+0x60/0x80
[  556.215340][ T3839]  sysrq_handle_crash+0x11/0x20
[  556.220644][ T3839]  __handle_sysrq+0x9b/0x190
[  556.225652][ T3839]  write_sysrq_trigger+0x24/0x40
[  556.231049][ T3839]  proc_reg_write+0x55/0xa0
[  556.235960][ T3839]  vfs_write+0xe9/0x3d0
[  556.240482][ T3839]  ? __count_memcg_events+0x4e/0xb0
[  556.246172][ T3839]  ? handle_mm_fault+0x9d/0x370
[  556.251473][ T3839]  ksys_write+0x6b/0xf0
[  556.255992][ T3839]  do_syscall_64+0x3f/0xa0
[  556.260805][ T3839]  entry_SYSCALL_64_after_hwframe+0x78/0xe2
[  556.267276][ T3839] RIP: 0033:0x7fdba44efba0
[  556.272087][ T3839] Code: 73 01 c3 48 8b 0d d0 72 2d 00 f7 d8 64 89 01 48 83 c8 ff c3 66 0f 1f 44 00 00 83 3d 1d d4 2d 00 00 75 10 b8 01 00 00 00 0f 05 <48> 3d 01 f0 ff ff 73 31 c3 48 83 ec 08 e8 7e cc 01 00 48 89 04 24
[  556.293977][ T3839] RSP: 002b:00007ffdbc7072f8 EFLAGS: 00000246 ORIG_RAX: 0000000000000001
[  556.303280][ T3839] RAX: ffffffffffffffda RBX: 0000000000000002 RCX: 00007fdba44efba0
[  556.312093][ T3839] RDX: 0000000000000002 RSI: 00007fdba53b9000 RDI: 0000000000000001
[  556.320904][ T3839] RBP: 00007fdba53b9000 R08: 000000000000000a R09: 00007fdba53b1740
[  556.329717][ T3839] R10: 00007fdba53b1740 R11: 0000000000000246 R12: 00007fdba47c8400
[  556.338526][ T3839] R13: 0000000000000002 R14: 0000000000000001 R15: 0000000000000000
[  556.347338][ T3839]  </TASK>
[  556.350643][ T3839] Kernel Offset: disabled
[  556.369810][ T3839] pstore: backend (erst) writing error (-28)
```

- pstore 支持多种后端：RAM (ramoops)、EFI 变量 (erst)、块设备 (pstore-blk)

## Anki Cards
<!-- ab68ad3f-884b-46be-9942-0f0fc113d7c5 -->

### Card 1: pstore 的作用与原理
**Q: 什么是 pstore？它的核心作用是什么？**

A:
**定义**: pstore (persistent storage) 是内核崩溃时保存日志的持久化存储机制。

**核心作用**:
- 系统崩溃后保留内核日志、panic 信息
- 下次启动时通过 `/sys/fs/pstore/` 读取崩溃信息

**典型应用场景**:
- 无串口/网口的生产环境服务器
- 嵌入式设备调试
- 间歇性崩溃问题的事后分析

---

### Card 2: pstore 配置与使用
**Q: 如何配置和使用 pstore？**

A:
**内核配置**:
```
CONFIG_PSTORE=y
CONFIG_PSTORE_RAM=y          # RAM 后端 (ramoops)
CONFIG_PSTORE_CONSOLE=y      # 记录控制台消息
CONFIG_PSTORE_PMSG=y         # 记录用户态消息
CONFIG_PSTORE_BLK=y          # 块设备后端
```

**常用后端**:
| 后端 | 配置 | 说明 |
|------|------|------|
| ramoops | PSTORE_RAM | 预留物理内存区域 |
| erst | ACPI_APEI_GHES | UEFI 运行时服务 |
| blk | PSTORE_BLK | 普通块设备分区 |

**使用方法**:
```bash
# 查看存储的崩溃信息
cat /sys/fs/pstore/dmesg-ramoops-0
cat /sys/fs/pstore/console-ramoops-0

# 清除记录 (读取后建议删除释放空间)
rm /sys/fs/pstore/*
```

**Q: ramoops 后端如何工作？为什么能保留崩溃信息？**

A:
**工作原理**:
1. **预留内存**: 通过内核参数 `ramoops.mem_address` 和 `ramoops.mem_size` 预留物理内存
2. **循环缓冲**: 使用 console 层注册的 `console_write` 回调持续写入
3. **断电保留**: 数据写入物理 RAM，依赖不掉电或快速重启保持
4. **重启恢复**: 启动时 ramoops 驱动重新映射该区域，通过 pstore 框架暴露

**关键限制**:
- 需要预留物理内存（无法使用 buddy allocator 的内存）
- 依赖硬件不掉电或快速重启
- 容量有限（通常几百 KB 到几 MB）

**内核参数示例**:
```
ramoops.mem_address=0x10000000 ramoops.mem_size=0x100000
ramoops.console_size=0x40000 ramoops.dump_oops=1
```

---

### Card 4: pstore 与 systemd-pstore
**Q: systemd-pstore.service 的作用是什么？**

A:
**作用**: 系统启动后将 pstore 中的崩溃信息持久化到磁盘。

**工作流程**:
1. 启动时检查 `/sys/fs/pstore/` 中的文件
2. 将日志保存到 `/var/lib/systemd/pstore/` 或指定目录
3. 可选：删除已保存的 pstore 记录释放空间
4. 可选：发送通知邮件

**配置**:
```ini
# /etc/systemd/pstore.conf
[PStore]
Storage=journal  # 或 persistent/external
Unlink=yes       # 读取后删除
```

**优势**:
- 避免 pstore 空间被占满导致新崩溃无法记录
- 集中管理崩溃日志，方便分析
- 与其他 systemd 工具集成

---

### Card 5: pstore-blk vs ramoops
**Q: pstore-blk 与 ramoops 相比有什么优势？**

A:
| 特性 | ramoops | pstore-blk |
|------|---------|------------|
| 存储介质 | 预留 RAM | 普通块设备 (SSD/eMMC) |
| 持久性 | 依赖不掉电 | 真正持久化 |
| 容量 | 受限于预留内存 | 可使用整个分区 |
| 配置复杂度 | 需预留物理内存 | 只需指定块设备 |
| 适用场景 | 早期启动问题 | 一般崩溃记录 |

**pstore-blk 配置**:
```
CONFIG_PSTORE_BLK=y
CONFIG_PSTORE_BLK_DEV="/dev/pstore"  # 指定分区
```

**核心洞察**: pstore-blk 更适合现代系统，无需特殊内存布局；ramoops 适合引导早期阶段的崩溃捕获。

---

### Card 6: pstore 错误码分析
**Q: `pstore: backend (erst) writing error (-28)` 表示什么问题？**

A:
**错误码 -28 (ENOSPC)**: 存储空间不足

**erst 后端**: UEFI Error Record Serialization Table
- 使用 UEFI 非易失性变量存储
- 空间通常非常有限（几十 KB）

**可能原因**:
1. 之前崩溃信息未清理，空间被占满
2. UEFI NVRAM 本身容量小
3. 连续多次崩溃，超出存储能力

**解决方案**:
```bash
# 1. 清理已有的 pstore 记录
rm /sys/fs/pstore/*

# 2. 切换后端（如使用 ramoops 或 pstore-blk）

# 3. 调整记录大小限制
# 设置 max_reason=2 (仅记录 panic，不记录 oops)
echo 2 > /sys/module/pstore/parameters/max_reason
```

```bash
# 查看 ramoops 模块参数（如已加载）
cat /sys/module/ramoops/parameters/mem_address 2>/dev/null || echo "ramoops not loaded"
cat /sys/module/ramoops/parameters/mem_size 2>/dev/null
cat /sys/module/ramoops/parameters/console_size 2>/dev/null
cat /sys/module/ramoops/parameters/pmsg_size 2>/dev/null
cat /sys/module/ramoops/parameters/dump_oops 2>/dev/null

# 查看当前内核启动参数中的 ramoops 配置
cat /proc/cmdline | grep -o 'ramoops[^ ]*'

# 查看所有 pstore 相关参数
ls /sys/module/pstore/parameters/ 2>/dev/null
cat /sys/module/pstore/parameters/max_reason 2>/dev/null
```


```bash
# 查看 pstore 目录内容
ls -la /sys/fs/pstore/

# 读取内核崩溃日志（如果有）
cat /sys/fs/pstore/dmesg-ramoops-0 2>/dev/null | head -100

# 读取控制台日志
cat /sys/fs/pstore/console-ramoops-0 2>/dev/null | head -100

# 读取用户态消息
cat /sys/fs/pstore/pmsg-ramoops-0 2>/dev/null | head -50

# 查看所有记录文件
find /sys/fs/pstore/ -type f -exec ls -la {} \;
```

#### 4. 手动触发测试（需谨慎）

```bash
# ===== 警告：以下操作会导致系统崩溃，仅用于测试环境 =====

# 方法 1: 通过 sysrq 触发 panic（需确保 pstore 已配置）
# echo c | sudo tee /proc/sysrq-trigger

# 方法 2: 通过内核模块触发（更安全的方式，可以测试 pstore 写入）
# 编译一个触发 oops 的内核模块进行测试

# 重启后检查崩溃是否被记录
sudo ls -la /sys/fs/pstore/
sudo cat /sys/fs/pstore/dmesg-ramoops-0 2>/dev/null
```

#### 5. pstore-blk 配置验证

```bash
# 检查是否配置了 pstore-blk 后端
grep CONFIG_PSTORE_BLK /boot/config-$(uname -r)

# 查看块设备后端配置
cat /sys/module/pstore_blk/parameters/blkdev 2>/dev/null

# 检查是否有专用的 pstore 分区
lsblk | grep -i pstore
blkid | grep -i pstore
```

#### 6. EFI/ACPI ERST 后端检查

```bash
# 检查 EFI 变量存储支持
grep CONFIG_EFI_VARS_PSTORE /boot/config-$(uname -r) 2>/dev/null

# 检查 ACPI ERST 支持
lsmod | grep -E '(erst|apei)'
dmesg | grep -i erst

# 查看 ACPI ERST 相关日志
dmesg | grep -i "pstore.*erst"
```

#### 7. systemd-pstore 服务验证

```bash
# 检查服务状态
systemctl status systemd-pstore.service

# 查看服务配置
cat /etc/systemd/pstore.conf 2>/dev/null || echo "Using default config"

# 手动触发 pstore 归档
sudo systemctl start systemd-pstore.service

# 查看归档的日志
ls -la /var/lib/systemd/pstore/ 2>/dev/null

# 查看 journal 中的 pstore 记录
journalctl -u systemd-pstore.service
```

#### 8. 模拟 pstore 写入（不触发崩溃）

```bash
# 如果配置了 pmsg，可以写入用户态消息
# 需加载 pstore_pmsg 模块
lsmod | grep pmsg

# 检查 pmsg 设备
cat /sys/fs/pstore/pmsg-ramoops-0 2>/dev/null

# 通过 /dev/pmsg0 写入（如果存在）
# echo "test message" | sudo tee /dev/pmsg0 2>/dev/null || echo "pmsg not available"
```

#### 9. 清理 pstore 记录

```bash
# 查看当前记录占用的空间
sudo du -sh /sys/fs/pstore/

# 删除所有记录（释放空间）
sudo rm -f /sys/fs/pstore/*

# 验证清理成功
ls -la /sys/fs/pstore/
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
