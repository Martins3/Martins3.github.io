- https://www.kernel.org/doc/html/latest/filesystems/autofs.html

当访问的时候，
自动 mount ，此外，不用的时候，自动 umount，
主要是给 nfs 之类的节省带宽。

## autofs 的作用
<!-- e308a335-027d-4407-bdb2-d3db88897571 -->

https://lwn.net/Articles/703785/

对网络存储的需求催生了 **NBD（网络块设备）** 和 **NFS（网络文件系统）**。与 PTY 不同，它们不仅提供用户空间接口供网络服务使用，还会**主动建立网络连接**并定义协议来传输数据和控制信息。这样设计的主要原因是：若由用户空间程序管理存储服务，可能引发死锁。例如，当该程序需要分配内存时，内核可能尝试通过写入存储设备释放内存，而该设备恰好又由该程序管理——死锁由此产生。因此，绕过用户空间、直接通过网络传输更安全。

尽管如此，NBD 和 NFS 仍可作为下游接口：它们能实例化块设备（NBD）或文件系统（NFS），并为其提供服务。这一机制已被 `amd`（后改名 `am-utils`）等自动挂载工具有效利用：这些工具呈现为一个仅包含目录和符号链接的 NFS 文件系统（避免死锁），并在首次访问时透明地挂载真实文件系统。

不过，NFS 并不完美：由于与 Linux 虚拟文件系统层（VFS）的交互受限，真实文件系统必须挂载在其他位置，NFS 中只能提供指向该位置的符号链接。为解决此问题，Linux 提供了专用的下游接口 **autofs**，支持 VFS 所需的额外交互，从而可直接在目录上自动挂载文件系统。

(我认为这个分析是不错的，就是试验到时候补充一下)
# autofs 深度分析：按需自动挂载机制

## 一句话总结

autofs 是 Linux 内核提供的**按需自动挂载（on-demand mount）**机制，通过在访问时才挂载远程文件系统（如 NFS）、空闲时自动卸载，解决了**系统启动时挂载依赖导致的死锁问题**和**网络资源长期占用问题**。


## 一、高屋建瓴：为什么需要 autofs？

### 1.1 根本问题：传统挂载的两大困境

#### 困境 1：启动时挂载的死锁风险

```
┌─────────────────────────────────────────────────────────────────────┐
│              传统 NFS 挂载的死锁问题示意图                             │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  场景：fstab 配置:                                                   │
│  ─────────────────                                                  │
│  nfs-server:/home  /home  nfs  defaults  0  0                       │
│                                                                     │
│  启动时会发生什么？                                                  │
│  ═══════════════════                                                │
│                                                                     │
│  ┌─────────────┐              网络不可用              ┌────────────┐ │
│  │  mount -a   │ ───────────────────────────────────▶ │  阻塞等待   │ │
│  │  (fstab)    │                                      │  网络恢复   │ │
│  └──────┬──────┘                                      └────────────┘ │
│         │                                                           │
│         ▼                                                           │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │                      更严重的问题：死锁                          │ │
│  ├────────────────────────────────────────────────────────────────┤ │
│  │                                                                │ │
│  │  1. 某个服务 S 需要访问 /data (NFS)                            │ │
│  │  2. /data 未挂载，触发 NFS 挂载                                 │ │
│  │  3. NFS 挂载需要解析域名，触发 DNS 查询                          │ │
│  │  4. DNS 服务需要写入日志到 /var/log                             │ │
│  │  5. /var/log 是独立分区，需要挂载...                            │ │
│  │  6. 如果挂载 /var/log 又依赖服务 S → 死锁！                     │ │
│  │                                                                │ │
│  │  结果：系统启动卡住，或进入紧急模式                               │ │
│  │                                                                │ │
│  └────────────────────────────────────────────────────────────────┘ │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

#### 困境 2：资源长期占用

```
┌─────────────────────────────────────────────────────────────────────┐
│                    NFS 资源占用问题                                  │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  场景：企业环境，1000+ 台服务器挂载 NAS                              │
│  ─────────────────────────────────────────                          │
│                                                                     │
│  问题 1: 连接数爆炸                                                  │
│  ═════════════════════                                               │
│  • 每台服务器启动时挂载 10 个 NFS 共享                                │
│  • NAS 需要维护 1000 × 10 = 10,000 个并发连接                         │
│  • 大部分连接长期空闲，占用 NAS 资源                                  │
│                                                                     │
│  问题 2: 网络风暴                                                    │
│  ═══════════════════                                                 │
│  • 所有服务器同时启动，同时发起挂载请求                               │
│  • 网络拥塞，DNS/NIS 服务器被压垮                                    │
│  • 部分挂载超时，服务启动失败                                        │
│                                                                     │
│  问题 3: 故障扩散                                                    │
│  ═══════════════════                                                 │
│  • NAS 暂时不可用，所有服务器挂起                                     │
│  • df 命令卡住，ps 无法显示完整信息                                   │
│  • 系统监控失效，无法定位问题                                        │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.2 autofs 的核心价值

