# vmstate
## 核心结构体

VMStateInfo
VMStateDescription

## 这个是经典例子了吧

```txt
     vmstate-types.c
     vmstate.c
```

```c
const VMStateDescription vmstate_arm_cpu = {
    .name = "cpu",
    .version_id = 22,
    .minimum_version_id = 22,
    .pre_save = cpu_pre_save,
    .post_save = cpu_post_save,
    .pre_load = cpu_pre_load,
    .post_load = cpu_post_load,
```

不知道为什么解析不出来:

- coroutine_trampoline
  - process_incoming_migration_co
    - qemu_loadvm_state
      - qemu_loadvm_state_main
        - qemu_loadvm_section_start_full
          - vmstate_load_state
            - cpu_pre_load

- coroutine_trampoline
  - process_incoming_migration_co
    - qemu_loadvm_state
      - qemu_loadvm_state_main
        - qemu_loadvm_section_start_full
          - vmstate_load_state
            - cpu_post_load


## 热迁移的兼容性是如何检查的

例如:
```txt
qemu-system-x86_64: Machine type received is 'pc-i440fx-9.2' and local is 'pc-i440fx-11.0'
qemu-system-x86_64: load of migration failed: Invalid argument: post load hook failed for: configuration, version_id: 1, minimum_version: 0, ret: -22
```

# QEMU 热迁移核心结构体关系

QEMU 设备迁移可以理解为三层：

```text
全局迁移调度
SaveState / SaveStateEntry
              ↓
对象布局描述
VMStateDescription / VMStateField
              ↓
单个字段编解码
VMStateInfo
```

从 register_savevm_live() 可以看出来，其实
```txt
    se->ops = ops;
    se->opaque = opaque;
    se->vmsd = NULL;
```
非结构化的，就不用 vmsd 了，

```c
static SaveState savevm_state = {
    .handlers = QTAILQ_HEAD_INITIALIZER(savevm_state.handlers),
    .handler_pri_head = { [MIG_PRI_DEFAULT ... MIG_PRI_MAX] = NULL },
    .global_section_id = 0,
};
```
的这个上面挂 SaveStateEntry

然后将 `savevm_ram_handlers` 和 `ram_state` 关联为其成员:
```c
register_savevm_live("ram", 0, 4, &savevm_ram_handlers, &ram_state);

static RAMState *ram_state;
```

## 1. 总体结构关系

```text
MigrationState
 └── to_dst_file ───────────────────────────────┐
                                                ↓
                                            QEMUFile
                                                ↑
全局 savevm_state: SaveState                    │
 └── handlers: QTAILQ<SaveStateEntry>           │
       ├── idstr / instance_id / section_id     │
       ├── version_id                           │
       ├── opaque ──────> DeviceState/设备对象  │
       │                                        │
       ├── vmsd ──> VMStateDescription ─────────┤
       │             ├── fields[]               │
       │             │    └── VMStateField      │
       │             │          ├── offset      │
       │             │          ├── size/num    │
       │             │          ├── flags       │
       │             │          ├── info ──> VMStateInfo
       │             │          └── vmsd ──> 嵌套结构描述
       │             └── subsections[]
       │
       └── ops ──> SaveVMHandlers
                    一般用于 RAM/VFIO/大型可迭代状态
```

后文涉及的核心类型是：

- `struct VMStateInfo`
- `struct VMStateField`
- `struct VMStateDescription`
- `struct SaveStateEntry`
- `struct MigrationState`


## 2. VMStateDescription：描述“一个对象”

`VMStateDescription`，简称 VMSD，描述：

> 一个 C 对象的哪些状态需要迁移，版本是什么，保存和加载前后需要执行什么回调。

典型定义：

```c
static const VMStateDescription vmstate_foo = {
    .name = "foo",
    .version_id = 2,
    .minimum_version_id = 1,

    .pre_save = foo_pre_save,
    .post_load = foo_post_load,

    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(status, FooState),
        VMSTATE_UINT64(counter, FooState),
        VMSTATE_END_OF_LIST()
    },

    .subsections = (const VMStateDescription * const []) {
        &vmstate_foo_extra,
        NULL
    },
};
```

