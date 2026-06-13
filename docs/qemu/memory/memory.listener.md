## MemoryListener
<!-- ead5800d-c9fe-4406-ba75-3932d75d6018 -->

当修改地址空间的映射规则的时候，有时候需要执行一下 hook 函数，最典型的就是 kvm 添加了一个 ram 的时候，
这个时候是需要通知内核的 kvm 模块的，而 tcg 的地址空间是纯粹软件模拟，就无需注册这个 hook

MemoryListener 的还有一个主要功能是 dirty memory 的记录, kvm 依赖内核模块，所以总是需要执行一下对应的通知内核操作。

思考经典问题，谁来触发，然后影响到谁?

忽然想到，如果 qemu 支持了 vfio-user ，那么按道理，vfio-user 也应该支持
也需要成为一个 memory listener 执行动作的者吧，所以有 hw/remote/proxy-memory-listener.c 这个文件看看

2026-04-13 忽然想到的问题 ， memory_region_clear_dirty_bitmap
调用的时候，为什么 vhost 没有注册 log_clear ? 似乎也容易理解，
因为 vhost 总是会去配置这些内容的。
```c
    hdev->memory_listener = (MemoryListener) {
        .name = "vhost",
        .begin = vhost_begin,
        .commit = vhost_commit,
        .region_add = vhost_region_addnop,
        .region_nop = vhost_region_addnop,
        .log_start = vhost_log_start,
        .log_stop = vhost_log_stop,
        .log_sync = vhost_log_sync,
        .log_global_start = vhost_log_global_start,
        .log_global_stop = vhost_log_global_stop,
        .priority = MEMORY_LISTENER_PRIORITY_DEV_BACKEND
    };
```

## memory listener 有哪些 hook
<!-- fb127158-e9b3-46df-880e-a8daa91a54b8 -->

看上去非常复杂，实际上就三个:
1. memory 的 flatview : 需要一些经典例子了
2. dirty log : 完全理解
3. ioeventfd : 大致可以理解，因为 ioeventfd 是注册到 kvm 中的，当 memory view 修改了