```
┌─────────────────────────────────────────────────────────────────────┐
│                    autofs 解决方案                                   │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  解决死锁：触发式挂载                                                │
│  ═══════════════════════                                            │
│                                                                     │
│  启动时:                                                            │
│  ┌──────────┐    启动 autofs    ┌──────────┐                       │
│  │ 系统启动 │ ───────────────▶  │ autofs   │ ← 不实际挂载 NFS       │
│  └──────────┘                   │ 守护进程  │                        │
│                                 └────┬─────┘                        │
│                                      │                              │
│  运行时:                            等待访问触发                     │
│                                 ┌────┴─────┐                        │
│                                 │ 用户访问  │                        │
│                                 │ /nfs/data│                        │
│                                 └────┬─────┘                        │
│                                      │                              │
│                                 ┌────┴─────┐                        │
│                                 │ autofs   │ 实际执行挂载            │
│                                 │ 拦截请求 │ mount nfs-server:/data  │
│                                 └──────────┘                        │
│                                                                     │
│  解决资源占用：空闲卸载                                              │
│  ═════════════════════════                                          │
│                                                                     │
│  ┌────────────────────────────────────────────────────────────────┐ │
│  │                    autofs 超时机制                              │ │
│  ├────────────────────────────────────────────────────────────────┤ │
│  │                                                                │ │
│  │   访问挂载点 ──▶ 自动挂载 ──▶ 使用 ──▶ 空闲 ──▶ 超时卸载         │ │
│  │       ↑                                          │             │ │
│  │       └───────────── 再次访问 ───────────────────┘             │ │
│  │                                                                │ │
│  │   默认超时: 5 分钟 (可配置)                                      │ │
│  │   效果: 连接数大幅减少，资源按需分配                              │ │
│  │                                                                │ │
│  └────────────────────────────────────────────────────────────────┘ │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.3 技术演进历史

```
┌─────────────────────────────────────────────────────────────────────┐
│                    autofs 演进时间线                                 │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  1989  ───────────────────────────────────────────────────────────► │
│   │                                                                 │
│   ├─ Sun NFS automounter (amd) 首次出现                              │
│   │   • 用户态实现                                                   │
│   │   • 通过 NFS 协议模拟符号链接                                    │
│   │   • 解决死锁问题，但性能受限                                     │
│   │                                                                 │
│  1992  ├─ Transarc automounter (AFS)                                │
│   │   • 为 AFS 设计的自动挂载                                        │
│   │                                                                 │
│  1997  ├─ Linux amd (am-utils) 移植                                 │
│   │   • 用户态实现                                                   │
│   │   • 配置复杂，功能丰富                                           │
│   │                                                                 │
│  1999  ├─ Linux 内核 autofs v1                                      │
│   │   • 内核态实现开始                                               │
│   │   • 使用 /proc 接口                                              │
│   │   • 功能有限，存在竞争条件                                       │
│   │                                                                 │
│  2001  ├─ Linux 内核 autofs v3                                      │
│   │   • 改进的协议                                                   │
│   │   • 支持直接挂载（direct mount）                                 │
│   │                                                                 │
│  2003  ├─ Linux 内核 autofs v4 (当前主流)                            │
│   │   • 全新的通信协议                                               │
│   │   • 使用 /dev/autofs 设备                                        │
│   │   • 支持 expire（自动卸载）                                      │
│   │   • 支持嵌套挂载                                                 │
│   │                                                                 │
│  2009  ├─ autofs v5 (用户态守护进程)                                 │
│   │   • 全新的用户态实现                                             │
│   │   • 支持更多文件系统类型                                         │
│   │   • 改进的 LDAP/NIS 支持                                         │
│   │                                                                 │
│  2015  ├─ 内核态 autofs 持续改进                                     │
│   │   • 性能优化                                                     │
│   │   • namespace 支持                                               │
│   │                                                                 │
│  现在  └─ autofs 仍然是 NFS 环境的标配                               │
│       • 容器环境中的自动挂载支持                                     │
│       • systemd 集成                                                 │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 二、核心概念：关键数据结构、算法与机制

### 2.1 核心架构

