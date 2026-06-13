## 基本知识

- https://docs.kernel.org/admin-guide/mm/numa_memory_policy.html
- https://man7.org/linux/man-pages/man2/set_mempolicy.2.html : 这个也是需要自习分析

范围:
- System Default Policy
- Task/Process Policy
- VMA Policy
- [ ] Shared Policy : 完全没有看懂哇

- Default Mode–MPOL_DEFAULT


- [ ] mempolicy 是进程相关的，所以是 vm_area_struct 持有 vm_policy，而内核使用的内存是有自己的方法的。
  - 大错特错
  - [ ] 找到 vma 持有 vm_policy 的位置

## 内核分配内存真的是 interleaved 吗
- 还是说，只是启动的时候是 interleaved

- 如果用户进程访问文件:
  - 形成的 page cache 是 interleaved 吗? 但是如果该文件是 shared ?
  - 因为该文件形成了很多 inode 之类的，因此分配的空间在什么地方?

## set_mempolicy() or mbind() 两个系统调用的区别是什么

## 系统启动的时候，mempolicy 是 interleave 的，之后切换为 local allocation

- [ ] 什么时候切换的?

## vma_dup_policy : 当 frok 的时候，默认继承 parent 的 policy
```txt
#0  vma_dup_policy (src=src@entry=0xffff8881212902f8, dst=dst@entry=0xffff888121290390) at mm/mempolicy.c:2387
#1  0xffffffff812e4710 in __split_vma (mm=mm@entry=0xffff888121518000, vma=vma@entry=0xffff8881212902f8, addr=addr@entry=140295976648704, new_below=new_below@entry=0) at mm/mmap.c:2222
#2  0xffffffff812e4914 in do_mas_align_munmap (mas=mas@entry=0xffffc9000005fd28, vma=0xffff8881212902f8, mm=mm@entry=0xffff888121518000, start=start@entry=140295976648704, end=end@entry=140295976849408, uf=uf@entry=0xffffc9000005fd18, downgrade=false) at mm/mmap.c:2341
#3  0xffffffff812e4dc2 in do_mas_munmap (mas=mas@entry=0xffffc9000005fd28, mm=mm@entry=0xffff888121518000, start=start@entry=140295976648704, len=len@entry=200704, uf=uf@entry=0xffffc9000005fd18, downgrade=downgrade@entry=false) at mm/mmap.c:2502
#4  0xffffffff812e4ec3 in __vm_munmap (start=start@entry=140295976648704, len=len@entry=200704, downgrade=downgrade@entry=false) at mm/mmap.c:2775
#5  0xffffffff812e4f87 in vm_munmap (start=start@entry=140295976648704, len=len@entry=200704) at mm/mmap.c:2793
#6  0xffffffff813cb166 in elf_map (filep=filep@entry=0xffff888121740200, addr=addr@entry=0, eppnt=eppnt@entry=0xffff888121332000, prot=prot@entry=1, type=2, total_size=<optimized out>) at fs/binfmt_elf.c:392
#7  0xffffffff813cc8e8 in load_elf_interp (arch_state=<synthetic pointer>, interp_elf_phdata=0xffff888121332000, no_base=94798853218304, interpreter=<optimized out>, interp_elf_ex=0xffff8881617b1580) at fs/binfmt_elf.c:638
#8  load_elf_binary (bprm=0xffff888161c2b400) at fs/binfmt_elf.c:1250
#9  0xffffffff8136e43e in search_binary_handler (bprm=0xffff888161c2b400) at fs/exec.c:1727
#10 exec_binprm (bprm=0xffff888161c2b400) at fs/exec.c:1768
#11 bprm_execve (flags=<optimized out>, filename=<optimized out>, fd=<optimized out>, bprm=0xffff888161c2b400) at fs/exec.c:1837
#12 bprm_execve (bprm=0xffff888161c2b400, fd=<optimized out>, filename=<optimized out>, flags=<optimized out>) at fs/exec.c:1799
#13 0xffffffff8136ee1b in kernel_execve (kernel_filename=kernel_filename@entry=0xffffffff827b40bb "/sbin/init", argv=argv@entry=0xffffffff82a14220 <argv_init>, envp=envp@entry=0xffffffff82a14100 <envp_init>) at fs/exec.c:2002
#14 0xffffffff81f34748 in run_init_process (init_filename=init_filename@entry=0xffffffff827b40bb "/sbin/init") at init/main.c:1435
#15 0xffffffff81f34753 in try_to_run_init_process (init_filename=init_filename@entry=0xffffffff827b40bb "/sbin/init") at init/main.c:1442
#16 0xffffffff81fa8ae0 in kernel_init (unused=<optimized out>) at init/main.c:1575
#17 0xffffffff81001a72 in ret_from_fork () at arch/x86/entry/entry_64.S:306
#18 0x0000000000000000 in ?? ()
```
## vm_operations_struct 的两个 hook : set_policy get_policy