```c
/**
 * struct MemoryListener: callbacks structure for updates to the physical memory map
 *
 * Allows a component to adjust to changes in the guest-visible memory map.
 * Use with memory_listener_register() and memory_listener_unregister().
 */
struct MemoryListener {
    /**
     * @begin:
     *
     * Called at the beginning of an address space update transaction.
     * Followed by calls to #MemoryListener.region_add(),
     * #MemoryListener.region_del(), #MemoryListener.region_nop(),
     * #MemoryListener.log_start() and #MemoryListener.log_stop() in
     * increasing address order.
     *
     * @listener: The #MemoryListener.
     */
    void (*begin)(MemoryListener *listener);

    /**
     * @commit:
     *
     * Called at the end of an address space update transaction,
     * after the last call to #MemoryListener.region_add(),
     * #MemoryListener.region_del() or #MemoryListener.region_nop(),
     * #MemoryListener.log_start() and #MemoryListener.log_stop().
     *
     * @listener: The #MemoryListener.
     */
    void (*commit)(MemoryListener *listener);

    /**
     * @region_add:
     *
     * Called during an address space update transaction,
     * for a section of the address space that is new in this address space
     * space since the last transaction.
     *
     * @listener: The #MemoryListener.
     * @section: The new #MemoryRegionSection.
     */
    void (*region_add)(MemoryListener *listener, MemoryRegionSection *section);

    /**
     * @region_del:
     *
     * Called during an address space update transaction,
     * for a section of the address space that has disappeared in the address
     * space since the last transaction.
     *
     * @listener: The #MemoryListener.
     * @section: The old #MemoryRegionSection.
     */
    void (*region_del)(MemoryListener *listener, MemoryRegionSection *section);

    /**
     * @region_nop:
     *
     * Called during an address space update transaction,
     * for a section of the address space that is in the same place in the address
     * space as in the last transaction.
     *
     * @listener: The #MemoryListener.
     * @section: The #MemoryRegionSection.
     */
    void (*region_nop)(MemoryListener *listener, MemoryRegionSection *section);

    /**
     * @log_start:
     *
     * Called during an address space update transaction, after
     * one of #MemoryListener.region_add(), #MemoryListener.region_del() or
     * #MemoryListener.region_nop(), if dirty memory logging clients have
     * become active since the last transaction.
     *
     * @listener: The #MemoryListener.
     * @section: The #MemoryRegionSection.
     * @old: A bitmap of dirty memory logging clients that were active in
     * the previous transaction.
     * @new: A bitmap of dirty memory logging clients that are active in
     * the current transaction.
     */
    void (*log_start)(MemoryListener *listener, MemoryRegionSection *section,
                      int old_val, int new_val);

    /**
     * @log_stop:
     *
     * Called during an address space update transaction, after
     * one of #MemoryListener.region_add(), #MemoryListener.region_del() or
     * #MemoryListener.region_nop() and possibly after
     * #MemoryListener.log_start(), if dirty memory logging clients have
     * become inactive since the last transaction.
     *
     * @listener: The #MemoryListener.
     * @section: The #MemoryRegionSection.
     * @old: A bitmap of dirty memory logging clients that were active in
     * the previous transaction.
     * @new: A bitmap of dirty memory logging clients that are active in
     * the current transaction.
     */
    void (*log_stop)(MemoryListener *listener, MemoryRegionSection *section,
                     int old_val, int new_val);

    /**
     * @log_sync:
     *
     * Called by memory_region_snapshot_and_clear_dirty() and
     * memory_global_dirty_log_sync(), before accessing QEMU's "official"
     * copy of the dirty memory bitmap for a #MemoryRegionSection.
     *
     * @listener: The #MemoryListener.
     * @section: The #MemoryRegionSection.
     */
    void (*log_sync)(MemoryListener *listener, MemoryRegionSection *section);

    /**
     * @log_sync_global:
     *
     * This is the global version of @log_sync when the listener does
     * not have a way to synchronize the log with finer granularity.
     * When the listener registers with @log_sync_global defined, then
     * its @log_sync must be NULL.  Vice versa.
     *
     * @listener: The #MemoryListener.
     * @last_stage: The last stage to synchronize the log during migration.
     * The caller should guarantee that the synchronization with true for
     * @last_stage is triggered for once after all VCPUs have been stopped.
     */
    void (*log_sync_global)(MemoryListener *listener, bool last_stage);

    /**
     * @log_clear:
     *
     * Called before reading the dirty memory bitmap for a
     * #MemoryRegionSection.
     *
     * @listener: The #MemoryListener.
     * @section: The #MemoryRegionSection.
     */
    void (*log_clear)(MemoryListener *listener, MemoryRegionSection *section);

    /**
     * @log_global_start:
     *
     * Called by memory_global_dirty_log_start(), which
     * enables the %DIRTY_LOG_MIGRATION client on all memory regions in
     * the address space.  #MemoryListener.log_global_start() is also
     * called when a #MemoryListener is added, if global dirty logging is
     * active at that time.
     *
     * @listener: The #MemoryListener.
     * @errp: pointer to Error*, to store an error if it happens.
     *
     * Return: true on success, else false setting @errp with error.
     */
    bool (*log_global_start)(MemoryListener *listener, Error **errp);

    /**
     * @log_global_stop:
     *
     * Called by memory_global_dirty_log_stop(), which
     * disables the %DIRTY_LOG_MIGRATION client on all memory regions in
     * the address space.
     *
     * @listener: The #MemoryListener.
     */
    void (*log_global_stop)(MemoryListener *listener);

    /**
     * @log_global_after_sync:
     *
     * Called after reading the dirty memory bitmap
     * for any #MemoryRegionSection.
     *
     * @listener: The #MemoryListener.
     */
    void (*log_global_after_sync)(MemoryListener *listener);

    /**
     * @eventfd_add:
     *
     * Called during an address space update transaction,
     * for a section of the address space that has had a new ioeventfd
     * registration since the last transaction.
     *
     * @listener: The #MemoryListener.
     * @section: The new #MemoryRegionSection.
     * @match_data: The @match_data parameter for the new ioeventfd.
     * @data: The @data parameter for the new ioeventfd.
     * @e: The #EventNotifier parameter for the new ioeventfd.
     */
    void (*eventfd_add)(MemoryListener *listener, MemoryRegionSection *section,
                        bool match_data, uint64_t data, EventNotifier *e);

    /**
     * @eventfd_del:
     *
     * Called during an address space update transaction,
     * for a section of the address space that has dropped an ioeventfd
     * registration since the last transaction.
     *
     * @listener: The #MemoryListener.
     * @section: The new #MemoryRegionSection.
     * @match_data: The @match_data parameter for the dropped ioeventfd.
     * @data: The @data parameter for the dropped ioeventfd.
     * @e: The #EventNotifier parameter for the dropped ioeventfd.
     */
    void (*eventfd_del)(MemoryListener *listener, MemoryRegionSection *section,
                        bool match_data, uint64_t data, EventNotifier *e);

    /**
     * @coalesced_io_add:
     *
     * Called during an address space update transaction,
     * for a section of the address space that has had a new coalesced
     * MMIO range registration since the last transaction.
     *
     * @listener: The #MemoryListener.
     * @section: The new #MemoryRegionSection.
     * @addr: The starting address for the coalesced MMIO range.
     * @len: The length of the coalesced MMIO range.
     */
    void (*coalesced_io_add)(MemoryListener *listener, MemoryRegionSection *section,
                               hwaddr addr, hwaddr len);

    /**
     * @coalesced_io_del:
     *
     * Called during an address space update transaction,
     * for a section of the address space that has dropped a coalesced
     * MMIO range since the last transaction.
     *
     * @listener: The #MemoryListener.
     * @section: The new #MemoryRegionSection.
     * @addr: The starting address for the coalesced MMIO range.
     * @len: The length of the coalesced MMIO range.
     */
    void (*coalesced_io_del)(MemoryListener *listener, MemoryRegionSection *section,
                               hwaddr addr, hwaddr len);
    /**
     * @priority:
     *
     * Govern the order in which memory listeners are invoked. Lower priorities
     * are invoked earlier for "add" or "start" callbacks, and later for "delete"
     * or "stop" callbacks.
     */
    unsigned priority;

    /**
     * @name:
     *
     * Name of the listener.  It can be used in contexts where we'd like to
     * identify one memory listener with the rest.
     */
    const char *name;

    /* private: */
    AddressSpace *address_space;
    QTAILQ_ENTRY(MemoryListener) link;
    QTAILQ_ENTRY(MemoryListener) link_as;
};
```

## 理解一下这个 commit 是什么含义

