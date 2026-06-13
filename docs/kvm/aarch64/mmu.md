## 基本流程
- kvm_handle_guest_abort
  - user_mem_abort
  - kvm_mmu_topup_memory_cache : 和 x86 一样，也是从 cache 中拿页

## 简单分析下 kernel 的 mmu 的实现
```txt
@[
    __tlb_switch_to_guest+0
    kvm_tlb_flush_vmid_range+196
    kvm_pgtable_stage2_unmap+176
    stage2_apply_range+116
    __unmap_stage2_range+52
    kvm_arch_flush_shadow_memslot+68
    kvm_set_memslot+444
    __kvm_set_memory_region+1068
    kvm_vm_ioctl+1524
    __arm64_sys_ioctl+180
    invoke_syscall+116
    el0_svc_common.constprop.0+72
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 68
@[
    __tlb_switch_to_guest+0
    kvm_tlb_flush_vmid_range+196
    kvm_pgtable_stage2_unmap+176
    stage2_apply_range+216
    __unmap_stage2_range+52
    kvm_arch_flush_shadow_memslot+68
    kvm_set_memslot+444
    __kvm_set_memory_region+1068
    kvm_vm_ioctl+1524
    __arm64_sys_ioctl+180
    invoke_syscall+116
    el0_svc_common.constprop.0+72
    do_el0_svc+36
    el0_svc+60
    el0t_64_sync_handler+288
    el0t_64_sync+404
]: 71
@[]: 81881
```

shit ，居然又是 shadow !

这个函数居然依旧观测不到了。
kvm_tlb_flush_vmid_range

## 有 async pf 吗?


## tdp 这个机制有吗?

## mmu notifier 的实现在什么地方?


## 为什么 hyp 下还有 page table

arch/arm64/kvm/hyp/pgtable.c

和  arch/arm64/kvm/mmu.c 的关系是什么?
```txt
@[
        __kvm_pgtable_walk+0
        kvm_pgtable_stage2_unmap+88
        stage2_apply_range+216
        __unmap_stage2_range+56
        kvm_arch_flush_shadow_memslot+72
        kvm_set_memslot+436
        kvm_set_memory_region+720
        kvm_vm_ioctl+1788
        __arm64_sys_ioctl+180
        invoke_syscall+80
        el0_svc_common.constprop.0+72
        do_el0_svc+36
        el0_svc+52
        el0t_64_sync_handler+268
        el0t_64_sync+400
]: 3539
```

## 硬件支持 dirty tracking 吗?

## 四个 tracepoint 只有这一个会调用
- trace_kvm_access_fault : 不会调用
- trace_kvm_guest_fault : good !
- trace_kvm_set_way_flush : 不会
- trace_kvm_toggle_cache : 不会

## io_mem_abort && user_mem_abort

- 这些含义是什么意思?
```c
/* For compatibility with fault code shared with 32-bit */
#define FSC_FAULT   ESR_ELx_FSC_FAULT
#define FSC_ACCESS  ESR_ELx_FSC_ACCESS
#define FSC_PERM    ESR_ELx_FSC_PERM
#define FSC_SEA     ESR_ELx_FSC_EXTABT
#define FSC_SEA_TTW0    (0x14)
#define FSC_SEA_TTW1    (0x15)
#define FSC_SEA_TTW2    (0x16)
#define FSC_SEA_TTW3    (0x17)
#define FSC_SECC    (0x18)
#define FSC_SECC_TTW0   (0x1c)
#define FSC_SECC_TTW1   (0x1d)
#define FSC_SECC_TTW2   (0x1e)
#define FSC_SECC_TTW3   (0x1f)
```

## ARM page table
- https://developer.arm.com/documentation/101811/0102/Translation-granule
- page walk 和 pagesize

## 这个当时是在搞啥来着?