使用位置:
- vma_replace_policy

- syscall mbind
  - kernel_mbind
    - do_mbind
- syscall set_mempolicy_home_node

上面两者调用:
- mbind_range
  - vma_replace_policy

是为了处理 shared policy 的

## mempolicy

1. Memory policies are a programming interface that a NUMA-aware application can take advantage of.
2. cpusets which is an administrative mechanism for restricting the nodes from which memory may be allocated by a set of processes.  cpuset 和 numa mempolicy 同时出现的时候，cpuset 优先
3. 一共四个粒度 和 两个 flags MPOL_F_STATIC_NODES 和 MPOL_F_RELATIVE_NODES (**flag 的作用有点迷**)
4. 还分析了一下 mol_put 和 mol_get 的问题

```c
// 获取 vma 对应的 policy ，解析出来 preferred_nid 和 nodemask 然后
struct page * alloc_pages_vma(gfp_t gfp, int order, struct vm_area_struct *vma, unsigned long addr, int node, bool hugepage)
```
> 感觉 mempolicy 并没有什么特殊的地方，只是提供一个 syscall 给用户。

- vma_merge


## THP 的分配如何被 mempolicy 影响

## hugetlb 的分配如何被 mempolicy 影响
- [ ] 还记得其中的 hugetlb 中的 nr_mempolicy 吗?

### 大页的创建
- 各个 NUMA 不是对称的，hugepage 的分配的时候，如何处理大页
  - interleave 分配的，直到一个 node 上无法分配

### [ ] 大页的使用

## 分配内存的时候，如何被 mempolicy 影响的

### 用户进程

```txt
#0  get_task_policy (p=0xffff8880251c53c0) at mm/mempolicy.c:163
#1  alloc_pages (gfp=gfp@entry=4197824, order=order@entry=1) at mm/mempolicy.c:2273
#2  0xffffffff812f9ae8 in __get_free_pages (gfp_mask=gfp_mask@entry=4197824, order=order@entry=1) at mm/page_alloc.c:5605
#3  0xffffffff810f566d in _pgd_alloc () at arch/x86/mm/pgtable.c:414
#4  pgd_alloc (mm=mm@entry=0xffff888100b7bb80) at arch/x86/mm/pgtable.c:430
#5  0xffffffff81109537 in mm_alloc_pgd (mm=0xffff888100b7bb80) at kernel/fork.c:726
#6  mm_init (mm=mm@entry=0xffff888100b7bb80, p=p@entry=0xffff88802881a180, user_ns=<optimized out>) at kernel/fork.c:1145
#7  0xffffffff8110a671 in dup_mm (tsk=tsk@entry=0xffff88802881a180, oldmm=0xffff888122986600) at kernel/fork.c:1523
#8  0xffffffff8110c34c in copy_mm (tsk=0xffff88802881a180, clone_flags=18874368) at arch/x86/include/asm/current.h:15
#9  copy_process (pid=pid@entry=0x0 <fixed_percpu_data>, trace=trace@entry=0, node=node@entry=-1, args=args@entry=0xffffc90001f2beb0) at kernel/fork.c:2253
#10 0xffffffff8110c5b2 in kernel_clone (args=args@entry=0xffffc90001f2beb0) at kernel/fork.c:2671
#11 0xffffffff8110c9e6 in __do_sys_clone (clone_flags=<optimized out>, newsp=<optimized out>, parent_tidptr=<optimized out>, child_tidptr=<optimized out>, tls=<optimized out>) at kernel/fork.c:2812
#12 0xffffffff81fa4c4b in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001f2bf58) at arch/x86/entry/common.c:50
#13 do_syscall_64 (regs=0xffffc90001f2bf58, nr=<optimized out>) at arch/x86/entry/common.c:80
#14 0xffffffff8200009b in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#15 0x0000000000000000 in ?? ()
```

