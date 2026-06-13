## L1 中观测到 kvm_set_pfn_dirty

在 L1 中启动 L2 ，可以观察到这样的路径:
```txt
@[
    kvm_set_pfn_dirty+5
    handle_changed_spte+615
    tdp_mmu_zap_leafs+640
    kvm_tdp_mmu_unmap_gfn_range+120
    kvm_unmap_gfn_range+421
    kvm_mmu_notifier_invalidate_range_start+581
    __mmu_notifier_invalidate_range_start+280
    change_protection+1550
    change_prot_numa+230
    task_numa_work+1554
    task_work_run+135
    xfer_to_guest_mode_handle_work+149
    kvm_arch_vcpu_ioctl_run+5545
    kvm_vcpu_ioctl+1537
    __se_sys_ioctl+115
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 80493
```

```txt
@[
    kvm_set_pfn_dirty+5
    mmu_spte_clear_track_bits+301
    drop_spte+18
    ept_prefetch_invalid_gpte.isra.0+75
    ept_sync_spte+225
    __kvm_sync_page+114
    mmu_sync_children+458
    kvm_mmu_sync_roots+248
    kvm_mmu_load+849
    kvm_arch_vcpu_ioctl_run+4005
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 12566
@[
    kvm_set_pfn_dirty+5
    mmu_spte_clear_track_bits+301
    drop_spte+18
    mmu_page_zap_pte+182
    __kvm_mmu_prepare_zap_page+111
    mmu_zap_unsync_children+287
    __kvm_mmu_prepare_zap_page+80
    kvm_mmu_track_write+517
    write_emulate+77
    emulator_read_write_onepage+233
    emulator_read_write+191
    x86_emulate_insn+1426
    x86_emulate_instruction+1056
    kvm_mmu_page_fault+322
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+1765
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 163662
```

似乎有不同，但是暂时理解不到位。

## dirty ring
https://kvm-forum.qemu.org/2020/kvm_dirty_ring_peter.pdf

代码在 virt/kvm/dirty_ring.c 中的

之前的都是 dirty bitmap 相关的:

ioctl(KVM_GET_DIRTY_LOG)
KVM_CAP_MANUAL_DIRTY_LOG_PROTECT2
KVM_CLEAR_DIRTY_LOG

KVM_DIRTY_LOG_MANUAL_PROTECT_ENABLE
KVM_DIRTY_LOG_INITIALLY_SET
KVM_MEM_LOG_DIRTY_PAGES

mmu_lock

Dirty Ring vs Dirty Logging

如果满了的话，vCPU 会暂停，exit reason 为:
KVM_EXIT_DIRTY_RING_FULL

```txt
#define KVM_DIRTY_LOG_MANUAL_CAPS   (KVM_DIRTY_LOG_MANUAL_PROTECT_ENABLE | \
					KVM_DIRTY_LOG_INITIALLY_SET)
```

## 热迁移和 userfaultfd 耦合会有问题吗?

1. 热迁移的时候，我需要知道哪些 page 不用迁移的
  - 但是，有的 page 本来没有映射，这个是如何跳过的
2. swap in / swap out / page fault / balloon out 不会影响 dirty bitmap 的影响
  - 用 nutanix 方案不希望 swap out 在 drity ，而如果是 swap 的，那么希望


## firecracker 使用 dirty log 来实现 snapshot 的

snapshot_memory_to_file

## 提纲挈领
```c
/**
 * kvm_get_dirty_log_protect - get a snapshot of dirty pages
 *	and reenable dirty page tracking for the corresponding pages.
 * @kvm:	pointer to kvm instance
 * @log:	slot id and address to which we copy the log
 *
 * We need to keep it in mind that VCPU threads can write to the bitmap
 * concurrently. So, to avoid losing track of dirty pages we keep the
 * following order:
 *
 *    1. Take a snapshot of the bit and clear it if needed.
 *    2. Write protect the corresponding page.
 *    3. Copy the snapshot to the userspace.
 *    4. Upon return caller flushes TLB's if needed.
 *
 * Between 2 and 4, the guest may write to the page using the remaining TLB
 * entry.  This is not a problem because the page is reported dirty using
 * the snapshot taken before and step 4 ensures that writes done after
 * exiting to userspace will be logged for the next call.
 *
 */
static int kvm_get_dirty_log_protect(struct kvm *kvm, struct kvm_dirty_log *log)
```
1. kvm_arch_sync_dirty_log : exit 的时候自动 sync ，所以只要退出就可以同步的。
2. 获取 dirty map 分为两种

两个都是要拷贝啊?
KVM_MMU_LOCK(kvm); 里面上的这个锁意味着什么?

而且为什么里面还需要继续使用 xchg
```txt
mask = xchg(&dirty_bitmap[i], 0);
```

为什么还需要 mark pt 的
kvm_arch_mmu_enable_log_dirty_pt_masked


应该是通过 KVM_RESET_DIRTY_RINGS -> kvm_vm_ioctl_reset_dirty_pages 来实现的吧。


qemu 中的 slots_lock 可以阻塞吗? 从 kvm_dirty_ring_reaper_thread 来看，
这些都是一步进行的。
```c
/* Must be with slots_lock held */
static uint64_t kvm_dirty_ring_reap_locked(KVMState *s, CPUState* cpu)
```

## vmx pml

可以通过 pml
PML_LOG_NR_ENTRIES 最多 512 项

通过 vmx_flush_pml_buffer 更新到

## 和 A/D bit 的关系

如果 ept ad ，那么无法对于 guest 内存进行 idle page tracking  了。