主要字段的意义：

- `name`：迁移 section 或 subsection 的标识。
- `version_id`：当前发送格式版本。
- `minimum_version_id`：本 QEMU 最老能接收的版本。
- `fields`：按顺序序列化的字段。
- `subsections`：带名称和版本的可选扩展块。
- `pre_save`：序列化前把运行时状态整理成可迁移状态。
- `pre_load`：读取字段前准备内存或清理旧状态。
- `post_load`：所有字段和 subsection 加载后重建派生状态。
- `needed`：决定整个 section 或 subsection 是否需要发送。
- `priority`：约束不同设备状态的保存顺序。
- `unmigratable`：明确标记设备不可迁移。

保存时的基本执行顺序是：

```text
needed()
  ↓
pre_save()
  ↓
fields[]
  ↓
subsections[]
  ↓
post_save()
```

加载时是：

```text
检查 version
  ↓
pre_load()
  ↓
fields[]
  ↓
subsections[]
  ↓
post_load()
```

对应的主要实现函数是：

- `vmstate_save_vmsd_v()`
- `vmstate_load_vmsd()`

## 3. VMStateField：描述“对象里的一个成员”

`VMStateField` 并不真正执行序列化。它描述如何在 `opaque` 对象中找到字段，以及这个字段是什么形状：

```c
struct VMStateField {
    const char *name;
    size_t offset;

    size_t size;
    int num;
    size_t num_offset;

    const VMStateInfo *info;
    enum VMStateFlags flags;

    const VMStateDescription *vmsd;

    int version_id;
    bool (*field_exists)(void *opaque, int version_id);
};
```

核心关系是：

```text
opaque
  +
field->offset
  =
字段内存地址
```

然后根据字段类型执行不同操作：

- 普通整数：调用 `field->info->save/load`。
- 数组：根据 `num` 循环。
- 变长数组：从 `opaque + num_offset` 取得元素数。
- 指针：先解引用。
- 嵌套结构：递归进入 `field->vmsd`。

例如：

```c
VMSTATE_UINT32(status, FooState)
```

宏最终大致生成：

```c
{
    .name   = "status",
    .offset = offsetof(FooState, status),
    .size   = sizeof(uint32_t),
    .flags  = VMS_SINGLE,
    .info   = &vmstate_info_uint32,
}
```

这些宏还利用编译期类型检查，避免把一个 `uint64_t` 字段错误地声明成 `VMSTATE_UINT32`。

## 4. VMStateInfo：描述“一个元素怎么编码”

`VMStateInfo` 是最底层的类型 codec：

```c
struct VMStateInfo {
    const char *name;

    bool (*load)(QEMUFile *f, void *pv, size_t size,
                 const VMStateField *field, Error **errp);

    bool (*save)(QEMUFile *f, void *pv, size_t size,
                 const VMStateField *field,
                 JSONWriter *vmdesc, Error **errp);
};
```

旧接口叫 `get/put`，新代码建议使用 `load/save`。

比如 `vmstate_info_uint32`：

```c
static bool load_uint32(QEMUFile *f, void *pv, size_t size,
                        const VMStateField *field, Error **errp)
{
    uint32_t *v = pv;
    qemu_get_be32s(f, v);
    return true;
}

static bool save_uint32(QEMUFile *f, void *pv, size_t size,
                        const VMStateField *field,
                        JSONWriter *vmdesc, Error **errp)
{
    uint32_t *v = pv;
    qemu_put_be32s(f, v);
    return true;
}

const VMStateInfo vmstate_info_uint32 = {
    .name = "uint32",
    .load = load_uint32,
    .save = save_uint32,
};
```

所以一个字段的完整调用链是：

```text
VMStateDescription.fields[i]
        ↓
VMStateField.offset
        ↓
pv = opaque + offset
        ↓
VMStateInfo.save(f, pv, ...)
        ↓
qemu_put_be32s()
        ↓
QEMUFile
```

