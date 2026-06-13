# Linux Pstore Crash Dump 深度分析

## 一句话总结

Pstore 是 Linux 内核的持久化存储机制，利用非易失性内存(NVM)或预留 RAM 在系统崩溃时保存内核日志、panic 信息等关键调试数据，供重启后分析。

---

## 1. 高屋建瓴：为什么存在？解决了什么根本问题？

### 1.1 核心问题：系统崩溃时的"黑盒"困境

```
┌─────────────────────────────────────────────────────────────────────┐
│                    传统崩溃调试的问题                                │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  场景: 服务器运行中突然崩溃，自动重启后...                           │
│                                                                     │
│  ┌──────────────┐          ┌──────────────┐          ┌───────────┐ │
│  │  正常运行    │   Crash  │  重启过程    │  Boot    │  运行中   │ │
│  │              │ ───────→ │              │ ───────→ │           │ │
│  │  dmesg 完整  │          │  内存清零    │          │ dmesg 丢失 │ │
│  └──────────────┘          └──────────────┘          └───────────┘ │
│                                                                     │
│  问题:                                                              │
│  • 崩溃瞬间的关键日志在内存中，重启后丢失                           │
│  • 无法获知崩溃原因 (oops/panic/hang)                               │
│  • 无法分析调用栈、寄存器状态                                        │
│  • 间歇性崩溃更难定位 (无法现场调试)                                 │
│                                                                     │
│  传统解决方案的局限:                                                 │
│  • netconsole: 依赖网络，崩溃时网络可能不可用                        │
│  • serial console: 依赖物理串口，云环境不可用                        │
│  • disk logging: 崩溃时文件系统可能已损坏                            │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.2 Pstore 的解决方案

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Pstore 的工作原理                                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  核心思想: 使用非易失性存储保存崩溃信息                              │
│                                                                     │
│  ┌─────────────────┐      ┌─────────────────┐      ┌─────────────┐ │
│  │   正常运行      │      │    崩溃发生     │      │   重启后    │ │
│  │                 │      │                 │      │             │ │
│  │ 注册 kmsg_dump  │─────→│ panic/oops 处理  │─────→│ 挂载 pstore │ │
│  │ 回调函数        │      │                 │      │ 文件系统    │ │
│  └─────────────────┘      │ 调用 pstore_dump│      │             │ │
│                           │                 │      │ 读取持久化  │ │
│                           │ 写入非易失存储  │      │ 数据        │ │
│                           │                 │      │             │ │
│                           │ • console log   │      │ • dmesg     │ │
│                           │ • panic message │      │ • panic log │ │
│                           │ • ftrace        │      │ • ftrace    │ │
│                           │ • pmsg          │      │ • pmsg      │ │
│                           └─────────────────┘      └─────────────┘ │
│                                                                     │
│  关键特性:                                                          │
│  • 不依赖工作文件系统                                               │
│  • 不依赖网络/串口                                                  │
│  • 支持多种后端存储 (RAM/ACPI ERST/EFI var/MTD/block)               │
│  • 崩溃时原子写入，保证数据完整性                                     │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.3 技术演进与历史

```
时间线:
2007 ─ Google 贡献 RAM console 实现
  │
2010 ─ Intel 贡献 pstore 框架 (Tony Luck)
  │    • 统一接口，支持多种后端
  │    • 集成到内核主线 2.6.39
  │
2012 ─ 添加 ramoops (RAM Oops/Panic logger)
  │
2014 ─ 添加 pstore block (block device 后端)
  │
2016 ─ 添加 ftrace 支持
  │
2018 ─ 添加 zstd 压缩支持
  │
2020 ─ 添加 pmsg (userspace message) 支持
  │
2023 ─ 完善 EFI secret storage 支持

架构演进:
┌─────────────────────────────────────────────────────────────────────┐
│  早期实现 (RAM console)                                             │
│  • 直接操作预留 RAM 区域                                             │
│  • 无统一接口，每个平台各自实现                                       │
├─────────────────────────────────────────────────────────────────────┤
│  Pstore 框架 (现代)                                                  │
│  • 统一 struct pstore_info 接口                                      │
│  • 支持多种后端 (ram, block, efi, acpi, mtd, ...)                   │
│  • 文件系统集成 (/sys/fs/pstore)                                     │
│  • 支持压缩、ECC、分区管理                                            │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.4 解决的问题与代价

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Pstore 的价值                                     │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  解决的问题:                                                        │
│  ✓ 崩溃现场信息丢失                                                  │
│  ✓ 无法事后分析                                                      │
│  ✓ 远程设备调试困难                                                  │
│  ✓ 间歇性问题难定位                                                  │
│                                                                     │
│  带来的新能力:                                                       │
│  ✓ 自动收集崩溃日志                                                  │
│  ✓ 云端/嵌入式设备远程诊断                                           │
│  ✓ 大规模集群崩溃分析                                                │
│  ✓ 硬件故障定位 (通过 mce 记录)                                       │
│                                                                     │
├─────────────────────────────────────────────────────────────────────┤
│                    权衡与限制                                        │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  代价/限制:                                                         │
│  • 需要预留非易失存储空间 (RAM/Flash)                                │
│  • 存储容量有限 (通常 KB 到 MB 级别)                                  │
│  • 写入性能受限 (特别是 Flash 后端)                                  │
│  • 某些后端需要固件/BIOS 支持                                        │
│  • 安全考虑: 敏感信息可能泄露                                         │
│                                                                     │
│  不适用场景:                                                        │
│  ✗ 需要持续日志记录 (非崩溃场景)                                     │
│  ✗ 大容量日志存储                                                    │
│  ✗ 需要高速写入                                                      │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 2. 核心概念：关键数据结构、算法、机制

