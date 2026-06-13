# slub 调试记录
https://access.redhat.com/solutions/358933

https://serverfault.com/questions/1020241/debugging-kmalloc-64-slab-allocations-memory-leak

https://www.kernel.org/doc/html/latest/mm/slub.html

## perf
docs/kernel/mm/mm-slub-perf.sh


## 启动参数

- `F` Sanity checks checks basic details about slab objects, such as making sure the current amount of objects on a slab is not greater than the maximum amount of objects that slab can hold, or the current amount of used slab objects is not more than the amount of objects on the slab (and various other small checks). This options is known to mitigate the freelist pointer corruption issue in Red Hat Enterprise Linux 7 systems.
- `Z` Red zoning adds a small amount of internal fragmentation to slabs known as "Redzones". These zones have an unlikely value (defined in include/linux/poison.h) written to them and these values are checked for consistency. Useful to detect times where part of a slab is overwritten.
- `P` Posioning writes an unlikely "poison" value to a slab object on allocation or freeing a slab object. On allocation, the slab object is "poisoned" with this value until the object is actually stored in the slab. On free, the object is overwritten with the poison value. Useful to detect "use-after-free" situations.
- `U` User tracking stores the PID and PID's kernel stack when that PID allocates or frees a slab object in an internal structure. Requires a vmcore to view.
- Specific slabs can be targeted for enabling debugging by adding one or more slab cache names after the flags as a comma-separated list. This also takes wildcard (*) to match multiple slab caches.

```txt
slub_debug=F                    # Enables sanity checks for all slabs.
slub_debug=FZUP,dentry          # Enables noted debugging for just the dentry slab cache
slub_debug=U,kmalloc-*,filp     # Enables user tracking for all slab caches whose name begins with 'kmalloc' and the file pointer slab cache
```

额外推荐的参数：
```txt
no_hash_pointers log_buf_len=10M
```

slub_debug=U,kmalloc-* no_hash_pointers log_buf_len=10M

测试了一下， 打开之后，系统直接卡死，真的很绝望。

## 通过 /sys/kernel/slab/dentry/trace

以前， 可以通过打开关闭:
```txt
echo 1 | sudo tee /sys/kernel/slab/dentry/trace
```

```c
static ssize_t trace_show(struct kmem_cache *s, char *buf)
{
	return sysfs_emit(buf, "%d\n", !!(s->flags & SLAB_TRACE));
}
```


不过这个功能被 drop 掉了，只能从开机的时候打开
```diff
History:        #0
Commit:         060807f841ac94d3826ce6fa3b4f3831cd0c015b
Author:         Vlastimil Babka <vbabka@suse.cz>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Fri 07 Aug 2020 02:18:45 PM CST
Committer Date: Sat 08 Aug 2020 02:33:22 AM CST

mm, slub: make remaining slub_debug related attributes read-only

SLUB_DEBUG creates several files under /sys/kernel/slab/<cache>/ that can
be read to check if the respective debugging options are enabled for given
cache.  Some options, namely sanity_checks, trace, and failslab can be
also enabled and disabled at runtime by writing into the files.

The runtime toggling is racy.  Some options disable __CMPXCHG_DOUBLE when
enabled, which means that in case of concurrent allocations, some can
still use __CMPXCHG_DOUBLE and some not, leading to potential corruption.
The s->flags field is also not updated or checked atomically.  The
simplest solution is to remove the runtime toggling.  The extended
slub_debug boot parameter syntax introduced by earlier patch should allow
to fine-tune the debugging configuration during boot with same
granularity.

Signed-off-by: Vlastimil Babka <vbabka@suse.cz>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
Reviewed-by: Kees Cook <keescook@chromium.org>
Acked-by: Roman Gushchin <guro@fb.com>
Cc: Christoph Lameter <cl@linux.com>
Cc: Jann Horn <jannh@google.com>
Cc: Vijayanand Jitta <vjitta@codeaurora.org>
Cc: David Rientjes <rientjes@google.com>
Cc: Joonsoo Kim <iamjoonsoo.kim@lge.com>
Cc: Pekka Enberg <penberg@kernel.org>
Link: http://lkml.kernel.org/r/20200610163135.17364-5-vbabka@suse.cz
Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

slub_debug : https://www.kernel.org/doc/Documentation/vm/slub.txt


## slub_nomerge

The kernel often works to merge other slab caches into pre-existing kmalloc slab caches which can complicate troubleshooting. If necessary for troubleshooting, the merging behavior can be disabled with the kernel parameter `slub_nomerge`.

```txt
For example, the kmalloc-512 slab cache below has a number of other slab caches merged with it;