`VMStateInfo` 不一定只处理基础类型。它也可以是一整个子系统的适配器，例如 virtio。

## 5. SaveStateEntry：运行时注册项

`VMStateDescription` 一般是静态 `const` 元数据。设备真正实例化后，还需要生成运行时注册项 `SaveStateEntry`：

```c
typedef struct SaveStateEntry {
    char idstr[256];
    uint32_t instance_id;
    int version_id;
    int section_id;

    const SaveVMHandlers *ops;
    const VMStateDescription *vmsd;
    void *opaque;
} SaveStateEntry;
```

它把三件事绑定起来：

```text
迁移流中的身份
idstr + instance_id + version_id

对象内存
opaque

序列化方法
vmsd 或 ops
```

对于普通 qdev 设备，class 初始化时设置：

```c
dc->vmsd = &vmstate_foo;
```

设备 realize 后，`device_set_realized()` 会自动调用：

```c
vmstate_register_with_alias_id()
```

`vmstate_register_with_alias_id()` 创建 `SaveStateEntry`，然后将其插入全局
`savevm_state.handlers`。

目的端启动时也会根据相同的 machine 和 device 拓扑创建自己的 `SaveStateEntry`。收到 section 后，通过：

```text
idstr + instance_id
```

找到目的端对应设备，再把内容加载到该 entry 的 `opaque` 中。

因此，热迁移要求源端和目的端的设备拓扑及稳定标识相匹配。

## 6. 迁移流中实际有什么

`save_section_header()` 和 `vmstate_save()` 将普通设备大致写成：

```text
QEMU_VM_SECTION_FULL
section_id
idstr
instance_id
version_id
VMSD payload
optional subsection payload
section footer
```

需要特别注意：

> 普通 `fields[]` 的字段名通常不在 wire format 中。

例如：

```c
VMSTATE_UINT32(a, FooState),
VMSTATE_UINT32(b, FooState),
```

在线上主要就是：

```text
4-byte a
4-byte b
```

它不是自描述的 TLV：

```text
"name=a, value=..."
"name=b, value=..."
```

所以随意调整字段顺序、删除字段或在中间插入字段，会破坏兼容性。

`subsection` 比较特殊，它携带 subsection 的名称和版本，因此适合追加可选状态。

## 7. SaveVMHandlers 和 VMSD 的关系

`SaveStateEntry` 有两种主要处理方式：

```text
se->vmsd
    普通、小型、一次性设备状态

se->ops = SaveVMHandlers
    RAM、VFIO、大型 bitmap、需要迭代发送的状态
    以及部分 legacy 设备
```

`SaveVMHandlers` 支持迁移生命周期：

```text
save_setup()
    ↓
save_live_iterate()   多次调用
    ↓
save_complete()       switchover 时最后一次
```

`migration_completion_precopy()` 和
`qemu_savevm_state_complete_precopy()` 实现的 precopy 最后阶段顺序是：

```text
源 VM 停止
  ↓
iterable 状态完成
  ↓
普通 non-iterable VMSD 设备状态
  ↓
EOF / flush
```

所以普通设备的 VMSD 状态一般是在 switchover downtime 内、VM 已停止时保存；RAM 则在 VM 运行期间已经迭代发送了很多轮。

## 8. virtio 如何嵌入这套结构

virtio 是一个很有代表性的“自定义 `VMStateInfo` 包装器”。

以 virtio-net 为例，最外层描述符 `vmstate_virtio_net` 是：

```c
static const VMStateDescription vmstate_virtio_net = {
    .name = "virtio-net",
    .fields = (const VMStateField[]) {
        VMSTATE_VIRTIO_DEVICE,
        VMSTATE_END_OF_LIST()
    },
};
```

`VMSTATE_VIRTIO_DEVICE` 展开后不是普通整数，而是：

```c
{
    .name = "virtio",
    .info = &virtio_vmstate_info,
    .flags = VMS_SINGLE,
}
```

这个自定义 `VMStateInfo`，即 `virtio_vmstate_info`，其 `put/get`
分别通过 `virtio_device_put()` 和 `virtio_device_get()` 调用：