### 2.1 整体架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                         Pstore 架构                                  │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  用户空间                    内核空间                                │
│  ┌──────────────┐           ┌─────────────────────────────────────┐│
│  │ /sys/fs/pstore│←─────────│        Pstore 文件系统层            ││
│  │              │  read()   │         (fs/pstore/inode.c)         ││
│  │ dmesg-ramoops-0│         │              ↓                      ││
│  │ panic-ramoops-1│         │    ┌──────────────────┐             ││
│  │ ftrace-ramoops-│         │    │  Pstore 核心层   │             ││
│  └──────────────┘         │    │ (fs/pstore/platform.c)│          ││
│       ↑                    │    │                  │             ││
│       │ mount              │    │ • kmsg_dump 回调 │             ││
│       │                    │    │ • 压缩/解压      │             ││
│  ┌──────────────┐          │    │ • 记录管理       │             ││
│  │ systemd-pstore│         │    └────────┬─────────┘             ││
│  │   服务       │          │             ↓                       ││
│  │ (存档/清理)  │          │    ┌──────────────────┐             ││
│  └──────────────┘          │    │   后端驱动层      │             ││
│                            │    │                  │             ││
│                            │    │ ┌──┐ ┌──┐ ┌──┐ │             ││
│                            │    │ │ram││blk││efi│ │...          ││
│                            │    │ └──┘ └──┘ └──┘ │             ││
│                            │    │   多种后端实现   │             ││
│                            │    └──────────────────┘             ││
│                            └─────────────────────────────────────┘│
│                                                                     │
│  硬件层:                                                            │
│  • 预留 RAM (ramoops)                                               │
│  • ACPI ERST (Error Record Serialization Table)                     │
│  • EFI Variables                                                     │
│  • MTD (NAND/NOR Flash)                                             │
│  • Block Device                                                      │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 2.2 关键数据结构

#### 2.2.1 pstore_info - 后端注册接口

```c
// include/linux/pstore.h
struct pstore_info {
    struct module *owner;           // 模块所有者
    const char *name;               // 后端名称 ("ramoops", "efi", ...)
    
    // 核心回调函数
    int (*open)(struct pstore_info *psi);           // 打开设备
    int (*close)(struct pstore_info *psi);          // 关闭设备
    ssize_t (*read)(struct pstore_record *record);  // 读取记录
    int (*write)(struct pstore_record *record);     // 写入记录 (可选)
    int (*write_user)(struct pstore_record *record, // 从用户空间写入
                      const char __user *buf);
    int (*erase)(struct pstore_record *record);     // 擦除记录
    
    // 设备信息
    struct pstore_buffer *buf;      // 缓冲区 (某些后端使用)
    size_t bufsize;                 // 缓冲区大小
    
    // 支持的数据类型位图
    struct pstore_zone_info *zone;  // 分区信息 (zone 后端)
    void *data;                     // 后端私有数据
    
    // 互斥锁
    struct mutex read_mutex;        // 读操作互斥
    raw_spinlock_t buf_lock;        // 缓冲区自旋锁
};
```

#### 2.2.2 pstore_record - 记录抽象

```c
// include/linux/pstore.h
struct pstore_record {
    struct pstore_info *psi;        // 所属后端
    enum pstore_type_id type;       // 记录类型
    u64 id;                         // 记录 ID (通常是时间戳或序号)
    ssize_t size;                   // 数据大小
    char *buf;                      // 数据缓冲区
    struct timespec64 time;         // 时间戳
    struct pstore_private *priv;    // 私有数据
    bool compressed;                // 是否压缩
    int flags;                      // 标志位
};

// 记录类型枚举
enum pstore_type_id {
    PSTORE_TYPE_DMESG = 0,      // 内核 dmesg (oops/panic)
    PSTORE_TYPE_MCE,            // Machine Check Exception
    PSTORE_TYPE_CONSOLE,        // Console 输出
    PSTORE_TYPE_FTRACE,         // Ftrace 日志
    PSTORE_TYPE_RTAS,           // PowerPC RTAS
    PSTORE_TYPE_POWERPC_OFW,    // PowerPC Open Firmware
    PSTORE_TYPE_POWERPC_COMMON, // PowerPC 通用
    PSTORE_TYPE_PMSG,           // Userspace pmsg
    PSTORE_TYPE_POWERPC_OPAL,   // PowerPC OPAL
    PSTORE_TYPE_MAX
};
```

#### 2.2.3 ramoops_context - RAM 后端上下文

```c
// drivers/pstore/ram.c
struct ramoops_context {
    // 分区区域 (支持多种类型)
    struct persistent_ram_zone **dprzs;  // Oops/panic dump zones
    struct persistent_ram_zone *cprz;    // Console zone
    struct persistent_ram_zone **fprzs;  // Ftrace zones
    struct persistent_ram_zone *mprz;    // Pmsg zone
    
    // 内存配置
    phys_addr_t phys_addr;      // 物理地址
    unsigned long size;         // 总大小
    unsigned int memtype;       // 内存类型 (uncached/write-combined)
    
    // 分区大小
    size_t record_size;         // 每个 dump 记录大小
    size_t console_size;        // console 缓冲区大小
    size_t ftrace_size;         // ftrace 缓冲区大小
    size_t pmsg_size;           // pmsg 缓冲区大小
    
    // ECC 支持
    struct persistent_ram_ecc_info ecc_info;
    
    // 计数器
    unsigned int max_dump_cnt;      // 最大 dump 数量
    unsigned int dump_write_cnt;    // 已写入 dump 数
    unsigned int dump_read_cnt;     // 已读取 dump 数
    
    struct pstore_info pstore;  // 注册到 pstore 框架
};
```

#### 2.2.4 persistent_ram_zone - 持久化 RAM 区域