### 设备分配 DMA 空间
取决于设备所在的 NUMA node 的，可以验证一下。

### slab 分配
slab 有 cpu cache 和 numa cache 机制，验证一下。

## 验证一下，是首先检查 cpuset，然后检查 memory policy 的

## 这个回答很肤浅，而且并不是正确的
https://stackoverflow.com/questions/59607742/what-is-default-memory-policy-flag-for-malloc

- default_policy

```c
static struct mempolicy default_policy = {
    .refcnt = ATOMIC_INIT(1), /* never free it */
    .mode = MPOL_PREFERRED,
    .flags = MPOL_F_LOCAL,
};
```

```diff
History:        #0
Commit:         7858d7bca7fbbbbd5b940d2ec371b2d060b21b84
Author:         Feng Tang <feng.tang@intel.com>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Thu 01 Jul 2021 09:51:00 AM CST
Committer Date: Thu 01 Jul 2021 11:47:29 AM CST

mm/mempolicy: don't handle MPOL_LOCAL like a fake MPOL_PREFERRED policy

MPOL_LOCAL policy has been setup as a real policy, but it is still handled
like a faked POL_PREFERRED policy with one internal MPOL_F_LOCAL flag bit
set, and there are many places having to judge the real 'prefer' or the
'local' policy, which are quite confusing.

In current code, there are 4 cases that MPOL_LOCAL are used:

1. user specifies 'local' policy

2. user specifies 'prefer' policy, but with empty nodemask

3. system 'default' policy is used

4. 'prefer' policy + valid 'preferred' node with MPOL_F_STATIC_NODES
   flag set, and when it is 'rebind' to a nodemask which doesn't contains
   the 'preferred' node, it will perform as 'local' policy

So make 'local' a real policy instead of a fake 'prefer' one, and kill
MPOL_F_LOCAL bit, which can greatly reduce the confusion for code reading.

For case 4, the logic of mpol_rebind_preferred() is confusing, as Michal
Hocko pointed out:

: I do believe that rebinding preferred policy is just bogus and it should
: be dropped altogether on the ground that a preference is a mere hint from
: userspace where to start the allocation.  Unless I am missing something
: cpusets will be always authoritative for the final placement.  The
: preferred node just acts as a starting point and it should be really
: preserved when cpusets changes.  Otherwise we have a very subtle behavior
: corner cases.

So dump all the tricky transformation between 'prefer' and 'local', and
just record the new nodemask of rebinding.

[feng.tang@intel.com: fix a problem in mpol_set_nodemask(), per Michal Hocko]
  Link: https://lkml.kernel.org/r/1622560492-1294-3-git-send-email-feng.tang@intel.com
[feng.tang@intel.com: refine code and comments of mpol_set_nodemask(), per Michal]
  Link: https://lkml.kernel.org/r/20210603081807.GE56979@shbuild999.sh.intel.com

Link: https://lkml.kernel.org/r/1622469956-82897-3-git-send-email-feng.tang@intel.com
Signed-off-by: Feng Tang <feng.tang@intel.com>
Suggested-by: Michal Hocko <mhocko@suse.com>
Acked-by: Michal Hocko <mhocko@suse.com>
Cc: Andi Kleen <ak@linux.intel.com>
Cc: Andrea Arcangeli <aarcange@redhat.com>
Cc: Ben Widawsky <ben.widawsky@intel.com>
Cc: Dan Williams <dan.j.williams@intel.com>
Cc: Dave Hansen <dave.hansen@intel.com>
Cc: David Rientjes <rientjes@google.com>
Cc: Huang Ying <ying.huang@intel.com>
Cc: Mel Gorman <mgorman@techsingularity.net>
Cc: Michal Hocko <mhocko@kernel.org>
Cc: Mike Kravetz <mike.kravetz@oracle.com>
Cc: Randy Dunlap <rdunlap@infradead.org>
Cc: Vlastimil Babka <vbabka@suse.cz>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

## MPOL_PREFERRED_MANY
```diff
commit b27abaccf8e8b012f126da0c2a1ab32723ec8b9f
Author: Dave Hansen <dave.hansen@linux.intel.com>
Date:   Thu Sep 2 15:00:06 2021 -0700

    mm/mempolicy: add MPOL_PREFERRED_MANY for multiple preferred nodes

    Patch series "Introduce multi-preference mempolicy", v7.

    This patch series introduces the concept of the MPOL_PREFERRED_MANY
    mempolicy.  This mempolicy mode can be used with either the
    set_mempolicy(2) or mbind(2) interfaces.  Like the MPOL_PREFERRED
    interface, it allows an application to set a preference for nodes which
    will fulfil memory allocation requests.  Unlike the MPOL_PREFERRED mode,
    it takes a set of nodes.  Like the MPOL_BIND interface, it works over a
    set of nodes.  Unlike MPOL_BIND, it will not cause a SIGSEGV or invoke the
    OOM killer if those preferred nodes are not available.

    Along with these patches are patches for libnuma, numactl, numademo, and
    memhog.  They still need some polish, but can be found here:
    https://gitlab.com/bwidawsk/numactl/-/tree/prefer-many It allows new
    usage: `numactl -P 0,3,4`

    The goal of the new mode is to enable some use-cases when using tiered memory
    usage models which I've lovingly named.

    1a. The Hare - The interconnect is fast enough to meet bandwidth and
        latency requirements allowing preference to be given to all nodes with
        "fast" memory.
    1b. The Indiscriminate Hare - An application knows it wants fast
        memory (or perhaps slow memory), but doesn't care which node it runs
        on.  The application can prefer a set of nodes and then xpu bind to
        the local node (cpu, accelerator, etc).  This reverses the nodes are
        chosen today where the kernel attempts to use local memory to the CPU
        whenever possible.  This will attempt to use the local accelerator to
        the memory.
    2.  The Tortoise - The administrator (or the application itself) is
        aware it only needs slow memory, and so can prefer that.

    Much of this is almost achievable with the bind interface, but the bind
    interface suffers from an inability to fallback to another set of nodes if
    binding fails to all nodes in the nodemask.

    Like MPOL_BIND a nodemask is given. Inherently this removes ordering from the
    preference.

    > /* Set first two nodes as preferred in an 8 node system. */
    > const unsigned long nodes = 0x3
    > set_mempolicy(MPOL_PREFER_MANY, &nodes, 8);

    > /* Mimic interleave policy, but have fallback *.
    > const unsigned long nodes = 0xaa
    > set_mempolicy(MPOL_PREFER_MANY, &nodes, 8);

    Some internal discussion took place around the interface. There are two
    alternatives which we have discussed, plus one I stuck in:

    1. Ordered list of nodes.  Currently it's believed that the added
       complexity is nod needed for expected usecases.
    2. A flag for bind to allow falling back to other nodes.  This
       confuses the notion of binding and is less flexible than the current
       solution.
    3. Create flags or new modes that helps with some ordering.  This
       offers both a friendlier API as well as a solution for more customized
       usage.  It's unknown if it's worth the complexity to support this.
       Here is sample code for how this might work:

    > // Prefer specific nodes for some something wacky
    > set_mempolicy(MPOL_PREFER_MANY, 0x17c, 1024);
    >
    > // Default
    > set_mempolicy(MPOL_PREFER_MANY | MPOL_F_PREFER_ORDER_SOCKET, NULL, 0);
    > // which is the same as
    > set_mempolicy(MPOL_DEFAULT, NULL, 0);
    >
    > // The Hare
    > set_mempolicy(MPOL_PREFER_MANY | MPOL_F_PREFER_ORDER_TYPE, NULL, 0);
    >
    > // The Tortoise
    > set_mempolicy(MPOL_PREFER_MANY | MPOL_F_PREFER_ORDER_TYPE_REV, NULL, 0);
    >
    > // Prefer the fast memory of the first two sockets
    > set_mempolicy(MPOL_PREFER_MANY | MPOL_F_PREFER_ORDER_TYPE, -1, 2);
    >

    This patch (of 5):

    The NUMA APIs currently allow passing in a "preferred node" as a single
    bit set in a nodemask.  If more than one bit it set, bits after the first
    are ignored.

    This single node is generally OK for location-based NUMA where memory
    being allocated will eventually be operated on by a single CPU.  However,
    in systems with multiple memory types, folks want to target a *type* of
    memory instead of a location.  For instance, someone might want some
    high-bandwidth memory but do not care about the CPU next to which it is
    allocated.  Or, they want a cheap, high capacity allocation and want to
    target all NUMA nodes which have persistent memory in volatile mode.  In
    both of these cases, the application wants to target a *set* of nodes, but
    does not want strict MPOL_BIND behavior as that could lead to OOM killer
    or SIGSEGV.

    So add MPOL_PREFERRED_MANY policy to support the multiple preferred nodes
    requirement.  This is not a pie-in-the-sky dream for an API.  This was a
    response to a specific ask of more than one group at Intel.  Specifically:

    1. There are existing libraries that target memory types such as
       https://github.com/memkind/memkind.  These are known to suffer from
       SIGSEGV's when memory is low on targeted memory "kinds" that span more
       than one node.  The MCDRAM on a Xeon Phi in "Cluster on Die" mode is an
       example of this.

    2. Volatile-use persistent memory users want to have a memory policy
       which is targeted at either "cheap and slow" (PMEM) or "expensive and
       fast" (DRAM).  However, they do not want to experience allocation
       failures when the targeted type is unavailable.

    3. Allocate-then-run.  Generally, we let the process scheduler decide
       on which physical CPU to run a task.  That location provides a default
       allocation policy, and memory availability is not generally considered
       when placing tasks.  For situations where memory is valuable and
       constrained, some users want to allocate memory first, *then* allocate
       close compute resources to the allocation.  This is the reverse of the
       normal (CPU) model.  Accelerators such as GPUs that operate on
       core-mm-managed memory are interested in this model.

    A check is added in sanitize_mpol_flags() to not permit 'prefer_many'
    policy to be used for now, and will be removed in later patch after all
    implementations for 'prefer_many' are ready, as suggested by Michal Hocko.

    [mhocko@kernel.org: suggest to refine policy_node/policy_nodemask handling]

    Link: https://lkml.kernel.org/r/1627970362-61305-1-git-send-email-feng.tang@intel.com
    Link: https://lore.kernel.org/r/20200630212517.308045-4-ben.widawsky@intel.com
    Link: https://lkml.kernel.org/r/1627970362-61305-2-git-send-email-feng.tang@intel.com
    Co-developed-by: Ben Widawsky <ben.widawsky@intel.com>
    Signed-off-by: Ben Widawsky <ben.widawsky@intel.com>
    Signed-off-by: Dave Hansen <dave.hansen@linux.intel.com>
    Signed-off-by: Feng Tang <feng.tang@intel.com>
    Cc: Michal Hocko <mhocko@kernel.org>
    Acked-by: Michal Hocko <mhocko@suse.com>
    Cc: Andrea Arcangeli <aarcange@redhat.com>
    Cc: Mel Gorman <mgorman@techsingularity.net>
    Cc: Mike Kravetz <mike.kravetz@oracle.com>
    Cc: Randy Dunlap <rdunlap@infradead.org>
    Cc: Vlastimil Babka <vbabka@suse.cz>
    Cc: Andi Kleen <ak@linux.intel.com>
    Cc: Dan Williams <dan.j.williams@intel.com>
    Cc: Huang Ying <ying.huang@intel.com>b
    Cc: Michal Hocko <mhocko@suse.com>
    Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
    Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

## MPOL_F_NUMA_BALANCING 引入的 patch
```diff
History:        #0
Commit:         bda420b985054a3badafef23807c4b4fa38a3dff
Author:         Huang Ying <ying.huang@intel.com>
Committer:      Linus Torvalds <torvalds@linux-foundation.org>
Author Date:    Thu 25 Feb 2021 04:09:43 AM CST
Committer Date: Thu 25 Feb 2021 05:38:34 AM CST

numa balancing: migrate on fault among multiple bound nodes

Now, NUMA balancing can only optimize the page placement among the NUMA
nodes if the default memory policy is used.  Because the memory policy
specified explicitly should take precedence.  But this seems too strict in
some situations.  For example, on a system with 4 NUMA nodes, if the
memory of an application is bound to the node 0 and 1, NUMA balancing can
potentially migrate the pages between the node 0 and 1 to reduce
cross-node accessing without breaking the explicit memory binding policy.

So in this patch, we add MPOL_F_NUMA_BALANCING mode flag to
set_mempolicy() when mode is MPOL_BIND.  With the flag specified, NUMA
balancing will be enabled within the thread to optimize the page placement
within the constrains of the specified memory binding policy.  With the
newly added flag, the NUMA balancing control mechanism becomes,

 - sysctl knob numa_balancing can enable/disable the NUMA balancing
   globally.

 - even if sysctl numa_balancing is enabled, the NUMA balancing will be
   disabled for the memory areas or applications with the explicit
   memory policy by default.

 - MPOL_F_NUMA_BALANCING can be used to enable the NUMA balancing for
   the applications when specifying the explicit memory policy
   (MPOL_BIND).

Various page placement optimization based on the NUMA balancing can be
done with these flags.  As the first step, in this patch, if the memory of
the application is bound to multiple nodes (MPOL_BIND), and in the hint
page fault handler the accessing node are in the policy nodemask, the page
will be tried to be migrated to the accessing node to reduce the
cross-node accessing.

If the newly added MPOL_F_NUMA_BALANCING flag is specified by an
application on an old kernel version without its support, set_mempolicy()
will return -1 and errno will be set to EINVAL.  The application can use
this behavior to run on both old and new kernel versions.

And if the MPOL_F_NUMA_BALANCING flag is specified for the mode other than
MPOL_BIND, set_mempolicy() will return -1 and errno will be set to EINVAL
as before.  Because we don't support optimization based on the NUMA
balancing for these modes.

In the previous version of the patch, we tried to reuse MPOL_MF_LAZY for
mbind().  But that flag is tied to MPOL_MF_MOVE.*, so it seems not a good
API/ABI for the purpose of the patch.

And because it's not clear whether it's necessary to enable NUMA balancing
for a specific memory area inside an application, so we only add the flag
at the thread level (set_mempolicy()) instead of the memory area level
(mbind()).  We can do that when it become necessary.

To test the patch, we run a test case as follows on a 4-node machine with
192 GB memory (48 GB per node).

1. Change pmbench memory accessing benchmark to call set_mempolicy()
   to bind its memory to node 1 and 3 and enable NUMA balancing.  Some
   related code snippets are as follows,

     #include <numaif.h>
     #include <numa.h>

	struct bitmask *bmp;
	int ret;

	bmp = numa_parse_nodestring("1,3");
	ret = set_mempolicy(MPOL_BIND | MPOL_F_NUMA_BALANCING,
			    bmp->maskp, bmp->size + 1);
	/* If MPOL_F_NUMA_BALANCING isn't supported, fall back to MPOL_BIND */
	if (ret < 0 && errno == EINVAL)
		ret = set_mempolicy(MPOL_BIND, bmp->maskp, bmp->size + 1);
	if (ret < 0) {
		perror("Failed to call set_mempolicy");
		exit(-1);
	}