```c
bool __read_mostly enable_ept_ad_bits = 1;
module_param_named(eptad, enable_ept_ad_bits, bool, 0444);
```
vmx_hardware_setup 中:
```c
	/*
	 * Only enable PML when hardware supports PML feature, and both EPT
	 * and EPT A/D bit features are enabled -- PML depends on them to work.
	 */
	if (!enable_ept || !enable_ept_ad_bits || !cpu_has_vmx_pml())
		enable_pml = 0;
```

但是在 13900k 中，没有 cpu_has_vmx_pml() ，所以没有打开。

## 嵌套虚拟化模拟 pml
原来就是使用最简单，朴实无华的解决办法
```txt
@[
    nested_vmx_write_pml_buffer+5
    ept_walk_addr_generic+2134
    ept_page_fault+107
    kvm_mmu_do_page_fault+276
    kvm_mmu_page_fault+130
    vmx_handle_exit+538
    vcpu_enter_guest.constprop.0+1613
    kvm_arch_vcpu_ioctl_run+855
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 17747
```

应该是有地方开启了 write protect 的，一般运行的时候，完全无法捕获到
nested_vmx_write_pml_buffer


## 内核中为什么频繁的调用 mark_page_dirty_in_slot
而且还不一样

物理机中调用的是这个:
```txt
@[
    mark_page_dirty_in_slot+5
    vcpu_enter_guest.constprop.0+2827
    kvm_arch_vcpu_ioctl_run+855
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 14299
```
是不是，如果支持 pml 就不会有这么多 mark_page_dirty_in_slot

嵌套中关闭 pml 之后，调用最后的还是
```txt
@[
    mark_page_dirty_in_slot+5
    kvm_arch_vcpu_ioctl_run+6415
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+736
    do_syscall_64+95
    entry_SYSCALL_64_after_hwframe+118
]: 96
```
似乎是优化方法不一样而已

既然如此，gdb ./arch/x86/kvm/kvm.ko 就可以很容易的找到:

```txt
$ l *(kvm_arch_vcpu_ioctl_run+6415)
0x3639f is in kvm_arch_vcpu_ioctl_run (arch/x86/kvm/x86.c:3728).
3723            unsafe_put_user(version, &st->version, out);
3724
3725     out:
3726            user_access_end();
3727     dirty:
3728            mark_page_dirty_in_slot(vcpu->kvm, ghc->memslot, gpa_to_gfn(ghc->gpa));
3729    }
3730
3731    int kvm_set_msr_common(struct kvm_vcpu *vcpu, struct msr_data *msr_info)
3732    {
```

## 抓抓 trace 信息看看

### 如果没有 pml ，那么如何实现热迁移的?
```txt
@[
    kvm_get_dirty_log_protect+1
    kvm_vm_ioctl+2068
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 36
```

原来是通过这个东西的，

kvm_setup_dirty_ring

这个东西居然真的就是内核中的吗?

的确，启动和后续之后的结果是不一样的:

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
                        - kvm_log_sync
                          - kvm_physical_sync_dirty_bitmap
                            - kvm_slot_get_dirty_log

- __clone3
  - start_thread
    - qemu_thread_start
      - migration_thread
        - migration_iteration_run
          - qemu_savevm_state_pending_exact
            - ram_state_pending_exact
              - migration_bitmap_sync_precopy
                - migration_bitmap_sync
                  - memory_global_dirty_log_sync
                    - memory_region_sync_dirty_bitmap (从这里分开，到底是用 ring 还是 bitmap ，所有的内容都是放到 KVMSlot::dirty_bmap )
                      - kvm_log_sync
                        - kvm_physical_sync_dirty_bitmap
                          - kvm_slot_get_dirty_log

为什么开启了 dirty ring 之后，这个 kvm_slot_get_dirty_log 就完全不会被调用了。

### 是不是，有无 pml ，都可以使用 ring buffer 的内容？

## dirty ring 的使用

先看文档: https://lwn.net/Articles/825743/

1. 不会 dirty ring 的使用会导致现在的方案做不下去，由于必需使用 bitmap 的?

### 理解这个意思
监控一下 dirty page 的使用:
```txt
            /*
             * This is not needed by KVM_GET_DIRTY_LOG because the
             * ioctl will unconditionally overwrite the whole region.
             * However kvm dirty ring has no such side effect.
             */
            kvm_slot_reset_dirty_pages(mem);
```

kvm_dirty_ring_reaper_thread

sudo perf trace -e kvm:kvm_dirty_ring_push

### 没有 pml 的时候，dirty ring 的构建过程
```txt
@[
    kvm_dirty_ring_push+5
      - fast_pf_fix_direct_spte (被 inline 的函数)
    fast_page_fault+942
    kvm_tdp_page_fault+70
    kvm_mmu_do_page_fault+492
    kvm_mmu_page_fault+130
    vmx_handle_exit+538
    vcpu_enter_guest.constprop.0+1613
    kvm_arch_vcpu_ioctl_run+855
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+160
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 160711
```

(想不到 kvm mmu 模块这么复杂了，如果不是找到这个 inline 函数，否则谁能理解这个东西啊)

在嵌套中，有 pml ，其测试的结果为:
```txt
@[
    kvm_dirty_ring_push+5
    vmx_handle_exit+451
    kvm_arch_vcpu_ioctl_run+2095
    kvm_vcpu_ioctl+290
    __x64_sys_ioctl+736
    do_syscall_64+95
    entry_SYSCALL_64_after_hwframe+118
]: 7173
```
所以，现在我们知道了，既然 13900k 上不支持 pml ，为什么还是可以热迁移。
总是可以获取到 dirty bitmap 的。

