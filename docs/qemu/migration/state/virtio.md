## virtio 有特殊的封装

```c
static const VMStateDescription vmstate_virtio_blk = {
    .name = "virtio-blk",
    .minimum_version_id = 2,
    .version_id = 2,
    .fields = (const VMStateField[]) {
        VMSTATE_VIRTIO_DEVICE,
        VMSTATE_END_OF_LIST()
    },
};
```

## 关于 virtio

```c
struct VirtioDeviceClass {
    // ....
     /* Saving and loading of a device; trying to deprecate save/load
     * use vmsd for new devices.
     */
    void (*save)(VirtIODevice *vdev, QEMUFile *f);
    int (*load)(VirtIODevice *vdev, QEMUFile *f, int version_id);
    /* Post load hook in vmsd is called early while device is processed, and
     * when VirtIODevice isn't fully initialized.  Devices should use this instead,
     * unless they specifically want to verify the migration stream as it's
     * processed, e.g. for bounds checking.
     */
    int (*post_load)(VirtIODevice *vdev);
```

不过，为什么 VirtioDeviceClass::save 和 virtio_net_class_init::load 只有 virtio-blk 注册

virtio-blk , virtio-scsi 和 virtio-net 全部都不一样的注册方法。

## 这个文档表达了什么: codex

这份文档讲的是：QEMU 如何保存和恢复 virtio 设备的迁移状态，以及其中容易踩坑的加载顺序与兼容性问题。

它并不是在描述整个 live migration 算法，而是聚焦 virtio 设备模型的 VMState 序列化。

### 核心问题：一个 virtio 设备的状态分散在三层

文档把状态分成三部分：

 层次           典型状态                                                   负责者
━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 virtio core    status、协商后的 features、队列数量、vring 地址和 index    VirtIODevice
─────────────  ─────────────────────────────────────────────────────────  ─────────────────────────────────────
 transport      PCI 配置、MSI-X、queue vector；或 MMIO/CCW 特有状态        virtio-pci、virtio-mmio、virtio-ccw
─────────────  ─────────────────────────────────────────────────────────  ─────────────────────────────────────
 具体设备       MAC、网卡 offload、未完成的块请求等                        virtio-net、virtio-blk 等

这正是文档所说的 “twisty maze”：docs/devel/migration/virtio.rst:10。

因此，不能简单地把一个 VirtIODevice 结构体复制过去。

### 保存过程

当前代码的大体顺序是：

具体设备的 VMState wrapper
        |
        v
virtio_save()
        |
        +-- transport.save_config()
        |     PCI config、MSI-X、config vector...
        |
        +-- 保存 virtio core
        |     status、ISR、features、config
        |
        +-- 对每条 virtqueue
        |     size、地址、last_avail_idx
        |     transport.save_queue()
        |
        +-- device.save()/device.vmsd
        |     net、blk 等设备特有状态
        |
        +-- virtio subsections
              高位 features、modern/packed queue 状态、
              endianness、额外 transport 状态...

入口可以直接看 hw/virtio/virtio.c:3056。

以 virtio-pci 为例，transport 层会保存 PCI 和 MSI-X 状态：

- PCI/MSI-X/config vector：hw/virtio/virtio-pci.c:87
- 每条 queue 使用的 MSI-X vector：hw/virtio/virtio-pci.c:212
- modern PCI queue 的 enable、desc/avail/used 地址：hw/virtio/virtio-pci.c:98

### 恢复过程及最重要的坑

加载顺序基本是保存顺序的镜像：

transport 状态
    ↓
virtio core 基础状态
    ↓
virtqueue 基础状态
    ↓
设备特有状态
    ↓
virtio subsections
    ↓
完整 feature 校验
    ↓
重建 ring cache、校验 index
    ↓
设备的最终 post_load

对应代码是 hw/virtio/virtio.c:3497。

最关键的问题是：

> 设备特有状态加载时，virtio subsections 可能还没有加载。

例如旧迁移格式最初只有低 32 位 feature；后来新增的高位 feature 被放进 subsection。加载设备状态时，完整的 feature bitmap 还未必可用。

因此，依赖完整 feature 的初始化不能过早进行：

device VMState load/post_load
        这时完整 subsection 可能尚未加载
                    ↓
load virtio subsections
                    ↓
恢复和验证完整 features
                    ↓
VirtioDeviceClass.post_load
        这里才适合做 feature-dependent setup