Raw
crash> tree -t rbtree  0xffff93e5537e0a20 -s kernfs_node -o kernfs_node.rb | grep -e ^fff -e name -e smylink -e target_kn | grep -B4 0xffff94449c7eb528 | grep name
  name = 0xffff9444abfc7ec0 "posix_timers_cache",
  name = 0xffff94449c7f2460 "sgpool-16",
  name = 0xffff94449c7f2160 "xfrm_dst_cache",
  name = 0xffff93e5593982a0 "khugepaged_mm_slot",
  name = 0xffff93e5537c3f00 "kmalloc-512",
  name = 0xffff94449c7f2220 "file_lock_cache",
  name = 0xffff93e5593981c0 "skbuff_fclone_cache",
```

没有关闭的时候:
```txt
 Active / Total Objects (% used)    : 220351 / 225206 (97.8%)
 Active / Total Slabs (% used)      : 6900 / 6900 (100.0%)
 Active / Total Caches (% used)     : 115 / 155 (74.2%)
 Active / Total Size (% used)       : 54184.28K / 55486.18K (97.7%)
 Minimum / Average / Maximum Object : 0.01K / 0.25K / 8.31K

  OBJS ACTIVE  USE OBJ SIZE  SLABS OBJ/SLAB CACHE SIZE NAME
 29016  29016 100%    0.10K    744       39      2976K buffer_head
 24870  24870 100%    0.13K    829       30      3316K kernfs_node_cache
 23961  23826  99%    0.19K   1141       21      4564K dentry
 14366  14366 100%    0.18K    653       22      2612K nfs_direct_cache
  9773   9773 100%    1.09K    337       29     10784K ext4_inode_cache
  8064   8064 100%    0.57K    288       28      4608K radix_tree_node
  7936   7790  98%    0.02K     31      256       124K kmalloc-16
  6834   6816  99%    0.04K     67      102       268K extent_status
  6144   6144 100%    0.01K     12      512        48K kmalloc-8
  5954   5954 100%    0.59K    229       26      3664K inode_cache
  5632   5481  97%    0.03K     44      128       176K kmalloc-32
  5548   5548 100%    0.05K     76       73       304K ftrace_event_field
  5264   5264 100%    0.07K     94       56       376K vmap_area
  5120   4858  94%    0.06K     80       64       320K kmalloc-64
  4998   4577  91%    0.04K     49      102       196K vma_lock
  4646   4483  96%    0.17K    202       23       808K vm_area_struct
  3990   3453  86%    0.09K     95       42       380K kmalloc-96
  3456   2950  85%    0.06K     54       64       216K kmalloc-cg-64
  2882   2816  97%    0.72K    131       22      2096K shmem_inode_cache
  2688   2688 100%    0.19K    128       21       512K kmalloc-192
  2320   2294  98%    0.50K    145       16      1160K kmalloc-512
  2144   2144 100%    0.25K    134       16       536K kmalloc-256
  2048   2048 100%    0.01K      4      512        16K kmalloc-cg-8
  2016   2016 100%    0.09K     48       42       192K trace_event_file
  1904   1053  55%    0.25K    119       16       476K maple_node
  1872   1621  86%    0.10K     48       39       192K anon_vma
  1472   1472 100%    0.06K     23       64        92K iommu_iova
  1296   1274  98%    1.00K     81       16      1296K kmalloc-1k
```
关闭之后
```txt
 Active / Total Objects (% used)    : 201313 / 205157 (98.1%)
 Active / Total Slabs (% used)      : 6255 / 6255 (100.0%)
 Active / Total Caches (% used)     : 162 / 227 (71.4%)
 Active / Total Size (% used)       : 46714.48K / 47648.04K (98.0%)
 Minimum / Average / Maximum Object : 0.01K / 0.23K / 8.31K

  OBJS ACTIVE  USE OBJ SIZE  SLABS OBJ/SLAB CACHE SIZE NAME
 26880  26880 100%    0.13K    896       30      3584K kernfs_node_cache
 22308  22308 100%    0.10K    572       39      2288K buffer_head
 17388  17340  99%    0.19K    828       21      3312K dentry
 14366  14366 100%    0.18K    653       22      2612K ext4_groupinfo_4k
  7424   7424 100%    0.02K     29      256       116K kmalloc-16
  7336   7336 100%    0.57K    262       28      4192K radix_tree_node
  6144   6140  99%    0.01K     12      512        48K kmalloc-8
  5632   5632 100%    0.03K     44      128       176K kmalloc-32
  5610   5186  92%    0.04K     55      102       220K vma_lock
  5460   5460 100%    0.59K    210       26      3360K inode_cache
  5336   4982  93%    0.17K    232       23       928K vm_area_struct
  5329   5329 100%    0.05K     73       73       292K ftrace_event_field
  5120   4753  92%    0.06K     80       64       320K kmalloc-64
  4756   4756 100%    1.09K    164       29      5248K ext4_inode_cache
  3864   3282  84%    0.09K     92       42       368K kmalloc-96
  3584   3584 100%    0.07K     64       56       256K vmap_area
  3468   3468 100%    0.04K     34      102       136K extent_status
  3264   3057  93%    0.06K     51       64       204K anon_vma_chain