```
┌─────────────────────────────────────────────────────────────────────┐
│                    autofs 系统架构                                   │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌─────────────────────────────────────────────────────────────────┐│
│  │                          用户空间                                ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────────┐  ││
│  │  │ autofs      │  │ 配置解析器   │  │ LDAP/NIS/文件 查询模块   │  ││
│  │  │ 守护进程    │◀─┤ (auto.master│◀─┤ (解析挂载目标)          │  ││
│  │  │ (automount) │  │  auto.*)    │  │                         │  ││
│  │  └──────┬──────┘  └─────────────┘  └─────────────────────────┘  ││
│  │         │                                                       ││
│  │         │ 通过 /dev/autors 通信                                 ││
│  │         │ (packet-based protocol)                               │││
│  └─────────┼───────────────────────────────────────────────────────┘│
│            │                                                        │
│  ══════════╪══════════════════════════════════════════════════════  │
│            │ 系统调用接口                                           │
│  ┌─────────┼───────────────────────────────────────────────────────┐│
│  │         ▼                                                       ││
│  │  ┌─────────────┐  ┌──────────────────────────────────────────┐  ││
│  │  │ /dev/autofs │  │         内核态 autofs 文件系统            │  ││
│  │  │  (字符设备)  │◀─┤  (fs/autofs/)                            │  ││
│  │  └─────────────┘  └──────────────────────────────────────────┘  ││
│  │                             │                                    ││
│  │                             ▼                                    ││
│  │  ┌────────────────────────────────────────────────────────────┐ ││
│  │  │                    VFS 层 (Virtual File System)             │ ││
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐    │ ││
│  │  │  │autofs    │  │  ext4    │  │   NFS    │  │   XFS    │    │ ││
│  │  │  │文件系统  │  │  文件系统 │  │  客户端  │  │  文件系统 │    │ ││
│  │  │  └────┬─────┘  └──────────┘  └────┬─────┘  └──────────┘    │ ││
│  │  └───────┼───────────────────────────┼────────────────────────┘ ││
│  │          │                           │                          ││
│  │          ▼                           ▼                          ││
│  │  ┌─────────────┐              ┌─────────────┐                   ││
│  │  │  触发挂载点  │ ───────────▶ │ 实际文件系统 │                   ││
│  │  │  (ghost)    │   mount()    │  (NFS/ext4)  │                   ││
│  │  └─────────────┘              └─────────────┘                   ││
│  └─────────────────────────────────────────────────────────────────┘│
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

### 2.2 关键数据结构

#### 2.2.1 内核数据结构

```c
// fs/autofs/autofs_i.h

/* autofs 超级块信息 */
struct autofs_sb_info {
    u32 magic;                    /* 魔数: AUTOFS_SBI_MAGIC */
    struct super_block *sb;       /* 指向 VFS super_block */
    struct pid *oz_pgrp;          /* 原始会话组长 */
    int catatonic;                /* 是否与守护进程失联 */
    int version;                  /* 协议版本 */
    int sub_version;              /* 子版本 */
    int min_proto;                /* 最小协议版本 */
    int max_proto;                /* 最大协议版本 */

    /* 过期卸载相关 */
    unsigned long exp_timeout;    /* 过期超时时间 (jiffies) */
    unsigned int flags;           /* 挂载标志 */

    /* 等待队列 */
    struct wait_queue_head_t *queues;  /* 等待挂载的进程队列 */
};

/* autofs inode 信息 */
struct autofs_info {
    struct inode *inode;
    struct autofs_sb_info *sbi;

    /* 挂载状态 */
    int flags;
    #define AUTOFS_INF_PENDING    0x01  /* 挂载挂起中 */
    #define AUTOFS_INF_EXPIRING   0x02  /* 正在过期 */
    #define AUTOFS_INF_MOUNTED    0x04  /* 已挂载 */
    #define AUTOFS_INF_WANT_EXPIRE 0x08 /* 请求过期 */

    /* 过期卸载时间 */
    unsigned long last_used;      /* 最后访问时间 */

    /* 实际挂载点 */
    struct dentry *dentry;
};

/* 等待挂载的进程 */
struct autofs_wait_queue {
    struct list_head list;        /* 链表节点 */
    struct autofs_sb_info *sbi;   /* 超级块 */
    struct dentry *dentry;        /* 等待的 dentry */

    /* 等待的进程 */
    pid_t pid;                    /* 进程 ID */
    pid_t tgid;                   /* 线程组 ID */

    /* 状态 */
    enum { AUTOFS_S_VALID, AUTOFS_S_FINISHED } status;

    /* 同步机制 */
    wait_queue_head_t queue;      /* 等待队列 */
    struct completion done;       /* 完成通知 */
};
```

#### 2.2.2 通信协议结构

```c
/* include/uapi/linux/auto_fs.h */

/* 协议版本 */
#define AUTOFS_PROTO_VERSION        5
#define AUTOFS_PROTO_SUBVERSION     0