```c
// drivers/pstore/ram_core.c
struct persistent_ram_zone {
    // 元数据 (存储在 RAM 头部)
    struct persistent_ram_buffer *buffer;   // 缓冲区头
    
    // 内存映射
    void *vaddr;                // 虚拟地址
    size_t buffer_size;         // 缓冲区大小
    
    // 状态
    u32 flags;
    bool ecc;
    size_t buffer_size_kaddr;   // 内核可访问大小
    
    // 当前数据
    char *old_log;              // 历史日志 (上一次启动)
    size_t old_log_size;        // 历史日志大小
    size_t old_log_footer;      // 尾部大小
    
    // ECC 缓冲区
    unsigned int ecc_block_size;
    unsigned int ecc_size;
    unsigned int ecc_symsize;
    unsigned int ecc_poly;
    char *ecc_buffer;
    char *par_buffer;
    
    enum pstore_type_id type;   // 区域类型
};

// RAM 缓冲区头部结构 (存在于预留 RAM 中)
struct persistent_ram_buffer {
    u32 sig;                    // 签名 (验证有效性)
    atomic_t start;             // 写入起始位置
    atomic_t size;              // 当前数据大小
    u8 data[];                  // 实际数据 (柔性数组)
};
```

### 2.3 核心机制

#### 2.3.1 崩溃写入流程

```
┌─────────────────────────────────────────────────────────────────────┐
│              Panic/Oops 时的写入流程                                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  触发源:                                                            │
│  • oops (内核异常)                                                  │
│  • panic (主动崩溃)                                                 │
│  • sysrq-trigger (手动触发)                                         │
│  • BUG_ON/WARN_ON                                                   │
│  • MCE (硬件错误)                                                   │
│       ↓                                                             │
│  kmsg_dump() 框架调用                                                │
│       ↓                                                             │
│  pstore_dump() [fs/pstore/platform.c]                               │
│       │                                                             │
│       ├── 1. 确定 dump 原因 (panic/oops/...)                         │
│       │                                                             │
│       ├── 2. 获取锁 (考虑 NMI/Panic 路径的特殊处理)                   │
│       │   • panic/NMI 路径: 使用 raw_spin_trylock (可能失败跳过)     │
│       │   • 其他路径: raw_spin_lock_irqsave                          │
│       │                                                             │
│       ├── 3. 迭代写入 (分块处理)                                      │
│       │   while (还有数据) {                                         │
│       │       psinfo->write(record)   // 后端写入                    │
│       │       record.id++             // 递增 ID                     │
│       │   }                                                         │
│       │                                                             │
│       └── 4. 释放锁，标记新条目                                      │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

#### 2.3.2 持久化 RAM 写入算法

```c
// drivers/pstore/ram_core.c
// 核心思想: 环形缓冲区 + 原子指针更新

// 写入数据到持久化 RAM
static void persistent_ram_write(struct persistent_ram_zone *prz,
                                 const void *s, unsigned int count)
{
    struct persistent_ram_buffer *buffer = prz->buffer;
    unsigned int start = atomic_read(&buffer->start);  // 当前起始位置
    unsigned int buf_size = prz->buffer_size;
    
    // 环形缓冲区写入
    // 如果空间不足，覆盖最旧的数据
    if (count > buf_size) {
        s += count - buf_size;  // 跳过超出的旧数据
        count = buf_size;
    }
    
    // 计算写入位置
    unsigned int pos = (start + atomic_read(&buffer->size)) % buf_size;
    
    // 分两次写入处理环形边界
    if (pos + count <= buf_size) {
        // 一次性写入
        memcpy(buffer->data + pos, s, count);
    } else {
        // 分两段写入
        unsigned int first = buf_size - pos;
        memcpy(buffer->data + pos, s, first);
        memcpy(buffer->data, s + first, count - first);
    }
    
    // 更新元数据
    if (atomic_read(&buffer->size) + count <= buf_size) {
        atomic_add(count, &buffer->size);
    } else {
        // 缓冲区已满，更新起始位置
        unsigned int excess = atomic_read(&buffer->size) + count - buf_size;
        atomic_set(&buffer->start, (start + excess) % buf_size);
        atomic_set(&buffer->size, buf_size);
    }
    
    // 内存屏障，确保数据先写入
    smp_wmb();
}
```

**环形缓冲区可视化**:
```
初始状态:
┌────────────────────────────────────────────────────────────┐
│ buffer_size = 64KB                                         │
│ start = 0                                                  │
│ size = 0                                                   │
│                                                            │
│ [empty]                                                    │
│  ↑ start                                                   │
└────────────────────────────────────────────────────────────┘

写入 20KB 后:
┌────────────────────────────────────────────────────────────┐
│ start = 0                                                  │
│ size = 20KB                                                │
│                                                            │
│ [DDDDDDDDDDDDDDDDDDDD][empty]                              │
│  ↑ start                ↑ start + size                     │
└────────────────────────────────────────────────────────────┘

写入 60KB 后 (快满):
┌────────────────────────────────────────────────────────────┐
│ start = 0                                                  │
│ size = 64KB (已满)                                         │
│                                                            │
│ [DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD] │
│  ↑ start                                                   │
└────────────────────────────────────────────────────────────┘