```text
virtio_device_put()
    └── virtio_save()

virtio_device_get()
    └── virtio_load()
```

而 `virtio_save()` 内部又继续调用：

```text
transport.save_config()
virtio core fields
transport.save_queue()
VirtioDeviceClass.save/vmsd
virtio subsections
```

因此，virtio 的实际结构是：

```text
DeviceClass.vmsd
vmstate_virtio_net                顶层注册和 section 身份
        ↓
VMStateField
VMSTATE_VIRTIO_DEVICE
        ↓
VMStateInfo
virtio_vmstate_info              自定义编解码适配器
        ↓
virtio_save()/virtio_load()
        ├── VirtioBusClass       PCI/MMIO/CCW transport
        ├── VirtIODevice         core 状态
        └── VirtioDeviceClass.vmsd
              virtio-net/blk 的内部设备状态
```

这里容易混淆的地方是：

- `DeviceClass::vmsd`：顶层 qdev 注册项。
- `VirtioDeviceClass::vmsd`：由 virtio core 手动调用的设备特有状态。

这正是 `docs/devel/migration/virtio.rst` 中三层状态在 VMState 框架里的具体落地。

## 9. 总结

一句话记忆：

> `VMStateDescription` 描述对象，`VMStateField` 定位成员，`VMStateInfo` 编解码一个成员，`SaveStateEntry` 把某个运行时对象注册到全局迁移流，`QEMUFile` 负责实际传输字节。


## VMStateDescription
```c
struct VMStateDescription {
    const char *name;
    int unmigratable;
    int version_id;
    int minimum_version_id;
    MigrationPriority priority;
    int (*pre_load)(void *opaque);
    int (*post_load)(void *opaque, int version_id);
    int (*pre_save)(void *opaque);
    int (*post_save)(void *opaque);
    bool (*needed)(void *opaque);
    bool (*dev_unplug_pending)(void *opaque);

    const VMStateField *fields;
    const VMStateDescription **subsections;
};
```

```c
static const VMStateDescription vmstate_hpet = {
    .name = "hpet",
    .version_id = 2,
    .minimum_version_id = 1,
    .pre_save = hpet_pre_save,
    .pre_load = hpet_pre_load,
    .post_load = hpet_post_load,
    .fields = (VMStateField[]) {
        VMSTATE_UINT64(config, HPETState),
        VMSTATE_UINT64(isr, HPETState),
        VMSTATE_UINT64(hpet_counter, HPETState),
        VMSTATE_UINT8_V(num_timers, HPETState, 2),
        VMSTATE_VALIDATE("num_timers in range", hpet_validate_num_timers),
        VMSTATE_STRUCT_VARRAY_UINT8(timer, HPETState, num_timers, 0,
                                    vmstate_hpet_timer, HPETTimer),
        VMSTATE_END_OF_LIST()
    },
    .subsections = (const VMStateDescription*[]) {
        &vmstate_hpet_rtc_irq_level,
        &vmstate_hpet_offset,
        NULL
    }
};
```

### post_copy 的调用时机是什么
和 post copy 机制无关。

- vmstate_load_state 中被调用

开始的执行一次:
```txt
#0  vmstate_load_state (f=f@entry=0x555556883d90, vmsd=vmsd@entry=0x5555561adc00 <vmstate_configuration>, opaque=opaque@entry=0x5555564ae940 <savevm_state>, version_id=version_id@entry=0) at ../migration/vmstate.c:80
#1  0x0000555555a18db9 in qemu_loadvm_state_header (f=0x555556883d90) at ../migration/savevm.c:2497
#2  qemu_loadvm_state (f=0x555556883d90) at ../migration/savevm.c:2701
#3  0x0000555555a0660e in process_incoming_migration_co (opaque=<optimized out>) at ../migration/migration.c:591
#4  0x0000555555d8d08b in coroutine_trampoline (i0=<optimized out>, i1=<optimized out>) at ../util/coroutine-ucontext.c:177
#5  0x00007ffff6a537a0 in ?? () from /nix/store/4nlgxhb09sdr51nc9hdm8az5b08vzkgx-glibc-2.35-163/lib/libc.so.6
```