/* 数据包类型 */
enum autofs_packet_type {
    autofs_ptype_missing = 0,     /* 请求挂载 */
    autofs_ptype_expire,          /* 请求卸载 */
    autofs_ptype_expire_multi,    /* 批量卸载 */
};

/* 挂载请求数据包 */
struct autofs_v5_packet {
    struct autofs_packet_hdr hdr; /* 头部 */
    autofs_wqt_t wait_queue_token; /* 等待队列令牌 */
    __u32 dev;                    /* 设备号 */
    __u32 ino;                    /* inode 号 */
    __u32 uid;                    /* 用户 ID */
    __u32 gid;                    /* 组 ID */
    __u32 pid;                    /* 进程 ID */
    __u32 tgid;                   /* 线程组 ID */
    __u32 len;                    /* 路径长度 */
    char name[NAME_MAX+1];        /* 路径名 */
};

/* 卸载请求数据包 */
struct autofs_v5_expire_packet {
    struct autofs_packet_hdr hdr;
    autofs_wqt_t wait_queue_token;
    __u32 len;
    char name[NAME_MAX+1];
};
```

### 2.3 核心机制详解

#### 2.3.1 触发挂载机制

```
┌─────────────────────────────────────────────────────────────────────┐
│                    触发挂载流程详解                                  │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  步骤 1: 用户访问触发                                                │
│  ═══════════════════════                                            │
│                                                                     │
│  $ ls /nfs/home/alice                                               │
│       │                                                             │
│       ▼                                                             │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │ VFS: path_lookupat() → link_path_walk()                     │   │
│  │                                                             │   │
│  │ 发现 /nfs 是 autofs 挂载点                                   │   │
│  │                                                             │   │
│  │ 调用 autofs_lookup()                                        │   │
│  └─────────────────────────────────────────────────────────────┘   │
│       │                                                             │
│       ▼                                                             │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │ autofs_lookup() 内核逻辑:                                    │   │
│  │                                                             │   │
│  │ 1. 检查是否已挂载 (AUTOFS_INF_MOUNTED)                       │   │
│  │   - 已挂载 → 返回实际 dentry，流程结束                        │   │
│  │   - 未挂载 → 继续下一步                                       │   │
│  │                                                             │   │
│  │ 2. 创建等待队列项 (autofs_wait_queue)                        │   │
│  │                                                             │   │
│  │ 3. 发送挂载请求到用户态守护进程                               │   │
│  │   → autofs_notify_daemon(AUTOFS_NOTIFY_UMOUNT)              │   │
│  │                                                             │   │
│  │ 4. 阻塞等待守护进程响应 (wait_event_interruptible)            │   │
│  └─────────────────────────────────────────────────────────────┘   │
│       │                                                             │
│       │ ioctl(AUTOFS_IOC_READY) / AUTOFS_IOC_FAIL                  │
│       ▼                                                             │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │ 用户态守护进程处理:                                          │   │
│  │                                                             │   │
│  │ 1. 读取 /dev/autofs 获取挂载请求                              │   │
│  │   → 解析出挂载点: /nfs/home                                 │   │
│  │                                                             │   │
│  │ 2. 查询配置 (auto.master / auto.nfs)                        │   │
│  │   → nfs-server:/export/home  /nfs/home                     │   │
│  │                                                             │   │
│  │ 3. 执行实际挂载                                              │   │
│  │   → mount -t nfs nfs-server:/export/home /nfs/home         │   │
│  │                                                             │   │
│  │ 4. 通知内核挂载完成                                          │   │
│  │   → ioctl(autofs_fd, AUTOFS_IOC_READY, token)              │   │
│  └─────────────────────────────────────────────────────────────┘   │
│       │                                                             │
│       ▼                                                             │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │ 内核恢复执行:                                                │   │
│  │                                                             │   │
│  │ 1. 被唤醒，检查挂载状态                                        │   │
│  │ 2. 设置 AUTOFS_INF_MOUNTED 标志                              │   │
│  │ 3. 返回实际 dentry                                            │   │
│  │ 4. VFS 继续路径解析，访问实际文件                               │   │
│  └─────────────────────────────────────────────────────────────┘   │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

#### 2.3.2 过期卸载机制

