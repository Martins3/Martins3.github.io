# qapi
<!-- 7d531f3b-f33d-48ed-b998-de853e6afec2 -->

## hmp

所有的命令就是利用这两个文件来处理:
- hmp-commands-info.hx
- hmp-commands.hx

## qmp

### qapi 基本结构
QAPI（QEMU API）是 QEMU 对外接口的单一事实来源：
用一份 JSON 格式的 schema 定义所有 QMP 命令、事件、数据类型，
构建时用 Python 生成器自动产出对应的 C 代码、文档和 introspection 数据。

```
  qapi/                    # schema 定义（输入）
    qapi-schema.json       # 入口，include 了其他所有模块
    control.json           # QMP 协议控制命令（qmp_capabilities...）
    block.json, net.json, migration.json...   # 按子系统分的 46 个模块
    meson.build            # 构建规则，把 schema 喂给生成器
    qmp-dispatch.c 等      # 少量手写的运行时核心（后面讲）
  scripts/qapi/            # 生成器（Python）
    parser.py  expr.py  schema.py  # 三段式前端
    types.py  visit.py  commands.py  events.py  introspect.py  # 代码生成后端
    main.py                # qapi-gen 入口
```

基本解析过程:

第一段：JSON 文本 → QObject 树（语法解析）

发生在更前面的 monitor 层（monitor/qmp.c）：

```
  JSON 字节流 ──json_message_parser_feed──▶ 词法+语法解析 ──▶ QDict/QList/QString/QInt...
```

- qobject/json-lexer.c + qobject/json-parser.c 负责把 {"execute":"qmp_capabilities",...} 这种文本解析成内存里的 QObject 树（QDict 就是 hash 表，value 是 QObject*）。
- 这阶段只关心"是不是合法 JSON"、"类型对不对"（比如 {...} 是 dict、[...] 是 list），不关心字段名合不合法、参数该不该出现。
- 解析完通过回调 handle_qmp_command 把 QObject 交给 qmp_dispatch。

第二段：QObject 树 → C struct（语义解析）

这就是 qmp_marshal_qmp_capabilities 干的事：

```txt
  QDict（已解析好的对象树）
     │  input visitor 按 schema 定义"读"
     ▼
  arg struct / handler 的 C 参数
```

它的检查是schema 语义层面的，JSON 语法层面早就过了：

- key 存在性 → has_enable 标志（可选参数）
- 字符串 "oob" → 枚举值 QMP_CAPABILITY_OOB（查 lookup 表）
- 类型匹配 → visit_type_int 遇到字符串会报错
- 未知字段 → visit_check_struct 报 "Unknown argument"


### MigrationParameters 和 MigrationParameter 什么关系?

qapi/migration.json 中包含的:


```txt
   类型                   形式      用途
  ━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   MigrationParameter     enum      表示“是哪一个迁移参数”
  ─────────────────────  ────────  ────────────────────────────────
   MigrationParameters    struct    保存一个或多个迁移参数的具体值
```

例如 max-bandwidth：

```txt
# 参数名枚举
{ 'enum': 'MigrationParameter',
  'data': [ ..., 'max-bandwidth', ... ] }

# 参数值结构体
{ 'struct': 'MigrationParameters',
  'data': {
      ...
      '*max-bandwidth': 'size',
      ...
  } }
```

生成的 C 代码大致对应：

```txt
typedef enum MigrationParameter {
    MIGRATION_PARAMETER_MAX_BANDWIDTH,
    ...
} MigrationParameter;

typedef struct MigrationParameters {
    bool has_max_bandwidth;
    uint64_t max_bandwidth;
    ...
} MigrationParameters;
```

实际调用链是：

```txt
HMP 输入：
migrate_set_parameter max-bandwidth 1G
            │
            ▼
MigrationParameter
识别参数名为 MIGRATION_PARAMETER_MAX_BANDWIDTH
            │
            ▼
MigrationParameters
设置 has_max_bandwidth=true、max_bandwidth=1G
            │
            ▼
qmp_migrate_set_parameters() 在 ./migration/migration-hmp-cmds.c:hmp_migrate_set_parameter 中
```


