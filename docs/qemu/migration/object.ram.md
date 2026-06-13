# 内存拷贝相关
- migration/ram.c
- migration/postcopy-ram.c
- migration/block-dirty-bitmap.c

- 和 memory region 的 hook memory listener 的关系

- `migrate_send_rp_recv_bitmap`
    - `migrate_send_rp_message` : 希望获取到一个 BITMAP
    - `ramblock_recv_bitmap_send` :


## [ ] zero page
- 为什么会出现？
    - xbzrle 和 postcopy 如何处理的

## compress
- `do_data_compress`

- 使用一个额外的线程来进行的

## [ ] softmmu/cpu-throttle.c
降低 Guest 的执行速度，从而让 memory dirty 的速度下降。

## `userfault_fd` 我们可以复用这个机制吗?

`migration_clear_memory_region_dirty_bitmap_range`


```c
static RAMBlockNotifier ram_mig_ram_notifier = {
    .ram_block_resized = ram_mig_ram_block_resized,
};
```

## SaveVMHandlers
<!-- 6d1dd292-6e3a-4a6b-9b02-8f5af734795e -->

SaveVMHandlers 只有复杂项目才需要打开，
对于 vmstate 之类的东西， 是不需要的，例如跟踪一下 vmstate_save_state_v :

所谓的复杂项目，就是数据量很大，需要多次迭代。

一共三个用户使用:
hw/vfio/migration.c
migration/ram.c
migration/block-dirty-bitmap.c


commit 7429aebe1cff ("vfio/migration: Remove VFIO migration protocol v1")
中把 SaveVMHandlers 给删掉了。

```c
static SaveVMHandlers savevm_slirp_state = {
    .save_state = net_slirp_state_save,
    .load_state = net_slirp_state_load,
};
```

```c
static SaveVMHandlers savevm_dirty_bitmap_handlers = {
    .save_setup = dirty_bitmap_save_setup,
    .save_live_complete_postcopy = dirty_bitmap_save_complete,
    .save_live_complete_precopy = dirty_bitmap_save_complete,
    .has_postcopy = dirty_bitmap_has_postcopy,
    .state_pending_exact = dirty_bitmap_state_pending,
    .state_pending_estimate = dirty_bitmap_state_pending,
    .save_live_iterate = dirty_bitmap_save_iterate,
    .is_active_iterate = dirty_bitmap_is_active_iterate,
    .load_state = dirty_bitmap_load,
    .save_cleanup = dirty_bitmap_save_cleanup,
    .is_active = dirty_bitmap_is_active,
};
```

普通的内存热迁移:
```c
static SaveVMHandlers savevm_ram_handlers = {
    .save_setup = ram_save_setup,
    .save_live_iterate = ram_save_iterate,
    .save_complete = ram_save_complete,
    .has_postcopy = ram_has_postcopy,
    .state_pending_exact = ram_state_pending_exact,
    .state_pending_estimate = ram_state_pending_estimate,
    .load_state = ram_load,
    .save_cleanup = ram_save_cleanup,
    .load_setup = ram_load_setup,
    .load_cleanup = ram_load_cleanup,
    .resume_prepare = ram_resume_prepare,
    .save_postcopy_prepare = ram_save_postcopy_prepare,
};
```

vfio 热迁移:
```c
static const SaveVMHandlers savevm_vfio_handlers = {
    .save_prepare = vfio_save_prepare,
    .save_setup = vfio_save_setup,
    .save_cleanup = vfio_save_cleanup,
    .state_pending_estimate = vfio_state_pending_estimate,
    .state_pending_exact = vfio_state_pending_exact,
    .is_active_iterate = vfio_is_active_iterate,
    .save_live_iterate = vfio_save_iterate,
    .save_complete = vfio_save_complete_precopy,
    .save_state = vfio_save_state,
    .load_setup = vfio_load_setup,
    .load_cleanup = vfio_load_cleanup,
    .load_state = vfio_load_state,
    .switchover_ack_needed = vfio_switchover_ack_needed,
    /*
     * Multifd support
     */
    .load_state_buffer = vfio_multifd_load_state_buffer,
    .switchover_start = vfio_switchover_start,
    .save_complete_precopy_thread = vfio_multifd_save_complete_precopy_thread,
};
```

- coroutine_trampoline
  - process_incoming_migration_co
    - qemu_loadvm_state
      - qemu_loadvm_state_main
        - qemu_loadvm_section_start_full
          - net_slirp_state_load

似乎通过 section 知道的:
```c
typedef struct SaveStateEntry {
    QTAILQ_ENTRY(SaveStateEntry) entry;
    char idstr[256];
    uint32_t instance_id;
    int alias_id;
    int version_id;
    /* version id read from the stream */
    int load_version_id;
    int section_id;
    /* section id read from the stream */
    int load_section_id;
    const SaveVMHandlers *ops;
    const VMStateDescription *vmsd;
    void *opaque;
    CompatEntry *compat;
    int is_ram;
} SaveStateEntry;
```