当前代码在加载 subsection 后才调用 virtio_set_features_nocheck...()，并最终执行 vdc->post_load()：hw/virtio/virtio.c:3606、hw/virtio/virtio.c:3694。

这就是文档最后强调的主要 caveat：docs/devel/migration/virtio.rst:104。

### 为什么 live migration 中 virtio 特别需要考虑

#### 1. Virtqueue 一半在 guest RAM，一半在设备内部

desc/avail/used ring 本身位于 guest RAM，因此 RAM migration 会搬走它们；但是 QEMU 或 vhost 还保存着 host-side 状态，例如：

- last_avail_idx
- used_idx
- packed ring wrap counter
- 已经 pop、但还没有放回 used ring 的请求
- notification 状态和 event index

例如：

guest 把请求 42 放进 avail ring
        ↓
QEMU 已经取走请求，last_avail_idx 前进
        ↓
后端 I/O 尚未完成，因此 used ring 还没有记录请求 42
        ↓
此时开始迁移

如果只迁移 guest RAM：

- 目的端可能再次执行请求 42，造成重复 I/O；
- 或目的端认为它已经被消费，却永远收不到 completion；
- interrupt 也可能丢失或重复。

所以必须让 ring 内存、host index 和设备的 in-flight 状态对应同一个逻辑时间点。当前代码还会重建 ring cache，并检查 guest/host index 是否一致：hw/virtio/
virtio.c:3635。

#### 2. Feature 决定状态应该怎样解释

同一组字段会因为协商 feature 不同而有不同含义。例如：

- legacy virtio 与 VIRTIO_F_VERSION_1
- split ring 与 VIRTIO_F_RING_PACKED
- EVENT_IDX
- indirect descriptor
- mergeable RX buffers
- 各种 net/blk offload

代码明确区分：

- legacy ring：根据 desc 地址重新计算 avail/used 地址；
- modern ring：直接恢复三个地址；
- packed ring：还要恢复 index 和 wrap counter。

见 hw/virtio/virtio.c:3640。

目的端还必须支持源端已经与 guest 协商成功的 features，否则不能安全恢复；当前代码会直接拒绝迁移状态：hw/virtio/virtio.c:3617。

#### 3. Transport 状态不能从设备状态推导出来

virtio-net 并不知道：

- 自己是挂在 PCI、MMIO 还是 CCW 上；
- MSI-X 是否启用；
- 每条 queue 对应哪个 interrupt vector；
- PCI modern common config 当前选中了哪个 queue/feature word。

但 guest 恢复后会继续按照迁移前的 transport 状态访问设备。因此这些状态必须由 transport 层单独迁移。

#### 4. 后端可能不在 QEMU userspace 中

virtio datapath 可能由以下组件处理：

- QEMU userspace
- kernel vhost
- vhost-user 外部进程
- vDPA 硬件或内核驱动

此时状态不只分散在 QEMU 对象中，还可能存在于内核、外部进程或硬件里。迁移切换点必须确保后端停止取新请求，并妥善处理 in-flight 请求，否则仅暂停 vCPU 不够。

这超出了该文档的主要范围，但在实际 live migration 中是非常重要的一层。

#### 5. 迁移格式必须长期向后兼容

早期 virtio 状态大量使用手工的顺序 stream：

field A → field B → field C

不能随意在中间插入字段，否则旧版本目的端会把后续字节全部错位。因此后来新增状态普遍通过可选 subsection 表达。

当前 core 已经有很多 subsection：

- device endian
- 64/128-bit features
- modern virtqueue 地址
- ring size
- packed virtqueue index/wrap counter
- transport extra state

见 hw/virtio/virtio.c:3034。

### 一句话总结

virtio 迁移的本质是：

> 在源端构造一个一致的快照，将 guest RAM 中的共享 virtqueue、QEMU 中的 virtio core 状态、transport 状态、具体设备状态以及可能位于外部 backend 的 in-flight 状态
> 一起转移；然后在完整 feature 已知后，按正确顺序重建目的端。

严格来说，所有有状态设备迁移都要考虑一致性；virtio 格外复杂，是因为它同时具有：

- guest/host 共享内存队列；
- core/transport/device 三层状态；
- feature 驱动的数据格式；
- 高频异步 I/O；
- vhost/vhost-user/vDPA 等外部 datapath；
- 长期积累下来的跨版本迁移格式。


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