### [ ] dirty ring 只是一个表现形式，最后还是需要把内容写入到 bitmap 中的吗?

但是 kvm_slot_get_dirty_log 只有在 bitmap 的时候才会调用

### [ ] 似乎 dirty ring 不是在一个 thread 中? 会导致本质的问题么?

1. 没有开启热迁移的时候，这个 thread 就会启动了，对于性能影响也不大，但是认为是可以优化的。
2. 为什么需要是在一个单独的 thread 中?
  - 因为 ring size 就是那么大，所以需要周期性的来 reap ，不然缓冲区就用完了。
    - 缓冲区用完了，怎么办？自动停止 vCPU thread 吗?


3. 不在一个 thread 中会导致额外的同步问题吗?

- __clone3
  - start_thread
    - qemu_thread_start
      - kvm_dirty_ring_reaper_thread
        - kvm_dirty_ring_reap
          - kvm_dirty_ring_reap_locked
            - kvm_dirty_ring_reap_one // 直接访问 dirty ring ，把内容存放到 KVMSlot::dirty_bmap 中
            - kvm_vm_ioctl(s, KVM_RESET_DIRTY_RINGS); // 重置一下

一些锁的观察:
1. 为什么 kvm_dirty_ring_reap 中需要使用 kvm_slots_lock ?
如果没有，结果会如何? (感觉是用来，让热迁移 thread 无法)
2. kvm_dirty_ring_reaper_thread 为什么使用 bql ?

dirty ring 也是按照每一个 vCPU 的，那么其实，可不可以，userfaultfd 也是多线程的?

dirty ring 如何把 dirty bitmap 收集起来的:

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
                        - kvm_log_sync_global
                          - kvm_dirty_ring_flush  (把所有的 dirty 都)
                            - kvm_dirty_ring_reap
                              - kvm_dirty_ring_reap_locked
                                - kvm_dirty_ring_reap_one

两个模式不同的地方在这里的:
kvm_memory_listener_register
```c
    if (s->kvm_dirty_ring_size) {
        kml->listener.log_sync_global = kvm_log_sync_global;
    } else {
        kml->listener.log_sync = kvm_log_sync;
        kml->listener.log_clear = kvm_log_clear;
    }
```


### 为什么需要提供两个 volatile 的变量出来，

```c
/*
 * KVM reaper instance, responsible for collecting the KVM dirty bits
 * via the dirty ring.
 */
struct KVMDirtyRingReaper {
    /* The reaper thread */
    QemuThread reaper_thr;
    volatile uint64_t reaper_iteration; /* iteration number of reaper thr */
    volatile enum KVMDirtyRingReaperState reaper_state; /* reap thr state */
};
```

## 消费者



## ram_save_target_page_legacy 通过才意识到，qemu 无法检测没有映射的 page

这会导致一个问题，就是热迁移的时候，那些被 balloon 去掉的 page ，其实会被都上来的一次。
这个 nutanix 第一个 kvm forum 解决来吗?

## 就现在而言，消费者和生产者都是在一个 thread 中的

migration_thread ，交替进行传输 dirty 和收集 dirty 的行为。

正确的解决问题的方法: 如何 swap in / swap out 的东西合并到 dirty bitmap 中去

写一个 merge bitmap 的操作， 这里还是那个关键问题，merge 的时候，如果这个时候，page fault 还是在发生，
这个时候才怎么办? 这里参考 dirty ring 的设计就可以了。什么上锁之类的。


## dirty page tracking

dirty_memory 划分为三种
```c
#define DIRTY_MEMORY_VGA       0
#define DIRTY_MEMORY_CODE      1
#define DIRTY_MEMORY_MIGRATION 2
#define DIRTY_MEMORY_NUM       3        /* num of dirty bits */
```
因为三种需求:
- vga : 绘制屏幕的时候，记录下仅仅被修改过的部分，从而加快渲染速度
- smc (self modified code) : TCG 中检测到 guest 的代码修改过，那么就需要 invalidate 这个 page 上管理的 TB
- migration : 当前虚拟机的内存被修改没有同步到远程的虚拟机中

关联的主要函数以及他们的调用者:
- tlb_set_dirty
- tlb_reset_dirty
- notdirty_write
   * atomic_mmu_lookup : 这是整个 atomic 机制调用的地方
   * probe_access / probe_access_flags : x86 guest 从来没有调用过，这个不是我能理解的
   * store_helper

- 在 cpu_physical_memory_set_dirty_lebitmap 中间, 如果没有打开 global_dirty_log 那么 client 就不会添加上 DIRTY_MEMORY_MIGRATION
- 在 memory_region_get_dirty_log_mask 中对于 DIRTY_MEMORY_CODE 和 DIRTY_MEMORY_MIGRATION 也是存在类似的特殊处理

- colo_incoming_start_dirty_log : https://wiki.qemu.org/Features/COLO
  - ramblock_sync_dirty_bitmap
    - cpu_physical_memory_sync_dirty_bitmap

- 在正常的 kvm 其中的操作过程中，cpu_physical_memory_test_and_clear_dirty 和  cpu_physical_memory_snapshot_and_clear_dirty 都不会被调用
- 在 tcg 模式下, 如果只是需要支持 SMC, 那么需要的函数不多，例如 cpu_physical_memory_test_and_clear_dirty

- vga_mem_write : 使用 memory_region_set_dirty 来调用设置 memory dirty 的操作
- 当 vga_draw_graphic 的时候，其可以 memory_region_snapshot_and_clear_dirty

#### migration