```txt
HMP: migrate_set_parameter <name> <value>
  │
  └─ hmp_migrate_set_parameter()
       │  构造 MigrationParameters
       ▼
     qmp_migrate_set_parameters()
       ├─ migrate_params_test_apply()   合并到临时配置
       ├─ migrate_params_check()        校验完整配置
       ├─ migrate_params_apply()        更新 MigrationState
       └─ migrate_post_update_params()  执行带副作用的更新
```

QMP 命令声明位于 migration.json 中的:

```txt
{ 'command': 'migrate-set-parameters', 'boxed': true,
  'data': 'MigrationParameters' }
```

所以，大致关联的结构为:
- QMP 接口定义：qapi/migration.json
- QMP 业务实现：migration/options.c
- HMP 到 QMP 的适配：migration/migration-hmp-cmds.c

其中 MigrateCapsSet 就是 qapi 自动生成的:
```c
/* Migration capabilities set */
struct MigrateCapsSet {
    int size;                       /* Capability set size */
    MigrationCapability caps[];     /* Variadic array of capabilities */
};
typedef struct MigrateCapsSet MigrateCapsSet;
```

### 什么时候需要修改 qapi ?
<!-- 81c46ee0-2df4-42e6-b040-2322b10f4750 -->

1. 添加 qmp 命令的时候
2. 对外暴露结构体的字段，例如

```json
##
# @MemoryBackendMemfdProperties:
#
# Properties for memory-backend-memfd objects.
#
# @hugetlb: if true, the file to be created resides in the hugetlbfs
#     filesystem (default: false)
#
# @hugetlbsize: the hugetlb page size on systems that support multiple
#     hugetlb page sizes (it must be a power of 2 value supported by
#     the system).  0 selects a default page size.  This option is
#     ignored if @hugetlb is false.  (default: 0)
#
# @seal: if true, create a sealed-file, which will block further
#     resizing of the memory (default: true)
#
# Since: 2.12
##
{ 'struct': 'MemoryBackendMemfdProperties',
  'base': 'MemoryBackendProperties',
  'data': { '*hugetlb': 'bool',
            '*hugetlbsize': 'size',
            '*seal': 'bool' },
  'if': 'CONFIG_LINUX' }
```

```txt
{ 'enum': 'MigrationCapability',
  'data': ['xbzrle', 'rdma-pin-all', 'auto-converge',
           'events', 'postcopy-ram',
           { 'name': 'x-colo', 'features': [ 'unstable' ] },
           'release-ram',
           'return-path', 'pause-before-switchover', 'multifd',
           'dirty-bitmaps', 'postcopy-blocktime', 'late-block-activate',
           { 'name': 'x-ignore-shared', 'features': [ 'unstable' ] },
           'validate-uuid', 'background-snapshot',
           'zero-copy-send', 'postcopy-preempt', 'switchover-ack',
           'dirty-limit', 'mapped-ram'] }
```

### 经典案例之 qapi/run-state.json
qapi/run-state.json 是 QEMU 的“虚拟机运行状态相关 QAPI schema”。它不是运行时读取的配置文件，也不是某个 QMP 命令主动打开它；它会在编
译阶段生成 C 类型、命令封送代码和事件发送函数。

最后对外支持三个 QMP 命令
- query-status：查询 VM 当前运行状态
- watchdog-set-action：设置 watchdog 超时后的动作
- set-action：统一设置 reboot、shutdown、panic、watchdog 对应动作

例如：

```txt
{ "execute": "query-status" }
```

返回：

```txt
{
  "return": {
    "running": true,
    "status": "running"
  }
}
```

## QEMU QMP / HMP 函数复用对照表