继续写入 10KB (覆盖最旧数据):
┌────────────────────────────────────────────────────────────┐
│ start = 10KB                                               │
│ size = 64KB (已满)                                         │
│                                                            │
│ [WWWWWWWWWWDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD]     │
│            ↑ start                                         │
│  ↑ wrap point                                              │
│  (W = 新写入, D = 旧数据)                                   │
└────────────────────────────────────────────────────────────┘
```

#### 2.3.3 压缩机制

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Pstore 压缩机制                                   │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  问题: 预留 RAM/Flash 空间有限，完整 dmesg 可能存不下               │
│                                                                     │
│  解决方案: 压缩 (默认 zlib deflate)                                  │
│                                                                     │
│  压缩流程:                                                          │
│  ┌──────────┐     ┌──────────┐     ┌──────────┐     ┌──────────┐   │
│  │ dmesg    │────→│ zlib     │────→│ 压缩数据 │────→│ pstore   │   │
│  │ 缓冲区   │     │ deflate  │     │          │     │ 后端     │   │
│  └──────────┘     └──────────┘     └──────────┘     └──────────┘   │
│       ~100KB          压缩率            ~30KB                         │
│                    (文本通常 60-70%)                                   │
│                                                                     │
│  实现细节:                                                          │
│  • 压缩缓冲区: big_oops_buf (psinfo->bufsize * 100 / 60)            │
│  • 压缩成功: 存储压缩数据，标记 compressed = true                   │
│  • 压缩失败: 回退到未压缩存储                                        │
│  • 读取时: 自动解压                                                  │
│                                                                     │
│  注意事项:                                                          │
│  • panic 路径避免复杂压缩 (可能嵌套panic)                            │
│  • 优先使用预留缓冲区，避免动态分配                                  │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

#### 2.3.4 ECC 错误校验

```c
// drivers/pstore/ram_core.c
// ECC 支持用于检测和纠正 RAM 中的位翻转

struct persistent_ram_ecc_info {
    int ecc_size;       // ECC 计算块大小
    int ecc_symsize;    // 符号大小
    int ecc_poly;       // 生成多项式
};

// 写入时计算 ECC
void persistent_ram_update_ecc(struct persistent_ram_zone *prz, 
                               unsigned int start, unsigned int count)
{
    unsigned int i, num;
    unsigned char *block;
    
    // 按块计算 ECC
    for (i = 0; i < count; i += prz->ecc_block_size) {
        block = prz->buffer->data + start + i;
        num = min(prz->ecc_block_size, count - i);
        
        // 计算 Reed-Solomon ECC
        encode_rs8(prz->rs_decoder, block, num, 
                   prz->par_buffer, 0);
        
        // 存储 ECC 到校验区
        memcpy(prz->ecc_buffer + (start + i) * prz->ecc_size / prz->ecc_block_size,
               prz->par_buffer, prz->ecc_size);
    }
}

// 读取时验证和纠正
int persistent_ram_decode_ecc(struct persistent_ram_zone *prz, 
                              void *data, size_t len)
{
    int numerr;
    
    // 尝试纠错
    numerr = decode_rs8(prz->rs_decoder, data, 
                        prz->par_buffer, len, NULL, 0, NULL, 0, NULL);
    
    if (numerr > 0) {
        pr_warn("ramoops: corrected %d errors\n", numerr);
    } else if (numerr < 0) {
        pr_err("ramoops: uncorrectable errors!\n");
    }
    
    return numerr;
}
```

### 2.4 后端类型对比

```
┌───────────────────────────────────────────────────────────────────────────────┐
│                          Pstore 后端对比                                       │
├─────────────┬────────────────┬─────────────┬─────────────┬────────────────────┤
│ 后端类型    │ 配置示例       │ 容量        │ 速度        │ 适用场景           │
├─────────────┼────────────────┼─────────────┼─────────────┼────────────────────┤
│ ramoops     │ mem=128M@0x... │ 64KB-16MB   │ 内存速度    │ 嵌入式/物理机      │
│ (预留RAM)   │ ramoops.size=  │             │             │ 最常用             │
├─────────────┼────────────────┼─────────────┼─────────────┼────────────────────┤
│ pstore-block│ blkdev=/dev/   │ MB-GB       │ 块设备速度  │ 有可用分区时       │
│ (块设备)    │ mmcblk0p5      │             │             │                    │
├─────────────┼────────────────┼─────────────┼─────────────┼────────────────────┤
│ efi-pstore  │ 自动检测       │ 通常 ~64KB  │ NV 变量     │ UEFI 系统          │
│ (EFI变量)   │                │             │             │ 需固件支持         │
├─────────────┼────────────────┼─────────────┼─────────────┼────────────────────┤
│ acpi-erst   │ 自动检测       │ 通常 ~64KB  │ ACPI 接口   │ 服务器             │
│ (ACPI表)    │                │             │             │ 需 BIOS 支持       │
├─────────────┼────────────────┼─────────────┼─────────────┼────────────────────┤
│ mtd         │ 自动检测       │ 取决于 Flash│ Flash       │ 嵌入式设备         │
│ (MTD设备)   │                │             │             │ 需 MTD 支持        │
├─────────────┼────────────────┼─────────────┼─────────────┼────────────────────┤
│ zone        │ 分区配置       │ 可配置      │ 取决于底层  │ 大容量需求         │
│ (分区管理)  │                │             │             │                    │
└─────────────┴────────────────┴─────────────┴─────────────┴────────────────────┘
```

---

## 3. 源码实现：关键函数和调用链

### 3.1 模块初始化

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Pstore 初始化流程                                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  1. Pstore 框架初始化 (fs/pstore/platform.c)                        │
│                                                                     │
│     pstore_init()                                                   │
│     ├── 分配压缩工作区 (如果启用压缩)                                 │
│     ├── 注册 kmsg_dump 回调: kmsg_dump_register(&pstore_dumper)     │
│     └── 初始化 sysfs 接口                                           │
│                                                                     │
│  2. 文件系统注册 (fs/pstore/inode.c)                                │
│                                                                     │
│     pstore_fs_init()                                                │
│     └── register_filesystem(&pstore_fs_type)                        │
│         └── pstore_mount() 在 mount 时调用                          │
│             └── pstore_fill_super()                                 │
│                 └── 遍历所有记录，创建 inode                          │
│                                                                     │
│  3. 后端驱动初始化 (如 drivers/pstore/ram.c)                        │
│                                                                     │
│     ramoops_probe()                                                 │
│     ├── 解析 DT/命令行参数 (mem_address, mem_size, ...)             │
│     ├── 请求内存区域 request_mem_region()                            │
│     ├── ioremap() 映射预留 RAM                                       │
│     ├── 初始化 persistent_ram_zone 结构                             │
│     │   └── 检查签名，恢复旧数据                                     │
│     ├── 注册到 pstore 框架: pstore_register()                       │
│     └── 创建 platform 设备                                          │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

```c
// fs/pstore/platform.c - Pstore 核心注册
int pstore_register(struct pstore_info *psi)
{
    mutex_lock(&psinfo_lock);
    
    // 检查是否已注册
    if (psinfo) {
        mutex_unlock(&psinfo_lock);
        return -EBUSY;
    }
    
    // 验证必要回调
    if (!psi->read || !psi->open || !psi->close) {
        mutex_unlock(&psinfo_lock);
        return -EINVAL;
    }
    
    psinfo = psi;
    mutex_init(&psinfo->read_mutex);
    raw_spin_lock_init(&psinfo->buf_lock);
    
    // 分配压缩缓冲区
    allocate_buf_for_compression();
    
    // 注册 kmsg_dump 回调
    kmsg_dump_register(&pstore_dumper);
    
    mutex_unlock(&psinfo_lock);
    
    // 触发更新 (如果支持运行时更新)
    if (pstore_update_ms >= 0)
        schedule_work(&pstore_work);
    
    return 0;
}
```

### 3.2 崩溃写入调用链

```
┌─────────────────────────────────────────────────────────────────────┐
│                    Panic 写入详细流程                                │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  panic() [kernel/panic.c]                                           │
│  └── kmsg_dump(KMSG_DUMP_PANIC)                                     │
│      └── 遍历所有 dump 设备，调用 registered_dumpers                │
│          └── pstore_dump() [fs/pstore/platform.c]                   │
│              ├── pstore_record_init()  // 初始化记录结构             │
│              ├── 检查 pstore_cannot_block_path()                    │
│              │   └── panic 路径返回 true                            │
│              ├── raw_spin_trylock_irqsave(&psinfo->buf_lock)        │
│              │   └── 如果获取失败，直接返回 (避免死锁)               │
│              ├── kmsg_dump_rewind()  // 定位到 dmesg 末尾           │
│              └── while (还有数据) {                                 │
│                  ├── kmsg_dump_get_buffer()  // 获取一块数据         │
│                  ├── 尝试压缩 (如果启用)                              │
│                  │   └── pstore_compress()                          │
│                  │       └── zlib_deflate()                         │
│                  ├── psinfo->write(record)  // 后端写入              │
│                  │   └── ramoops_write()  [ram 后端]                │
│                  │       └── persistent_ram_write()                 │
│                  │           └── 写入到预留 RAM                      │
│                  └── record.id++                                    │
│              }                                                        │
│              └── raw_spin_unlock_irqrestore()                       │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 3.3 文件系统读取调用链

