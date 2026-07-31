# migration 为什么需要有优先级
<!-- 4ff4b23d-acb0-443c-b87c-dc04ff6fda06 -->

我可以理解为什么需要 priority 的，但是为什么

```c
typedef enum {
    MIG_PRI_UNINITIALIZED = 0,  /* An uninitialized priority field maps to */
                                /* MIG_PRI_DEFAULT in save_state_priority */

    MIG_PRI_LOW,                /* Must happen after default */
    MIG_PRI_DEFAULT,
    MIG_PRI_IOMMU,              /* Must happen before PCI devices */
    MIG_PRI_PCI_BUS,            /* Must happen before IOMMU */
    MIG_PRI_VIRTIO_MEM,         /* Must happen before IOMMU */
    MIG_PRI_GICV3_ITS,          /* Must happen before PCI devices */
    MIG_PRI_GICV3,              /* Must happen before the ITS */
    MIG_PRI_MAX,
} MigrationPriority;
```

savevm_state_handler_insert 的时候参考 VMStateDescription::priority ，例如:

- object_property_set
  - property_set_bool
    - device_set_realized
      - apic_common_realize
        - vmstate_register_with_alias_id
          - savevm_state_handler_insert

- save 时：高 priority 的设备状态先写入 migration stream。
- load 时：按照 stream 中的顺序加载，所以也是高 priority 先加载。

也就是说，数值越大，越早处理:
本地文档也明确写着：“Numerically higher priorities are loaded earlier”，见 docs/devel/migration.rst:524。

## virtio-mem 使用 MIG_PRI_VIRTIO_MEM
看一个 qemu 的经典提交
```diff
History:        #0
Commit:         0fd7616e0f1171b8149bb71f59e23ab048a8df83
Author:         David Hildenbrand <david@kernel.org>
Committer:      Eduardo Habkost <ehabkost@redhat.com>
Author Date:    Tue 13 Apr 2021 05:55:27 PM CST
Committer Date: Fri 09 Jul 2021 03:54:45 AM CST

vfio: Support for RamDiscardManager in the vIOMMU case

vIOMMU support works already with RamDiscardManager as long as guests only
map populated memory. Both, populated and discarded memory is mapped
into &address_space_memory, where vfio_get_xlat_addr() will find that
memory, to create the vfio mapping.

Sane guests will never map discarded memory (e.g., unplugged memory
blocks in virtio-mem) into an IOMMU - or keep it mapped into an IOMMU while
memory is getting discarded. However, there are two cases where a malicious
guests could trigger pinning of more memory than intended.

One case is easy to handle: the guest trying to map discarded memory
into an IOMMU.

The other case is harder to handle: the guest keeping memory mapped in
the IOMMU while it is getting discarded. We would have to walk over all
mappings when discarding memory and identify if any mapping would be a
violation. Let's keep it simple for now and print a warning, indicating
that setting RLIMIT_MEMLOCK can mitigate such attacks.

We have to take care of incoming migration: at the point the
IOMMUs get restored and start creating mappings in vfio, RamDiscardManager
implementations might not be back up and running yet: let's add runstate
priorities to enforce the order when restoring.
```

配合 memory_translate_iotlb 中的，真的是相当复杂啊:
```c
    } else if (memory_region_has_ram_discard_manager(mr)) {
        RamDiscardManager *rdm = memory_region_get_ram_discard_manager(mr);
        MemoryRegionSection tmp = {
            .mr = mr,
            .offset_within_region = xlat,
            .size = int128_make64(len),
        };
        /*
         * Malicious VMs can map memory into the IOMMU, which is expected
         * to remain discarded. vfio will pin all pages, populating memory.
         * Disallow that. vmstate priorities make sure any RamDiscardManager
         * were already restored before IOMMUs are restored.
         */
        if (!ram_discard_manager_is_populated(rdm, &tmp)) {
            error_setg(errp, "iommu map to discarded memory (e.g., unplugged"
                         " via virtio-mem): %" HWADDR_PRIx "",
                         iotlb->translated_addr);
            return NULL;
        }
    }
```
## IOMMU 和 PCI 为什么也是需要有依赖的?

```txt
    MIG_PRI_IOMMU,              /* Must happen before PCI devices */
    MIG_PRI_PCI_BUS,            /* Must happen before IOMMU */
```