| 方向               | 典型模式                                                                                   | 数量级        |
| ------------------ | ------------------------------------------------------------------------------------------ | ------------- |
| **HMP → QMP**      | `hmp_xxx()` 内直接调用 `qmp_xxx()`，参数从 QDict 翻译后转交                                | 约 100 个命令 |
| **HMP info → QMP** | `.cmd_info_hrt = qmp_x_query_xxx`，直接注册 QMP 函数为 info 命令（返回 HumanReadableText） | 8 个          |

**QMP → HMP** 存在 `human-monitor-command` → `qmp_human_monitor_command()` →
`handle_hmp_command()` 特殊情况，展示不用考虑

另外还有一条"共享底层 helper"路径：HMP 和 QMP 实现**都调用同一个非 qmp 内部函数**
（如 `qdev_device_add`、`net_client_init1`、`user_creatable_add_from_str`、`save_snapshot` 等）。

### 表 1：HMP 执行命令 → QMP 函数（1:1 薄包装）

#### block 子系统（`block/monitor/block-hmp-cmds.c`）

| HMP 命令                        | HMP handler                           | QMP 函数                                     |
| ------------------------------- | ------------------------------------- | -------------------------------------------- |
| block_resize                    | `hmp_block_resize`                    | `qmp_block_resize`                           |
| block_stream                    | `hmp_block_stream`                    | `qmp_block_stream`                           |
| block_job_set_speed             | `hmp_block_job_set_speed`             | `qmp_block_job_set_speed`                    |
| block_job_cancel                | `hmp_block_job_cancel`                | `qmp_block_job_cancel`                       |
| block_job_complete              | `hmp_block_job_complete`              | `qmp_block_job_complete`                     |
| block_job_pause                 | `hmp_block_job_pause`                 | `qmp_block_job_pause`                        |
| block_job_resume                | `hmp_block_job_resume`                | `qmp_block_job_resume`                       |
| block_set_io_throttle           | `hmp_block_set_io_throttle`           | `qmp_block_set_io_throttle`                  |
| eject                           | `hmp_eject`                           | `qmp_eject`                                  |
| drive_del                       | `hmp_drive_del`                       | `qmp_blockdev_del`                           |
| drive_mirror                    | `hmp_drive_mirror`                    | `qmp_drive_mirror`                           |
| drive_backup                    | `hmp_drive_backup`                    | `qmp_drive_backup`                           |
| snapshot_blkdev                 | `hmp_snapshot_blkdev`                 | `qmp_blockdev_snapshot_sync`                 |
| snapshot_blkdev_internal        | `hmp_snapshot_blkdev_internal`        | `qmp_blockdev_snapshot_internal_sync`        |
| snapshot_delete_blkdev_internal | `hmp_snapshot_delete_blkdev_internal` | `qmp_blockdev_snapshot_delete_internal_sync` |
| nbd_server_add                  | `hmp_nbd_server_add`                  | `qmp_nbd_server_add`                         |
| nbd_server_remove               | `hmp_nbd_server_remove`               | `qmp_nbd_server_remove`                      |
| nbd_server_stop                 | `hmp_nbd_server_stop`                 | `qmp_nbd_server_stop`                        |

#### migration 子系统（`migration/migration-hmp-cmds.c`）

| HMP 命令 | HMP handler | QMP 函数 |
|---|---|---|
| migrate | `hmp_migrate` | `qmp_migrate` |
| migrate_cancel | `hmp_migrate_cancel` | `qmp_migrate_cancel` |
| migrate_continue | `hmp_migrate_continue` | `qmp_migrate_continue` |
| migrate_incoming | `hmp_migrate_incoming` | `qmp_migrate_incoming` |
| migrate_recover | `hmp_migrate_recover` | `qmp_migrate_recover` |
| migrate_pause | `hmp_migrate_pause` | `qmp_migrate_pause` |
| migrate_set_capability | `hmp_migrate_set_capability` | `qmp_migrate_set_capabilities`（复数转换） |
| migrate_set_parameter | `hmp_migrate_set_parameter` | `qmp_migrate_set_parameters` |
| migrate_start_postcopy | `hmp_migrate_start_postcopy` | `qmp_migrate_start_postcopy` |
| x_colo_lost_heartbeat | `hmp_x_colo_lost_heartbeat` | `qmp_x_colo_lost_heartbeat`（x- 不稳定） |

