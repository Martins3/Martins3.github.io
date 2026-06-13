## tracepoint 的积累已经很多了
Documentation/trace/tracepoint-analysis.rst
Documentation/trace/tracepoints.rst

核心实现文件: kernel/tracepoint.c

## 测试和读取
1. 测试一下内核中间各种类型的 context switch 的数量
  - irq
  - syscall
  - context switch 的数量


## kvm 为什么没有办法添加 trace 了
(2025-07-11 应该是当时理解有误导致的)

例如 handle_pause 中添加 trace_hi ，最后链接最后会有错误的

应该是依赖加错了，注意，kvm kvm_intel 依赖关系

trace 添加到这里是可以的

```txt
diff --git a/arch/x86/kvm/mmu/mmu.c b/arch/x86/kvm/mmu/mmu.c
index 7813d28b082f..3a1af7f81749 100644
--- a/arch/x86/kvm/mmu/mmu.c
+++ b/arch/x86/kvm/mmu/mmu.c
@@ -3203,6 +3203,7 @@ static int direct_map(struct kvm_vcpu *vcpu, struct kvm_page_fault *fault)
 	kvm_mmu_hugepage_adjust(vcpu, fault);

 	trace_kvm_mmu_spte_requested(fault);
+	trace_hi(fault->addr);
 	for_each_shadow_entry(vcpu, fault->addr, it) {
 		/*
 		 * We cannot overwrite existing page tables with an NX
diff --git a/arch/x86/kvm/mmu/mmutrace.h b/arch/x86/kvm/mmu/mmutrace.h
index 195d98bc8de8..c5bfeee8a0ca 100644
--- a/arch/x86/kvm/mmu/mmutrace.h
+++ b/arch/x86/kvm/mmu/mmutrace.h
@@ -366,6 +366,19 @@ TRACE_EVENT(
 	)
 );

+
+	TRACE_EVENT(hi,
+
+		    TP_PROTO(int count),
+
+		    TP_ARGS(count),
+
+		    TP_STRUCT__entry(__field(u32, count)),
+
+		    TP_fast_assign(__entry->count = count;),
+
+		    TP_printk("hi : %lu ", (unsigned long)__entry->count));
+
 TRACE_EVENT(
 	kvm_mmu_spte_requested,
 	TP_PROTO(struct kvm_page_fault *fault),
```

## 即便是不同的 trace point subsystem 不要用相同的名称，
他们不会有任何报错，没有任何警告， 容易搞混的



## 原来在 tracepoint 中可以直接输出 dumpstack 的

```txt
  4372.916 kworker/u129:2/2493613 bcachefs:journal_entry_close(dev: 8388608, str: "entry size: 1.27 KiB
[<0>] journal_write_work+0x8d/0xa0 [bcachefs]
[<0>] process_one_work+0x18f/0x3b0
[<0>] worker_thread+0x233/0x340
[<0>] kthread+0xcd/0x100
[<0>] ret_from_fork+0x31/0x50
[<0>] ret_from_fork_asm+0x1a/0x30
")
```

## tracepoint 如果使用 const char * 直接 printf 将会触发这个警告