## APIC
```txt
History:        #0
Commit:         d943cef76090b5255e68ba38ce6ddf20537b07bc
Author:         Yanfei Xu <yanfei.xu@bytedance.com>
Committer:      Peter Xu <peterx@redhat.com>
Author Date:    Mon 18 Aug 2025 09:11:27 PM CST
Committer Date: Fri 03 Oct 2025 09:48:02 PM CST

migration: ensure APIC is loaded prior to VFIO PCI devices

The load procedure of VFIO PCI devices involves setting up IRT
for each VFIO PCI devices. This requires determining whether an
interrupt is single-destination interrupt to decide between
Posted Interrupt(PI) or remapping mode for the IRTE. However,
determining this may require accessing the VM's APIC registers.

For example:
ioctl(vbasedev->fd, VFIO_DEVICE_SET_IRQS, irqs)
  ...
    kvm_arch_irq_bypass_add_producer
      kvm_x86_call(pi_update_irte)
        vmx_pi_update_irte
          kvm_intr_is_single_vcpu

If the LAPIC has not been loaded yet, interrupts will use remapping
mode. To prevent the fallback of interrupt mode, keep APIC is always
loaded prior to VFIO PCI devices.
```

## 总结 kimi
priority 是什么、解决什么问题

VMState 的每个 section 本来按设备 realize（注册 vmstate）的先后顺序迁移。问题在于：恢复一个设备的状态时，经常会"顺带"访问或重建其他设备的状态，于是恢复顺序就成了
隐式的跨设备依赖。priority 就是把这个隐式顺序变成显式声明。

机制在 migration/savevm.c:749 的 savevm_state_handler_insert：

- 每个 SaveStateEntry 从 VMStateDescription::priority 取优先级（save_state_priority，未设置 = MIG_PRI_DEFAULT，见 savevm.c:741）。
- 插入时按枚举值从大到小排进 savevm_state.handlers 链表：值越大越靠前。所以 MIG_PRI_GICV3(7) 最先，MIG_PRI_LOW(1) 最后。
- 发送端 qemu_savevm_state_iterate / complete_precopy 按链表顺序写 section；接收端按流中 section 到达顺序逐个 vmstate_load。所以恢复顺序 = 发送顺序 = priority 顺序。


当前排序（include/migration/vmstate.h:175）：

```
  GICV3 > GICV3_ITS > APIC > VIRTIO_MEM > PCI_BUS > IOMMU > DEFAULT > LOW
```

为什么这些依赖存在

- GICv3 先于 ITS，ITS/APIC/IOMMU 先于 PCI 设备：恢复 PCI 设备配置时会触发中断路由/IOMMU 映射重建，中断控制器必须先就位。
- PCI_BUS 先于 IOMMU：IOMMU 的 post_load（如 vtd_post_load）会 replay 全部映射、遍历 PCI 设备做地址翻译，这要求 bridge 的 MEM/IO 窗口和地址空间已经恢复，否则翻译 落到错误的地址空间。
- VIRTIO_MEM 先于 IOMMU：就是你文档里 0fd7616e 那个 commit——vfio 在 IOMMU restore 时会 pin 页，memory_translate_iotlb 要查 RamDiscardManager 的 populated 位图 ，virtio-mem 状态必须先恢复。
- APIC 先于 VFIO PCI：你文档里 d943cef7 那个 commit——VFIO_DEVICE_SET_IRQS → vmx_pi_update_irte → kvm_intr_is_single_vcpu 要读 LAPIC 状态，APIC 没恢复就会错误退化 成 remapping 模式。
- MIG_PRI_LOW 目前只有一个用户：hw/vfio/cpr-legacy.c:124 的 vfio-container，注释写明 "Must happen after devices and groups"。

新设备想"最后迁移"怎么办

在你的 VMStateDescription 里加一行：

```c
  static const VMStateDescription vmstate_mydev = {
      .name = "mydev",
      .priority = MIG_PRI_LOW,
      ...
  };
```

但有三个坑：

1. 必须走 vmstate 注册路径（dc->vmsd 或 vmstate_register*）。老式 register_savevm_live（SaveVMHandlers ops）的 entry se->vmsd 为 NULL，优先级永远是 DEFAULT，没法设。
2. MIG_PRI_LOW 不等于"绝对最后一个"。savevm_state_handler_insert 对同优先级的 entry 保持注册（realize）顺序。如果你必须排在 vfio-container 这种已有的 MIG_PRI_LOW
   之后，就得在枚举里新增一个更小的值（插在 MIG_PRI_LOW 之前）。注意：枚举值不上线传输，只是两端本地的排序依据，section 顺序完全由发送端决定，所以只要发送端 QEMU
   带了你的改动即可，不破坏迁移流兼容性。
3. 先想清楚是否真需要"最后"。通常需求只是"晚于某个我依赖的设备"，那选一个比它低的优先级即可；"全局最后"本身几乎不是真实需求

另外值得检查的一点：如果你的设备恢复时依赖的对象目前也是 DEFAULT，
那单纯靠注册顺序往往"碰巧"就是对的——加 priority 的意义在于把运气变成契约，避免将来有人调整 realize 顺序时悄悄踩雷。

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