#### monitor 基础（`monitor/hmp-cmds.c`）

| HMP 命令 | HMP handler | QMP 函数 |
|---|---|---|
| quit\|q | `hmp_quit` | `qmp_quit` |
| stop\|s | `hmp_stop` | `qmp_stop` |
| cont\|c | `hmp_cont` | `qmp_cont` |
| exit_preconfig | `hmp_exit_preconfig` | `qmp_x_exit_preconfig` |
| getfd | `hmp_getfd` | `qmp_getfd` |
| closefd | `hmp_closefd` | `qmp_closefd` |
| dumpdtb | `hmp_dumpdtb` | `qmp_dumpdtb` |
| change | `hmp_change` | 分派：vnc→`qmp_change_vnc_password`；介质→`qmp_blockdev_change_medium`（见表 4） |

#### machine / 系统（`hw/core/machine-hmp-cmds.c`、`system/runstate-hmp-cmds.c`）

| HMP 命令 | HMP handler | QMP 函数 |
|---|---|---|
| system_reset | `hmp_system_reset` | `qmp_system_reset` |
| system_powerdown | `hmp_system_powerdown` | `qmp_system_powerdown` |
| system_wakeup | `hmp_system_wakeup` | `qmp_system_wakeup` |
| memsave | `hmp_memsave` | `qmp_memsave` |
| pmemsave | `hmp_pmemsave` | `qmp_pmemsave` |
| nmi | `hmp_nmi` | `qmp_inject_nmi` |
| balloon | `hmp_balloon` | `qmp_balloon` |
| watchdog_action | `hmp_watchdog_action` | `qmp_watchdog_set_action` |

#### chardev / UI（`chardev/char-hmp-cmds.c`、`ui/ui-hmp-cmds.c`）

| HMP 命令 | HMP handler | QMP 函数 |
|---|---|---|
| ringbuf_write | `hmp_ringbuf_write` | `qmp_ringbuf_write` |
| ringbuf_read | `hmp_ringbuf_read` | `qmp_ringbuf_read` |
| chardev-change | `hmp_chardev_change` | `qmp_chardev_change` |
| chardev-remove | `hmp_chardev_remove` | `qmp_chardev_remove` |
| chardev-send-break | `hmp_chardev_send_break` | `qmp_chardev_send_break` |
| screendump | `hmp_screendump` | `qmp_screendump` |
| sendkey | `hmp_sendkey` | `qmp_send_key` |
| set_password | `hmp_set_password` | `qmp_set_password` |
| expire_password | `hmp_expire_password` | `qmp_expire_password` |
| client_migrate_info | `hmp_client_migrate_info` | `qmp_client_migrate_info` |

#### net / qom / 其他

