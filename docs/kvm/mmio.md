似乎关注下 KVM_FAST_MMIO_BUS
```c
enum kvm_bus {
	KVM_MMIO_BUS,
	KVM_PIO_BUS,
	KVM_VIRTIO_CCW_NOTIFY_BUS,
	KVM_FAST_MMIO_BUS,
	KVM_NR_BUSES
};
```
mmio 聚合?

## KVM_CAP_COALESCED_MMIO

Documentation/virt/kvm/api.rst
```txt
4.116 KVM_(UN)REGISTER_COALESCED_MMIO
-------------------------------------

:Capability: KVM_CAP_COALESCED_MMIO (for coalesced mmio)
	     KVM_CAP_COALESCED_PIO (for coalesced pio)
:Architectures: all
:Type: vm ioctl
:Parameters: struct kvm_coalesced_mmio_zone
:Returns: 0 on success, < 0 on error

Coalesced I/O is a performance optimization that defers hardware
register write emulation so that userspace exits are avoided.  It is
typically used to reduce the overhead of emulating frequently accessed
hardware registers.

When a hardware register is configured for coalesced I/O, write accesses
do not exit to userspace and their value is recorded in a ring buffer
that is shared between kernel and userspace.

Coalesced I/O is used if one or more write accesses to a hardware
register can be deferred until a read or a write to another hardware
register on the same device.  This last access will cause a vmexit and
userspace will process accesses from the ring buffer before emulating
it. That will avoid exiting to userspace on repeated writes.

Coalesced pio is based on coalesced mmio. There is little difference
between coalesced mmio and pio except that coalesced pio records accesses
to I/O ports.
```
原理看上去是相当简单的。

检查下 qemu 的使用:

- main 
  - qemu_init 
    - qmp_x_exit_preconfig 
      - qemu_init_board 
        - machine_run_board_init 
          - pc_init1 
            - pc_vga_init 
              - pci_create_simple 
                - pci_realize_and_unref 
                  - qdev_realize_and_unref 
                    - qdev_realize 
                      - object_property_set_bool 
                        - object_property_set_qobject 
                          - object_property_set 
                            - property_set_bool 
                              - device_set_realized 
                                - pci_qdev_realize 
                                  - pci_std_vga_realize 
                                    - vga_init 
                                      - memory_region_set_coalescing 
                                        - memory_region_add_coalescing 
                                          - memory_region_update_coalesced_range 
                                            - flat_range_coalesced_io_notify 
                                              - kvm_coalesce_mmio_region 

看了他的使用场景，就更加清晰了，原来是给 vga 用的。

## mmio 会映射 memory slot 吗?

## mark_mmio_spte 什么时候会被调用，暂时没发现

## 看看 dbuegfs 中相关的统计

## parameter mmio_caching 是做什么的?

## 关注这个 tracepoint 中的内容 kvm::kvm_mmio

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