内核之后改进:
```diff
History:        #0
Commit:         c726200dd106d4c58a281eea7159b8ba28a4ab34
Author:         Christoffer Dall <christoffer.dall@arm.com>
Committer:      Marc Zyngier <maz@kernel.org>
Author Date:    Fri 11 Oct 2019 07:07:05 PM CST
Committer Date: Tue 22 Oct 2019 01:59:44 AM CST

KVM: arm/arm64: Allow reporting non-ISV data aborts to userspace

For a long time, if a guest accessed memory outside of a memslot using
any of the load/store instructions in the architecture which doesn't
supply decoding information in the ESR_EL2 (the ISV bit is not set), the
kernel would print the following message and terminate the VM as a
result of returning -ENOSYS to userspace:

  load/store instruction decoding not implemented

The reason behind this message is that KVM assumes that all accesses
outside a memslot is an MMIO access which should be handled by
userspace, and we originally expected to eventually implement some sort
of decoding of load/store instructions where the ISV bit was not set.

However, it turns out that many of the instructions which don't provide
decoding information on abort are not safe to use for MMIO accesses, and
the remaining few that would potentially make sense to use on MMIO
accesses, such as those with register writeback, are not used in
practice.  It also turns out that fetching an instruction from guest
memory can be a pretty horrible affair, involving stopping all CPUs on
SMP systems, handling multiple corner cases of address translation in
software, and more.  It doesn't appear likely that we'll ever implement
this in the kernel.

What is much more common is that a user has misconfigured his/her guest
and is actually not accessing an MMIO region, but just hitting some
random hole in the IPA space.  In this scenario, the error message above
is almost misleading and has led to a great deal of confusion over the
years.

It is, nevertheless, ABI to userspace, and we therefore need to
introduce a new capability that userspace explicitly enables to change
behavior.

This patch introduces KVM_CAP_ARM_NISV_TO_USER (NISV meaning Non-ISV)
which does exactly that, and introduces a new exit reason to report the
event to userspace.  User space can then emulate an exception to the
guest, restart the guest, suspend the guest, or take any other
appropriate action as per the policy of the running system.

Reported-by: Heinrich Schuchardt <xypron.glpk@gmx.de>
Signed-off-by: Christoffer Dall <christoffer.dall@arm.com>
Reviewed-by: Alexander Graf <graf@amazon.com>
Signed-off-by: Marc Zyngier <maz@kernel.org>
```
- 对应的 link : https://lore.kernel.org/linux-arm-kernel/20190909121337.27287-2-christoffer.dall@arm.com/

QEMU 对应的位置:
```diff
History:   #0
Commit:    694bcaa81f41b7fc5e07273debe1dc309b3dcf03
Author:    Beata Michalska <beata.michalska@linaro.org>
Committer: Peter Maydell <peter.maydell@linaro.org>
Date:      Fri 03 Jul 2020 11:59:42 PM CST

target/arm: kvm: Handle DABT with no valid ISS

On ARMv7 & ARMv8 some load/store instructions might trigger a data abort
exception with no valid ISS info to be decoded. The lack of decode info
makes it at least tricky to emulate those instruction which is one of the
(many) reasons why KVM will not even try to do so.

Add support for handling those by requesting KVM to inject external
dabt into the quest.

Signed-off-by: Beata Michalska <beata.michalska@linaro.org>
Reviewed-by: Andrew Jones <drjones@redhat.com>
Message-id: 20200629114110.30723-2-beata.michalska@linaro.org
Signed-off-by: Peter Maydell <peter.maydell@linaro.org>
```

### [ ] 难道对于 mmio 是可以不用注册 memslot 的吗

### [ ] 为什么 ISV

### [ ] 什么样子的 load/store 会触发 abort 同时是没有 valid ISS 的

### External Data Abort 的含义是什么
- 难道还有 internal data abort ?

- el0_ia 和 el0_da 的处理都是相似的，调用 do_mem_abort

- [x] 哪一个字段描述是 external 还是 internal : EA，ESR_ELx_EA，但是似乎有问题的

- 这里开始: https://developer.arm.com/documentation/ddi0344/b/programmer-s-model/exceptions/aborts
- 还是 stackoverflow 讲解的清楚一点: https://stackoverflow.com/questions/33304717/external-abort-in-arm-processor

### [ ] external data abort 被注入的结果是什么

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