| HMP 命令 | HMP handler | QMP 函数 | 位置 |
|---|---|---|---|
| set_link | `hmp_set_link` | `qmp_set_link` | `net/net-hmp-cmds.c` |
| announce_self | `hmp_announce_self` | `qmp_announce_self` | `net/net-hmp-cmds.c` |
| netdev_del | `hmp_netdev_del` | `qmp_netdev_del` | `net/net-hmp-cmds.c` |
| qom-list | `hmp_qom_list` | `qmp_qom_list` | `qom/qom-hmp-cmds.c` |
| qom-get | `hmp_qom_get` | `qmp_qom_get` | `qom/qom-hmp-cmds.c` |
| qom-set | `hmp_qom_set` | `qmp_qom_set` | `qom/qom-hmp-cmds.c` |
| dump-guest-memory | `hmp_dump_guest_memory` | `qmp_dump_guest_memory` | `dump/dump-hmp-cmds.c` |
| dump-skeys | `hmp_dump_skeys` | `qmp_dump_skeys` | `hw/s390x/s390-skeys.c` |
| trace-event | `hmp_trace_event` | `qmp_trace_event_set_state` | `trace/trace-hmp-cmds.c` |
| replay_break | `hmp_replay_break` | `qmp_replay_break` | `replay/replay-debugging.c` |
| replay_delete_break | `hmp_replay_delete_break` | `qmp_replay_delete_break` | 同上 |
| replay_seek | `hmp_replay_seek` | `qmp_replay_seek` | 同上 |
| calc_dirty_rate | `hmp_calc_dirty_rate` | `qmp_calc_dirty_rate` | `migration/dirtyrate.c` |
| set_vcpu_dirty_limit | `hmp_set_vcpu_dirty_limit` | `qmp_set_vcpu_dirty_limit` | `system/dirtylimit.c` |
| cancel_vcpu_dirty_limit | `hmp_cancel_vcpu_dirty_limit` | `qmp_cancel_vcpu_dirty_limit` | 同上 |
| device_del | `hmp_device_del` | `qmp_device_del` | `system/qdev-monitor.c` |

---

### 表 2：HMP info 命令 → QMP query 函数

| info 命令 | HMP handler | QMP 函数 |
|---|---|---|
| info version | `hmp_info_version` | `qmp_query_version` |
| info name | `hmp_info_name` | `qmp_query_name` |
| info status | `hmp_info_status` | `qmp_query_status` |
| info cpus | `hmp_info_cpus` | `qmp_query_cpus_fast` |
| info block | `hmp_info_block` | `qmp_query_block` + `qmp_query_named_block_nodes`（表 4） |
| info blockstats | `hmp_info_blockstats` | `qmp_query_blockstats` |
| info block-jobs | `hmp_info_block_jobs` | `qmp_query_block_jobs` |
| info chardev | `hmp_info_chardev` | `qmp_query_chardev` |
| info pci | `hmp_info_pci` | `qmp_query_pci` |
| info kvm | `hmp_info_kvm` | `qmp_query_kvm` |
| info accelerators | `hmp_info_accelerators` | `qmp_query_accelerators` |
| info migrate | `hmp_info_migrate` | `qmp_query_migrate` |
| info migrate_capabilities | `hmp_info_migrate_capabilities` | `qmp_query_migrate_capabilities` |
| info migrate_parameters | `hmp_info_migrate_parameters` | `qmp_query_migrate_parameters` |
| info balloon | `hmp_info_balloon` | `qmp_query_balloon` |
| info uuid | `hmp_info_uuid` | `qmp_query_uuid` |
| info mice | `hmp_info_mice` | `qmp_query_mice` |
| info vnc | `hmp_info_vnc` | `qmp_query_vnc_servers` |
| info spice | `hmp_info_spice` | `qmp_query_spice` |
| info tpm | `hmp_info_tpm` | `qmp_query_tpm` |
| info memdev | `hmp_info_memdev` | `qmp_query_memdev` |
| info memory-devices | `hmp_info_memory_devices` | `qmp_query_memory_devices` |
| info iothreads | `hmp_info_iothreads` | `qmp_query_iothreads` |
| info dump | `hmp_info_dump` | `qmp_query_dump` |
| info hotpluggable-cpus | `hmp_hotpluggable_cpus` | `qmp_query_hotpluggable_cpus` |
| info vm-generation-id | `hmp_info_vm_generation_id` | `qmp_query_vm_generation_id` |
| info memory_size_summary | `hmp_info_memory_size_summary` | `qmp_query_memory_size_summary` |
| info vcpu_dirty_limit | `hmp_info_vcpu_dirty_limit` | `qmp_query_vcpu_dirty_limit` |
| info sgx | `hmp_info_sgx` | `qmp_query_sgx` |
| info stats | `hmp_info_stats` | `qmp_query_stats` + `qmp_query_stats_schemas`（表 4） |
| info trace-events | `hmp_info_trace_events` | `qmp_trace_event_get_state` |
| info cryptodev | `hmp_info_cryptodev` | `qmp_query_cryptodev` |
| info firmware-log | `hmp_info_firmware_log` | `qmp_query_firmware_log` |
| info rocker | `hmp_rocker` | `qmp_query_rocker` |
| info rocker-ports | `hmp_rocker_ports` | `qmp_query_rocker_ports` |
| info rocker-of-dpa-flows | `hmp_rocker_of_dpa_flows` | `qmp_query_rocker_of_dpa_flows` |
| info rocker-of-dpa-groups | `hmp_rocker_of_dpa_groups` | `qmp_query_rocker_of_dpa_groups` |
| info virtio | `hmp_virtio_query` | `qmp_x_query_virtio`（x-） |
| info virtio-status | `hmp_virtio_status` | `qmp_x_query_virtio_status` |
| info virtio-queue-status | `hmp_virtio_queue_status` | `qmp_x_query_virtio_queue_status` |
| info virtio-vhost-queue-status | `hmp_vhost_queue_status` | `qmp_x_query_virtio_vhost_queue_status` |
| info virtio-queue-element | `hmp_virtio_queue_element` | `qmp_x_query_virtio_queue_element` |