QEMU 记录的 dirty page 已经发送到只剩下最后的 max_size 的时候，调用 migration_bitmap_sync 进行 dirty page 同步，
该函数最终会调用到 ioctl(KVM_GET_DIRTY_LOG) 上，将 dirty page 记录到 ram_list.dirty_memory 中。
- migration_bitmap_sync
  - memory_global_dirty_log_sync : 调用 MemoryListener::log_sync，比如 kvm, 这是为将 dirty memory 从 kernel 中同步过来，**tcg** 因为只是操作 ramlist.dirty_memory 的，所以这一步函数对于其为空操作
    - memory_region_sync_dirty_bitmap
      - MemoryListener::log_sync
         - kvm_log_sync
            - kvm_physical_sync_dirty_bitmap
              - kvm_slot_get_dirty_log  : 使用 KVM_GET_DIRTY_LOG ioctl
              - kvm_slot_sync_dirty_pages
                - cpu_physical_memory_set_dirty_lebitmap : 这个将从 kvm 中得到的 dirty map 的信息放到 ram_list.dirty_memory
      - MemoryListener::log_sync_global
  - ramblock_sync_dirty_bitmap : 将 dirty memory 同步到 RAMState 中，准备发送出去
    - cpu_physical_memory_sync_dirty_bitmap