```c
static const VMStateDescription vmstate_xhci_intr = {
    .name = "xhci-intr",
    .version_id = 1,
    .fields = (const VMStateField[]) {
        /* registers */
        VMSTATE_UINT32(iman,          XHCIInterrupter),
        VMSTATE_UINT32(imod,          XHCIInterrupter),
        VMSTATE_UINT32(erstsz,        XHCIInterrupter),
        VMSTATE_UINT32(erstba_low,    XHCIInterrupter),
        VMSTATE_UINT32(erstba_high,   XHCIInterrupter),
        VMSTATE_UINT32(erdp_low,      XHCIInterrupter),
        VMSTATE_UINT32(erdp_high,     XHCIInterrupter),

```
```txt
vmstate_save_state_loop xhci-intr/iman[1]
vmstate_save_state_loop xhci-intr/imod[1]
vmstate_save_state_loop xhci-intr/erstsz[1]
vmstate_save_state_loop xhci-intr/erstba_low[1]
vmstate_save_state_loop xhci-intr/erstba_high[1]
vmstate_save_state_loop xhci-intr/erdp_low[1]
vmstate_save_state_loop xhci-intr/erdp_high[1]
vmstate_save_state_loop xhci-intr/msix_used[1]
vmstate_save_state_loop xhci-intr/er_pcs[1]
vmstate_save_state_loop xhci-intr/er_start[1]
vmstate_save_state_loop xhci-intr/er_size[1]
vmstate_save_state_loop xhci-intr/er_ep_idx[1]
```

真的是难以相信啊:
```txt
[martins3:configuration_pre_save:310] virt-10.1
```

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

## [ ] RAMState 是如何创建的
<!-- f59bf708-4be9-4985-acc9-dc068ed17494 -->

(2026-04-12 感觉这里的代码可以优化一下，不过记住，RamState 是一个全局概念
只有一个)

目前看几乎所有的位置实际上都是在引用 ram.c 中定义的这个变量:
```c
static RAMState *ram_state;
```

```c
void ram_mig_init(void)
{
    qemu_mutex_init(&XBZRLE.lock);
    register_savevm_live("ram", 0, 4, &savevm_ram_handlers, &ram_state);
    ram_block_notifier_add(&ram_mig_ram_notifier);
}
```
这个代码极其逆天，ram.c 中一会直接使用参数，一会直接引用 ram_state 。

看上去，可以移除更多才可以:
```diff
History:        #0
Commit:         6a39ba7cab67da05b91e215142ce5781e77e5d9f
Committer:      Peter Xu <peterx@redhat.com>
Author Date:    Thu 17 Oct 2024 02:42:53 PM CST
Committer Date: Fri 01 Nov 2024 03:48:18 AM CST

migration: Remove "rs" parameter in migration_bitmap_sync_precopy

The global static variable ram_state in fact is referred to by the
"rs" parameter in migration_bitmap_sync_precopy. For ease of calling
by the callees, use the global variable directly in
migration_bitmap_sync_precopy and remove "rs" parameter.

The migration_bitmap_sync_precopy will be exported in the next commit.
```


## RAMState 中的 bitmap_mutex
<!-- 5e8bf3c3-e187-41ea-85b2-2d5b15c8b83e -->

1. struct RAMState 定义在 migration/ram.c 中，管理热迁移的状态
2. 持有两个 PageSearchStatus 分别管理
```c
/* State of RAM for migration */
struct RAMState {
    /*
     * PageSearchStatus structures for the channels when send pages.
     * Protected by the bitmap_mutex.
     */
    PageSearchStatus pss[RAM_CHANNEL_MAX];

    // ...

    /* number of dirty bits in the bitmap */
    uint64_t migration_dirty_pages;
    /*
     * Protects:
     * - dirty/clear bitmap
     * - migration_dirty_pages
     * - pss structures
     */
    QemuMutex bitmap_mutex;

    // ...
```

bitmap_mutex 一共使用的地方:
- migration_bitmap_sync : 从 kvm 哪里同步，需要持有锁
- ram_save_queue_pages : 又是 postcopy ，好烦
- ram_save_host_page : 只有 postcopy 模式才需要
- qemu_guest_free_page_hint
- ram_save_iterate
- ram_save_complete

- thread_start
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_iterate
            - ram_save_iterate