为什么不去把这个警告变为静态检查的?
```txt
[ 4800.570816] simplefs: '/dev/loop0' mount success
[ 4828.368008] ------------[ cut here ]------------
[ 4828.368550] event 'simplefs_lookup' has unsafe pointer field 'd_name'
[ 4828.368562] WARNING: CPU: 0 PID: 6114 at kernel/trace/trace.c:3732 ignore_event+0x229/0x250
[ 4828.370235] Modules linked in: simplefs(O) loop ext4 mbcache jbd2 dm_mod kvm_amd sd_mod ccp sha1_generic input_leds psmouse atkbd vivaldi_fmap libps2 kvm ahci libahci libata virtio_console virtio_balloon 9pnet_virtio virtio_net virtio_scsi i8042 serio evdev sch_fq_codel rfkill rpcsec_gss_krb5 ip6table_mangle iptable_mangle ip6table_filter ip6_tables iptable_filter af_packet auth_rpcgss openvswitch nsh sg iptable_nat ip_tables x_tables nf_nat nf_conntrack nf_defrag_ipv6 nf_defrag_ipv4 nf_tables libcrc32c crc32c_intel 9p 9pnet nfsv4 dns_resolver nfs lockd grace sunrpc netfs configs configfs fuse nfnetlink virtio_pci virtio_pci_modern_dev virtio_pci_legacy_dev ipv6 [last unloaded: simplefs(O)]
[ 4828.376161] CPU: 0 UID: 0 PID: 6114 Comm: cat Tainted: G           O       6.13.0 #94
[ 4828.377016] Tainted: [O]=OOT_MODULE
[ 4828.377433] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2012-3-4
[ 4828.378213] RIP: 0010:ignore_event+0x229/0x250
[ 4828.378745] Code: 01 49 8b 74 24 f8 41 8b 44 24 38 90 f6 c4 02 75 0c a8 08 74 08 48 85 f6 74 03 48 8b 36 48 c7 c7 f0 4d 23 82 e8 d8 72 e8 ff 90 <0f> 0b 90 90 e9 20 ff ff ff 8b 3e 49 89 e8 4c 2b 46 ec 49 39 f8 0f
[ 4828.380631] RSP: 0018:ffffc900012b7d38 EFLAGS: 00010286
[ 4828.381257] RAX: 0000000000000000 RBX: ffff8880071a2d58 RCX: ffff88803e61ca88
[ 4828.382070] RDX: 0000000000000027 RSI: 0000000000000027 RDI: 0000000000000001
[ 4828.382890] RBP: ffff888006d98638 R08: 00000000ffffbfff R09: 0000000000000001
[ 4828.383651] R10: 00000000ffffbfff R11: ffff88807c2a0000 R12: ffffffffc0ded600
[ 4828.384485] R13: ffff88800347f014 R14: ffffffffc0ded7b0 R15: ffff888006128000
[ 4828.385302] FS:  00007f30e8069740(0000) GS:ffff88803e600000(0000) knlGS:0000000000000000
[ 4828.386221] CS:  0010 DS: 0000 ES: 0000 CR0: 0000000080050033
[ 4828.386909] CR2: 00007f30e8047000 CR3: 0000000015294000 CR4: 0000000000750ef0
[ 4828.387660] PKRU: 55555554
[ 4828.388053] Call Trace:
[ 4828.388399]  <TASK>
[ 4828.388683]  ? __warn+0x89/0x130
[ 4828.389141]  ? ignore_event+0x229/0x250
[ 4828.389592]  ? report_bug+0x164/0x190
[ 4828.390085]  ? handle_bug+0x54/0x90
[ 4828.390503]  ? exc_invalid_op+0x17/0x70
[ 4828.391002]  ? asm_exc_invalid_op+0x1a/0x20
[ 4828.391483]  ? ignore_event+0x229/0x250
[ 4828.391973]  ? ignore_event+0x228/0x250
[ 4828.392438]  trace_event_printf+0x58/0xc0
[ 4828.392952]  trace_raw_output_simplefs_lookup+0x3b/0x50 [simplefs]
[ 4828.393586]  tracing_read_pipe+0x185/0x360
[ 4828.394067]  vfs_read+0xe8/0x370
[ 4828.394444]  ? trace_hardirqs_on+0x21/0x80
[ 4828.394941]  ? handle_mm_fault+0xbc/0x300
[ 4828.395357]  ksys_read+0x6e/0xe0
[ 4828.395747]  do_syscall_64+0xbc/0x210
[ 4828.396190]  entry_SYSCALL_64_after_hwframe+0x77/0x7f
[ 4828.396686] RIP: 0033:0x7f30e8161ec1
[ 4828.397136] Code: 00 48 8b 15 59 2f 0d 00 f7 d8 64 89 02 b8 ff ff ff ff eb c3 e8 90 c4 01 00 f3 0f 1e fa 80 3d a5 b4 0d 00 00 74 13 31 c0 0f 05 <48> 3d 00 f0 ff ff 77 57 c3 66 0f 1f 44 00 00 48 83 ec 28 48 89 54
[ 4828.398891] RSP: 002b:00007ffdbdede6b8 EFLAGS: 00000246 ORIG_RAX: 0000000000000000
[ 4828.399610] RAX: ffffffffffffffda RBX: 0000000000020000 RCX: 00007f30e8161ec1
[ 4828.400365] RDX: 0000000000020000 RSI: 00007f30e8048000 RDI: 0000000000000003
[ 4828.401129] RBP: 0000000000020000 R08: 00000000ffffffff R09: 0000000000000000
[ 4828.401855] R10: 0000000000000022 R11: 0000000000000246 R12: 00007f30e8048000
[ 4828.402633] R13: 0000000000000003 R14: 0000000000000000 R15: 0000000000020000
[ 4828.403427]  </TASK>
[ 4828.403734] ---[ end trace 0000000000000000 ]---
[ 5564.681948] simplefs: unmounted disk
[ 5564.711921] simplefs: module unloaded
[ 5564.737096] simplefs: module loaded
[ 5564.743064] loop0: detected capacity change from 0 to 102400
[ 5564.743768] simplefs: '/dev/loop0' mount success
```

