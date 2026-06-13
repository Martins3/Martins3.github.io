## PV_TLB_FLUSH
<!-- 92e5f9ea-5286-454f-a8b3-ce1d9f197d99 -->

- https://blog.blog.kernel.love/para-virt-remote-tlb-flush.html

因为 remote tlb 是 busy wait 的，但是 target cpu 也许没有在运行。

## 物理机中使用

参考: arch/x86/mm/tlb.c

实现的 api 比想象的复杂。

## 基本的执行流程
kvm_arch_vcpu_ioctl_run => record_steal_time

```sh
sudo bpftrace -e 'tracepoint:kvm:kvm_pv_tlb_flush { printf("pv tlb\n"); }'
```
guest 中:
- collapse_file
  - try_to_unmap_flush
    - arch_tlbbatch_flush
      - flush_tlb_multi
        - kvm_flush_tlb_multi


发起者:
```c
static void kvm_flush_tlb_multi(const struct cpumask *cpumask,
			const struct flush_tlb_info *info)
```

通过设置 kvm_steal_time::preempted

在 host 中的接受者:
```c
		trace_kvm_pv_tlb_flush(vcpu->vcpu_id,
				       st_preempted & KVM_VCPU_FLUSH_TLB);

		if (st_preempted & KVM_VCPU_FLUSH_TLB)
			kvm_vcpu_flush_tlb_guest(vcpu);
```

最终调用到 vmx_flush_tlb_guest 中，最终使用 invlpg 或者 invpcid 指令

## 简简单单看一个问题

似乎只要没有内存换出，都好说！

- https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/log/?qt=grep&q=CVE-2019-3016


### x86/kvm: Be careful not to clear KVM_VCPU_FLUSH_TLB bit


### x86/kvm: Introduce kvm_(un)map_gfn()

后面使用了 ../map-cache.md 的新技术替代了
具体的 commit 是 commit 357a18ad230f ("KVM: Kill kvm_map_gfn() / kvm_unmap_gfn() and gfn_to_pfn_cache")

在其中说明了 kvm_map_gfn 和 kvm_unmap_gfn 的问题

### x86/kvm: Cache gfn to pfn translation


### x86/KVM: Make sure KVM_VCPU_FLUSH_TLB flag is not missed

https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=b043138246a41064527cf019a3d51d9f015e9796

所有问题的根本:
```txt
commit b043138246a41064527cf019a3d51d9f015e9796
Author: Boris Ostrovsky <boris.ostrovsky@oracle.com>
Date:   Thu Dec 5 03:45:32 2019 +0000

    x86/KVM: Make sure KVM_VCPU_FLUSH_TLB flag is not missed

    There is a potential race in record_steal_time() between setting
    host-local vcpu->arch.st.steal.preempted to zero (i.e. clearing
    KVM_VCPU_PREEMPTED) and propagating this value to the guest with
    kvm_write_guest_cached(). Between those two events the guest may
    still see KVM_VCPU_PREEMPTED in its copy of kvm_steal_time, set
    KVM_VCPU_FLUSH_TLB and assume that hypervisor will do the right
    thing. Which it won't.

    Instad of copying, we should map kvm_steal_time and that will
    guarantee atomicity of accesses to @preempted.

    This is part of CVE-2019-3016.

    Signed-off-by: Boris Ostrovsky <boris.ostrovsky@oracle.com>
    Reviewed-by: Joao Martins <joao.m.martins@oracle.com>
    Cc: stable@vger.kernel.org
    Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

所以，需要在清零之前，首先来


### 后续的工作

```diff
History:        #0
Commit:         7e2175ebd695f17860c5bd4ad7616cce12ed4591
Author:         David Woodhouse <dwmw2@infradead.org>
Committer:      Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Wed 03 Nov 2021 01:36:39 AM CST
Committer Date: Thu 11 Nov 2021 11:56:19 PM CST

KVM: x86: Fix recording of guest steal time / preempted status