```txt
History:        #0
Commit:         f39b7d2b96e3e73c01bb678cd096f7baf0b9ab39
Author:         David Hildenbrand <david@redhat.com>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Fri 11 Nov 2022 11:47:58 PM CST
Committer Date: Wed 11 Jan 2023 04:59:39 PM CST

kvm: Atomic memslot updates

If we update an existing memslot (e.g., resize, split), we temporarily
remove the memslot to re-add it immediately afterwards. These updates
are not atomic, especially not for KVM VCPU threads, such that we can
get spurious faults.

Let's inhibit most KVM ioctls while performing relevant updates, such
that we can perform the update just as if it would happen atomically
without additional kernel support.

We capture the add/del changes and apply them in the notifier commit
stage instead. There, we can check for overlaps and perform the ioctl
inhibiting only if really required (-> overlap).

To keep things simple we don't perform additional checks that wouldn't
actually result in an overlap -- such as !RAM memory regions in some
cases (see kvm_set_phys_mem()).

To minimize cache-line bouncing, use a separate indicator
(in_ioctl_lock) per CPU.  Also, make sure to hold the kvm_slots_lock
while performing both actions (removing+re-adding).

We have to wait until all IOCTLs were exited and block new ones from
getting executed.

This approach cannot result in a deadlock as long as the inhibitor does
not hold any locks that might hinder an IOCTL from getting finished and
exited - something fairly unusual. The inhibitor will always hold the BQL.

AFAIKs, one possible candidate would be userfaultfd. If a page cannot be
placed (e.g., during postcopy), because we're waiting for a lock, or if the
userfaultfd thread cannot process a fault, because it is waiting for a
lock, there could be a deadlock. However, the BQL is not applicable here,
because any other guest memory access while holding the BQL would already
result in a deadlock.

Nothing else in the kernel should block forever and wait for userspace
intervention.

Note: pause_all_vcpus()/resume_all_vcpus() or
start_exclusive()/end_exclusive() cannot be used, as they either drop
the BQL or require to be called without the BQL - something inhibitors
cannot handle. We need a low-level locking mechanism that is
deadlock-free even when not releasing the BQL.

Signed-off-by: David Hildenbrand <david@redhat.com>
Signed-off-by: Emanuele Giuseppe Esposito <eesposit@redhat.com>
Tested-by: Emanuele Giuseppe Esposito <eesposit@redhat.com>
Message-Id: <20221111154758.1372674-4-eesposit@redhat.com>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

## memory listener 的注册


是给 AddressSpace 来注册 MemoryListener 的，而不是给 memory region 来注册的
```c
void memory_listener_register(MemoryListener *listener, AddressSpace *as)
```

所以，这个就是可以解释，为什么 memory_region_sync_dirty_bitmap 只是用来遍历 MemoryListener 就可以了，
因为需要关联的

- memory_listener_register
  - 将 memory_listener 添加到全局链表 memory_listeners 和 AddressSpace::listeners
  - listener_add_address_space
    - 调用 begin region_add log_start commit 等 hook

将启动的 listeners 和 AddressSpace 都打印出来:

kvm 模式启动:
```txt
kvm-memory memory
kvm-io I/O
vhost memory
vhost memory
(null) device-memory
virtio memory
virtio memory
virtio memory
virtio memory
vfio memory
vhost memory
virtio memory
vhost memory
virtio memory
virtio memory
virtio memory
virtio memory
```

tcg 模式启动:
```txt
vhost memory
vhost memory
tcg cpu-memory-0
tcg cpu-smm-0
(null) device-memory
virtio memory
virtio memory
virtio memory
virtio memory
vhost memory
virtio memory
vhost memory
virtio memory
virtio memory
virtio memory
virtio memory
```
才意识到，在 tcg 模式下，是不需要 eventfd 和 irqfd 的，所以就自然没有
kvm-io 和 kvm-memory 这两个 listener 了。

### VirtIODevice::listener
为什么每一个 virtio device 都是需要注册一个 listener ?

这个就真的理解起来有点难了

### device-memory

machine_memory_devices_init

这个也是很难理解的了，为什么之前没有看到过这个东西
## user : tcg
实话实话，没有完全理解

- CPUAddressSpace 只是 tcg 特有的
- cpu_address_space_init 中注册 memory listener

一共注册两个 hook:
```c
    if (tcg_enabled()) {
        newas->tcg_as_listener.log_global_after_sync = tcg_log_global_after_sync;
        newas->tcg_as_listener.commit = tcg_commit;
        memory_listener_register(&newas->tcg_as_listener, as);
    }
```

- tcg_commit
  - 当 memory_region_transaction_commit 和 将 listener 添加到 (memory_listener_register -> listener_add_address_space) 中间的时候调用
  - 处理一些 RCU，iothread 的问题
  - tlb_flush
- tcg_log_global_after_sync : 当 dirty map sync 之后，需要为了针对于 tcg 特殊调用的 hook

```c
/**
 * CPUAddressSpace: all the information a CPU needs about an AddressSpace
 * @cpu: the CPU whose AddressSpace this is
 * @as: the AddressSpace itself
 * @memory_dispatch: its dispatch pointer (cached, RCU protected)
 * @tcg_as_listener: listener for tracking changes to the AddressSpace
 */