```c
// fs/autofs/expire.c

/* 过期检查主函数 */
int autofs_expire_run(struct super_block *sb)
{
    struct autofs_sb_info *sbi = autofs_sbi(sb);
    struct dentry *dentry;

    /* 遍历所有 dentry */
    list_for_each_entry(dentry, &sb->s_root->d_subdirs, d_child) {
        struct autofs_info *info = autofs_dentry_info(dentry);

        /* 检查过期条件 */
        if (!info || !(info->flags & AUTOFS_INF_MOUNTED))
            continue;

        /* 检查是否在使用中 */
        if (autofs_mount_busy(dentry))
            continue;

        /* 检查超时 */
        if (time_after(jiffies, info->last_used + sbi->exp_timeout)) {
            /* 发送过期请求给守护进程 */
            autofs_notify_daemon(sbi, dentry, NFY_EXPIRE);
        }
    }
    return 0;
}

/* 检查挂载点是否在使用中 */
static int autofs_mount_busy(struct dentry *dentry)
{
    struct dentry *d;

    /* 检查是否有进程在使用该挂载点 */
    list_for_each_entry(d, &dentry->d_subdirs, d_child) {
        if (d->d_inode && d_count(d) > 0)
            return 1;  /* 在使用中 */
    }

    /* 检查当前工作目录 */
    if (autofs_check_cwd(dentry))
        return 1;

    return 0;  /* 空闲 */
}
```

#### 2.3.3 直接挂载 vs 间接挂载

```
┌─────────────────────────────────────────────────────────────────────┐
│              直接挂载 (Direct Mount) vs 间接挂载                      │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  直接挂载 (Direct Mount)                                             │
│  ═══════════════════════                                            │
│                                                                     │
│  配置示例 (/etc/auto.master):                                       │
│  /-     /etc/auto.direct                                            │
│                                                                     │
│  配置文件 (/etc/auto.direct):                                       │
│  /mnt/data   -fstype=nfs,rw  nfs-server:/export/data                │
│  /home       -fstype=nfs,rw  nfs-server:/export/home                │
│                                                                     │
│  文件系统结构:                                                       │
│  /                                                                    │
│  ├── bin                                                              │
│  ├── etc                                                              │
│  ├── mnt                                                              │
│  │   └── data  ←─ 直接挂载点 (autofs 直接挂载在这里)                 │
│  ├── home      ←─ 直接挂载点                                         │
│  │   ├── alice  ←─ NFS 内容                                          │
│  │   └── bob                                                        │
│  └── ...                                                              │
│                                                                     │
│  特点:                                                               │
│  • 挂载点就是实际的目录路径                                          │
│  • 适合已知固定路径的场景                                            │
│  • 配置直观，但不够灵活                                              │
│                                                                     │
│  ─────────────────────────────────────────────────────────────────  │
│                                                                     │
│  间接挂载 (Indirect Mount)                                           │
│  ═════════════════════════                                          │
│                                                                     │
│  配置示例 (/etc/auto.master):                                       │
│  /nfs   /etc/auto.nfs                                               │
│                                                                     │
│  配置文件 (/etc/auto.nfs):                                          │
│  data    -fstype=nfs,rw  nfs-server:/export/data                    │
│  home    -fstype=nfs,rw  nfs-server:/export/home                    │
│  backup  -fstype=nfs,ro  backup-server:/archive                     │
│                                                                     │
│  文件系统结构:                                                       │
│  /                                                                    │
│  ├── bin                                                              │
│  ├── nfs     ←─ autofs 挂载点 (触发目录)                             │
│  │   ├── data  ←─ 访问时触发挂载 nfs-server:/export/data            │
│  │   ├── home  ←─ 访问时触发挂载 nfs-server:/export/home            │
│  │   └── backup                                                       │
│  └── ...                                                              │
│                                                                     │
│  特点:                                                               │
│  • 触发目录 (/nfs) 由 autofs 管理                                    │
│  • 子目录动态创建，按需挂载                                          │
│  • 更灵活，适合大量动态挂载点                                        │
│  • 支持通配符和变量替换                                              │
│                                                                     │
│  通配符示例:                                                         │
│  *    -fstype=nfs,rw  nfs-server:/export/&                          │
│  # & 被替换为匹配的关键字                                            │
│  # 访问 /nfs/project1 → 挂载 nfs-server:/export/project1            │
│                                                                     │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 三、源码实现：关键函数与调用链

### 3.1 核心文件结构

```
fs/autofs/
├── autofs_i.h           # 内部头文件
├── autofs_i.c           # 基础函数
├── dir.c                # 目录操作 (lookup, readdir)
├── root.c               # 根目录操作
├── symlink.c            # 符号链接支持
├── expire.c             # 过期卸载逻辑
├── waitq.c              # 等待队列管理
├── dev-ioctl.c          # /dev/autofs ioctl 接口
└── Kconfig / Makefile

include/uapi/linux/auto_fs.h    # 用户态 API
include/uapi/linux/auto_fs4.h   # v4 协议定义
```

### 3.2 关键调用链

#### 3.2.1 查找触发挂载

```c
// fs/autofs/dir.c