---

### 表 3：info 命令直接注册 QMP 函数（`.cmd_info_hrt`，连 HMP 包装都不用写）

这些 info 命令没有 `hmp_*` 包装，直接在命令表中挂 QMP 函数，
由 `monitor/hmp.c` 的 `hmp_info_human_readable_text()` 统一打印返回的 HumanReadableText：

| info 命令 | 注册方式 | QMP 函数 |
|---|---|---|
| info irq | `hmp-commands-info.hx` | `qmp_x_query_irq` |
| info pic | `hmp-commands-info.hx` | `qmp_x_query_interrupt_controllers` |
| info numa | `hmp-commands-info.hx` | `qmp_x_query_numa` |
| info usb | `hmp-commands-info.hx` | `qmp_x_query_usb` |
| info roms | `hmp-commands-info.hx` | `qmp_x_query_roms` |
| info ramblock | `hmp-commands-info.hx` | `qmp_x_query_ramblock` |
| info jit | 运行时 `monitor_register_hmp_info_hrt("jit", ...)`（`accel/tcg/monitor.c`） | `qmp_x_query_jit` |
| info accel | 运行时注册（`accel/accel-system.c`） | `qmp_x_accel_stats` |
| info usbhost | 运行时 `monitor_register_hmp("usbhost", ...)`（`hw/usb/host-libusb.c`） | `hmp_info_usbhost`（libusb 专属） |

---

### 表 4：一个 HMP 命令复用多个 QMP 函数

| HMP 命令 | HMP handler | 调用的 QMP 函数 |
|---|---|---|
| change | `hmp_change` | 分派：`qmp_change_vnc_password`（vnc 目标）/ `qmp_blockdev_change_medium`（其他） |
| nbd_server_start | `hmp_nbd_server_start` | `qmp_nbd_server_add` + `qmp_nbd_server_stop` + `qmp_query_block`（自动补全可导出设备） |
| info block | `hmp_info_block` | `qmp_query_block` + `qmp_query_named_block_nodes` |
| info stats | `hmp_info_stats` | `qmp_query_stats` + `qmp_query_stats_schemas` |

---

### 表 5：反向复用 —— QMP 命令调用 HMP

| QMP 命令 | 实现 | 调用 |
|---|---|---|
| `human-monitor-command` | `qmp_human_monitor_command()`（`monitor/qmp-cmds.c`） | `handle_hmp_command()` → 任意 HMP 命令，把 stdout 作为字符串返回 |

> 这是唯一"官方"的 QMP→HMP 通道，注释里明确说明它只是过渡方案（stop-gap），不保证稳定。

---

### 表 6：共享底层 helper（HMP 与 QMP 各自实现，但共用同一个内部函数）