```
的确是有区别的。

当使用 slub_nomerge 的时候，结果为:
ls -la /sys/kernel/slab/ 下软链接会消失。

## slub_nomerge 的基本作用
- find_mergeable : 构建 slab cache 的时候才有用

slabinfo -a，源码位于内核源码树下的 tools/mm/slabinfo.c

http://raverstern.site/en/posts/slab-merging/

其实就是多个 slab cache 公用一个

```txt
:0000024     <- avtab_node audit_buffer fsnotify_mark_connector hashtab_node
:0000032     <- ext4_io_end_vec extended_perms_data i915_lut_handle lsm_file_cache sw_flow_stats numa_policy
:0000040     <- khugepaged_mm_slot bio_crypt_ctx ext4_system_zone avtab_extended_perms nfs4_clnt_odstate
:0000048     <- shared_policy_node xfs_log_ticket i915_priolist ksm_mm_slot xfs_refc_intent xfs_ifork Acpi-Namespace avc_xperms_decision_node
:0000056     <- damon_region Acpi-Parse ftrace_event_field xfs_extfree_intent avc_xperms_node zswap_entry file_lock_ctx zspage-zswap1
:0000064     <- io fanotify_path_event ksm_stable_node iommu_iova jbd2_inode ebitmap_node ksm_rmap_item xfs_defer_pending dmaengine-unmap-2
:0000072     <- vmap_area nf_conncount_tuple fanotify_fid_event avc_node lsm_inode_cache drm_buddy_block Acpi-Operand xfs_bmap_intent
:0000080     <- kernfs_iattrs_cache Acpi-State nfsd_file_mark audit_tree_mark Acpi-ParseExt xfs_exchmaps_intent xfs_rmap_intent
:0000088     <- configfs_dir_cache blkdev_ioc xfs_attr_intent
:0000096     <- nf_conncount_rb trace_event_file
:0000128     <- nfsd_file iwl_cmd_pool:0000:00:14.3 xe_hw_fence active_node net_bridge_fdb_entry nfsd_cacherep btree_node backing_aio
:0000136     <- kvm_async_pf kernfs_node_cache
:0000160     <- fuse_request file_lease_cache
:0000176     <- xfs_bud_item xfs_iul_item xfs_xmd_item xfs_cud_item xfs_rud_item xfs_attrd_item
:0000184     <- nf-frags nfs4_ol_stateid ip6-frags xfs_icr
:0000192     <- aio_kiocb mfc_cache sdebug_queued_cmd uid_cache skbuff_ext_cache mfc6_cache bio_integrity_payload rtable inet_peer dmaengine-unmap-16 drm_sched_fence file_lock_cache
:0000208     <- xfs_bui_item xfs_attri_item nf_conntrack_expect
:0000216     <- xfs_refcbt_cur xfs_rtrefcountbt_cur xfs_inobt_cur
:0000232     <- xfs_trans xfs_bnobt_cur
:0000256     <- maple_node sgpool-8 key_jar biovec-16 rpc_tasks nvmet-bvec xe_sched_job
:0000280     <- xfs_rmapbt_cur nfs4_file
:0000320     <- xfrm_dst i915_vma_resource
:0000432     <- xfs_cui_item xfs_efi_item
:0000440     <- nfs4_delegation xfs_efd_item
:0000512     <- sgpool-16 skbuff_fclone_cache pool_workqueue
:0000640     <- i915_vma task_group dio
:0001024     <- sgpool-32 iommu_iova_magazine biovec-64
:0001328     <- nfs4_client perf_event
:0002048     <- rpc_buffers sgpool-64 biovec-128
:0004096     <- fgraph_stack sgpool-128 nvme-chap-buf-cache biovec-max
:A-0000032   <- io_buffer dnotify_struct
:a-0000032   <- pending_reservation jbd2_revoke_record_s
:A-0000040   <- pde_opener vma_lock
:A-0000048   <- fasync_cache ip_fib_trie
:a-0000056   <- jbd2_journal_handle mb_cache_entry ext4_free_data
:A-0000064   <- anon_vma_chain fs_cache eventpoll_pwq
:A-0000080   <- sigqueue dnotify_mark inotify_inode_mark fanotify_mark
:a-0000104   <- buffer_head ext4_fc_dentry_update
:A-0000128   <- eventpoll_epi pte_list_desc tcp_bind_bucket tcp_bind2_bucket fib6_node
:A-0000192   <- cred pid pid_2
:A-0000256   <- ip6_dst_cache
:a-0000256   <- jbd2_transaction_s
:A-0000256   <- task_delay_info
:a-0000256   <- dquot
:A-0001024   <- UNIX UNIX-STREAM PING
:A-0001152   <- signal_cache UDP UDP-Lite
:A-0001344   <- UDPv6 UDPLITEv6
```
原来只是如此而已。

## 另外一个问题

构建一个  bpftrace 类似的
./trace -I 'linux/slab.h' -I 'linux/slub_def.h' \
'kmem_cache_alloc(struct kmem_cache *s, gfp_t gfpflags) (STRCMP("task_delay_info", s->name))'

用这个观察，所有的 cache 的分配的都是谁。


## 经典
https://access.redhat.com/solutions/5375971

## 为什么 cpu_slabs 会搞过限制 cpu_partial

```txt
[root@bogon kmalloc-512]# cat cpu_slabs
86 N0=86
[root@bogon kmalloc-512]# cat cpu_partial
52
```

使用这个命令:
```sh
sudo bpftrace -e 'kfunc:vmlinux:cpu_partial_show { printf("%d\n", args->s->cpu_partial_slabs); }'
```
输出的就是 4

补充的就是两个地方:
- __slab_free
- ____slab_alloc  -> get_partial_node -> put_cpu_partial

好吧，是我智障了，cpu_partial 显示的是一个 numa 节点的，是这个 numa 节点所有的 CPU 的
partial 之和。如果想要看每一个 CPU 有多少个 partial ，需要看 slab_cpu_pa

在 32 core 机器上，可以看到 slabs_cpu_partial 记录为 65 个 cpu partial slab + 32 ，正好是 cpu_slabs 97
```txt
[root@bogon kmalloc-512]# cat slabs_cpu_partial
1040(65) C0=32(2) C1=64(4) C2=16(1) C3=64(4) C4=32(2) C5=16(1) C6=32(2) C7=16(1) C8=48(3) C9=16(1) C10=32(2) C11=32(2) C12=32(2) C13=64(4) C14=16(1) C15=16(1) C16=32(2) C17=16(1) C18=32(2) C19=16(1) C20=64(4) C21=32(2) C22=48(3) C23=48(3) C24=16(1) C25=64(4) C26=32(2) C27=32(2) C29=16(1) C30=48(3) C31=16(1)
[root@bogon kmalloc-512]# cat cpu_slabs
97 N0=97
```

## commit 9198ffbd2b49 ("mm/slub: Reduce memory consumption in extreme scenarios")
这个是之前没有理解的一个代码，当时感觉到两个事情非常奇怪:
1. 为什么 slub 会不先清理本地的 page cache ，而是会直接去另外的 numa node 中找一个 slab
2. 为什么不去直接用其他的 node 中的 partial ，而是会分配一个完整的 slab 出来

> [!NOTE]
> 参考神奇海螺的意见，有待验证

(认为基本上是对的，但是我没时间核查了)

第一个问题的回答:
  - get_partial() / get_any_partial() 是 SLUB 自己的“对象级复用”逻辑，成本很低，只是在已有 slab 里找空对象。
  - “释放 page cache”属于更下面 page allocator / reclaim 的事情，成本高很多，可能触发 direct reclaim、writeback、stall，甚至不保证马上在目标 node 上拿到可用页。

第二个问题的回答:

  旧路径实际上是：

  1. 先找目标 node 的 partial slab
  2. 没找到就结束 partial 搜索
  3. 直接进入 new_slab(s, gfpflags, node)
  4. new_slab() 再去 page allocator 要页
  5. 因为 gfpflags 没有 __GFP_THISNODE，page allocator 可以 fallback 到别的 node 分配页
  6. 结果就是“在别的 node 上新建一个完整 slab”

  也就是说，旧实现不是“故意更喜欢新建完整 slab，而不喜欢远端 partial”；而是代码结构上把“远端 partial 搜索”这条路直接跳过去了。

  这正是 9198ffbd2b49 修的点：

  - 先仍然只看目标 node partial
  - 如果失败，先试一次只在目标 node 新建 slab
  - 再失败，才允许回到原始 gfpflags
  - 这时 get_partial() 才会继续走到 get_any_partial()，于是会去其他 node 找 partial，而不是立刻新建远端 slab


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