之后多次执行：
- ??
  - coroutine_trampoline
    - process_incoming_migration_co
      - qemu_loadvm_state
        - qemu_loadvm_state_main
          - qemu_loadvm_section_start_full
            - vmstate_load_state

- vmstate_load_state
  - hpet_pre_load
  - 我不知道中间发生了什么 ?
  - hpet_post_load

### 为什么需要拆分为 pre_load 和 post_load

可以。如果资源只在加载完成后才需要，那么放进 post_load() 往往更合理。pre_load() 存在，是因为部分资源本身就是“字段加载的目标”，必须先准备好。

例如 vhost：

```txt
static bool vhost_inflight_buffer_pre_load(void *opaque, Error **errp)
{
    struct vhost_inflight *inflight = opaque;

    inflight->addr = qemu_memfd_alloc(..., inflight->size, ...);
    return inflight->addr != NULL;
}
```

接下来 VMState 会直接把迁移数据写入这个地址：

```txt
VMSTATE_VBUFFER_UINT64(addr, struct vhost_inflight, 0, NULL, size)
```

顺序是：

```txt

父 VMState 加载 size
        ↓
子 VMState pre_load：根据 size 分配 addr
        ↓
加载 buffer：直接写入 addr
        ↓
post_load：检查和启用
```

源码在 hw/virtio/vhost.c:2022。

如果把分配放进 post_load()：

加载 buffer，试图写入 addr == NULL
        ↓
还没执行到 post_load 就已经失败

另一个重要用途是处理“可选字段/可选 subsection 缺失”。

```txt
static int cpu_common_pre_load(void *opaque)
{
    CPUState *cpu = opaque;

    cpu->exception_index = -1;  /* 缺失时的协议默认值 */
    return 0;
}
```

然后：

- 迁移流里有 subsection：加载值覆盖 -1
- 迁移流里没有 subsection：保留 -1

如果放在 post_load() 无条件赋值，就会把刚加载进来的正确值覆盖掉。虽然也可以在 post_load() 判断“字段是否出现”，但这通常需要额外的 presence flag；
而 pre_load 设置默认值 → load 可选覆盖 更直接。

所以可以按资源用途来划分：

 工作                                 合适阶段
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━
 加载字段所需的内存、数组、缓冲区     pre_load
───────────────────────────────────  ───────────
 可选字段缺失时的默认值               pre_load
───────────────────────────────────  ───────────
 不依赖迁移数据的提前合法性检查       pre_load
───────────────────────────────────  ───────────
 根据已加载字段创建运行时资源         post_load
───────────────────────────────────  ───────────
 跨字段校验、旧版本转换               post_load
───────────────────────────────────  ───────────
 恢复 timer、IRQ、KVM/backend 状态    post_load

例如下面这种情况，确实没必要使用 pre_load：

```c
/* fields 只加载 length、configuration 等普通字段 */

static int device_post_load(void *opaque, int version_id)
{
    Device *s = opaque;

    s->runtime_buffer = g_malloc(s->length);
    rebuild_runtime_state(s);
    return 0;
}
```

核心判断就是：

> 资源是“load 的前提”，放 pre_load；资源是“load 结果的派生物”，放 post_load。

另外，post_load() 只有在全部字段和 subsection 都成功加载之后才会执行；因此把能够延迟的分配放到 post_load()，确实还能减少迁移流中途损坏时的资源清理问题。

**这个分析看，qemu 还有一些实现的问题**

## 一些代码分析

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - qemu_savevm_state_header
          - qemu_savevm_send_configuration
            - vmstate_save_vmsd_v