struct CPUAddressSpace {
    CPUState *cpu;
    AddressSpace *as;
    struct AddressSpaceDispatch *memory_dispatch;
    MemoryListener tcg_as_listener;
};
```

#### memory listener hook 的调用位置
实际上，这些 hook 都是 KVM 注册的:

- address_space_set_flatview 会调用两次 address_space_update_topology_pass，进而调用 log_start log_stop region_del region_add 之类的, 因为更新了新的 Flatview 之类，所以也是需要进行一下比如对于 kvm 的通知吧
- memory_listener_register -> listener_add_address_space : address_space 首次注册上 memory listener, 所以将这些 flat range 分别调用一下 listener hook 还是很有必要的
- memory_region_sync_dirty_bitmap
    - log_sync / log_sync_global
- memory_global_dirty_log_start
- memory_global_after_dirty_log_sync
    - log_global_after_sync
- address_space_add_del_ioeventfds : 将经过 memory model 变动之后还存在的 ioeventfd 保存起来
    - eventfd_add / eventfd_del

总结一下，基本就是前面两个 , dirty map 更加复杂一点还要中间几个， 最后一个处理 ioeventfd 的


## user : kvm
- kvm_region_add : 这个很重要，这会让 KVM 最终对于这个 memory section 调用 KVM_SET_USER_MEMORY_REGION, 也即是映射出来一个地址空间来
- kvm_log_start : 其实很容易，这是这个 region 的 flag, 从现在开始记录 kvm 了
- kvm_region_commit 三个配合

- log_sync : 将内核的 dirty log 读去出来，调用者为 memory_region_sync_dirty_bitmap

现在分析出来，实际上，kvm 注册 memory listener 多出来的就只是 dirty log 了


为了 io 也是注册一个的:
```c
static MemoryListener kvm_io_listener = {
    .name = "kvm-io",
    .coalesced_io_add = kvm_coalesce_pio_add,
    .coalesced_io_del = kvm_coalesce_pio_del,
    .eventfd_add = kvm_io_ioeventfd_add,
    .eventfd_del = kvm_io_ioeventfd_del,
    .priority = MEMORY_LISTENER_PRIORITY_DEV_BACKEND,
};
```

## user : vfio
显然 vfio 也是需要的，因为当地址空间变化的时候，vfio 需要修改其内核

- clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - kvm_handle_io
            - address_space_rw
              - address_space_write
                - flatview_write
                  - flatview_write_continue
                    - flatview_write_continue_step
                      - memory_region_dispatch_write
                        - access_with_adjusted_size
                          - memory_region_write_accessor
                            - virtio_write_config
                              - pci_default_write_config
                                - pci_update_mappings
                                  - memory_region_transaction_commit
                                    - memory_region_transaction_commit
                                      - address_space_set_flatview
                                        - address_space_update_topology_pass
                                          - vfio_listener_region_add
                                            - vfio_legacy_dma_map

- clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - kvm_handle_io
            - address_space_rw
              - address_space_write
                - flatview_write
                  - flatview_write_continue
                    - flatview_write_continue_step
                      - memory_region_dispatch_write
                        - access_with_adjusted_size
                          - memory_region_write_accessor
                            - vfio_pci_write_config
                              - pci_default_write_config
                                - pci_update_mappings
                                  - memory_region_transaction_commit
                                    - memory_region_transaction_commit
                                      - address_space_set_flatview
                                        - address_space_update_topology_pass
                                          - vfio_listener_region_add
                                            - vfio_legacy_dma_map


- clone3
  - start_thread
    - qemu_thread_start
      - kvm_vcpu_thread_fn
        - kvm_cpu_exec
          - kvm_handle_io
            - address_space_rw
              - address_space_write
                - flatview_write
                  - flatview_write_continue
                    - flatview_write_continue_step
                      - memory_region_dispatch_write
                        - access_with_adjusted_size
                          - memory_region_write_accessor
                            - memory_region_transaction_commit
                              - memory_region_transaction_commit
                                - address_space_set_flatview
                                  - address_space_update_topology_pass
                                    - vfio_listener_region_add
                                      - vfio_legacy_dma_map

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_create_cli_devices
        - qemu_opts_foreach
          - device_init_func
            - qdev_device_add
              - qdev_device_add_from_qdict
                - qdev_realize
                  - object_property_set_bool
                    - object_property_set_qobject
                      - object_property_set
                        - property_set_bool
                          - device_set_realized
                            - pci_qdev_realize
                              - vfio_realize
                                - vfio_attach_device
                                  - vfio_legacy_attach_device
                                    - vfio_get_group
                                      - vfio_connect_container
                                        - memory_listener_register
                                          - listener_add_address_space
                                            - vfio_listener_region_add
                                              - vfio_legacy_dma_map

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_create_cli_devices
        - qemu_opts_foreach
          - device_init_func
            - qdev_device_add
              - qdev_device_add_from_qdict
                - qdev_realize
                  - object_property_set_bool
                    - object_property_set_qobject
                      - object_property_set
                        - property_set_bool
                          - device_set_realized
                            - pci_qdev_realize
                              - vfio_realize
                                - vfio_attach_device
                                  - vfio_legacy_attach_device
                                    - vfio_get_group
                                      - vfio_connect_container
                                        - memory_listener_register
                                          - listener_add_address_space
                                            - vfio_listener_region_add
                                              - vfio_legacy_dma_map


## vhost

vhost_log_alloc 就是我们该参考的

基本的使用:

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - qemu_savevm_state_setup
          - ram_save_setup
            - ram_init_all
              - ram_init_bitmaps
                - migration_bitmap_sync_precopy
                  - migration_bitmap_sync
                    - memory_global_dirty_log_sync
                      - memory_region_sync_dirty_bitmap
                        - vhost_sync_dirty_bitmap

### 为什么 vhost 自己也需要记录 dirty ?

### vhost_dev_init ，他为什么会注册这么整齐的东西

这个是的确没有想到
- main
  - qemu_init
    - qemu_create_late_backends
      - net_init_clients
        - qemu_opts_foreach
          - net_client_init
            - net_client_init1
              - net_init_tap
                - net_init_tap_one
                  - vhost_net_init
                    - vhost_dev_init

这两个的区别是什么?

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_create_cli_devices
        - qemu_opts_foreach
          - device_init_func
            - qdev_device_add
              - qdev_device_add_from_qdict
                - qdev_realize
                  - object_property_set_bool
                    - object_property_set_qobject
                      - object_property_set
                        - property_set_bool
                          - device_set_realized
                            - pci_qdev_realize
                              - object_property_set_bool
                                - object_property_set_qobject
                                  - object_property_set
                                    - property_set_bool
                                      - device_set_realized
                                        - virtio_device_realize
                                          - vuf_device_realize
                                            - vhost_dev_init

- main
  - qemu_init
    - qmp_x_exit_preconfig
      - qemu_create_cli_devices
        - qemu_opts_foreach
          - device_init_func
            - qdev_device_add
              - qdev_device_add_from_qdict
                - qdev_realize
                  - object_property_set_bool
                    - object_property_set_qobject
                      - object_property_set
                        - property_set_bool
                          - device_set_realized
                            - pci_qdev_realize
                              - object_property_set_bool
                                - object_property_set_qobject
                                  - object_property_set
                                    - property_set_bool
                                      - device_set_realized
                                        - virtio_device_realize
                                          - vhost_user_blk_device_realize
                                            - vhost_user_blk_realize_connect
                                              - vhost_user_blk_connect
                                                - vhost_dev_init


### vhost 的 dirty 行为

```c
static void
vu_log_write(VuDev *dev, uint64_t address, uint64_t length)
{
    uint64_t page;

    // 没有热迁移，这个会直接跳过
    if (!(dev->features & (1ULL << VHOST_F_LOG_ALL)) ||
        !dev->log_table || !length) {
        return;
    }

    assert(dev->log_size > ((address + length - 1) / VHOST_LOG_PAGE / 8));

    page = address / VHOST_LOG_PAGE;
    while (page * VHOST_LOG_PAGE < address + length) {
        vu_log_page(dev->log_table, page);
        page += 1;
    }

    vu_log_kick(dev);
}
```

如果热迁移打开，而且对于 vhost 后端的 page 打印，那么很容易的可以看到:

- coroutine_trampoline
  - vu_blk_virtio_process_req
    - vu_blk_req_complete
      - vu_queue_push
        - vu_queue_fill
          - vu_queue_fill
            - vu_log_queue_fill
              - vu_log_write
                - vu_log_write
                  - vu_log_page

我才意识到，内存的修改的跟踪都是基于 page table 的，内存共享给 vhost ，当 vhost 内存之后，其实完全感知不到的。

## hook : commit


## hook : eventfd

- access_with_adjusted_size
  - memory_region_write_accessor
    - virtio_pci_common_write
      - virtio_pci_start_ioeventfd
        - virtio_bus_start_ioeventfd
          - virtio_device_start_ioeventfd_impl
            - memory_region_transaction_commit
              - memory_region_transaction_commit
                - address_space_update_ioeventfds
                  - address_space_update_ioeventfds
                    - address_space_add_del_ioeventfds
                      - kvm_mem_ioeventfd_add

直接添加了不就好了吗？为什么搞这么复杂?

memory_region_transaction_commit 就是为了这个 ioeventfd 的！

## [ ] hook : log_global_start log_global_stop
1. 为什么这个只有 vhost 和 vfio xen
2. 而且为什么 log_global_start 只有内核中

log_global_start 基本可以说，就是 memory_global_dirty_log_start 调用的

dirtyrate 的路径:

- __clone3
  - start_thread
    - qemu_thread_start
      - get_dirtyrate_thread
        - calculate_dirtyrate
          - calculate_dirtyrate_dirty_bitmap
            - memory_global_dirty_log_start

热迁移的路径:
- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - qemu_savevm_state_setup
          - ram_save_setup
            - ram_init_all
              - ram_init_bitmaps
                - memory_global_dirty_log_start

## [ ] hook : log_start log_stop

## 分析 memory_region_transaction_commit 的动作

- listener_add_address_space
- listener_del_address_space

只有 MemoryListener::commit 会调用

- 调用 kvm_region_commit : 就是检查新增加的或者修改的是不是会内核相同，如果不相同，那么就作出修改，同步到内核中。


memory_region_transaction_commit 的调用时间

## MEMORY_LISTENER_CALL_GLOBAL 为什么还有方向上的区别?

## 为什么 listener 也是有优先级的

```c
#define MEMORY_LISTENER_PRIORITY_MIN            0
#define MEMORY_LISTENER_PRIORITY_ACCEL          10
#define MEMORY_LISTENER_PRIORITY_DEV_BACKEND    10
```


## 为什么 kvm 需要 kvm_log_clear 但是 vhost 不需要？

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_iterate
            - ram_save_iterate
              - ram_find_and_save_block
                - ram_save_host_page
                  - migration_bitmap_clear_dirty
                    - migration_clear_memory_region_dirty_bitmap
                      - migration_clear_memory_region_dirty_bitmap
                        - memory_region_clear_dirty_bitmap
                          - kvm_log_clear

也是 kvm 的特殊原因导致的，kvm 需要调用 ioctl 告诉 kernel ，一切开始重新计算了。
clear 的操作是，就是刚才标脏的 page 现在重新告知给内核 clear 掉。

vhost 的 sync 中: vhost_dev_sync_region
```txt
        /* Data must be read atomically. We don't really need barrier semantics
         * but it's easier to use atomic_* than roll our own. */
        log = qatomic_xchg(from, 0);