In commit b043138246a4 ("x86/KVM: Make sure KVM_VCPU_FLUSH_TLB flag is
not missed") we switched to using a gfn_to_pfn_cache for accessing the
guest steal time structure in order to allow for an atomic xchg of the
preempted field. This has a couple of problems.

Firstly, kvm_map_gfn() doesn't work at all for IOMEM pages when the
atomic flag is set, which it is in kvm_steal_time_set_preempted(). So a
guest vCPU using an IOMEM page for its steal time would never have its
preempted field set.

Secondly, the gfn_to_pfn_cache is not invalidated in all cases where it
should have been. There are two stages to the GFN->PFN conversion;
first the GFN is converted to a userspace HVA, and then that HVA is
looked up in the process page tables to find the underlying host PFN.
Correct invalidation of the latter would require being hooked up to the
MMU notifiers, but that doesn't happen---so it just keeps mapping and
unmapping the *wrong* PFN after the userspace page tables change.

In the !IOMEM case at least the stale page *is* pinned all the time it's
cached, so it won't be freed and reused by anyone else while still
receiving the steal time updates. The map/unmap dance only takes care
of the KVM administrivia such as marking the page dirty.

Until the gfn_to_pfn cache handles the remapping automatically by
integrating with the MMU notifiers, we might as well not get a
kernel mapping of it, and use the perfectly serviceable userspace HVA
that we already have.  We just need to implement the atomic xchg on
the userspace address with appropriate exception handling, which is
fairly trivial.

Cc: stable@vger.kernel.org
Fixes: b043138246a4 ("x86/KVM: Make sure KVM_VCPU_FLUSH_TLB flag is not missed")
Signed-off-by: David Woodhouse <dwmw@amazon.co.uk>
Message-Id: <3645b9b889dac6438394194bb5586a46b68d581f.camel@infradead.org>
[I didn't entirely agree with David's assessment of the
 usefulness of the gfn_to_pfn cache, and integrated the outcome
 of the discussion in the above commit message. - Paolo]
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```


```diff
History:        #0
Commit:         901d3765fa804ce42812f1d5b1f3de2dfbb26723
Author:         Paolo Bonzini <pbonzini@redhat.com>
Author Date:    Thu 04 Aug 2022 09:28:32 PM CST
Committer Date: Thu 11 Aug 2022 03:08:22 AM CST

KVM: x86: revalidate steal time cache if MSR value changes

Commit 7e2175ebd695 ("KVM: x86: Fix recording of guest steal time
/ preempted status", 2021-11-11) open coded the previous call to
kvm_map_gfn, but in doing so it dropped the comparison between the cached
guest physical address and the one in the MSR.  This cause an incorrect
cache hit if the guest modifies the steal time address while the memslots
remain the same.  This can happen with kexec, in which case the steal
time data is written at the address used by the old kernel instead of
the old one.

While at it, rename the variable from gfn to gpa since it is a plain
physical address and not a right-shifted one.

Reported-by: Dave Young <ruyang@redhat.com>
Reported-by: Xiaoying Yan  <yiyan@redhat.com>
Analyzed-by: Dr. David Alan Gilbert <dgilbert@redhat.com>
Cc: David Woodhouse <dwmw@amazon.co.uk>
Cc: stable@vger.kernel.org
Fixes: 7e2175ebd695 ("KVM: x86: Fix recording of guest steal time / preempted status")
Signed-off-by: Paolo Bonzini <pbonzini@redhat.com>
```

## 原来背后的工作这么多啊
- https://lore.kernel.org/all/20220909181351.23983-1-risbhat@amazon.com/
- https://lore.kernel.org/all/537fdcc6af80ba6285ae0cdecdb615face25426f.camel@infradead.org/T/#mb321136c9709790050da7e01091a390a540b93da

## [ ] pv tlb flush 的判断要求好多啊
```c
static bool pv_tlb_flush_supported(void)
{
	return (kvm_para_has_feature(KVM_FEATURE_PV_TLB_FLUSH) &&
		!kvm_para_has_hint(KVM_HINTS_REALTIME) &&
		kvm_para_has_feature(KVM_FEATURE_STEAL_TIME) &&
		!boot_cpu_has(X86_FEATURE_MWAIT) &&
		(num_possible_cpus() != 1));
}
```

逐个理解一下。

## [ ] 为什么这个问题主要影响 AMD
https://www.openwall.com/lists/oss-security/2020/01/30/4

## 为什么 steal 需要 version 的保护?

guest 中:
```c
	do {
		version = src->version;
		virt_rmb();
		steal = src->steal;
		virt_rmb();
	} while ((version & 1) || (version != src->version));
```

注意，host 中进行的是一个 加 操作，这个不是一个 atomic 的:
```c
	unsafe_get_user(steal, &st->steal, out);
	steal += current->sched_info.run_delay -
		vcpu->arch.st.last_steal;
	vcpu->arch.st.last_steal = current->sched_info.run_delay;
	unsafe_put_user(steal, &st->steal, out);
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