/* 目录查找入口 - 触发挂载的核心 */
static struct dentry *autofs_lookup(struct inode *dir,
                                    struct dentry *dentry,
                                    unsigned int flags)
{
    struct autofs_sb_info *sbi = autofs_sbi(dir->i_sb);
    struct autofs_info *ino = autofs_dentry_info(dentry);
    struct autofs_info *dir_ino = autofs_dentry_info(dentry->d_parent);
    struct autofs_wait_queue *wq;
    int oz_mode = autofs_oz_mode(sbi);

    /* 调试输出 */
    autofs_dbg("autofs_lookup: %pd", dentry);

    /* 如果已经挂载，直接返回 */
    if (autofs_dentry_mountpoint(dentry)) {
        autofs_dbg("already mounted");
        return NULL;
    }

    /* 如果是负 dentry 且不是触发模式，返回 NULL */
    if (dentry->d_inode == NULL && !oz_mode)
        return NULL;

    /* 检查是否需要触发挂载 */
    if (!oz_mode && autofs_automount_dentry(dentry)) {
        /* 创建等待队列 */
        wq = autofs_wait_prepare(dentry);
        if (!wq)
            return ERR_PTR(-ENOMEM);

        /* 通知守护进程 */
        if (!autofs_notify_daemon(sbi, wq, NFY_MOUNT)) {
            autofs_wait_release(wq);
            return ERR_PTR(-EINTR);
        }

        /* 等待守护进程响应 */
        autofs_wait(wq);
        autofs_wait_release(wq);

        /* 检查挂载是否成功 */
        if (!autofs_dentry_mountpoint(dentry))
            return ERR_PTR(-ENOENT);
    }

    return NULL;
}

/* 检查是否需要自动挂载 */
static inline int autofs_automount_dentry(struct dentry *dentry)
{
    struct autofs_info *ino = autofs_dentry_info(dentry);

    /* 已挂载不需要再挂载 */
    if (ino->flags & AUTOFS_INF_MOUNTED)
        return 0;

    /* 正在挂载中不需要重复触发 */
    if (ino->flags & AUTOFS_INF_PENDING)
        return 0;

    return 1;
}
```

#### 3.2.2 守护进程通信

```c
// fs/autofs/waitq.c

/* 通知守护进程 */
int autofs_notify_daemon(struct autofs_sb_info *sbi,
                         struct autofs_wait_queue *wq,
                         int type)
{
    struct autofs_v5_packet packet;
    struct file *file;
    int ret;

    /* 准备数据包 */
    packet.hdr.proto_version = sbi->version;
    packet.hdr.type = type;
    packet.wait_queue_token = wq->wait_queue_token;
    packet.dev = sbi->sb->s_dev;
    packet.ino = wq->dentry->d_inode->i_ino;
    packet.uid = from_kuid_munged(current_user_ns(), current_uid());
    packet.gid = from_kgid_munged(current_user_ns(), current_gid());
    packet.pid = current->pid;
    packet.tgid = current->tgid;
    packet.len = wq->name.len;
    memcpy(packet.name, wq->name.name, wq->name.len);
    packet.name[wq->name.len] = '\0';

    /* 发送到守护进程 */
    file = sbi->pipe;
    if (!file) {
        /* 管道不存在，进入 catatonic 模式 */
        sbi->catatonic = 1;
        return -EPIPE;
    }

    ret = kernel_write(file, &packet, sizeof(packet), &file->f_pos);
    if (ret != sizeof(packet))
        return -EPIPE;

    return 0;
}

/* ioctl 处理 - 守护进程响应 */
// fs/autofs/dev-ioctl.c

static int autofs_dev_ioctl_ready(struct file *file,
                                   struct autofs_sb_info *sbi,
                                   struct autofs_dev_ioctl *param)
{
    autofs_wqt_t token = param->wait_queue_token;
    struct autofs_wait_queue *wq;

    /* 查找对应的等待队列 */
    wq = autofs_waitq_lookup(token);
    if (!wq)
        return -EINVAL;

    /* 唤醒等待的进程 */
    autofs_wake_up(wq);