```
为了防止的情况是，读上来是 0 ，spdk 中间写入了 1 ，但是我无条件的写 0 ，导致被漏掉了。

kvm 解决办法，只有读上来是 1 的才会写入 0 ，所以没有同步问题。

## 为什么 QEMU 引入了 memory listener
<!-- 0c377eb1-41b4-4363-9172-1b3dab496f67 -->

> [!NOTE]
> 参考神奇海螺的意见，有待验证

1. 客户机物理地址空间（AddressSpace / MemoryRegion）在运行期会动态变化
- RAM / 设备热插拔
- NUMA / memory-backend-file / hugepage
- VFIO 直通
- KVM / TCG / WHPX 等多加速器
- vhost、RDMA、CXL、confidential VM

(是吗，测试一下热插之后，到底是不是会有变化的?)

(也就是，到底谁应该触发 memory listeners ，到底有什么 memory listeners)

2. 之所以使用 listener ，原因是，无法确定执行了操作之后，到底有多少后端同样的来操作:
现在知道的一个例子，当获取 dirty memory 的时候，
dirty 可能来自于 vhost 和 CPU ，所以需要做什么操作都需要询问一下这个两个?

### 1. vfio 直通的虚拟机，热插的内存会自动的被 iommu map 上
是的，基本流程如下

基本上符合预期，从 vfio_container_region_add 到 vfio_legacy_dma_map

- main
  - qemu_default_main
    - qemu_main_loop
      - main_loop_wait
        - os_host_main_loop_wait
          - glib_pollfds_poll
            - g_main_context_dispatch
              - g_main_context_dispatch_unlocked
                - tcp_chr_read
                  - monitor_read
                    - readline_handle_byte
                      - monitor_command_cb
                        - handle_hmp_command
                          - handle_hmp_command_exec
                            - handle_hmp_command_exec
                              - hmp_device_add
                                - qdev_device_add
                                  - qdev_device_add_from_qdict
                                    - qdev_realize
                                      - object_property_set_bool
                                        - object_property_set_qobject
                                          - object_property_set
                                            - property_set_bool
                                              - device_set_realized
                                                - pc_machine_device_plug_cb
                                                  - pc_memory_plug
                                                    - pc_dimm_plug
                                                      - memory_device_plug
                                                        - memory_region_transaction_commit
                                                          - memory_region_transaction_commit
                                                            - address_space_set_flatview
                                                              - address_space_update_topology_pass
                                                                - vfio_container_region_add
                                                                  - vfio_legacy_dma_map

## 先要搞清楚，为什么会出现这么多次的重建吧

例如配合上这个 diff ，region 被操作好几次，但是每一个导致所有的 memory region 都重置的原因都是什么呢?
```diff
diff --git a/hw/vfio/listener.c b/hw/vfio/listener.c
index 2d7d3a464577..43f64e0332f7 100644
--- a/hw/vfio/listener.c
+++ b/hw/vfio/listener.c
@@ -608,6 +608,7 @@ void vfio_container_region_add(VFIOContainer *bcontainer,
         }
     }

