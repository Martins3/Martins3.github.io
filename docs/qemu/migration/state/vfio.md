# vfio

## 关键参考
- hw/vfio/migration.c
- qemu/docs/devel/migration/vfio.rst
- https://whenderson.dev/blog/nutanix-internship/
- [vfio v2](https://gitlab.com/qemu-project/kvm-forum/-/raw/main/_attachments/2024/KVM_Forum_2024_-_VFIO_5LSTtyJ.pdf)
- [vfio multifd](https://gitlab.com/qemu-project/kvm-forum/-/raw/main/_attachments/2024/kvm-forum-2024-multifd-device-state-transfer_3K5EQIG.pdf)
- https://blog.csdn.net/huang987246510/article/details/128403256

## 两种 dirty 数据的来源
- 设备内部的 dirty 状态
- 设备对于内存做 DMA ，不经过 CPU page table write fault，也不经过普通 KVM dirty logging。

第一个问题的解决办法，也就是会类似 ram_save_iterate 来不断的发送
ram 内存，其中 vfio_save_block 的实现非常简单，就是从 data_fd 中接受数据，然后发送数据就可以了:
```c
static const SaveVMHandlers savevm_vfio_handlers = {
    .save_prepare = vfio_save_prepare,
    .save_setup = vfio_save_setup,
    .save_cleanup = vfio_save_cleanup,
    .save_query_pending = vfio_state_pending,
    .is_active_iterate = vfio_is_active_iterate,
    .save_live_iterate = vfio_save_iterate,
    .save_complete = vfio_save_complete_precopy,
    .save_state = vfio_save_state,
    .load_setup = vfio_load_setup,
    .load_cleanup = vfio_load_cleanup,
    .load_state = vfio_load_state,
    /*
     * Multifd support
     */
    .load_state_buffer = vfio_multifd_load_state_buffer,
    .switchover_start = vfio_switchover_start,
    .save_complete_precopy_thread = vfio_multifd_save_complete_precopy_thread,
};
```

第二个问题的解决:
```c
static const MemoryListener vfio_memory_listener = {
    .name = "vfio",
    .begin = vfio_listener_begin,
    .commit = vfio_listener_commit,
    .region_add = vfio_listener_region_add,
    .region_del = vfio_listener_region_del,
    .log_global_start = vfio_listener_log_global_start,
    .log_global_stop = vfio_listener_log_global_stop,
    .log_sync = vfio_listener_log_sync,
};
```

对应的，在驱动层次的实现为:

```c
static const struct vfio_migration_ops mlx5vf_pci_mig_ops = {
	.migration_set_state = mlx5vf_pci_set_device_state,
	.migration_get_state = mlx5vf_pci_get_device_state,
	.migration_get_data_size = mlx5vf_pci_get_data_size,
};

static const struct vfio_log_ops mlx5vf_pci_log_ops = {
	.log_start = mlx5vf_start_page_tracker,
	.log_stop = mlx5vf_stop_page_tracker,
	.log_read_and_clear = mlx5vf_tracker_read_and_clear,
};
```


## v2

commit 115dcec65f61 ("vfio: Define device migration protocol v2")

记录了 v1 的问题

## switchover ack && return-path
```txt
(qemu) info migrate
globals:
store-global-state: on
only-migratable: off
send-configuration: on
send-section-footer: on
send-switchover-start: on
clear-bitmap-shift: 18
```

switchover-ack 依赖 return-path :
```txt
migrate_set_capability  return-path on
migrate_set_capability  switchover-ack on
```

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - migration_completion
            - migration_completion_precopy
              - migration_switchover_start

send-switchover-start 在 target 端是执行
loadvm_postcopy_handle_switchover_start
这是是一个最近刚刚加入的东西:


```diff
History:        #0
Commit:         4e55cb3cdeb099cb65f75f5d3b061e3e1319cf3b
Author:         Maciej S. Szmigiero <maciej.szmigiero@oracle.com>
Committer:      Cédric Le Goater <clg@redhat.com>
Author Date:    Wed 05 Mar 2025 06:03:32 AM CST
Committer Date: Thu 06 Mar 2025 01:47:33 PM CST

migration: Add MIG_CMD_SWITCHOVER_START and its load handler

This QEMU_VM_COMMAND sub-command and its switchover_start SaveVMHandler is
used to mark the switchover point in main migration stream.

It can be used to inform the destination that all pre-switchover main
migration stream data has been sent/received so it can start to process
post-switchover data that it might have received via other migration
channels like the multifd ones.

Add also the relevant MigrationState bit stream compatibility property and
its hw_compat entry.

Reviewed-by: Fabiano Rosas <farosas@suse.de>
Reviewed-by: Zhang Chen <zhangckid@gmail.com> # for the COLO part
Signed-off-by: Maciej S. Szmigiero <maciej.szmigiero@oracle.com>
Link: https://lore.kernel.org/qemu-devel/311be6da85fc7e49a7598684d80aa631778dcbce.1741124640.git.maciej.szmigiero@oracle.com
Signed-off-by: Cédric Le Goater <clg@redhat.com>
```

## 基本理论

1. QEMU 负责迁移编排、数据封装和网络传输；
2. 内核 VFIO vendor driver 负责冻结设备、生成/恢复硬件内部状态， 并通过 data_fd 向 QEMU 提供一个不透明字节流。

### 1. 迁移内容分成三类

Guest RAM
  └─ QEMU RAM migration
       └─ VFIO device / IOMMU dirty logging 提供 DMA 脏页

PCI 配置等虚拟化状态
  └─ QEMU VMState 保存/恢复

物理设备内部状态
  └─ VFIO migration data_fd
       └─ vendor driver/firmware 生成和消费不透明数据流

因此，“设备迁移数据”和“设备 DMA 写脏的 guest RAM”是两个独立问题：

- data_fd 搬运队列、上下文、firmware 状态等设备内部状态。
- dirty logging 保证设备 DMA 修改过的 guest RAM 被重新发送。

### 2. 内核 VFIO 接口

核心 uAPI 位于 include/uapi/linux/vfio.h:1036。

QEMU 首先查询 VFIO_DEVICE_FEATURE_MIGRATION，内核返回能力：

- VFIO_MIGRATION_STOP_COPY：基本迁移能力，必须支持。
- VFIO_MIGRATION_PRE_COPY：设备内部状态可以在 VM 运行期间预拷贝。
- VFIO_MIGRATION_P2P：可以安全协调存在设备间 P2P DMA 的多设备迁移。

#### 状态机

 状态            含义
━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 RUNNING         设备正常运行，可 DMA、产生中断
──────────────  ──────────────────────────────────────
 PRE_COPY        正常运行，同时跟踪并输出设备内部状态
──────────────  ──────────────────────────────────────
 RUNNING_P2P     运行但不能发起新的 P2P DMA
──────────────  ──────────────────────────────────────
 PRE_COPY_P2P    PRE_COPY 和 P2P quiesce 的组合
──────────────  ──────────────────────────────────────
 STOP            设备完全静止，不再改变内部或外部状态
──────────────  ──────────────────────────────────────
 STOP_COPY       设备静止，并输出最终设备状态
──────────────  ──────────────────────────────────────
 RESUMING        设备静止，接收并恢复迁移状态
──────────────  ──────────────────────────────────────
 ERROR           状态机失效，只能通过 reset 恢复

完整语义定义在 include/uapi/linux/vfio.h:1238。

状态转换通过：

VFIO_DEVICE_FEATURE_SET |
VFIO_DEVICE_FEATURE_MIG_DEVICE_STATE

完成。进入以下状态时内核可能返回新 FD：

- PRE_COPY/PRE_COPY_P2P/STOP_COPY saving group：只读 data_fd
- RESUMING：只写 data_fd

VFIO core 调用 vendor driver 的 migration_set_state()，然后把驱动返回的 struct file 安装成用户态 FD，见 drivers/vfio/vfio_main.c:900。

#### 驱动接口

具体硬件驱动需要注册 include/linux/vfio.h:208：

```txt
struct vfio_migration_ops {
    struct file *(*migration_set_state)(...);
    int (*migration_get_state)(...);
    int (*migration_get_data_size)(...);
};
```

VFIO core 本身不知道怎么保存硬件状态。它只负责：

- uAPI 参数检查；
- 状态能力管理；
- struct file 到用户态 FD 的转换；
- 提供 vfio_mig_get_next_state()，把组合转换拆成若干合法状态弧。

例如 QEMU直接请求 RUNNING -> STOP_COPY，驱动可能实际执行：

RUNNING -> RUNNING_P2P -> STOP -> STOP_COPY

拆分逻辑见 drivers/vfio/vfio_main.c:674。

#### mlx5 驱动示例

mlx5 是比较完整的实现：

- 状态转换和 firmware suspend/save/load：
  drivers/vfio/pci/mlx5/main.c:1091

- 创建 saving 匿名 FD：
  drivers/vfio/pci/mlx5/main.c:627

- 创建 resuming 匿名 FD：
  drivers/vfio/pci/mlx5/main.c:1010

- saving FD 实现 read/poll/ioctl：
  drivers/vfio/pci/mlx5/main.c:583

- resuming FD 实现 write：
  drivers/vfio/pci/mlx5/main.c:1003

这说明普通 vfio-pci passthrough 本身并不会自动获得迁移能力，必须有 mlx5、QAT、virtio VF、Xe 等设备专用 VFIO variant driver 和 firmware 支持。

### 3. QEMU 实现

QEMU 主要代码在 /home/martins3/data/qemu/hw/vfio/migration.c:1。

设备 realize 时，vfio_migration_init()：

1. 查询内核 migration flags。
2. 要求至少支持 STOP_COPY。
3. 探测 pre-copy、P2P 和 DMA dirty logging。
4. 注册 SaveVMHandlers。
5. 注册 VM 状态和 migration 状态 notifier。

入口见 vfio_migration_init

#### 源端 pre-copy

典型状态流：

RUNNING
   ↓ save_setup
PRE_COPY
   ↓ 反复 read(data_fd)
PRE_COPY_P2P
   ↓ VM 停机
STOP_COPY
   ↓ drain data_fd 到真正 EOF
STOP

具体过程：

1. vfio_save_setup() 查询 stop_copy_length，分配缓冲区；支持 pre-copy 时进入 PRE_COPY，获得 saving data_fd。
2. VFIO_MIG_GET_PRECOPY_INFO 查询：
    - initial_bytes
    - dirty_bytes

3. vfio_save_iterate() 从 FD 读取一块设备状态，包装进 QEMU migration stream。
4. pre-copy 暂时没有数据时，驱动返回 ENOMSG，不是最终 EOF。
5. QEMU 根据 RAM、设备 precopy_bytes 和 stopcopy_bytes 判断是否进入停机阶段。
6. VM 停机后进入 STOP_COPY，vfio_save_complete_precopy() 一直读取，直到 read() 返回 0。

如果设备不支持 PRE_COPY，设备内部状态全部在 STOP_COPY 阶段传输，downtime 会更长。

#### 目的端恢复

典型流程：

STOP/initial state
   ↓ load_setup
RESUMING
   ↓ write(data_fd)
STOP
   ↓ P2P 协调
RUNNING_P2P
   ↓
RUNNING

vfio_load_setup() 进入 RESUMING 并取得 write-only data_fd；vfio_load_state() 从 QEMU migration stream 解析块，然后写入 FD。离开 RESUMING 时，内核驱动完成最终校验并
提交设备状态。

设备数据对 QEMU 完全不透明。QEMU 只添加自己的 tag、长度和结束标记，必须原样保持字节顺序。

### 4. Guest RAM DMA 脏页

设备即使在 pre-copy，也可能继续 DMA 写 guest RAM。因此 QEMU memory listener 会：

1. 启动 dirty tracking。
2. 周期性读取 dirty bitmap。
3. 把对应 guest RAM page 标脏。
4. 由普通 RAM migration 重新发送。

当前存在两条路径：

- 设备自己记录 DMA：VFIO_DEVICE_FEATURE_DMA_LOGGING_*，内核映射到 vfio_log_ops。
- IOMMU/VFIO container/IOMMUFD 提供 dirty bitmap。

如果两种方式都不可用，QEMU 只能保守地持续把相关 RAM 标脏，迁移可能难以收敛。


### 5. 几个重要限制

- VFIO 当前不支持 postcopy，QEMU会直接拒绝。
- 多个 VFIO 设备一起迁移时，当前 QEMU要求全部支持 P2P migration。
- 源和目的设备必须由 vendor driver/firmware 保证兼容；VFIO opaque stream 没有跨厂商通用格式。
- ERROR 状态只能通过 VFIO_DEVICE_RESET 恢复。QEMU状态转换失败时会尝试回退状态，回退失败则 reset。

## vfio drity 的过程

### 2. 第一次进入 PRE_COPY

QEMU将设备从：

RUNNING → PRE_COPY

内核驱动返回 saving data_fd，同时开始设备状态变化跟踪。

以 mlx5 为例，驱动执行一次：

SAVE_VHCA_STATE
    incremental = 0
    set_track    = 1

含义是：

- 保存初始完整状态 S0；
- 从现在开始跟踪后续内部状态变化。

这部分在 drivers/vfio/pci/mlx5/cmd.c:752。

### 3. QEMU如何“发现 device state dirty”

QEMU migration thread 周期性执行精确 pending 查询：

vfio_state_pending(exact=true)
  ├─ VFIO_DEVICE_FEATURE_MIG_DATA_SIZE
  └─ ioctl(data_fd, VFIO_MIG_GET_PRECOPY_INFO)

后一个 ioctl 返回：

struct vfio_precopy_info {
    initial_bytes;
    dirty_bytes;
};

QEMU代码在：

这里的 dirty_bytes 是：

> vendor driver 估算的、相对于此前已输出状态的新变化数据量。

它不是设备状态地址位图，也不是 QEMU通过比较两份数据得到的。

#### mlx5 的发现过程

mlx5 的 VFIO_MIG_GET_PRECOPY_INFO

它向 firmware 发出：

QUERY_VHCA_MIGRATION_STATE
    incremental = 1

firmware 返回 required_umem_size，也就是当前增量状态大约需要多少空间：

QEMU
  → VFIO_MIG_GET_PRECOPY_INFO
    → mlx5 driver
      → QUERY_VHCA_MIGRATION_STATE(incremental=1)
        → inc_length

对应函数为 drivers/vfio/pci/mlx5/cmd.c:88。

### 4. dirty 被发现后，如何生成增量数据

如果：

- 当前已经输出到 data stream 尾部；
- inc_length > 0；

mlx5 驱动会：

1. 分配 DMA buffer；
2. 发出 SAVE_VHCA_STATE；
3. 设置 incremental=1；
4. 保持 set_track=1，继续跟踪 SAVE 期间产生的新变化。

即：

SAVE_VHCA_STATE
    incremental = 1
    set_track    = 1

firmware 异步完成后，drivers/vfio/pci/mlx5/cmd.c:676 会：

- 给增量 image 加 vendor header；
- 把 header 和 image 加到 migf->buf_list；
- 增加 stream 的 max_pos；
- 唤醒正在等待的 read(data_fd)。

如果上一批数据还没有读完，驱动通常先把：

尚未读取的数据 + 新发现的 inc_length

一起报告为 dirty_bytes；等 QEMU读到当前 stream 尾部后，下次 query 再生成新增量。这样可保持流内顺序。

### 5. 与 guest RAM dirty 的本质区别

 项目          Guest RAM dirty       VFIO device-state dirty
━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 如何发现      dirty bitmap          vendor/firmware query
────────────  ────────────────────  ───────────────────────────────
 是否有地址    有，GPA/IOVA/page     没有通用地址概念
────────────  ────────────────────  ───────────────────────────────
 是否清 bit    查询/传输后清理       vendor stream 自己维护代际
────────────  ────────────────────  ───────────────────────────────
 如何重传      重发对应 RAM page     追加发送 incremental image
────────────  ────────────────────  ───────────────────────────────
 谁负责合并    QEMU RAM migration    目的端 vendor driver/firmware
────────────  ────────────────────  ───────────────────────────────
 最终收敛      停 vCPU、停止 DMA     设备进入 STOP_COPY

最准确的理解不是“设备数据脏了以后重发原数据”，而是：
源驱动持续从设备生成有序的状态更新日志，目的驱动依次回放；
STOP_COPY 生成并传输最后一条增量，从而封闭整个状态历史。


## misc

1.
- `vfio_migration_probe`
    - `vfio_get_dev_region_info` : 到底得到是什么东西？
    - `vfio_migration_init`
        - `vfio_region_setup`
        - `register_savevm_live`

在 linux-headers/linux/vfio.h 中详细的描述了 VFIO migration 的过程中，内核的升级过程。

1. 分析主要 Hook

- `vfio_save_setup`
    - `vfio_region_mmap`
    - `vfio_migration_set_state`
        - `vfio_mig_read`
            - `vfio_mig_access` ：就是对于 fd 进行读写的
        - `vfio_mig_write`

## Linux kernel 中 struct vfio_device_migration_info 中

- a8a24f3f6e38 "vfio: UAPI for migration interface for device state" — 2019 年引入这套 v1 UAPI；
- 0f3f9cd7f752 "vfio: Remove migration protocol v1 documentation" — v1 协议被移除时，连同文档一起清掉了，但 include/uapi/linux/vfio.h 里的定义原样保留。

代码已经彻底移除了，但是 API 的定义还在:

```c
struct vfio_device_migration_info {
	__u32 device_state;         /* VFIO device state */
#define VFIO_DEVICE_STATE_V1_STOP      (0)
#define VFIO_DEVICE_STATE_V1_RUNNING   (1 << 0)
#define VFIO_DEVICE_STATE_V1_SAVING    (1 << 1)
#define VFIO_DEVICE_STATE_V1_RESUMING  (1 << 2)
#define VFIO_DEVICE_STATE_MASK      (VFIO_DEVICE_STATE_V1_RUNNING | \
				     VFIO_DEVICE_STATE_V1_SAVING |  \
				     VFIO_DEVICE_STATE_V1_RESUMING)

#define VFIO_DEVICE_STATE_VALID(state) \
	(state & VFIO_DEVICE_STATE_V1_RESUMING ? \
	(state & VFIO_DEVICE_STATE_MASK) == VFIO_DEVICE_STATE_V1_RESUMING : 1)

#define VFIO_DEVICE_STATE_IS_ERROR(state) \
	((state & VFIO_DEVICE_STATE_MASK) == (VFIO_DEVICE_STATE_V1_SAVING | \
					      VFIO_DEVICE_STATE_V1_RESUMING))

#define VFIO_DEVICE_STATE_SET_ERROR(state) \
	((state & ~VFIO_DEVICE_STATE_MASK) | VFIO_DEVICE_STATE_V1_SAVING | \
					     VFIO_DEVICE_STATE_V1_RESUMING)

	__u32 reserved;
	__aligned_u64 pending_bytes;
	__aligned_u64 data_offset;
	__aligned_u64 data_size;
};
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