```c
static const VMStateDescription vmstate_configuration = {
    .name = "configuration",
    .version_id = 1,
    .pre_load = configuration_pre_load,
    .post_load = configuration_post_load,
    .pre_save = configuration_pre_save,
    .post_save = configuration_post_save,
    .fields = (const VMStateField[]) {
        VMSTATE_UINT32(len, SaveState),
        VMSTATE_VBUFFER_ALLOC_UINT32(name, SaveState, 0, NULL, len),
        VMSTATE_END_OF_LIST()
    },
    .subsections = (const VMStateDescription * const []) {
        &vmstate_target_page_bits,
        &vmstate_capabilites,
        &vmstate_uuid,
        NULL
    }
};
```


首先
- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - qemu_savevm_state_header
          - vmstate_save_state
            - vmstate_save_state_v

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_iterate (那些注册了 SaveVMHandlers 的先完成这些 iterate 操作)
            - ram_save_iterate
	    - dirty_bitmap_save_iterate
          - migration_completion
            - migration_completion_precopy
              - qemu_savevm_state_complete_precopy
                - qemu_savevm_state_complete_precopy_non_iterable
                  - vmstate_save
                    - vmstate_save_state
                      - vmstate_save_state_v
                        - virtio_gpu_save (调用到具体 hook 中)

在 qemu_savevm_state_complete_precopy_non_iterable 中
```c
    QTAILQ_FOREACH(se, &savevm_state.handlers, entry) {
        if (se->vmsd && se->vmsd->early_setup) {
            /* Already saved during qemu_savevm_state_setup(). */
            continue;
        }
        printf(" %s\n",  se->idstr);
```

:sort u 之后，结果为:
```txt
0000:00:00.0/I440FX
0000:00:01.0/PIIX3
0000:00:01.1/ide
0000:00:01.2/uhci
0000:00:01.3/piix4_pm
0000:00:02.0/0:1:0/scsi-disk
0000:00:02.0/virtio-scsi
0000:00:03.0/virtio-blk
0000:00:04.0/virtio-blk
0000:00:05.0/virtio-net
0000:00:06.0/virtio-net
0000:00:07.0/virtio-balloon
0000:00:08.0/ipmi-interface-pci-kcs
0000:00:0a.0/virtio-gpu
0000:00:0b.0/virtio-console
0000:00:0e.0/pcie-root-port
0000:00:0f.0/virtio-input
0000:00:10.0/2/usb-kbd
0000:00:10.0/3/usb-ptr
0000:00:10.0/xhci
PCIBUS
PCIHost
acpi_build
apic
cpu
cpu_common
dirty-bitmap
dma
fdc
fw_cfg
globalstate
i2c_bus
i8254
i8259
ioapic
ipmi-bmc-sim
kvm-tpr-opt
kvmclock
mc146818rtc
pckbd
pcspk
port92
ps2kbd
ps2mouse
ram
serial
slirp
smbus-eeprom
timer
vmmouse
```
可以看到这里是有 ram 的

所有的东西都是保存这个全局变量中:
```c
static SaveState savevm_state = {
    .handlers = QTAILQ_HEAD_INITIALIZER(savevm_state.handlers),
    .handler_pri_head = { [0 ... MIG_PRI_MAX] = NULL },
    .global_section_id = 0,
};
```

例如这里的东西，

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qmp_x_exit_preconfig
        - qemu_init_board
          - machine_run_board_init
            - pc_init1
              - x86_cpus_init
                - x86_cpu_new
                  - qdev_realize
                    - object_property_set_bool
                      - object_property_set_qobject
                        - object_property_set
                          - property_set_bool
                            - device_set_realized
                              - x86_cpu_realizefn
                                - cpu_exec_realizefn
                                  - cpu_vmstate_register
                                    - vmstate_register
                                      - vmstate_register_with_alias_id
                                        - savevm_state_handler_insert

ram 的注册方法:
```txt
register_savevm_live("ram", 0, 4, &savevm_ram_handlers, &ram_state);
```
而一般在 vmstate_register_with_alias_id 可以看到，是没有注册 ops 的

从这个的条件的理解，那么 SaveVMHandlers 如何被修改?

不过，现在，我们知道这两个结构体是做什么的了:

```c
typedef struct SaveState {
  // ...
};


struct SaveStateEntry {
  // ...
};
```


- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run

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