    return 0;
}
```

### 3.3 系统调用流程图

```
┌─────────────────────────────────────────────────────────────────────┐
│                    autofs 系统调用完整流程                           │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  用户进程                    内核 VFS                    autofs    │
│  ═════════                   ═════════                   ═══════    │
│     │                            │                          │      │
│     │  open("/nfs/data/file")    │                          │      │
│     │───────────────────────────▶│                          │      │
│     │                            │                          │      │
│     │                            │  path_openat()           │      │
│     │                            │────┐                     │      │
│     │                            │    │                     │      │
│     │                            │◀───┘                     │      │
│     │                            │                          │      │
│     │                            │  link_path_walk()        │      │
│     │                            │────┐                     │      │
│     │                            │    │                     │      │
│     │                            │    │  walk_component()   │      │
│     │                            │    │────────▶            │      │
│     │                            │    │                     │      │
│     │                            │    │  /nfs 是 autofs?    │      │
│     │                            │    │  lookup_one_len()   │      │
│     │                            │    │────────▶            │      │
│     │                            │    │                     │      │
│     │                            │◀───┘◀────────────────────│      │
│     │                            │  autofs_lookup() ◀───────┘      │
│     │                            │                          │      │
│     │                            │  未挂载 → 触发挂载流程    │      │
│     │                            │────────────────────────▶│      │
│     │                            │                          │      │
│     │                            │  创建等待队列             │      │
│     │                            │  autofs_wait_prepare()   │      │
│     │                            │                          │      │
│     │                            │  通知守护进程             │      │
│     │                            │  autofs_notify_daemon()  │      │
│     │                            │                          │      │
│     │                            │  阻塞等待                 │      │
│     │                            │  autofs_wait()           │      │
│     │                            │◀─────────────────────────│      │
│     │                            │                          │      │
│     │◀───────────────────────────│  挂载完成，继续          │      │
│     │                            │                          │      │
│     │  访问实际文件               │                          │      │
│     │───────────────────────────▶│                          │      │
│     │                            │  VFS → NFS 客户端         │      │
│     │                            │─────────────────────────────▶   │
│     │                            │                          │      │
│     │◀───────────────────────────│                          │      │
│     │                            │◀─────────────────────────────   │
│     │                            │                          │      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 四、实际场景：生产环境应用与常见问题

### 4.1 典型应用场景

| 场景 | 配置示例 | 优势 |
|------|----------|------|
| **NFS 家目录** | `/home /etc/auto.home` | 用户登录时才挂载其家目录 |
| **NFS 应用目录** | `/apps /etc/auto.apps` | 按需加载应用，减少存储压力 |
| **备份存储** | `/backup /etc/auto.backup` | 仅在备份时连接，平时断开 |
| **多 NFS 服务器** | `/data /etc/auto.data` | 一个触发点管理多个服务器 |
| **容器持久化** | 结合 CSI driver | 动态 PVC 挂载 |

### 4.2 生产环境配置示例

```bash
# /etc/auto.master
# 格式: 挂载点  配置文件  [选项]

# 间接挂载 - 用户家目录
/home   /etc/auto.home   --timeout=600 --ghost

# 间接挂载 - 应用目录
/apps   /etc/auto.apps   --timeout=3600

# 直接挂载 - 固定路径
/-      /etc/auto.direct

# 包含其他配置文件
+dir:/etc/auto.master.d

# NIS/LDAP 支持
+auto.master
```

```bash
# /etc/auto.home
# 格式: 关键字  [选项]  位置

# 基于用户名的动态挂载
*   -fstype=nfs,rw,hard,intr,nosuid,nodev   nfs-server:/export/home/&

# 特定用户特殊配置
admin   -fstype=nfs,rw,hard,intr,sync       nfs-server:/export/admin
```

```bash
# /etc/auto.direct
# 直接挂载 - 固定路径

/mnt/data    -fstype=nfs,rw,nosuid,nodev    nfs-data:/export/data
/mnt/backup  -fstype=nfs,ro,nosuid,nodev    nfs-backup:/export/backup
```

### 4.3 常见问题与排查

#### 4.3.1 挂载不触发

```bash
# 问题: 访问 /nfs/data 没有触发挂载

# 1. 检查 autofs 服务状态
systemctl status autofs

# 2. 检查配置文件语法
automount -f -v  # 前台运行，查看详细日志

# 3. 检查 /etc/auto.master 是否生效
cat /proc/mounts | grep autofs

# 4. 检查触发目录是否存在
ls -la /nfs
# 应该是空的或由 autofs 管理

# 5. 检查守护进程日志
journalctl -u autofs -f
```

#### 4.3.2 卸载失败

```bash
# 问题: 自动卸载不工作

# 1. 检查是否有进程占用
lsof /nfs/data
fuser -m /nfs/data

# 2. 检查当前工作目录
pwd  # 确保不在挂载点内
cd /

# 3. 手动强制过期
automount -m  # 查看当前状态
kill -USR1 $(cat /var/run/autofs.pid)  # 强制检查过期

# 4. 检查超时配置
grep timeout /etc/auto.master
```

#### 4.3.3 性能问题