```
┌─────────────────────────────────────────────────────────────────────┐
│              读取 Pstore 文件流程                                    │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  用户空间: cat /sys/fs/pstore/dmesg-ramoops-0                       │
│                                                                     │
│  内核路径:                                                           │
│  vfs_read()                                                         │
│  └── pstore_file_read() [fs/pstore/inode.c]                         │
│      ├── 如果是 ftrace 类型: seq_read()                             │
│      └── 其他类型: simple_read_from_buffer()                        │
│          └── ps->record->buf (已经在 mount 时读取)                   │
│                                                                     │
│  Mount 时的预读取 (pstore_repopulate())                             │
│  └── pstore_get_records()                                           │
│      ├── psinfo->open()                                             │
│      └── while (psinfo->read(record) > 0) {                         │
│          ├── 读取记录到 record.buf                                  │
│          ├── 如果是压缩数据: 解压                                    │
│          │   └── pstore_decompress()                                │
│          ├── pstore_get_backend_record()                            │
│          └── pstore_mkfile()  // 创建 sysfs 文件                    │
│              └── 创建 inode，设置操作函数                            │
│      }                                                                │
│      └── psinfo->close()                                            │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

```c
// fs/pstore/inode.c - 创建 pstore 文件
static int pstore_mkfile(struct dentry *root, struct pstore_record *record)
{
    struct dentry *dentry;
    struct inode *inode;
    struct pstore_private *private;
    
    // 分配私有数据
    private = kzalloc(sizeof(*private), GFP_KERNEL);
    private->record = record;
    
    // 创建 inode
    inode = pstore_get_inode(root->d_sb);
    inode->i_private = private;
    inode->i_size = record->size;
    
    // 设置文件操作
    inode->i_fop = &pstore_file_operations;
    inode->i_mode = S_IFREG | 0444;  // 只读
    
    // 构建文件名: "type-backend-id"
    snprintf(name, sizeof(name), "%s-%s-%llu",
             pstore_type_to_name(record->type),
             record->psi->name, record->id);
    
    dentry = d_alloc_name(root, name);
    d_add(dentry, inode);
    
    return 0;
}
```

### 3.4 RAM 后端核心代码

```c
// drivers/pstore/ram.c - RAM 后端写入

