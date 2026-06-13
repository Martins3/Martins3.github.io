# pfncache.c 以及其他的辅助函数

## 1. 安全且复杂

CONFIG_HAVE_KVM_PFNCACHE

```c
struct gfn_to_pfn_cache {
	u64 generation;
	gpa_t gpa;
	unsigned long uhva;
	struct kvm_memory_slot *memslot;
	struct kvm *kvm;
	struct list_head list;
	rwlock_t lock;
	struct mutex refresh_lock;
	void *khva;
	kvm_pfn_t pfn;
	bool active;
	bool valid;
};
```

主要是 xen 的使用，其余的使用只有:

kvm_arch_vcpu_create
```txt
	kvm_gpc_init(&vcpu->arch.pv_time, vcpu->kvm);
```

经常的调用路线是:
```txt
@[
    kvm_gpc_activate+5
    kvm_write_system_time # 这个
    kvm_set_msr_common+880
    vmx_set_msr+2684
    __kvm_set_msr+183
    kvm_emulate_wrmsr+84
    vmx_handle_exit+1205
    kvm_arch_vcpu_ioctl_run+6873
    kvm_vcpu_ioctl+1537
    __se_sys_ioctl+115
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 65
```

简单看了下，之所以搞这么复杂就是为了 mmu notifier 如果过来把 page table 清理了，
由于 cache 没有感知到，那么可能缓存的是错误的

### 为什么 system-time 需要用 gfn

在 kvm_write_system_time 中调用到 kvm_gpc_activate 之后，这个 GPA 是 guest os 传递过来的
通过这个 gpa 需要获取到 hva ，这里的 hva 有两个含义， uhva 和 khva :
```c
struct gfn_to_pfn_cache {
	unsigned long uhva;
	void *khva;
};
```
当然，如果被 kernel 直接映射了，就不要考虑 uaccess 的问题了


如何使用的就有点曲折了，兄弟们

在 kvm_guest_time_update -> kvm_setup_guest_pvclock 中:
```c
  // 首先开始上锁
	read_lock_irqsave(&gpc->lock, flags);
	while (!kvm_gpc_check(gpc, offset + sizeof(*guest_hv_clock))) {
		read_unlock_irqrestore(&gpc->lock, flags);

		if (kvm_gpc_refresh(gpc, offset + sizeof(*guest_hv_clock)))
			return;

		read_lock_irqsave(&gpc->lock, flags);
	}
  // 这里使用 khva 来直接访问的，没有什么奇怪的 uaccess 的
	guest_hv_clock = (void *)(gpc->khva + offset);
```

gfn_to_pfn_cache_invalidate_start 被 kvm_mmu_notifier_invalidate_range_start 调用


### [ ] IOMAP 的问题

```c
static void *gpc_map(kvm_pfn_t pfn)
{
	if (pfn_valid(pfn))
		return kmap(pfn_to_page(pfn));

#ifdef CONFIG_HAS_IOMEM
	return memremap(pfn_to_hpa(pfn), PAGE_SIZE, MEMREMAP_WB);
#else
	return NULL;
#endif
}
```

### 原始提交的

```diff
History:        #0
Commit:         982ed0de4753ed6e71dbd40f82a5a066baf133ed
Author:         David Woodhouse <dwmw@amazon.co.uk>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Sat 11 Dec 2021 12:36:21 AM CST
Committer Date: Fri 07 Jan 2022 11:44:44 PM CST

KVM: Reinstate gfn_to_pfn_cache with invalidation support

This can be used in two modes. There is an atomic mode where the cached
mapping is accessed while holding the rwlock, and a mode where the
physical address is used by a vCPU in guest mode.

For the latter case, an invalidation will wake the vCPU with the new
KVM_REQ_GPC_INVALIDATE, and the architecture will need to refresh any
caches it still needs to access before entering guest mode again.

Only one vCPU can be targeted by the wake requests; it's simple enough
to make it wake all vCPUs or even a mask but I don't see a use case for
that additional complexity right now.

Invalidation happens from the invalidate_range_start MMU notifier, which
needs to be able to sleep in order to wake the vCPU and wait for it.

This means that revalidation potentially needs to "wait" for the MMU
operation to complete and the invalidate_range_end notifier to be
invoked. Like the vCPU when it takes a page fault in that period, we
just spin — fixing that in a future patch by implementing an actual
*wait* may be another part of shaving this particularly hirsute yak.

As noted in the comments in the function itself, the only case where
the invalidate_range_start notifier is expected to be called *without*
being able to sleep is when the OOM reaper is killing the process. In
that case, we expect the vCPU threads already to have exited, and thus
there will be nothing to wake, and no reason to wait. So we clear the
KVM_REQUEST_WAIT bit and send the request anyway, then complain loudly
if there actually *was* anything to wake up.

Signed-off-by: David Woodhouse <dwmw@amazon.co.uk>
Message-Id: <20211210163625.2886-3-dwmw2@infradead.org>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

## 2. kvm_vcpu_map
```c
struct kvm_host_map hv_evmcs_map;
```

```c
struct kvm_host_map {
	/*
	 * Only valid if the 'pfn' is managed by the host kernel (i.e. There is
	 * a 'struct page' for it. When using mem= kernel parameter some memory
	 * can be used as guest memory but they are not managed by host
	 * kernel).
	 * If 'pfn' is not managed by the host kernel, this field is
	 * initialized to KVM_UNMAPPED_PAGE.
	 */
	struct page *page;
	void *hva;
	kvm_pfn_t pfn;
	kvm_pfn_t gfn;
};
```
目前只是配合 kvm_vcpu_unmap 和 kvm_vcpu_map 使用的，而且基本用于嵌套虚拟化:

例如: vmx_has_nested_events 中直接使用了 virtual_apic_map
```c
	void *vapic = vmx->nested.virtual_apic_map.hva;