```bash
# 问题: 首次访问延迟高

# 1. 使用 background 挂载
*   -fstype=nfs,rw,bg,soft    server:/export/&
# bg: 后台挂载，不阻塞

# 2. 使用缓存
*   -fstype=nfs,rw,ac,actimeo=600   server:/export/&
# ac: 属性缓存

# 3. 增加 autofs 超时时间
--timeout=7200  # 减少重复挂载

# 4. 使用 --ghost 选项预创建目录
# 减少 lookup 延迟
```

### 4.4 监控与调试

```bash
# 查看当前挂载状态
automount -m

# 输出示例:
# Mount point: /home
# source(s):
#   /etc/auto.home
#   type: file  format: sun  map: auto.home

# /home 的条目:
# user1 | -fstype=nfs,rw nfs-server:/export/home/user1

# 查看详细日志
automount -f -d  # 前台调试模式

# 使用 strace 跟踪
strace -f -e trace=ioctl automount -f

# 内核调试信息
echo 'file fs/autofs/* +p' > /sys/kernel/debug/dynamic_debug/control
cat /sys/kernel/debug/autofs/*
```

---

## 五、学习路径：如何系统掌握

### 5.1 前置知识

```
基础知识:
├── Linux 文件系统
│   ├── VFS 层概念
│   ├── mount/umount 系统调用
│   └── /proc/mounts 格式
├── NFS
│   ├── NFS 客户端工作原理
│   ├── NFS 挂载选项
│   └── NFS 故障排查
└── 进程管理
    ├── 信号处理
    └── 守护进程编写
```

### 5.2 学习资源

| 类型 | 资源 | 说明 |
|------|------|------|
| **手册** | `man 5 autofs` | 配置文件格式 |
| **手册** | `man 8 automount` | 守护进程用法 |
| **文档** | `/usr/share/doc/autofs/` | 示例配置 |
| **源码** | `fs/autofs/` | 内核实现 |
| **源码** | `daemon/` (autofs 包) | 用户态实现 |
| **文章** | LWN "Autofs4" | 技术原理 |

### 5.3 动手实验

```bash
# 实验 1: 基础配置
# 1. 安装 autofs
sudo apt-get install autofs  # Debian/Ubuntu
sudo yum install autofs      # RHEL/CentOS

# 2. 创建测试配置
cat > /etc/auto.test << 'EOF'
testdir  -fstype=tmpfs,size=100M  :
EOF

echo "/test /etc/auto.test" > /etc/auto.master

# 3. 启动服务
sudo systemctl restart autofs

# 4. 测试触发挂载
ls /test/testdir  # 触发挂载
df -h /test/testdir

# 实验 2: NFS 自动挂载
# 1. 搭建 NFS 服务器 (或使用现有)
showmount -e nfs-server

# 2. 配置 autofs
echo "/nfs /etc/auto.nfs" >> /etc/auto.master
cat > /etc/auto.nfs << 'EOF'
share  -fstype=nfs,rw  nfs-server:/export/share
EOF

# 3. 测试
sudo systemctl restart autofs
ls /nfs/share  # 首次访问触发挂载
time ls /nfs/share  # 再次访问更快

# 实验 3: 内核调试
# 1. 启用动态调试
echo 'file fs/autofs/dir.c +p' | sudo tee /sys/kernel/debug/dynamic_debug/control

# 2. 查看调试输出
sudo dmesg -w | grep autofs &

# 3. 触发挂载
ls /nfs/share

# 4. 分析输出
# 观察 lookup → wait → mount → complete 的完整流程
```

## 六、Anki 卡片

### 核心概念卡片

**Q: autofs 的核心作用是什么？**
A: 按需自动挂载（访问时挂载）和空闲自动卸载，解决启动死锁和资源占用问题

**Q: autofs 解决的两个根本问题是什么？**
A: 1) 启动时挂载依赖导致的死锁 2) 网络资源长期占用

**Q: autofs 的两种挂载模式是什么？**
A: 直接挂载（Direct）和间接挂载（Indirect）

**Q: autofs 内核模块和用户态守护进程如何通信？**
A: 通过 /dev/autofs 字符设备和数据包协议

**Q: autofs 过期卸载的默认超时时间是？**
A: 5 分钟（300秒），可通过 --timeout 配置

**Q: 什么是 "ghost" 目录？**
A: autofs 预创建的目录结构，即使未挂载也可见，减少 lookup 延迟

---

## 参考链接

1. [Kernel Doc - autofs](https://www.kernel.org/doc/html/latest/filesystems/autofs.html)
2. [man 5 autofs](https://man7.org/linux/man-pages/man5/autofs.5.html)
3. [man 8 automount](https://man7.org/linux/man-pages/man8/automount.8.html)
4. [LWN - Autofs4](https://lwn.net/Articles/703785/)
5. [Red Hat - Managing NFS and autofs](https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/8/html/managing_file_systems/managing-nfs-and-autofs)

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