这一类不算"hmp 调 qmp"，而是两边都调同一个模块函数，是最容易混淆的复用：

| HMP 命令                | HMP 路径              | QMP 命令        | QMP 路径              | 共享的底层函数                                                                          |
| ----------------------- | --------------------- | --------------- | --------------------- | --------------------------------------------------------------------------------------- |
| device_add              | `hmp_device_add`      | device_add      | `qmp_device_add`      | `qdev_device_add*`（HMP 走 `qdev_device_add()`，QMP 走 `qdev_device_add_from_qdict()`） |
| netdev_add              | `hmp_netdev_add`      | netdev_add      | `qmp_netdev_add`      | `net_client_init1()`                                                                    |
| object_add              | `hmp_object_add`      | object-add      | `qmp_object_add`      | `user_creatable_add_from_str()`                                                         |
| object_del              | `hmp_object_del`      | object-del      | `qmp_object_del`      | `user_creatable_del()`                                                                  |
| savevm / loadvm / delvm | `hmp_savevm` 等       | （无 QMP 对应） | —                     | `save_snapshot()` / `load_snapshot()` / `delete_snapshot()`                             |
| info dirty_rate         | `hmp_info_dirty_rate` | calc-dirty-rate | `qmp_calc_dirty_rate` | `query_dirty_rate_info()`                                                               |
| chardev-add             | `hmp_chardev_add`     | chardev-add     | `qmp_chardev_add`     | `qemu_chr_new_from_opts()`                                                              |

---

### 表 7：HMP 独有实现（完全不复用 QMP）

这些命令只有 HMP 版本，没有对应 QMP 命令（或 QMP 语义差异太大）：

| 类别           | HMP 命令                                                                                                                                                                                                                                                           |
| -------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| 调试/内存访问  | `x` `xp` `print\|p` `i` `o` `sum` `gpa2hva` `gpa2hpa` `gva2gpa`                                                                                                                                                                                                    |
| gdb/调试器     | `gdbserver` `cpu` `log` `logfile` `one-insn-per-tb` `info registers` `info lapic` `info tlb` `info mem` `info mtree` `info jit`*                                                                                                                                   |
| 快照（无 QMP） | `savevm` `loadvm` `delvm` `info snapshots`                                                                                                                                                                                                                         |
| 交互类         | `help\|?` `clear` `commit` `mouse_move` `mouse_button` `mouse_set` `wavcapture` `stopcapture` `boot_set` `qemu-io`                                                                                                                                                 |
| 网络 slirp     | `hostfwd_add` `hostfwd_remove` `info usernet`                                                                                                                                                                                                                      |
| 其他           | `sync-profile` `migration_mode` `drive_add` `mce` `pcie_aer_inject_error` `xen-event-inject` `xen-event-list` `info history` `info qtree` `info qdm` `info qom-tree` `info skeys` `info cmma` `info via`* `info capture` `info network` `info sev`* `info replay`* |

\* `info jit`/`info accel`/`info via` 属于表 3/表 6 的特殊情况：`jit`、`accel`
运行时注册 QMP 函数；`via` 是在设备 C 文件里写了一个 static 的
`qmp_x_query_via()`（QMP 风格但未注册进 QMP 表）。 `info sev` / `info replay`
在当前配置下是 stub（直接打印"not available"）。

## 结论

**HMP 是 QMP 的下游**：绝大多数 HMP 命令只是把 `QDict` 参数翻译成 QAPI 结构体后转调 `qmp_*()`，因此 QMP 才是真正的"实现层"，HMP 是"展示层 + 参数翻译层"。
info 命令是最彻底的复用`.cmd_info_hrt` 直接指向 QMP 命令，连翻译代码都不用写，前提是 QMP 命令返回 `HumanReadableText`。

可以看到 hmp-commands-info.hx hmp-commands.hx 相对于 ./qapi 是简单很多的，
因为具体的解析工作主要都是 qapi 做的

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