+    printf("[martins3:%s:%d] %s %lx %lx %lx\n", __func__, __LINE__, section->mr->name, vaddr, iova, int128_getlo(llsize));
     ret = vfio_container_dma_map(bcontainer, iova, int128_get64(llsize),
                                  vaddr, section->readonly, section->mr);
     if (ret) {
@@ -711,6 +712,8 @@ static void vfio_listener_region_del(MemoryListener *listener,
         try_unmap = false;
     }

+    printf("[martins3:%s:%d] %s %lx %lx\n", __func__, __LINE__,
+           section->mr->name, iova, int128_getlo(llsize));
     if (try_unmap) {
         bool unmap_all = false;
```

```txt
[martins3:vfio_container_region_add:611] mem0 7efe53fff000 0 a0000
[martins3:vfio_container_region_add:611] pc.rom 7f0058200000 c0000 20000
[martins3:vfio_container_region_add:611] pc.bios 7f0058420000 e0000 20000
[martins3:vfio_container_region_add:611] mem0 7efe540ff000 100000 bff00000
[martins3:vfio_container_region_add:611] pc.bios 7f0058400000 fffc0000 40000
[martins3:vfio_container_region_add:611] mem0 7eff13fff000 100000000 140000000
[martins3:virtio_dummy_instance_init:120] 0x564f0a031740
GPU instance init
GPU Realize
[martins3:vfio_listener_region_del:715] pc.rom c0000 20000
[martins3:vfio_listener_region_del:715] pc.bios e0000 20000
[martins3:vfio_listener_region_del:715] mem0 100000 bff00000
[martins3:vfio_container_region_add:611] mem0 7efe540bf000 c0000 10000
[martins3:vfio_container_region_add:611] pc.rom 7f0058210000 d0000 10000
[martins3:vfio_container_region_add:611] pc.bios 7f0058420000 e0000 10000
[martins3:vfio_container_region_add:611] mem0 7efe540ef000 f0000 bff10000
[martins3:vfio_listener_region_del:715] mem0 c0000 10000
[martins3:vfio_listener_region_del:715] pc.rom d0000 10000
[martins3:vfio_listener_region_del:715] pc.bios e0000 10000
[martins3:vfio_listener_region_del:715] mem0 f0000 bff10000
[martins3:vfio_container_region_add:611] mem0 7efe540bf000 c0000 bff40000
[martins3:vfio_container_region_add:611] vga.vram 7efdc5a00000 fe000000 800000
[martins3:vfio_container_region_add:611] 0000:01:00.0 BAR 0 mmaps[0] 7efdb6000000 fa000000 88000
[martins3:vfio_container_region_add:611] 0000:01:00.0 BAR 0 mmaps[0] 7efdb6089000 fa089000 f77000
[martins3:vfio_container_region_add:611] 0000:01:00.0 BAR 1 mmaps[0] 7efda0000000 4280000000 10000000
[martins3:vfio_container_region_add:611] 0000:01:00.0 BAR 3 mmaps[0] 7efdb2000000 4290000000 2000000
[martins3:vfio_container_region_add:611] 0000:01:00.1 BAR 0 mmaps[0] 7f0058444000 fd410000 4000
[martins3:vfio_container_region_add:611] gpu-fb-mem 7efdb4e00000 fc000000 1000000
[martins3:vfio_container_region_add:611] virtio-vga.rom 7efe50600000 fd400000 10000
[martins3:vfio_listener_region_del:715] virtio-vga.rom fd400000 10000
[martins3:vfio_container_region_add:611] virtio-net-pci.rom 7efe50400000 fd380000 40000
[martins3:vfio_listener_region_del:715] virtio-net-pci.rom fd380000 40000
[martins3:vfio_container_region_add:611] virtio-net-pci.rom 7efe50200000 fd3c0000 40000
[martins3:vfio_listener_region_del:715] virtio-net-pci.rom fd3c0000 40000
[martins3:vfio_listener_region_del:715] mem0 c0000 bff40000
[martins3:vfio_container_region_add:611] mem0 7efe540bf000 c0000 c000
[martins3:vfio_container_region_add:611] mem0 7efe540cb000 cc000 3000
[martins3:vfio_container_region_add:611] mem0 7efe540ce000 cf000 1000
[martins3:vfio_container_region_add:611] mem0 7efe540cf000 d0000 20000
[martins3:vfio_container_region_add:611] mem0 7efe540ef000 f0000 10000
[martins3:vfio_container_region_add:611] mem0 7efe540ff000 100000 bff00000
[martins3:vfio_listener_region_del:715] mem0 cf000 1000
[martins3:vfio_listener_region_del:715] mem0 d0000 20000
[martins3:vfio_container_region_add:611] mem0 7efe540ce000 cf000 19000
[martins3:vfio_container_region_add:611] mem0 7efe540e7000 e8000 8000
[martins3:vfio_listener_region_del:715] vga.vram fe000000 800000
[martins3:vfio_container_region_add:611] vga.vram 7efdc5a00000 fe000000 800000
[martins3:vfio_listener_region_del:715] 0000:01:00.0 BAR 0 mmaps[0] fa000000 88000
[martins3:vfio_listener_region_del:715] 0000:01:00.0 BAR 0 mmaps[0] fa089000 f77000
[martins3:vfio_listener_region_del:715] 0000:01:00.0 BAR 1 mmaps[0] 4280000000 10000000
[martins3:vfio_listener_region_del:715] 0000:01:00.0 BAR 3 mmaps[0] 4290000000 2000000
[martins3:vfio_container_region_add:611] 0000:01:00.0 BAR 0 mmaps[0] 7efdb6000000 fa000000 88000
[martins3:vfio_container_region_add:611] 0000:01:00.0 BAR 0 mmaps[0] 7efdb6089000 fa089000 f77000
[martins3:vfio_container_region_add:611] 0000:01:00.0 BAR 1 mmaps[0] 7efda0000000 4280000000 10000000
[martins3:vfio_container_region_add:611] 0000:01:00.0 BAR 3 mmaps[0] 7efdb2000000 4290000000 2000000
[martins3:vfio_listener_region_del:715] 0000:01:00.1 BAR 0 mmaps[0] fd410000 4000
[martins3:vfio_container_region_add:611] 0000:01:00.1 BAR 0 mmaps[0] 7f0058444000 fd410000 4000
[martins3:vfio_listener_region_del:715] gpu-fb-mem fc000000 1000000
[martins3:vfio_container_region_add:611] gpu-fb-mem 7efdb4e00000 fc000000 1000000
```

### 似乎我们疑惑这个问题太多次了

在  vfio_dma_map 中增加
```c
    printf("[martins3:%s:%d] %lx %lx\n", __FUNCTION__, __LINE__, iova, size);
```

启动的是:
```txt
[martins3:vfio_dma_map:671] 0 a0000
[martins3:vfio_dma_map:671] c0000 20000
[martins3:vfio_dma_map:671] e0000 20000
[martins3:vfio_dma_map:671] 100000 bff00000
[martins3:vfio_dma_map:671] fffc0000 40000
[martins3:vfio_dma_map:671] 100000000 740000000 # 启动的 32G 的空间全部注册上
[martins3:vfio_dma_map:671] c0000 10000
[martins3:vfio_dma_map:671] d0000 10000
[martins3:vfio_dma_map:671] e0000 10000
[martins3:vfio_dma_map:671] f0000 bff10000
[martins3:vfio_dma_map:671] c0000 bff40000
[martins3:vfio_dma_map:671] fd000000 1000000
[martins3:vfio_dma_map:671] fea51000 3000
[martins3:vfio_dma_map:671] fea40000 10000
[martins3:vfio_dma_map:671] fea00000 40000
[martins3:vfio_dma_map:671] c0000 b000
[martins3:vfio_dma_map:671] cb000 3000
[martins3:vfio_dma_map:671] ce000 2000
[martins3:vfio_dma_map:671] d0000 20000
[martins3:vfio_dma_map:671] f0000 10000
[martins3:vfio_dma_map:671] 100000 bff00000
[martins3:vfio_dma_map:671] ce000 1a000
[martins3:vfio_dma_map:671] e8000 8000
```

Guest 启动之后，似乎发生了重新映射:
```txt
[    0.369150] PCI host bridge to bus 0000:00
[    0.370060] pci_bus 0000:00: root bus resource [io  0x0000-0x0cf7 window]
[    0.371060] pci_bus 0000:00: root bus resource [io  0x0d00-0xffff window]
[    0.372060] pci_bus 0000:00: root bus resource [mem 0x000a0000-0x000bffff window]
[    0.373061] pci_bus 0000:00: root bus resource [mem 0x40000000-0xfebfffff window]
[    0.374060] pci_bus 0000:00: root bus resource [mem 0x100000000-0x17fffffff window]
[    0.375060] pci_bus 0000:00: root bus resource [bus 00-ff]
[    0.376154] pci 0000:00:00.0: [8086:1237] type 00 class 0x060000
[    0.378143] pci 0000:00:01.0: [8086:7000] type 00 class 0x060100
[    0.381087] pci 0000:00:01.1: [8086:7010] type 00 class 0x010180
[    0.387197] pci 0000:00:01.1: reg 0x20: [io  0xd300-0xd30f]
[    0.390166] pci 0000:00:01.1: legacy IDE quirk: reg 0x10: [io  0x01f0-0x01f7]
[    0.391060] pci 0000:00:01.1: legacy IDE quirk: reg 0x14: [io  0x03f6]
[    0.392060] pci 0000:00:01.1: legacy IDE quirk: reg 0x18: [io  0x0170-0x0177]
[    0.393060] pci 0000:00:01.1: legacy IDE quirk: reg 0x1c: [io  0x0376]
[    0.394160] pci 0000:00:01.3: [8086:7113] type 00 class 0x068000
[    0.396166] pci 0000:00:01.3: quirk: [io  0x0600-0x063f] claimed by PIIX4 ACPI
[    0.397065] pci 0000:00:01.3: quirk: [io  0x0700-0x070f] claimed by PIIX4 SMB
[    0.398172] pci 0000:00:02.0: [1234:1111] type 00 class 0x030000
[martins3:vfio_dma_map:671] fd000000 1000000
[    0.401060] pci 0000:00:02.0: reg 0x10: [mem 0xfd000000-0xfdffffff pref]
[martins3:vfio_dma_map:671] fd000000 1000000
[martins3:vfio_dma_map:671] fd000000 1000000
[    0.406061] pci 0000:00:02.0: reg 0x18: [mem 0xfea58000-0xfea58fff]
[martins3:vfio_dma_map:671] fd000000 1000000
[martins3:vfio_dma_map:671] fd000000 1000000
[martins3:vfio_dma_map:671] fd000000 1000000
[martins3:vfio_dma_map:671] fd000000 1000000
[    0.415061] pci 0000:00:02.0: reg 0x30: [mem 0xfea40000-0xfea4ffff pref]
[    0.416088] pci 0000:00:02.0: Video device with shadowed ROM at [mem 0x000c0000-0x000dffff]
[    0.418061] pci 0000:00:03.0: [1af4:1001] type 00 class 0x010000
[    0.421061] pci 0000:00:03.0: reg 0x10: [io  0xd100-0xd17f]
[    0.424061] pci 0000:00:03.0: reg 0x14: [mem 0xfea59000-0xfea59fff]
[    0.432060] pci 0000:00:03.0: reg 0x20: [mem 0xfe200000-0xfe203fff 64bit pref]
[    0.438079] pci 0000:00:04.0: [1b36:0001] type 01 class 0x060400
[    0.442060] pci 0000:00:04.0: reg 0x10: [mem 0xfea5a000-0xfea5a0ff 64bit]
[    0.448106] pci 0000:00:05.0: [1af4:1000] type 00 class 0x020000
[    0.451060] pci 0000:00:05.0: reg 0x10: [io  0xd2c0-0xd2df]
[    0.454059] pci 0000:00:05.0: reg 0x14: [mem 0xfea5b000-0xfea5bfff]
[    0.461061] pci 0000:00:05.0: reg 0x20: [mem 0xfe204000-0xfe207fff 64bit pref]
[    0.464061] pci 0000:00:05.0: reg 0x30: [mem 0xfea00000-0xfea3ffff pref]
[    0.468180] pci 0000:00:06.0: [10ec:8168] type 00 class 0x020000
[martins3:vfio_dma_map:671] fea51000 3000
[    0.475064] pci 0000:00:06.0: reg 0x10: [io  0xd000-0xd0ff]
[martins3:vfio_dma_map:671] fea51000 3000
[martins3:vfio_dma_map:671] fea51000 3000
[    0.481063] pci 0000:00:06.0: reg 0x18: [mem 0xfea5c000-0xfea5cfff 64bit]
[martins3:vfio_dma_map:671] fea51000 3000
[    0.485063] pci 0000:00:06.0: reg 0x20: [mem 0xfea50000-0xfea53fff 64bit]
[martins3:vfio_dma_map:671] fea51000 3000
[    0.490210] pci 0000:00:06.0: supports D1 D2
[    0.494065] pci 0000:00:07.0: [1af4:1009] type 00 class 0x000200
[    0.497061] pci 0000:00:07.0: reg 0x10: [io  0xd2e0-0xd2ff]
[    0.500061] pci 0000:00:07.0: reg 0x14: [mem 0xfea5d000-0xfea5dfff]
[    0.507061] pci 0000:00:07.0: reg 0x20: [mem 0xfe208000-0xfe20bfff 64bit pref]
[    0.513142] pci 0000:00:08.0: [1b36:0010] type 00 class 0x010802
[    0.515156] pci 0000:00:08.0: reg 0x10: [mem 0xfea54000-0xfea57fff 64bit]
[    0.523204] pci 0000:00:09.0: [1af4:1001] type 00 class 0x010000
[    0.526061] pci 0000:00:09.0: reg 0x10: [io  0xd180-0xd1ff]
[    0.529061] pci 0000:00:09.0: reg 0x14: [mem 0xfea5e000-0xfea5efff]
[    0.536061] pci 0000:00:09.0: reg 0x20: [mem 0xfe20c000-0xfe20ffff 64bit pref]
[    0.542117] pci 0000:00:0a.0: [1af4:1004] type 00 class 0x010000
[    0.545060] pci 0000:00:0a.0: reg 0x10: [io  0xd280-0xd2bf]
[    0.548060] pci 0000:00:0a.0: reg 0x14: [mem 0xfea5f000-0xfea5ffff]
[    0.555061] pci 0000:00:0a.0: reg 0x20: [mem 0xfe210000-0xfe213fff 64bit pref]
[    0.562081] pci 0000:00:0b.0: [1af4:1001] type 00 class 0x010000
[    0.565061] pci 0000:00:0b.0: reg 0x10: [io  0xd200-0xd27f]
[    0.568061] pci 0000:00:0b.0: reg 0x14: [mem 0xfea60000-0xfea60fff]
[    0.576061] pci 0000:00:0b.0: reg 0x20: [mem 0xfe214000-0xfe217fff 64bit pref]
[    0.597128] pci_bus 0000:01: extended config space not accessible
[    0.599065] pci 0000:01:01.0: [1b36:0010] type 00 class 0x010802
[    0.601156] pci 0000:01:01.0: reg 0x10: [mem 0xfe800000-0xfe803fff 64bit]
[    0.608108] pci 0000:01:01.0: 0.000 Gb/s available PCIe bandwidth, limited by Unknown x0 link at 0000:00:04.0 (capable of 2.000 Gb/s with 2.5 GT/s PCIe x1 link)
[    0.634107] pci 0000:00:04.0: PCI bridge to [bus 01]
[    0.635069] pci 0000:00:04.0:   bridge window [io  0xc000-0xcfff]
[    0.636068] pci 0000:00:04.0:   bridge window [mem 0xfe800000-0xfe9fffff]
[    0.637075] pci 0000:00:04.0:   bridge window [mem 0xfe000000-0xfe1fffff 64bit pref]
```
简而言之，就是使用的所有的物理映射，全部都要搞上。

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