```c
    if (s->kvm_dirty_ring_size) {
        kml->listener.log_sync_global = kvm_log_sync_global;
    } else {
        kml->listener.log_sync = kvm_log_sync;
        kml->listener.log_clear = kvm_log_clear;
    }
```
[log_sync_global](https://www.youtube.com/watch?v=YsQJ-Vll3sg) 接口都是 Peter Xu 在 2020 添加的

## dirty log
```c
static bool is_dirty_spte(u64 spte)
{
  // 存在一些 ad bit 的奇怪东西
  // 但是没有 ad bit 的支持，似乎 dirty log 是没有办法进行的
	u64 dirty_mask = spte_shadow_dirty_mask(spte);
	return dirty_mask ? spte & dirty_mask : spte & PT_WRITABLE_MASK;
}
```
- [ ] 找到传输 dirty map 的内容
- [ ] 这是函数是处理 shadow page table 的吧

```c
/**
 * kvm_vm_ioctl_get_dirty_log - get and clear the log of dirty pages in a slot
 * @kvm: kvm instance
 * @log: slot id and address to which we copy the log
 *
 * Steps 1-4 below provide general overview of dirty page logging. See
 * kvm_get_dirty_log_protect() function description for additional details.
 *
 * We call kvm_get_dirty_log_protect() to handle steps 1-3, upon return we
 * always flush the TLB (step 4) even if previous step failed  and the dirty
 * bitmap may be corrupt. Regardless of previous outcome the KVM logging API
 * does not preclude user space subsequent dirty log read. Flushing TLB ensures
 * writes will be marked dirty for next log read.
 *
 *   1. Take a snapshot of the bit and clear it if needed.
 *   2. Write protect the corresponding page.
 *   3. Copy the snapshot to the userspace.
 *   4. Flush TLB's if needed.
 */
static int kvm_vm_ioctl_get_dirty_log(struct kvm *kvm,
```

```c
/**
 * kvm_get_dirty_log_protect - get a snapshot of dirty pages
 *	and reenable dirty page tracking for the corresponding pages.
 * @kvm:	pointer to kvm instance
 * @log:	slot id and address to which we copy the log
 *
 * We need to keep it in mind that VCPU threads can write to the bitmap
 * concurrently. So, to avoid losing track of dirty pages we keep the
 * following order:
 *
 *    1. Take a snapshot of the bit and clear it if needed.
 *    2. Write protect the corresponding page.
 *    3. Copy the snapshot to the userspace.
 *    4. Upon return caller flushes TLB's if needed.
 *
 * Between 2 and 4, the guest may write to the page using the remaining TLB
 * entry.  This is not a problem because the page is reported dirty using
 * the snapshot taken before and step 4 ensures that writes done after
 * exiting to userspace will be logged for the next call.
 *
 */
static int kvm_get_dirty_log_protect(struct kvm *kvm, struct kvm_dirty_log *log)

	dirty_bitmap = memslot->dirty_bitmap;

	kvm_arch_sync_dirty_log(kvm, memslot);

	n = kvm_dirty_bitmap_bytes(memslot);
	flush = false;
	if (kvm->manual_dirty_log_protect) { // TODO 这一行的 git message
		/*
		 * Unlike kvm_get_dirty_log, we always return false in *flush,
		 * because no flush is needed until KVM_CLEAR_DIRTY_LOG.  There
		 * is some code duplication between this function and
		 * kvm_get_dirty_log, but hopefully all architecture
		 * transition to kvm_get_dirty_log_protect and kvm_get_dirty_log
		 * can be eliminated.
		 */
		dirty_bitmap_buffer = dirty_bitmap;
```

```c
void kvm_arch_sync_dirty_log(struct kvm *kvm, struct kvm_memory_slot *memslot)
{
	/*
	 * Flush potentially hardware-cached dirty pages to dirty_bitmap.
	 */
	if (kvm_x86_ops.flush_log_dirty)
		kvm_x86_ops.flush_log_dirty(kvm);
}
```

==> 实际上注册的函数
```c
static void vmx_flush_log_dirty(struct kvm *kvm)
{
	kvm_flush_pml_buffers(kvm);
}
```

- [ ] PML buffer 是什么 ?
  - [ ] PMP buffer 为什么在 VMEXITs 的时候被刷新出去了
```c
/*
 * Flush all vcpus' PML buffer and update logged GPAs to dirty_bitmap.
 * Called before reporting dirty_bitmap to userspace.
 */
static void kvm_flush_pml_buffers(struct kvm *kvm)
{
	int i;
	struct kvm_vcpu *vcpu;
	/*
	 * We only need to kick vcpu out of guest mode here, as PML buffer
	 * is flushed at beginning of all VMEXITs, and it's obvious that only
	 * vcpus running in guest are possible to have unflushed GPAs in PML
	 * buffer.
	 */
	kvm_for_each_vcpu(i, vcpu, kvm)
		kvm_vcpu_kick(vcpu);
}
```


#### dirty flags
- [ ]  https://events.static.linuxfound.org/sites/events/files/slides/Guangrong-fast-write-protection.pdf

- [x] 应该存在软件 和 硬件方式实现 dirty log

kvm_mmu_slot_set_dirty : 将 slot 对应 spte 全部设置为 dirty

这时候的想法是没有问题的 : 通过将所有的 page 全部 write protection, 从而可以知道那些是 dirty 的

- [ ] mmu_spte_update
    - caller : 只是改变 flags 而不修改指向的内容
      - [ ] spte_set_dirty && spte_clear_dirty
      - [ ] spte_write_protect

```c
/*
 * Write-protect on the specified @sptep, @pt_protect indicates whether
 * spte write-protection is caused by protecting shadow page table.
 *
 * Note: write protection is difference between dirty logging and spte
 * protection:
 * - for dirty logging, the spte can be set to writable at anytime if
 *   its dirty bitmap is properly set.
 * - for spte protection, the spte can be writable only after unsync-ing
 *   shadow page.
 *
 * Return true if tlb need be flushed.
 */
static bool spte_write_protect(u64 *sptep, bool pt_protect)
{
	u64 spte = *sptep;

	if (!is_writable_pte(spte) &&
	      !(pt_protect && spte_can_locklessly_be_made_writable(spte)))
		return false;

	rmap_printk("rmap_write_protect: spte %p %llx\n", sptep, *sptep);

	if (pt_protect)
		spte &= ~SPTE_MMU_WRITEABLE;
	spte = spte & ~PT_WRITABLE_MASK;

	return mmu_spte_update(sptep, spte);
}
```
- [x] dirty logging 和 spte protection 如何实现区分
  - [ ] pt_protect 参数就是用于区分到底谁才是 pt_protect

```c
/*
 * Currently, we have two sorts of write-protection, a) the first one
 * write-protects guest page to sync the guest modification, b) another one is
 * used to sync dirty bitmap when we do KVM_GET_DIRTY_LOG. The differences
 * between these two sorts are:
 * 1) the first case clears SPTE_MMU_WRITEABLE bit.
 * 2) the first case requires flushing tlb immediately avoiding corrupting
 *    shadow page table between all vcpus so it should be in the protection of
 *    mmu-lock. And the another case does not need to flush tlb until returning
 *    the dirty bitmap to userspace since it only write-protects the page
 *    logged in the bitmap, that means the page in the dirty bitmap is not
 *    missed, so it can flush tlb out of mmu-lock.
 *
 * So, there is the problem: the first case can meet the corrupted tlb caused
 * by another case which write-protects pages but without flush tlb
 * immediately. In order to making the first case be aware this problem we let
 * it flush tlb if we try to write-protect a spte whose SPTE_MMU_WRITEABLE bit
 * is set, it works since another case never touches SPTE_MMU_WRITEABLE bit.
 *
 * Anyway, whenever a spte is updated (only permission and status bits are
 * changed) we need to check whether the spte with SPTE_MMU_WRITEABLE becomes
 * readonly, if that happens, we need to flush tlb. Fortunately,
 * mmu_spte_update() has already handled it perfectly.
 *
 * The rules to use SPTE_MMU_WRITEABLE and PT_WRITABLE_MASK:
 * - if we want to see if it has writable tlb entry or if the spte can be
 *   writable on the mmu mapping, check SPTE_MMU_WRITEABLE, this is the most
 *   case, otherwise
 * - if we fix page fault on the spte or do write-protection by dirty logging,
 *   check PT_WRITABLE_MASK.
 *
 * TODO: introduce APIs to split these two cases.
 */
static inline int is_writable_pte(unsigned long pte)
{
	return pte & PT_WRITABLE_MASK;
}
```

- [ ] kvm_memory_slot::dirty_bitmap
    - [ ] gfn_to_memslot_dirty_bitmap
    - [ ] mark_page_dirty_in_slot
- [ ] intel 手册 关于 pml 怎么说的

## ad bit
https://stackoverflow.com/questions/32093036/setting-of-intel-ept-accessed-and-dirty-flags-for-guest-page-tables

https://stackoverflow.com/questions/55589131/what-is-the-relation-between-ept-pte-and-host-pte-entry
> 当访问的时候，host pte 和 ept 都会设置 access / dirty bit (前提是 enable 了)

- [ ] 嵌套虚拟化的中的虚拟机还可以迁移吗？
  - [ ] shadow page table 的 tracking 和 ad bit 的 tracking 可以分别观察下
- [ ] 那么 shadow page table 的 ad bit 是如何处理的

#### ad bit manual
IA32_VMX_EPT_VPID_CAP

When accessed and dirty flags for EPT are enabled, software can track writes to guest-physical addresses using a
feature called page-modification logging.

*Software can enable page-modification logging by setting the “enable PML” VM-execution control (see Table 24-7
in Section 24.6.2).*

**If the processor updated a dirty flag for EPT (changing it from 0 to 1), it then operates
as follows:**
> 当 update a dirty flag for EPT 的自动将地址写入到 PML 中间

#### PML white paper
page modification logging

https://www.intel.com/content/dam/www/public/us/en/documents/white-papers/page-modification-logging-vmm-white-paper.pdf

> extended page-table mechanism (EPT) by allowing that software to
monitor the guest-physical pages modified during guest virtual machine (VM) execution more efficiently.

- [ ] 在没有翻译的时候，ad bit 在什么位置 ?

> This feature(AD bit) enables VMMs
to implement memory-management and page-classification algorithms efficiently so
as to optimize VM memory operations such as defragmentation, paging, etc. Without
accessed and dirty flags, *VMMs may need to emulate them (by marking EPT pagingstructures as not-present or read-only)
and incur the overhead of the resulting EPT violations: VM exits and associated software processing.*

- [ ] 为什么通过 ept 的 read only 可以 intercept
  - [ ] read only 不可以吧，访问没有遇到任何阻碍，还是对于 EPT pagingstructures 存在疑惑
- [ ] 通过 intercept 可以实现什么东西

> For some usage models, VMMs may benefit from additional support to monitor the
working set size. As accessed and dirty flags for EPT are set without invoking the
VMM, there is no opportunity for the VMM to accumulate working-set statistics during
operation. To calculate such statistics the VMM must scan the EPT paging structures
to aggregate the information from the accessed and dirty flags. Such scans impose
both latency and bandwidth costs that may be unacceptable in some circumstances.

- [ ] 获取 working set 真的需要对于 ept 的 dirty bit 进行访问吗 ?
  - [ ] 不就是统计 vm 的内存使用量吗 ?

PML builds upon the processor’s support for accessed and dirty flags for EPT,
extending the processing that occurs when dirty flags for EPT are set. *When PML is
active each write that sets a dirty flag for EPT also generates an entry in an inmemory log, reporting the guest-physical address of the write (aligned to 4 KBytes).
When the log is full, a VM exit occurs, notifying the VMM.*


#### PML code

```c
static int handle_pml_full(struct kvm_vcpu *vcpu)
{
	unsigned long exit_qualification;

	trace_kvm_pml_full(vcpu->vcpu_id);

	exit_qualification = vmx_get_exit_qual(vcpu);

	/*
	 * PML buffer FULL happened while executing iret from NMI,
	 * "blocked by NMI" bit has to be set before next VM entry.
	 */
	if (!(to_vmx(vcpu)->idt_vectoring_info & VECTORING_INFO_VALID_MASK) &&
			enable_vnmi &&
			(exit_qualification & INTR_INFO_UNBLOCK_NMI))
		vmcs_set_bits(GUEST_INTERRUPTIBILITY_INFO,
				GUEST_INTR_STATE_NMI);

	/*
	 * PML buffer already flushed at beginning of VMEXIT. Nothing to do
	 * here.., and there's no userspace involvement needed for PML.
	 */
	return 1;
}
```
- [ ] PML buffer 在 VMEXIT 的时候被 flush 到哪里去了
- [ ] white paper 中间提到 exit_qualification 和 vmcs_set_bits 的原因

## 所以

似乎一共存在三种情况来做 ad 的

1. ept 无 pml
2. ept 有 pml
3. shadow page table 的

## vmx 中，ad bit 是如何借助硬件实现的，也是通过 write protect 吗？还是硬件可以自动跟踪？
<!-- b0e20403-7926-4acf-91f4-ee2c077454ec -->

这个问题，问问 gemimi 3 pro 吧

## qemu 中的 clear_bmap 是如何工作的
<!-- e3e70769-a7fa-4f77-9711-abec061c0a2a -->

### qemu 中的工作
• clear_bmap 是源端 RAM 迁移里的“延迟清 dirty log 的辅助位图”，不是普通的迁移脏页位图 bmap。定义在 include/system/ramblock.h:74。它的
  特点是 1 bit 不一定对应 1 个 guest page，而是对应 1 << clear_bmap_shift 个 page；默认 shift=18，如果 guest page 是 4 KiB，那就是 1
  GiB 一个 bit，范围在 migration/migration.h:52。

  它的工作流程是这样：

  1. 初始化时，QEMU 给每个 RAMBlock 分配 bmap 和 clear_bmap，migration/ram.c:2839。
  2. 做 dirty bitmap sync 时，physical_memory_sync_dirty_bitmap() 先把底层 dirty log 里的脏页并入 rb->bmap；如果启用了 clear_bmap，它
     不立刻清底层 dirty log，而是只在 clear_bmap 里把对应 chunk 标成“之后需要 clear”migration/ram.c:940。
  3. 真正发送某个页之前，migration_bitmap_clear_dirty() 会先调用 migration_clear_memory_region_dirty_bitmap()；它检查当前页所在 chunk
     的 clear_bmap bit，如果置位，就把这个 bit 清掉，并对整个 chunk 调一次 memory_region_clear_dirty_bitmap() migration/ram.c:749,
     migration/ram.c:829。

  核心原因是：必须“在发送前”清底层 dirty log，重新 arm 一次脏页跟踪。这样页面在发送之后如果又被 guest 写脏，下次 sync 还能看到。
  如果等到“发送后”才 clear，就可能把“发送期间重新写脏”的信息一起抹掉，因为那个底层 dirty bit 之前已经是 1 了。

  可以把它理解成：

  - bmap：哪些 guest page 需要发送
  - clear_bmap：哪些大块 dirty log 还没正式清，等到真正要发这块里的页时再清一次

  两个补充点：

  - 它只在 source side 用，且受 ram_state.bitmap_mutex 保护 include/system/ramblock.h:71。
  - 丢弃页、free page hint 这些“等价于已经处理过”的情况，也会顺手清对应 chunk 的底层 dirty log，避免下一轮又被 sync 回来 migration/
    ram.c:777, migration/ram.c:2958。

### kernel 中配套的工作
先看 kernel 中是如何工作的吧，关键在于 kvm_arch_mmu_enable_log_dirty_pt_masked

然后根据是否启用 PML 决定“怎么重新开启跟踪”。 代码在 arch/x86/kvm/mmu/mmu.c:1464。
- 如果 kvm->arch.cpu_dirty_log_size != 0，说明走 CPU dirty logging/PML 路线，调用 kvm_mmu_clear_dirty_pt_masked()。
  它会对目标 SPTE 清 Dirty 位；如果该 SPTE 因别的原因本来就必须写保护，那就清 Writable 位。
- 如果没启用 PML，就调用 kvm_mmu_write_protect_pt_masked()，直接把这些 4K SPTE 写保护。下次 guest 写入时会触发 fault，KVM 再把页记脏。

```c
/*
 * Gets the GFN ready for another round of dirty logging by clearing the
 *	- D bit on ad-enabled SPTEs, and
 *	- W bit on ad-disabled SPTEs.
 * Returns true iff any D or W bits were cleared.
 */
static bool __rmap_clear_dirty(struct kvm *kvm, struct kvm_rmap_head *rmap_head,
			       const struct kvm_memory_slot *slot)
```


## swap out 然后被 swap in 的页面是可以自动的 dirty page tracking 的吧
<!-- 9efa0f2a-7026-4e23-8de8-6a911f156fa2 -->

• 会，前提是这个 memslot 的 dirty logging 仍然是开启状态。swap in 不需要你再额外调用一次 kvm_arch_mmu_enable_log_dirty_pt_masked()。

  更准确地说，流程是这样：

  - host 页被 swap out / 回收时，mmu notifier 会让 KVM 先把对应的二级页表映射打掉，见 virt/kvm/kvm_main.c:727。
  - 之后 guest 再访问这页，会走一次新的 KVM fault 路径；KVM 通过 hva_to_pfn() / __kvm_faultin_pfn() 把 host 页重新 fault in，见 virt/kvm/
    kvm_main.c:3008 和 virt/kvm/kvm_main.c:3072。
  - 新 SPTE 重建时，会按当前 memslot 的 dirty logging 状态来建，而不是“沿用 swap 前那个 SPTE”。实际建 SPTE 的地方在
   arch/x86/kvm/mmu/spte.c:make_spte。
  - 如果该 memslot 开着 dirty tracking，KVM 会继续按 dirty logging 语义处理；比如新建的是可写 SPTE 时，会立刻把页记到 dirty bitmap/ring，见

make_spte 中的，如果这次 fault 本身已经说明 guest 要写这页了，KVM 可以“现在就记脏，然后直接装一个可写 SPTE”，
没必要让后续每次写都再 trap :
```c
	/*
	 * Mark the memslot dirty *after* modifying it for access tracking.
	 * Unlike folios, memslots can be safely marked dirty out of mmu_lock,
	 * i.e. in the fast page fault handler.
	 */
	if ((spte & PT_WRITABLE_MASK) && kvm_slot_dirty_track_enabled(slot)) {
		/* Enforced by kvm_mmu_hugepage_adjust. */
		WARN_ON_ONCE(level > PG_LEVEL_4K);
		mark_page_dirty_in_slot(vcpu->kvm, slot, gfn);
	}
```

如果 swap in 进来，对于 read fault ，那么就自动的配置 shadow_dirty_mask ，这个逻辑太奇怪了。

## 热迁移为什么不会出现 dirty 丢失
<!-- e014940a-08f6-4727-93d5-4c6978d30bfd -->

问题: 我们发现 log sync 和 log clear 是划分为两个动作的:
```c
kml->listener.log_sync = kvm_log_sync;
kml->listener.log_clear = kvm_log_clear;
```

既然 kvm_log_sync 和 kvm_log_clear 不是同时进行的，那么按道理，会不会出现，
A : get dirty bitmap
(然后将页面发送到 target 端)
B : page 被 dirty 了
C : 重新打开 dirty log ，开启 dirty log 的记录（通过 kvm_log_clear )
D : 后续没有 dirty

