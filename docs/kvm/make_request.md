## 使用 tracepoint 来跟踪 kvm_check_request
```diff
diff --git a/include/linux/kvm_host.h b/include/linux/kvm_host.h
index 401439bb21e3..5480fed9a495 100644
--- a/include/linux/kvm_host.h
+++ b/include/linux/kvm_host.h
@@ -2221,8 +2221,10 @@ static inline void __kvm_make_request(int req, struct kvm_vcpu *vcpu)
 	set_bit(req & KVM_REQUEST_MASK, (void *)&vcpu->requests);
 }

+void martins3(u64 req);
 static __always_inline void kvm_make_request(int req, struct kvm_vcpu *vcpu)
 {
+	martins3(req);
 	/*
 	 * Request that don't require vCPU action should never be logged in
 	 * vcpu->requests.  The vCPU won't clear the request, so it will stay
diff --git a/include/trace/events/kvm.h b/include/trace/events/kvm.h
index fc7d0f8ff078..8b904c00c157 100644
--- a/include/trace/events/kvm.h
+++ b/include/trace/events/kvm.h
@@ -473,6 +473,18 @@ TRACE_EVENT(kvm_dirty_ring_exit,
 	TP_printk("vcpu %d", __entry->vcpu_id)
 );

+	TRACE_EVENT(hi,
+
+		    TP_PROTO(u64 count),
+
+		    TP_ARGS(count),
+
+		    TP_STRUCT__entry(__field(u64, count)),
+
+		    TP_fast_assign(__entry->count = count;),
+
+		    TP_printk("hi : %llx ", __entry->count));
+
 TRACE_EVENT(kvm_unmap_hva_range,
 	TP_PROTO(unsigned long start, unsigned long end),
 	TP_ARGS(start, end),
diff --git a/virt/kvm/kvm_main.c b/virt/kvm/kvm_main.c
index de2c11dae231..5f2c7a105122 100644
--- a/virt/kvm/kvm_main.c
+++ b/virt/kvm/kvm_main.c
@@ -703,6 +703,12 @@ bool kvm_mmu_unmap_gfn_range(struct kvm *kvm, struct kvm_gfn_range *range)
 	return kvm_unmap_gfn_range(kvm, range);
 }

+
+void martins3(u64 req){
+	trace_hi(req);
+}
+EXPORT_SYMBOL_GPL(martins3);
+
 static int kvm_mmu_notifier_invalidate_range_start(struct mmu_notifier *mn,
 					const struct mmu_notifier_range *range)
 {
```
修改 kvm 之后，运行 kvm 1 即可。

sudo bpftrace -e 'kfunc:martins3 { @ = hist(arg->req) }'

虚拟机放这不动

req 的分布

这都是 hex 的输出:
```txt
  49.70%  hi : 10 # #define KVM_REQ_STEAL_UPDATE		KVM_ARCH_REQ(8)
  38.84%  hi : 2 #  KVM_REQ_UNBLOCK
   5.73%  hi : 300 # KVM_REQ_TLB_FLUSH
   5.73%  hi : 8 # #define KVM_REQ_MIGRATE_TIMER		KVM_ARCH_REQ(0) # 因为切换 CPU
```

```c
#define KVM_REQUEST_MASK           GENMASK(7,0)
#define KVM_REQUEST_NO_WAKEUP      BIT(8)
#define KVM_REQUEST_WAIT           BIT(9)
#define KVM_REQUEST_NO_ACTION      BIT(10)
/*
 * Architecture-independent vcpu->requests bit members
 * Bits 3-7 are reserved for more arch-independent bits.
 */
#define KVM_REQ_TLB_FLUSH		(0 | KVM_REQUEST_WAIT | KVM_REQUEST_NO_WAKEUP)
#define KVM_REQ_VM_DEAD			(1 | KVM_REQUEST_WAIT | KVM_REQUEST_NO_WAKEUP)
#define KVM_REQ_UNBLOCK			2
#define KVM_REQ_DIRTY_RING_SOFT_FULL	3
#define KVM_REQUEST_ARCH_BASE		8
```

KVM_REQ_LOAD_EOI_EXITMAP


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
