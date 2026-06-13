## 为什么需要 arch/x86/kvm/mmu/paging_tmpl.h 来处理各种情况
<!-- 4e00f2c8-cbc2-4b9f-ad92-991853929887 -->

```txt
  x86 架构有 3种不同的页表格式：

   模式                      PTE大小   层级    特性
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   32-bit分页                4字节     2级     传统32位
   64-bit分页 (PAE/IA-32e)   8字节     4/5级   NX位、大页等
   EPT (扩展页表)            8字节     4级     嵌套虚拟化专用
```

```txt
   模板代码              PTTYPE=64               PTTYPE=32               PTTYPE_EPT
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   FNAME(fetch)          paging64_fetch          paging32_fetch          ept_fetch
   FNAME(walk_addr)      paging64_walk_addr      paging32_walk_addr      ept_walk_addr
   FNAME(page_fault)     paging64_page_fault     paging32_page_fault     ept_page_fault
   FNAME(pte_prefetch)   paging64_pte_prefetch   paging32_pte_prefetch   ept_pte_prefetch
```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

核心功能模块:

- 客户机页表遍历（Guest Page Table Walk）
```c
  static int FNAME(walk_addr)(struct guest_walker *walker,
                              struct kvm_vcpu *vcpu, gpa_t addr, u64 access)
  {
      // 模拟硬件MMU遍历客户机页表
      // 从CR3开始，逐级读取GPTE
      // 检查权限、A/D位、保留位等
      // 返回最终的GFN和访问权限
  }
```

- 影子页表填充（Shadow Page Table Fill）

```c
  static int FNAME(fetch)(struct kvm_vcpu *vcpu,
                          struct kvm_page_fault *fault,
                          struct guest_walker *gw)
  {
      // 遍历影子页表，按需创建中间页表
      // 建立 GFN → PFN 的映射
      // 处理大页、NX等特殊情况
  }
```

- A/D位更新

```c
  static int FNAME(update_accessed_dirty_bits)(...)
  {
      // 更新客户机页表的Accessed/Dirty位
      // 不同模式A/D位位置不同（EPT vs 普通页表）
  }
```

- 页Fault处理
```c
  static int FNAME(page_fault)(struct kvm_vcpu *vcpu,
                               struct kvm_page_fault *fault)
  {
      // 影子分页的缺页处理入口
      // 协调 walker + fetch 完成映射建立
  }
```

简而言之，有时候，需要 walk guest 的 page table 。


最后一个经典案例:
```txt
@[
    ept_walk_addr_generic+1
    ept_page_fault+106
    kvm_mmu_do_page_fault+287
    kvm_mmu_page_fault+130
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+1765
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 17468
```

总结一下就是:
```c
/*
 * Page fault handler.  There are several causes for a page fault:
 *   - there is no shadow pte for the guest pte
 *   - write access through a shadow pte marked read only so that we can set
 *     the dirty bit
 *   - write access to a shadow pte marked read only so we can update the page
 *     dirty bitmap, when userspace requests it
 *   - mmio access; in this case we will never install a present shadow pte
 *   - normal guest page fault due to the guest pte marked not present, not
 *     writable, or not executable
 *
 *  Returns: 1 if we need to emulate the instruction, 0 otherwise, or
 *           a negative value on error.
 */
static int FNAME(page_fault)(struct kvm_vcpu *vcpu, struct kvm_page_fault *fault)
```

- ept_page_fault
  - ept_walk_addr : 尝试 page walk L1 guest
  - kvm_faultin_pfn
    - `__kvm_faultin_pfn`
  - ept_fetch : (应该是遍历 guest page table 的)
    - kvm_mmu_get_shadow_page
      - `__kvm_mmu_get_shadow_page`
        - kvm_mmu_find_shadow_page
        - kvm_mmu_alloc_shadow_page
          - account_shadowed
            - kvm_slot_page_track_add_page : 将 shadow page 关联的 page 监控起来
    - link_shadow_page


## 一共定义了哪些函数
```txt
󰆼  guest_walkerEPT
󰊕  ept_gpte_to_gfn_lvl
󰊕  ept_protect_clean_gpte
󰊕  ept_is_present_gpte
󰊕  ept_is_bad_mt_xwr
󰊕  ept_is_rsvd_bits_set
󰊕  ept_prefetch_invalid_gpte
󰊕  ept_gpte_access
󰊕  ept_update_accessed_dirty_bits
󰊕  ept_gpte_pkeys
󰊕  ept_is_last_gpte
󰊕  ept_walk_addr_generic
󰊕  ept_walk_addr
󰊕  ept_prefetch_gpte
󰊕  ept_gpte_changed
󰊕  ept_pte_prefetch
󰊕  ept_fetch
󰊕  ept_page_fault
󰊕  ept_get_level1_sp_gpa
󰊕  ept_gva_to_gpa
󰊕  ept_sync_spte
```

## tdp mmu 为什么需要这个 walk guest 的 page table
<!-- d576574f-2463-415f-9871-23ca8d1b7c28 -->

这不算太奇怪，首先一些模拟操作是需要 walk guest 的 page table 的，
此外，虚拟机有 nonpaging 的模式，然后有 paging64 的模式
```txt
@[
    paging64_gva_to_gpa+5
    emulator_read_write_onepage+172
    emulator_read_write+191
    x86_emulate_insn+1763
    x86_emulate_instruction+778
    kvm_arch_vcpu_ioctl_run+1713
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+59
    entry_SYSCALL_64_after_hwframe+110
]: 80
```

```txt
@[
    paging64_gva_to_gpa+5
    kvm_fetch_guest_virt+84
    __do_insn_fetch_bytes+441
    x86_decode_insn+1735
    x86_decode_emulated_instruction+58
    x86_emulate_instruction+522
    vmx_handle_exit+2021
    kvm_arch_vcpu_ioctl_run+1718
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 14418
```

```txt
@[
    nonpaging_gva_to_gpa+5
    emulator_read_write_onepage+172
    emulator_read_write+191
    x86_emulate_insn+1724
    x86_emulate_instruction+1070
    vmx_handle_exit+2021
    kvm_arch_vcpu_ioctl_run+1718
    kvm_vcpu_ioctl+587
    __x64_sys_ioctl+148
    do_syscall_64+197
    entry_SYSCALL_64_after_hwframe+111
]: 39276
```

当 Guest 发生切换的时候，这些调用的函数最后就切换了，那么可以知道是什么时候切换的吗?

```txt
@[
    paging64_walk_addr_generic+1
    paging64_gva_to_gpa+83
    kvm_fetch_guest_virt+84
    __do_insn_fetch_bytes+441
    x86_decode_insn+219
    x86_decode_emulated_instruction+58
    x86_emulate_instruction+1882
    kvm_mmu_page_fault+322
    vmx_handle_exit+2045
    kvm_arch_vcpu_ioctl_run+1765
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 7644
```

```txt
@[
        nonpaging_gva_to_gpa+5
        vcpu_mmio_gva_to_gpa+126
        emulator_read_write_onepage+123
        emulator_read_write+209
        x86_emulate_insn+1437
        x86_emulate_instruction+910
        npf_interception+147
        vcpu_enter_guest.constprop.0+1623
        vcpu_run+55
        kvm_arch_vcpu_ioctl_run+739
        kvm_vcpu_ioctl+724
        __x64_sys_ioctl+161
        do_syscall_64+97
        entry_SYSCALL_64_after_hwframe+118
]: 41960
```

只有 32bit 的空的:
```txt
sudo bpftrace -e "kprobe:paging32_gva_to_gpa { @[kstack] = count(); }"
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