所以 B 时间的被 dirty 没有记录下来。如果要解决这个问题，
似乎需要是在 log sync 和 log clear 两个动作需要合并为一个原子的动作才可以的?

> [!NOTE]
> 参考神奇海螺的意见，有待验证

• 会想到这个问题是对的，但按现在这套语义，你画的 A-B-C-D 这个场景不会漏记。关键是先区分两种模式。

  先纠正一下概念：kvm_log_clear() 不是“重新打开 dirty log”，而是对一批页做“清 dirty 位 + 重新开启跟踪/重新写保护”。
  QEMU 的顺序确实是先 log_sync，再处理/清自己的 bitmap，最后 log_clear，见 system/memory.c:2293 和 accel/kvm/kvm-all.c:1959。

  分两种情况看：

  1. 旧模式，没有 manual_dirty_log_protect
     kvm_log_clear() 直接是 no-op，见 accel/kvm/kvm-all.c:kvm_physical_log_clear。
     这时语义靠 KVM_GET_DIRTY_LOG 本身保证。内核文档说明：它返回“自上次调用以来”的 dirty 页；
     而 MANUAL_DIRTY_LOG_PROTECT2 没开时，GET 会顺手做清位和重新保护。
  2. 新模式，开启 KVM_CAP_MANUAL_DIRTY_LOG_PROTECT2
     文档明确说：开了 KVM_DIRTY_LOG_MANUAL_PROTECT_ENABLE 后，KVM_GET_DIRTY_LOG 不会自动 clear+write-protect，
     必须之后用 KVM_CLEAR_DIRTY_LOG 单独做。QEMU 也就是在这时才真的走 kvm_log_clear()

  这时再看你的时序：

  - 如果某页在 A 时还是 clean，然后在 B 才第一次被写脏
    那它不在 C 的 clear bitmap 里，所以 KVM_CLEAR_DIRTY_LOG 不会把它清掉。它会继续留在 KVM 的 dirty bitmap 中，下一次 GET 还能看到。
    所以你担心的“B 被 dirty 但后面没再 dirty，于是丢了”在这里不会发生。
  - 如果某页在 A 时已经 dirty，然后 B 又被写了一次
    这也不算丢，因为它本来就已经在本轮要发送的集合里了。更重要的是，
    迁移代码要求必须在发送该页内容之前先 clear/reprotect，否则才会有问题，
    见 migration/ram.c:migration_bitmap_clear_dirty。
    也就是说：
      - C 之前的写，通常会体现在本轮真正读取到的页内容里；
      - C 之后再写，会因为重新保护而进入下一轮 dirty 统计。

  真正危险的反而是两种事，QEMU 代码里都点明了：

  - clear 了“还没 sync 到 userspace 的 unknown dirty bits”，这会直接 data loss，见 accel/kvm/kvm-all.c:1225
  - 先读页内容，后 clear/reprotect，这样才会把后续写漏掉，见 migration/ram.c:migration_bitmap_clear_dirty

  所以结论是：

  你这个 A-B-C-D 担忧，按 KVM/QEMU 现在的 dirty logging 协议设计，不会导致漏记。
  真正的保证点不是“log_sync 和 log_clear 同时发生”，而是：

  - 旧模式：GET_DIRTY_LOG 自带原子语义
  - 新模式：CLEAR_DIRTY_LOG 只清“本轮已经知道的 dirty 页”，而且必须在真正读页数据前完成

  参考：

  - KVM API 文档：KVM_CLEAR_DIRTY_LOG / KVM_CAP_MANUAL_DIRTY_LOG_PROTECT2
    https://docs.kernel.org/virt/kvm/api.html

