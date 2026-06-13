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

## 经典问题，为什么 nvme 不可以被热迁移?

我猜测是重连的问题，容易出现数据丢失之类的问题。

## 通过 stop / cont 就可以观察到吧?

观察不到 : virtio_gpu_save

```txt
stop
hmp_stop()
vm_stop(RUN_STATE_PAUSED)
cpu_stop_current()
qemu_clock_pause()

cont

hmp_cont()
vm_start()
qemu_clock_resume()
cpu_resume()
```

才发现 stop/cont 不会去保存 cpu 的状态，只是用来 savevm 和 stopvm 而已。

```txt
stop/cont savevm migration
```

## 热迁移的兼容性是如何检查的

例如:
```txt
qemu-system-x86_64: Machine type received is 'pc-i440fx-9.2' and local is 'pc-i440fx-11.0'
qemu-system-x86_64: load of migration failed: Invalid argument: post load hook failed for: configuration, version_id: 1, minimum_version: 0, ret: -22
```

## 热迁移为什么需要有优先级
<!-- 4ff4b23d-acb0-443c-b87c-dc04ff6fda06 -->

都是和 vfio 热迁移，viommu 有关

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

### virtio-mem 使用 MIG_PRI_VIRTIO_MEM
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
###  IOMMU 和 PCI 为什么也是需要有依赖的?

```txt
    MIG_PRI_IOMMU,              /* Must happen before PCI devices */
    MIG_PRI_PCI_BUS,            /* Must happen before IOMMU */
```

### APIC
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