static ssize_t ramoops_pstore_read(struct pstore_record *record)
{
    struct ramoops_context *cxt = record->psi->data;
    struct persistent_ram_zone *prz = NULL;
    ssize_t size = 0;
    
    // 根据类型和 ID 获取对应的 prz
    switch (record->type) {
    case PSTORE_TYPE_DMESG:
        prz = ramoops_get_next_prz(cxt->dprzs, cxt->dump_read_cnt++, ...);
        break;
    case PSTORE_TYPE_CONSOLE:
        prz = cxt->cprz;
        break;
    case PSTORE_TYPE_FTRACE:
        prz = ramoops_get_next_prz(cxt->fprzs, cxt->ftrace_read_cnt++, ...);
        break;
    case PSTORE_TYPE_PMSG:
        prz = cxt->mprz;
        break;
    default:
        return -EINVAL;
    }
    
    if (!prz)
        return 0;
    
    // 解析头部获取时间戳和压缩标志
    header_length = ramoops_read_kmsg_hdr(buffer, &record->time, &compressed);
    
    // 读取数据
    record->buf = kzalloc(prz->old_log_size, GFP_KERNEL);
    memcpy(record->buf, prz->old_log + header_length, 
           prz->old_log_size - header_length);
    
    record->size = prz->old_log_size - header_length;
    record->compressed = compressed;
    
    return record->size;
}

static int ramoops_pstore_write(struct pstore_record *record)
{
    struct ramoops_context *cxt = record->psi->data;
    struct persistent_ram_zone *prz;
    int header_len;
    char header_buf[HEADER_SIZE];
    
    // 选择目标 zone
    switch (record->type) {
    case PSTORE_TYPE_DMESG:
        if (cxt->dump_write_cnt >= cxt->max_dump_cnt)
            return -ENOSPC;
        prz = cxt->dprzs[cxt->dump_write_cnt++];
        break;
    // ... 其他类型
    }
    
    // 构建头部: "====timestamp-C\n" (C 表示压缩)
    header_len = snprintf(header_buf, sizeof(header_buf),
                          RAMOOPS_KERNMSG_HDR "%lld.%09lu-%c\n",
                          record->time.tv_sec, record->time.tv_nsec,
                          record->compressed ? 'C' : 'D');
    
    // 写入头部
    persistent_ram_write(prz, header_buf, header_len);
    
    // 写入数据
    persistent_ram_write(prz, record->buf, record->size);
    
    // 更新 ECC
    if (prz->ecc)
        persistent_ram_update_ecc(prz, 0, header_len + record->size);
    
    return 0;
}
```

---

## 4. 实际场景：生产环境中的应用和常见问题

### 4.1 配置示例

#### 4.1.1 基于预留 RAM 的配置

```bash
# 方法1: 命令行参数 (推荐)
# 在 GRUB 中添加:
linux /vmlinuz root=... memmap=4M\$0x30000000 ramoops.mem_address=0x30000000 ramoops.mem_size=0x400000

# 参数说明:
# memmap=4M\$0x30000000  - 预留 4MB 物理内存 (从 0x30000000 开始，\$ 需要转义)
# ramoops.mem_address    - ramoops 使用的物理地址
# ramoops.mem_size       - ramoops 总大小

# 可选参数:
# ramoops.record_size=0x10000   # 每个 panic 记录大小 (64KB)
# ramoops.console_size=0x20000  # console 缓冲区大小 (128KB)
# ramoops.ftrace_size=0x10000   # ftrace 缓冲区大小 (64KB)
# ramoops.pmsg_size=0x10000     # pmsg 缓冲区大小 (64KB)
# ramoops.ecc=1                 # 启用 ECC
# ramoops.mem_type=1            # 内存类型: 0=wc, 1=uc, 2=default
```

```c
// 方法2: Device Tree (ARM/嵌入式)
// arch/arm64/boot/dts/xxx.dts
reserved-memory {
    #address-cells = <2>;
    #size-cells = <2>;
    ranges;

    ramoops: ramoops@30000000 {
        compatible = "ramoops";
        reg = <0 0x30000000 0 0x400000>;  /* 4MB @ 0x30000000 */
        record-size = <0x10000>;           /* 64KB per record */
        console-size = <0x20000>;          /* 128KB console */
        ftrace-size = <0x10000>;           /* 64KB ftrace */
        pmsg-size = <0x10000>;             /* 64KB pmsg */
        ecc-size = <16>;                   /* ECC block size */
    };
};
```

#### 4.1.2 基于块设备的配置

```bash
# 准备分区 (需要 raw 访问，不能是文件系统)
# 创建一个 64MB 分区用于 pstore

# 加载模块时指定
modprobe pstore-blk blkdev=/dev/mmcblk0p5 kmsg_size=512 console_size=128

# 或使用 cmdline
pstore_blk.blkdev=/dev/sda5 pstore_blk.kmsg_size=1024
```

### 4.2 使用流程

```bash
# 1. 检查 pstore 是否挂载
$ mount | grep pstore
pstore on /sys/fs/pstore type pstore (rw,relatime)

# 2. 查看存储的崩溃记录
$ ls -la /sys/fs/pstore/
-r--r--r-- 1 root root 28456 Jan 15 10:23 dmesg-ramoops-0
-r--r--r-- 1 root root  8192 Jan 15 10:23 console-ramoops-0
-r--r--r-- 1 root root  4096 Jan 15 10:23 ftrace-ramoops-0

# 3. 读取崩溃日志
$ cat /sys/fs/pstore/dmesg-ramoops-0
====1556872345.123456789-D
[    0.000000] Linux version 5.15.0-...
...
[  123.456789] BUG: unable to handle page fault at 0000000000000000
[  123.456790] IP: [<ffffffff12345678>] bad_function+0x42/0x100
...
[  123.456800] Call Trace:
[  123.456801]  dump_stack+0x46/0x60
[  123.456802]  panic+0xd5/0x227
...

# 4. 配置 systemd 自动保存 (防止被覆盖)
$ systemctl enable systemd-pstore
$ systemctl start systemd-pstore

# 默认保存到 /var/lib/systemd/pstore/
$ ls /var/lib/systemd/pstore/
dmesg-ramoops-0  console-ramoops-0

# 5. 手动清理 (读取并确认保存后)
$ rm /sys/fs/pstore/dmesg-ramoops-0
```

### 4.3 生产环境问题排查

#### 问题1: Pstore 未挂载

```bash
# 症状: /sys/fs/pstore 不存在或为空