所以，其实关键的问题在于:
1. 什么时候发送 dirty page ，需要在 clear log 之后才可以发送 page
2. clear_log 中，只有将 spte 的 dirty bit 清理掉，不会出现将 dirty bitmap 清理的情况

也就是
A : get dirty bitmap
B : page 被 dirty 了
C : 重新打开 dirty log ，开启 dirty log 的记录（通过 kvm_log_clear )
(就是 clear log 之后才可以发送的)
D : 后续没有 dirty

### clear_log 的时序问题
内核里 KVM_CLEAR_DIRTY_LOG 的顺序：先清位，再重新保护，再 flush TLB

在 kvm_clear_dirty_log_protect() 里顺序是：

- 先 kvm_arch_sync_dirty_log()，把 CPU/PML 等硬件缓冲先冲到 dirty_bitmap 里。
- 然后对用户传入的 mask 执行 atomic_long_fetch_andnot(mask, p)，原子地把 dirty bitmap 对应位清掉。
- 对“真的被清掉的那些位”，调用 kvm_arch_mmu_enable_log_dirty_pt_masked()，重新开启这一批页的 dirty tracking。
- 最后如果有必要，kvm_flush_remote_tlbs_memslot()。

这里“重新开启 tracking”在 x86 上本质上是：
- A/D 模式下清 SPTE dirty bit，或
- 非 A/D 模式下清 writable bit / write-protect