```

```txt
@[
    vmx_has_nested_events+5
    kvm_check_and_inject_events+448
    kvm_arch_vcpu_ioctl_run+2394
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 141
```

映射的地方是: nested_get_vmcs12_pages
```txt
@[
    nested_get_vmcs12_pages+1
    nested_vmx_enter_non_root_mode+386
    nested_vmx_run+293
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+1765
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 145951
```

感觉这里其实是有错误的，没有出现问题只是运气好而已。
没人使用嵌套虚拟化而已。 kvm_vcpu_map 没有 pin 地址，实际上会带来问题的。


```txt
History:        #0
Commit:         357a18ad230f0867791b788d2b1d6f280f6f6e61
Author:         David Woodhouse <dwmw@amazon.co.uk>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Tue 16 Nov 2021 12:50:27 AM CST
Committer Date: Thu 18 Nov 2021 03:03:45 PM CST

KVM: Kill kvm_map_gfn() / kvm_unmap_gfn() and gfn_to_pfn_cache

In commit 7e2175ebd695 ("KVM: x86: Fix recording of guest steal time /
preempted status") I removed the only user of these functions because
it was basically impossible to use them safely.

There are two stages to the GFN->PFN mapping; first through the KVM
memslots to a userspace HVA and then through the page tables to
translate that HVA to an underlying PFN. Invalidations of the former
were being handled correctly, but no attempt was made to use the MMU
notifiers to invalidate the cache when the HVA->GFN mapping changed.

As a prelude to reinventing the gfn_to_pfn_cache with more usable
semantics, rip it out entirely and untangle the implementation of
the unsafe kvm_vcpu_map()/kvm_vcpu_unmap() functions from it.

All current users of kvm_vcpu_map() also look broken right now, and
will be dealt with separately. They broadly fall into two classes:

* Those which map, access the data and immediately unmap. This is
  mostly gratuitous and could just as well use the existing user
  HVA, and could probably benefit from a gfn_to_hva_cache as they
  do so.

* Those which keep the mapping around for a longer time, perhaps
  even using the PFN directly from the guest. These will need to
  be converted to the new gfn_to_pfn_cache and then kvm_vcpu_map()
  can be removed too.

Signed-off-by: David Woodhouse <dwmw@amazon.co.uk>
Message-Id: <20211115165030.7422-8-dwmw2@infradead.org>
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

## 3. uaccess
### 常规的

```c
struct gfn_to_hva_cache {
	u64 generation;
	gpa_t gpa;
	unsigned long hva;
	unsigned long len;
	struct kvm_memory_slot *memslot;
};
```

典型的使用是: kvm_gfn_to_hva_cache_init

例如 pv eoi 的:

```c
	struct {
		u64 msr_val;
		struct gfn_to_hva_cache data;
	} pv_eoi;
```

kvm_lapic_set_pv_eoi 中初始化
```c
	struct gfn_to_hva_cache *ghc = &vcpu->arch.pv_eoi.data;
	ret = kvm_gfn_to_hva_cache_init(vcpu->kvm, ghc, addr, new_len);
```

之后的使用也是很简单了:

```c
static int pv_eoi_get_user(struct kvm_vcpu *vcpu, u8 *val)
{

	return kvm_read_guest_cached(vcpu->kvm, &vcpu->arch.pv_eoi.data, val,
				      sizeof(*val));
}
```

### 特殊的
例如在 `record_steal_time` 中为了使用 xchg ，没有使用 copy from user :

`ghc` 结构体存放到:
```c
	struct gfn_to_hva_cache *ghc = &vcpu->arch.st.cache;
```

如果不匹配，那么就重新使用刷新，否则直接使用，
```c
	if (unlikely(slots->generation != ghc->generation ||
		     gpa != ghc->gpa ||
		     kvm_is_error_hva(ghc->hva) || !ghc->memslot)) {
		/* We rely on the fact that it fits in a single page. */
		BUILD_BUG_ON((sizeof(*st) - 1) & KVM_STEAL_VALID_BITS);

		if (kvm_gfn_to_hva_cache_init(vcpu->kvm, ghc, gpa, sizeof(*st)) ||
		    kvm_is_error_hva(ghc->hva) || !ghc->memslot)
			return;
	}

	st = (struct kvm_steal_time __user *)ghc->hva;
```
gpa 可以不用 page 对齐的，可以是任意的地址的。

映射了之后，不去使用 khva 来映射，而是使用 qemu 的地址空间。

应该是可以触发 page fault 的，之所以不去使用 copy_from_user 的方法，显然
因为遇到的问题是当两个.

## 4. 没有缓存的
类似访问 page table 之类的东西，都是需要访问 guest os 的内存

kvm_fetch_guest_virt -> kvm_vcpu_read_guest_page

这里没有做 cache ，每次都需要进行一次调用从 gfn 到 memslot 或者 hva 的地址。


## 思考
为什么搞这么复杂的 CONFIG_HAVE_KVM_PFNCACHE
1. copy from user 是会睡眠的，而 KVM_PFNCACHE 是阻塞其他的人

这里的 slot 需要有什么锁吧

这几个方法是如何避免 slot 被修改的，前面刚 cache 的，后面就修改了

## TODO
- kvm_read_guest_cached 和 kvm_vcpu_read_guest_page 什么关系?

整理掉这个:
### gfn_to_hva_cache 是什么 cache
kvm_gfn_to_hva_cache_init : 不就是 gfa 到 hva 吗?

vm_gfn_to_hva_cache_init : 似乎不是 tlb ，而是各种检查的问题吧

什么时候使用?

```txt
sudo bpftrace -e "kprobe:kvm_read_guest_offset_cached { @[kstack] = count(); }"
Attaching 1 probe...
^C

@[
    kvm_read_guest_offset_cached+5
    kvm_arch_can_dequeue_async_page_present+104
    kvm_check_async_pf_completion+150
    kvm_arch_vcpu_ioctl_run+3851
    kvm_vcpu_ioctl+1537
    __se_sys_ioctl+115
    do_syscall_64+237
    entry_SYSCALL_64_after_hwframe+119
]: 64
```

作为对比 kvm_write_guest 和 kvm_write_guest_cached

可以看到，似乎是需要解决一下，如果 gpa 跨多个 memslot 的问题，其实没有什么的

### kvm_vcpu_unmap 就是给嵌套用的

如果没有开嵌套， kvm_vcpu_unmap 完全不会调用

如果开启嵌套，从 L0 可以观测到。
当然，L1 中还是观测不到 kvm_vcpu_unmap 的调用

```txt
@[
    kvm_vcpu_unmap+5
    nested_vmx_vmexit+544
    nested_ept_inject_page_fault+252
    ept_page_fault+430
    kvm_mmu_do_page_fault+287
    kvm_mmu_page_fault+130
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+1765
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 2217
@[
    kvm_vcpu_unmap+5
    nested_vmx_vmexit+564
    nested_vmx_reflect_vmexit+1633
    vmx_handle_exit+116
    kvm_arch_vcpu_ioctl_run+1765
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 82281
@[
    kvm_vcpu_unmap+5
    nested_vmx_vmexit+584
    nested_vmx_reflect_vmexit+1633
    vmx_handle_exit+116
    kvm_arch_vcpu_ioctl_run+1765
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 82281
@[
    kvm_vcpu_unmap+5
    nested_vmx_vmexit+544
    nested_vmx_reflect_vmexit+1633
    vmx_handle_exit+116
    kvm_arch_vcpu_ioctl_run+1765
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 82283
@[
    kvm_vcpu_unmap+5
    nested_get_vmcs12_pages+1130
    nested_vmx_enter_non_root_mode+386
    nested_vmx_run+293
    vmx_handle_exit+300
    kvm_arch_vcpu_ioctl_run+1765
    kvm_vcpu_ioctl+558
    __x64_sys_ioctl+148
    do_syscall_64+193
    entry_SYSCALL_64_after_hwframe+119
]: 85971
```

kvm_vcpu_map 和 kvm_vcpu_unmap 就是为了解决 vmcs page 的

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