# 排查步骤:
# 1. 检查内核配置
grep PSTORE /boot/config-$(uname -r)
CONFIG_PSTORE=y
CONFIG_PSTORE_RAM=y
CONFIG_PSTORE_CONSOLE=y

# 2. 检查是否有后端注册
dmesg | grep -i pstore
[    0.000000] pstore: Registered ramoops as persistent store backend

# 如果没有，检查 ramoops 参数是否正确
# 检查预留内存是否成功
dmesg | grep -i "reserved"
[    0.000000] Reserving 4MB of memory at 768MB for ramoops

# 3. 检查内存预留 (x86)
cat /proc/iomem | grep ramoops
  30000000-303fffff : reserved  # 应该有这一行

# 4. 手动挂载
mount -t pstore pstore /sys/fs/pstore
```

#### 问题2: 崩溃后没有记录

```bash
# 症状: 系统崩溃重启后 /sys/fs/pstore 为空

# 排查步骤:
# 1. 确认 pstore 被触发
dmesg | grep -i "pstore\|ramoops\|panic"
# 应该看到 panic 处理和 pstore 写入日志

# 2. 检查签名是否有效 (ramoops)
# 签名损坏会导致记录被丢弃
dmesg | grep -i "ramoops.*signature"

# 3. 检查是否有空间
# 如果之前的记录未清理，可能没有空间写入新记录
ls -la /sys/fs/pstore/

# 4. 检查 ECC 错误
dmesg | grep -i "ramoops.*ecc"

# 5. 测试触发
# 手动触发 panic (谨慎操作!)
echo c > /proc/sysrq-trigger
# 重启后检查是否有记录
```

#### 问题3: 记录被覆盖

```bash
# 症状: 只看到最新的崩溃记录，旧的丢失

# 原因分析:
# 1. 分区大小不足，循环缓冲区覆盖
# 2. 多个 panic 在短时间内发生

# 解决方案:
# 1. 增大预留内存
# 2. 使用 systemd-pstore 服务自动保存

# 查看当前配置
cat /sys/module/ramoops/parameters/mem_size
4194304  # 4MB

# 增大到 16MB
# 修改 GRUB 参数: ramoops.mem_size=0x1000000
```

#### 问题4: 压缩失败

```bash
# 症状: 日志中显示 "pstore: compression failed"

# 可能原因:
# 1. 压缩缓冲区不足
# 2. panic 路径中内存分配失败

# 解决方案:
# 禁用压缩 (使用压缩反而可能失败)
modprobe ramoops compress=none

# 或在命令行:
ramoops.compress=none
```

### 4.4 监控和告警

```bash
#!/bin/bash
# /usr/local/bin/pstore-monitor.sh
# 监控 pstore 并发送告警

PSTORE_DIR="/sys/fs/pstore"
ALERT_DIR="/var/log/pstore-alerts"
mkdir -p "$ALERT_DIR"

# 检查新记录
for file in "$PSTORE_DIR"/*; do
    if [ -f "$file" ]; then
        filename=$(basename "$file")
        if [ ! -f "$ALERT_DIR/$filename" ]; then
            # 新记录!
            echo "New crash record found: $filename"
            echo "Timestamp: $(date)"
            echo "=== Content ==="
            head -50 "$file"
            
            # 发送到日志服务器或邮件
            # logger -t pstore-alert "Crash detected: $filename"
            
            # 保存副本
            cp "$file" "$ALERT_DIR/$filename"
            
            # 可选: 自动转存后删除，释放空间
            # mv "$file" /var/crash/
        fi
    fi
done
```

```bash
# Cron 定时检查
*/5 * * * * root /usr/local/bin/pstore-monitor.sh
```

---

## 5. 学习路径：如何系统掌握这个知识点

### 5.1 知识依赖图

```
                    ┌─────────────────────┐
                    │   Pstore 核心       │
                    └──────────┬──────────┘
                               │
          ┌────────────────────┼────────────────────┐
          ↓                    ↓                    ↓
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│   后端驱动层    │  │   核心框架层    │  │   用户态工具    │
│  (ram/block/efi)│  │ (platform/inode)│  │ (systemd-pstore)│
└────────┬────────┘  └────────┬────────┘  └────────┬────────┘
         │                    │                    │
         └────────────────────┼────────────────────┘
                              ↓
                    ┌─────────────────┐
                    │  基础依赖知识   │
                    │                 │
                    │ • 内存管理(MM)  │
                    │ • 文件系统(VFS) │
                    │ • 内核调试      │
                    │ • 平台驱动      │
                    │ • 设备树(DT)    │
                    └─────────────────┘
```

### 5.2 分阶段学习路径

#### 阶段1: 用户态使用 (1天)

```bash
# 实验1: 配置 ramoops
# 在虚拟机中测试

# 1. 修改 GRUB 配置
sudo grubby --update-kernel=ALL --args="memmap=4M\$0x30000000 ramoops.mem_address=0x30000000 ramoops.mem_size=0x400000"

# 2. 重启
sudo reboot

# 3. 验证挂载
ls /sys/fs/pstore/

# 4. 手动触发 panic (虚拟机中测试!)
echo c | sudo tee /proc/sysrq-trigger

# 5. 重启后查看记录
cat /sys/fs/pstore/dmesg-ramoops-0
```

#### 阶段2: 内核配置与调试 (2-3天)

```
阅读顺序:
1. Documentation/admin-guide/ramoops.rst
   - 官方文档，了解配置参数

2. fs/pstore/platform.c
   - pstore_dump() - 核心写入函数
   - pstore_register() - 注册接口
   - 理解 kmsg_dump 集成

3. fs/pstore/inode.c
   - pstore 文件系统实现
   - 理解用户态接口