2. Run a memory eater on node 3 to use 40 GB memory before running pmbench.

3. Run pmbench with 64 processes, the working-set size of each process
   is 640 MB, so the total working-set size is 64 * 640 MB = 40 GB.  The
   CPU and the memory (as in step 1.) of all pmbench processes is bound
   to node 1 and 3. So, after CPU usage is balanced, some pmbench
   processes run on the CPUs of the node 3 will access the memory of
   the node 1.

4. After the pmbench processes run for 100 seconds, kill the memory
   eater.  Now it's possible for some pmbench processes to migrate
   their pages from node 1 to node 3 to reduce cross-node accessing.

Test results show that, with the patch, the pages can be migrated from
node 1 to node 3 after killing the memory eater, and the pmbench score
can increase about 17.5%.

Link: https://lkml.kernel.org/r/20210120061235.148637-2-ying.huang@intel.com
Signed-off-by: "Huang, Ying" <ying.huang@intel.com>
Acked-by: Mel Gorman <mgorman@suse.de>
Cc: Peter Zijlstra <peterz@infradead.org>
Cc: Ingo Molnar <mingo@redhat.com>
Cc: Rik van Riel <riel@surriel.com>
Cc: Johannes Weiner <hannes@cmpxchg.org>
Cc: "Matthew Wilcox (Oracle)" <willy@infradead.org>
Cc: Dave Hansen <dave.hansen@intel.com>
Cc: Andi Kleen <ak@linux.intel.com>
Cc: Michal Hocko <mhocko@suse.com>
Cc: David Rientjes <rientjes@google.com>
Signed-off-by: Andrew Morton <akpm@linux-foundation.org>
Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>
```

## TODO
- MPOL_F_NUMA_BALANCING : 对应的文档没有，可以深入理解之后，将这个补充一下