## enable 的写法
```c
__visible void __irq_entry smp_reschedule_interrupt(struct pt_regs *regs)
{
	ack_APIC_irq();
	inc_irq_stat(irq_resched_count);
	kvm_set_cpu_l1tf_flush_l1d();

	if (trace_resched_ipi_enabled()) {
		/*
		 * scheduler_ipi() might call irq_enter() as well, but
		 * nested calls are fine.
		 */
		irq_enter();
		trace_reschedule_entry(RESCHEDULE_VECTOR);
		scheduler_ipi();
		trace_reschedule_exit(RESCHEDULE_VECTOR);
		irq_exit();
		return;
	}
	scheduler_ipi();
}
```

更加有趣的例子是:

在 io_uring/io_uring.h 中有一个这个:
```c
static __always_inline bool io_fill_cqe_req(struct io_ring_ctx *ctx,
					    struct io_kiocb *req)
{
	struct io_uring_cqe *cqe;

	/*
	 * If we can't get a cq entry, userspace overflowed the
	 * submission (by quite a lot). Increment the overflow count in
	 * the ring.
	 */
	if (unlikely(!io_get_cqe(ctx, &cqe)))
		return false;


	memcpy(cqe, &req->cqe, sizeof(*cqe));
	if (ctx->flags & IORING_SETUP_CQE32) {
		memcpy(cqe->big_cqe, &req->big_cqe, sizeof(*cqe));
		memset(&req->big_cqe, 0, sizeof(req->big_cqe));
	}

	if (trace_io_uring_complete_enabled())
		trace_io_uring_complete(req->ctx, req, cqe);
	return true;
}
```
这里有两个问题:
1. 之前发现头文件中无法定义 tracepoint ，现在看来，只是操作不得当
2. 这里为什么需要首先判断 if (trace_io_uring_complete_enabled()) 然后去调用 trace_io_uring_complete(req->ctx, req, cqe);

### active 写法
```c
static inline void page_ref_dec(struct page *page)
{
	atomic_dec(&page->_refcount);
	if (page_ref_tracepoint_active(page_ref_mod))
		__page_ref_mod(page, -1);
}
```


## tracepoint 也需要对外 export symbol
<!-- 141af87c-7390-411b-a7a0-238e1ed09ece -->

EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_exit);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_fast_mmio);
EXPORT_TRACEPOINT_SYMBOL_GPL(kvm_inj_virq);

因为 kvm_intel 之类的使用了这些 tracepoint
```txt
arch/arm64/kvm/arm.c
1280:           trace_kvm_exit(ret, kvm_vcpu_trap_get_class(vcpu), *vcpu_pc(vcpu));

arch/powerpc/kvm/book3s_pr.c
1148:   trace_kvm_exit(exit_nr, vcpu);

arch/powerpc/kvm/booke.c
1052:   trace_kvm_exit(exit_nr, vcpu);

arch/x86/kvm/svm/svm.c
4531:   trace_kvm_exit(vcpu, KVM_ISA_SVM);
```

## DEFINE_EVENT 的定义的函数有四个地方

1. include/linux/tracepoint.h
2. include/trace/bpf_probe.h
3. include/trace/bpf_perf.h
4. include/trace/trace_events.h


## 如今可以使用 btf ，那么写这么复杂的 trace 代码真的有必要吗?

```c
/**
 * io_uring_queue_async_work - called before submitting a new async work
 *
 * @req:	pointer to a submitted request
 * @rw:		type of workqueue, hashed or normal
 *
 * Allows to trace asynchronous work submission.
 */
TRACE_EVENT(io_uring_queue_async_work,

	TP_PROTO(struct io_kiocb *req, int rw),

	TP_ARGS(req, rw),

	TP_STRUCT__entry (
		__field(  void *,			ctx		)
		__field(  void *,			req		)
		__field(  u64,				user_data	)
		__field(  u8,				opcode		)
		__field(  unsigned long long,		flags		)
		__field(  struct io_wq_work *,		work		)
		__field(  int,				rw		)

		__string( op_str, io_uring_get_opcode(req->opcode)	)
	),

	TP_fast_assign(
		__entry->ctx		= req->ctx;
		__entry->req		= req;
		__entry->user_data	= req->cqe.user_data;
		__entry->flags		= (__force unsigned long long) req->flags;
		__entry->opcode		= req->opcode;
		__entry->work		= &req->work;
		__entry->rw		= rw;

		__assign_str(op_str, io_uring_get_opcode(req->opcode));
	),

	TP_printk("ring %p, request %p, user_data 0x%llx, opcode %s, flags 0x%llx, %s queue, work %p",
		__entry->ctx, __entry->req, __entry->user_data,
		__get_str(op_str), __entry->flags,
		__entry->rw ? "hashed" : "normal", __entry->work)
);
```

1. tracepoint 可以插入到任何位置
2. tracepoint 性能更好


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