4. drivers/pstore/ram.c
   - ramoops 驱动
   - 参数解析
   - 读写实现

5. drivers/pstore/ram_core.c
   - persistent_ram 核心
   - 环形缓冲区实现
   - ECC 支持
```

```c
// 实验2: 添加自定义 pstore 后端

#include <linux/pstore.h>
#include <linux/module.h>

static int my_pstore_open(struct pstore_info *psi)
{
    printk("My pstore backend opened\n");
    return 0;
}

static ssize_t my_pstore_read(struct pstore_record *record)
{
    // 实现读取逻辑
    return 0;
}

static int my_pstore_write(struct pstore_record *record)
{
    printk("Writing record type=%d size=%zu\n", record->type, record->size);
    // 实现写入逻辑
    return 0;
}

static struct pstore_info my_psinfo = {
    .owner = THIS_MODULE,
    .name = "mybackend",
    .open = my_pstore_open,
    .read = my_pstore_read,
    .write = my_pstore_write,
    .bufsize = 4096,
};

static int __init my_pstore_init(void)
{
    return pstore_register(&my_psinfo);
}

static void __exit my_pstore_exit(void)
{
    // unregister
}

module_init(my_pstore_init);
module_exit(my_pstore_exit);
```

#### 阶段3: 深入理解 (3-5天)

```
主题1: kmsg_dump 框架
- kernel/printk/printk.c
- kmsg_dump_register / kmsg_dump_unregister
- dump 原因: PANIC, OOPS, EMERG, RESTART, HALT, POWEROFF

主题2: 压缩实现
- 理解 zlib 在 panic 路径的使用限制
- 为什么使用 deflate 而不是 zstd (panic 路径简单性)

主题3: 多后端支持
- 如何同时支持 ramoops + efi-pstore?
- pstore 后端优先级

主题4: 安全考虑
- /sys/fs/pstore 文件权限
- 敏感信息过滤
- EFI secret storage (安全启动相关)
```

#### 阶段4: 高级应用 (持续)

```
主题1: 自定义记录类型
- 添加 PSTORE_TYPE_CUSTOM
- 用于保存特定驱动状态

主题2: 远程崩溃收集
- 结合 pstore + netconsole
- 云端崩溃分析系统

主题3: 嵌入式优化
- MTD/Flash 后端优化
- 磨损均衡考虑

主题4: 硬件辅助调试
- ARM Coresight
- Intel PT (Processor Trace)
- 与 pstore 集成
```

### 5.3 推荐学习资源

```
官方文档:

内核源码:
• fs/pstore/ - 核心框架
• drivers/pstore/ - 后端驱动
• include/linux/pstore.h - 接口定义

论文/博客:
• "Persistent Storage for a Persistent Kernel" - Tony Luck (LPC 2010)
• https://blogs.oracle.com/linux/post/pstore-linux-kernel-persistent-storage-file-system
• https://lwn.net/Articles/421209/ (Pstore introduction)

工具:
• systemd-pstore.service - 自动保存
• crash 工具分析 - 配合 vmcore
```

### 5.4 自测问题

```
Q1: 为什么 pstore 需要预留内存，而不是动态分配？
   A: 崩溃时内核可能已不稳定，动态分配可能失败；
      预留内存保证写入时有确定可用的空间。

Q2: ramoops 环形缓冲区如何防止被覆盖？
   A: 支持多个分区 (max_dump_cnt)，每个 panic 使用一个分区；
      分区满后停止写入，避免覆盖旧记录。

Q3: pstore 在 panic 路径中如何处理锁？
   A: 使用 raw_spin_trylock_irqsave，避免死锁；
      如果获取锁失败，跳过写入。

Q4: ECC 在 ramoops 中的作用是什么？
   A: 检测和纠正预留 RAM 中的位翻转；
      因为预留 RAM 可能长期不被刷新，易发生软错误。

Q5: 为什么 pstore 文件是只读的？
   A: 防止误删除重要的崩溃记录；
      删除操作需要调用后端特定的 erase() 函数。
```

---

## 6. 总结与思考

### 6.1 设计哲学

```
Pstore 的核心设计原则:

1. 简单性 > 功能性
   • 崩溃时环境不稳定，避免复杂逻辑
   • 有限的特性集，但保证可靠性

2. 可靠性 > 性能
   • 写入速度慢可以接受
   • 关键是崩溃时一定能写入

3. 标准化接口
   • 统一的后端注册接口
   • 不同硬件平台一致的用户体验

4. 分层设计
   • 核心框架与后端分离
   • 易于添加新的存储后端
```

### 6.2 适用场景决策树

```
是否需要 pstore?
│
├── 系统经常崩溃/不稳定？
│   ├── 是 → 启用 pstore
│   └── 否 → 继续评估
│
├── 是嵌入式/远程设备？
│   ├── 是 → 启用 pstore (难以现场调试)
│   └── 否 → 继续评估
│
├── 需要分析间歇性问题？
│   ├── 是 → 启用 pstore
│   └── 否 → 可选
│
└── 有可用非易失存储？
    ├── 是 → 启用 pstore
    └── 否 → 考虑 netconsole/serial
```

### 6.3 未来趋势

```
Pstore 发展方向:

1. 更大容量
   • 新型 NV 内存 (Intel Optane 等)
   • 分区管理优化

2. 更丰富数据
   • eBPF 程序状态
   • 内核性能计数器
   • 硬件调试信息

3. 云端集成
   • 自动上传崩溃信息
   • 集中化分析平台

4. 安全增强
   • 加密存储
   • 安全擦除
   • 访问控制
```

---

**参考源码**: `/home/martins3/data/vn/docs/kernel/tutorial/crash/pstore.md`
**内核版本**: Linux 6.x
**文档日期**: 2025-02

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