这个分析结果和我想象的完全不一样，这相当于 dirty bitmap 的获取和使用也是 mutex 互斥的
这里锁让我豁然开朗啊，原来是给
```c
static int ram_save_iterate(QEMUFile *f, void *opaque)
{
    // ...
    /*
     * We'll take this lock a little bit long, but it's okay for two reasons.
     * Firstly, the only possible other thread to take it is who calls
     * qemu_guest_free_page_hint(), which should be rare; secondly, see
     * MAX_WAIT (if curious, further see commit 4508bd9ed8053ce) below, which
     * guarantees that we'll at least released it in a regular basis.
     */
```
qemu_guest_free_page_hint() 是 balloon 来优化掉那些不需要热迁移的 page 的。


第二个问题，为什么需要 pss 来跟踪遍历到哪里了，以及为什么需要 bitmap_mutex 来保护?

### migration_bitmap_sync 中的 rcu 是做什么的?
```c
    WITH_QEMU_LOCK_GUARD(&rs->bitmap_mutex) {
        WITH_RCU_READ_LOCK_GUARD() {
            RAMBLOCK_FOREACH_NOT_IGNORED(block) {
                ramblock_sync_dirty_bitmap(rs, block);
            }
            stat64_set(&mig_stats.dirty_bytes_last_sync, ram_bytes_remaining());
        }
    }
```

## pss 细节问题
如何理解 find_dirty_block 中的这一段代码?

哇，绕了一个好大的圈子
```c
    if (pss->complete_round && pss->block == rs->last_seen_block &&
        pss->page >= rs->last_page) {
        /*
         * We've been once around the RAM and haven't found anything.
         * Give up.
         */
        return PAGE_ALL_CLEAN;
    }
```

## pss

```txt
History:        #0
Commit:         d9e474ea564bc109bc6fc81323ae90a7c9e7f04f
Author:         Peter Xu <peterx@redhat.com>
Committer:      Juan Quintela <quintela@trasno.org>
Author Date:    Wed 12 Oct 2022 05:55:52 AM CST
Committer Date: Thu 15 Dec 2022 05:30:37 PM CST

migration: Teach PSS about host page

Migration code has a lot to do with host pages.  Teaching PSS core about
the idea of host page helps a lot and makes the code clean.  Meanwhile,
this prepares for the future changes that can leverage the new PSS helpers
that this patch introduces to send host page in another thread.

Three more fields are introduced for this:

  (1) host_page_sending: this is set to true when QEMU is sending a host
      page, false otherwise.

  (2) host_page_{start|end}: these point to the start/end of host page
      we're sending, and it's only valid when host_page_sending==true.

For example, when we look up the next dirty page on the ramblock, with
host_page_sending==true, we'll not try to look for anything beyond the
current host page boundary.  This can be slightly efficient than current
code because currently we'll set pss->page to next dirty bit (which can be
over current host page boundary) and reset it to host page boundary if we
found it goes beyond that.

With above, we can easily make migration_bitmap_find_dirty() self contained
by updating pss->page properly.  rs* parameter is removed because it's not
even used in old code.

When sending a host page, we should use the pss helpers like this:

  - pss_host_page_prepare(pss): called before sending host page
  - pss_within_range(pss): whether we're still working on the cur host page?
  - pss_host_page_finish(pss): called after sending a host page

Then we can use ram_save_target_page() to save one small page.

Currently ram_save_host_page() is still the only user. If there'll be
another function to send host page (e.g. in return path thread) in the
future, it should follow the same style.

Reviewed-by: Dr. David Alan Gilbert <dgilbert@redhat.com>
Signed-off-by: Peter Xu <peterx@redhat.com>
Reviewed-by: Juan Quintela <quintela@redhat.com>
Signed-off-by: Juan Quintela <quintela@redhat.com>
```

这么想，就是这个东西引入了复杂度的
```c
/* used by the search for pages to send */
struct PageSearchStatus {
    /* The migration channel used for a specific host page */
    QEMUFile    *pss_channel;
    /* Last block from where we have sent data */
    RAMBlock *last_sent_block;
    /* Current block being searched */
    RAMBlock    *block;
    /* Current page to search from */
    unsigned long page;
    /* Set once we wrap around */
    bool         complete_round;

    /* Whether we're sending a host page */
    bool          host_page_sending;
    /* The start/end of current host page.  Invalid if host_page_sending==false */
    unsigned long host_page_start;
    unsigned long host_page_end;
};
```


- thread_start
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_iterate
            - ram_save_iterate
              - ram_find_and_save_block
                - pss_init
                - find_dirty_block (首先找一次 page)
                  - pss_find_next_dirty
                - ram_save_host_page (这里有一个大循环 ，不断的 find and search ，但是 boundary 只是一个 hostpage)
                  - pss_find_next_dirty

(我感觉这里有可以优化的可能，第一次 find_dirty_block 就是查找了，但是没有 clear bit ，
然后第二次又去查找，发送了 page 之后才去 clear bit )


似乎需要先 clear ，然后:
```txt
test_and_clear_bit(page, rb->bmap)
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