```c
	KVM_MMU_LOCK(kvm);
	for (offset = log->first_page, i = offset / BITS_PER_LONG,
		 n = DIV_ROUND_UP(log->num_pages, BITS_PER_LONG); n--;
	     i++, offset += BITS_PER_LONG) {
		unsigned long mask = *dirty_bitmap_buffer++; // 用户态传递过来的清理
		atomic_long_t *p = (atomic_long_t *) &dirty_bitmap[i]; // kvm 关闭的 dirty_bitmap
		if (!mask)
			continue;

		mask &= atomic_long_fetch_andnot(mask, p);

		/*
		 * mask contains the bits that really have been cleared.  This
		 * never includes any bits beyond the length of the memslot (if
		 * the length is not aligned to 64 pages), therefore it is not
		 * a problem if userspace sets them in log->dirty_bitmap.
		*/
		if (mask) {
			flush = true;
			kvm_arch_mmu_enable_log_dirty_pt_masked(kvm, memslot,
								offset, mask);
		}
	}
	KVM_MMU_UNLOCK(kvm);

	if (flush)
		kvm_flush_remote_tlbs_memslot(kvm, memslot);
```

这个时序问题，我想想，其实还好，首先，不是将 memslot->dirty_bitmap 全部都配置为 0 ，
而是仅仅用户态传递过来的清理掉，而用户态传递过来，是因为用户态计划发送这些 dirty page 的。

基本逻辑是这样的:
```txt
---- ( clear --- protect again ) ---- send ---
```

我们思考问题的方式，就是，现在 write 可能发生在任何位置，那么是否会漏掉。


如果这样的，其实也是可以的，例如如果一个写操作发生在 protect again 到 clear 之间，
那么的确不会记录下来，但是还是会被 send 走的。
```txt
---- ( protect again --- clear ) ---- send ---
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
