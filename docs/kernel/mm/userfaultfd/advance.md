# userfaultfd 高级话题探讨
## codex 的分析
有，但要分场景。

userfaultfd 本身不是 zero-copy 数据通道，它是 fault 通知 + 一组解析 fault 的
ioctl。 常见的 UFFDIO_COPY 不是 zero-copy：实现里会分配 folio，然后
copy_from_user() 把 src 内容拷到目标页，见 mm/userfaultfd.c:241 和
mm/userfaultfd.c:276。

真正接近/属于 zero-copy 的路径有这些：

- UFFDIO_MOVE：是明确的 zero-copy remap。代码注释写明 move_pages() “remaps
  arbitrary anonymous pages atomically in zero copy”， 并且“works purely via
  pagetables”，见 mm/userfaultfd.c:1709。限制是只能用于非 shared anonymous
  pages，源页通常要 mapcount == 1，目标范围必须是 unmapped hole，源/目标 VMA
  protection 要一致。

- UFFDIO_CONTINUE：用于 minor fault，把已经存在于 page cache
  的页映射进去，不拷贝页内容；文档说它 “maps an existing, previously-populated
  page”， 见 Documentation/admin-guide/mm/userfaultfd.rst:150。

- UFFDIO_ZEROPAGE：不是搬运数据的 zero-copy，而是把 zero page 映射进去；如果
  mm_forbids_zeropage()，会退化为分配 zeroed folio，见 mm/ userfaultfd.c:349。

所以一句话：从任意 userspace buffer 填 faulting page，UFFDIO_COPY 会
copy；如果你能先把数据放在一个匿名页里，再用 UFFDIO_MOVE 把页表映射移动到 fault
地 址，那就是 userfaultfd 支持的 zero-copy 路径。

另外，这棵树的 UAPI 里没有 UFFDIO_ZEROCOPY 这个 ioctl，只有 UFFDIO_ZEROPAGE 和
UFFDIO_MOVE，见 include/uapi/linux/userfaultfd.h:50。文档里 QEMU 段落提 到的
UFFDIO_ZEROCOPY 看起来应按 UAPI 理解为 zero-page 相关描述，而不是一个真实
ioctl。

## 但是我们应该是去看这个，这是才是准确的
commit adef440691ba ("userfaultfd: UFFDIO_MOVE uABI")

(基本上可以猜到是这样实现的了，不然就太复杂了，但是不知道 google 拿着这个做什么用，
而且单个页面，真的会有性能提升吗?)
### codex 简单分析

UFFDIO_MOVE 的本质不是“把数据 copy 到 fault 地址”，而是把一个已有匿名页的页表映射从 src 搬到 dst。
所以它是 zero-copy：物理页不动，内核只改 PTE/PMD、rmap、anon_vma 等元数据。

UAPI 很小，在 include/uapi/linux/userfaultfd.h:359：

```c
struct uffdio_move {
    __u64 dst;
    __u64 src;
    __u64 len;
    __u64 mode;
    __s64 move;
};
```

典型用途是 userfaultfd handler 先把后端数据读到一个外部匿名 buffer，然后调用：

UFFDIO_MOVE(dst = fault_addr, src = buffer_addr, len = page_size)

内核把 buffer_addr 对应的物理页直接挂到 faulting address 上，buffer_addr 原位置变成 hole。这样比 UFFDIO_COPY 少一次内存 copy。

为什么要求这么严

核心限制在 mm/userfaultfd.c:1688 和 mm/userfaultfd.c:1918。

1. 只支持 anonymous、non-shared VMA

源码注释说得很直接：只允许 non-shared anonymous pages，因为这类页可以安全 relocation，不会在 rmap 里制造 non-linear anon_vma 问题，见 mm/
userfaultfd.c:1861。

文件页、shmem、memfd shared mapping 都不是单纯“某个进程私有的匿名页”。它们背后有 page cache、file offset、mapping/index 语义。把一个页从 file offset A
搬到 anonymous buffer 或另一个 offset，不只是改 PTE，还涉及 page cache 一致性，所以当前 UFFDIO_MOVE 不碰。

2. dst_vma 必须注册在当前 uffd ctx

当前语义是“用 move 来 resolve 一个 userfaultfd fault”。所以目标地址必须是被这个 uffd 管理的区域：

if (!dst_vma->vm_userfaultfd_ctx.ctx ||
    dst_vma->vm_userfaultfd_ctx.ctx != ctx)
    return -EINVAL;

见 mm/userfaultfd.c:1712。这就是为什么它天然是“外部 buffer -> 监听区”，不是反向。

3. 源和目标 VMA 权限必须一致

检查在 mm/userfaultfd.c:1692：VM_ACCESS_FLAGS 和 vm_page_prot 必须一样。

原因是 PTE 不是孤立数据。一个页从 src 挂到 dst 后，目标地址的读写/执行权限、dirty/accessed、write protect 语义必须和 VMA 匹配。权限不一致时直接搬 PTE
容易绕过 mprotect、W^X、uffd-wp 等语义。

4. 两个 VMA 的 mlock 状态必须一致

见 mm/userfaultfd.c:1697。如果一个 mlocked、一个不 mlocked，move 后页的 unevictable/accounting 语义会变复杂。当前实现选择拒绝，而不是在 move 里重做
mlock 迁移语义。

5. 当前只允许 writable VMA

见 mm/userfaultfd.c:1701。这主要是降低复杂度：UFFD fault resolution 通常目标是可写匿名内存，权限一致时只检查 source writable 即可。

6. 排除 PFNMAP/IO/HUGETLB/MIXEDMAP/SHADOW_STACK

见 mm/userfaultfd.c:1682。这些 VMA 的 PTE 背后不是普通可迁移匿名页，或者有特殊硬件/安全语义，不能按普通 anon folio 搬。

7. source 页必须独占

在 PTE 路径里，内核会检查 PageAnonExclusive()，见 mm/userfaultfd.c:1516。如果页被 fork/COW 共享，直接搬走会影响另一个映射的语义，所以返回 -EBUSY。注释
建议 source buffer 可以 MADV_DONTFORK，避免 fork 后 mapcount 变复杂。

基本实现流程

高层流程在 mm/userfaultfd.c:1918：

1. 检查 src/dst/len page aligned，不能 wrap。
2. 找到并锁住 dst_vma 和 src_vma。
3. 拿 ctx->map_changing_lock，避免和 uffd 的 non-cooperative events 并发。
4. 校验 shared、范围、权限、anonymous、uffd ctx。
5. 按 PMD/PTE 逐段处理：
    - source 没有 PMD/PTE：默认 -ENOENT，除非 ALLOW_SRC_HOLES。
    - destination 已经有映射：-EEXIST，防止覆盖已有页。
    - source 是 THP 且能整 PMD 搬：走 huge PMD move。
    - 否则 split THP 或逐 PTE 搬。

PTE 级核心在 mm/userfaultfd.c:1399：

1. 发起 mmu_notifier_invalidate_range_start()，通知 KVM/IOMMU/secondary MMU 这段映射要变。
2. 找到 src_pte 和 dst_pte。
3. 要求 dst_pte 是 none，否则 -EEXIST。
4. 要求 src_pte 存在，否则 -ENOENT 或按 hole 成功。
5. 如果 source 是 present page：
    - 找 folio；
    - 要求 anonymous exclusive；
    - lock folio；
    - 大 folio 必要时 split；
    - 调 move_present_ptes() 真正清 source、填 destination。

6. 如果 source 是 swap entry：
    - 要求 swap entry exclusive；
    - 处理 swapcache folio；
    - 调 move_swap_pte() 搬 swap PTE。

7. 结束时 mmu_notifier_invalidate_range_end()。

所以它的核心保证是：

- 不 copy page 内容。
- 不创建/销毁 VMA。
- source 原地址原子变空。
- destination 原子得到原来的页。
- 不允许覆盖 destination 已有映射。
- 不允许偷偷搬共享页。

这也解释了为什么做“反向 UFFDIO_MOVE”在匿名内存上自然：只要把“dst 必须 uffd 注册”换成“src 必须 uffd 注册”，大部分页表搬运逻辑能复用。但对你 slock 里的
memfd shared guest memory，这套 move_pages() 约束正好不适用。

### target 也不可以是 share memory

代码里有两层限制：

1. 在 mm/userfaultfd.c:1957：

if (dst_vma->vm_flags & VM_SHARED)
    goto out_unlock;

所以 MAP_SHARED 的 VMA 直接 -EINVAL。

2. 在 mm/userfaultfd.c:1717：

if (!vma_is_anonymous(src_vma) || !vma_is_anonymous(dst_vma))
    return -EINVAL;

所以 dst 还必须是 anonymous VMA。普通 file mapping、shmem、memfd MAP_SHARED 都不行。

它允许的目标端大致是：

- 当前 uffd ctx 注册过的 VMA；
- anonymous；
- 非 VM_SHARED，也就是 private anonymous；
- writable；
- 与 source 有相同访问权限和 vm_page_prot；
- 与 source mlock 状态一致；
- 目标 range 必须是空洞，已有 PTE 会返回 -EEXIST；
- 不能是 PFNMAP、IO、hugetlb、mixedmap、shadow stack 等特殊 VMA。

原因是 UFFDIO_MOVE 不是 copy，而是页表级“搬页”。shared memory 背后有 file/shmem page cache 和 offset 语义，不能把匿名页直接挂进去当成 shared file 页。

### 为什么 google 使用这个方案

(简单看了下，这个需求也就是经典的 swap 了)

原始引入提交是：

adef440691ba userfaultfd: UFFDIO_MOVE uABI
作者 Andrea Arcangeli，Suren Baghdasaryan 送入，2023-12-06，Andrew Morton 合入。提交说明里把需求和性能说得很直接。

为什么需要 UFFDIO_MOVE

它不是主要为 VM swap 设计的，原始动机是 heap compaction / GC compaction 这类场景。

以前 handler 处理 UFFD fault 主要靠 UFFDIO_COPY：

1. 用户态准备一页数据在 src buffer。
2. UFFDIO_COPY 让内核在 faulting dst 分配/映射一页。
3. 内核从 src memcpy 到 dst。
4. 用户态还要把 src 那页释放或回收，常见是 madvise()。

UFFDIO_MOVE 的目标是把第 2、3、4 步变成页表操作：

src anonymous page  --move PTE-->  dst uffd registered address
src becomes hole
dst gets the original physical page

所以如果用户态本来就有可复用的页，例如 compaction 时把对象移动到临时/空闲页里，再把这页挂回目标地址，UFFDIO_MOVE 可以省掉：

- destination page allocation；
- page-sized memcpy；
- source page madvise/release；
- 某些 VMA split，比如同一 VMA 内移动 swapped-out pages 时，原来只能靠 mremap()，但 mremap() 会拆 VMA。

源码注释也对应这个定位，见 mm/userfaultfd.c:1861：它说这是 pagetable-only、zero-copy 的 remap，适合处理 userspace page faults。

有没有性能提升

有，但条件很明确。

原始 patch message 给了两组判断：

- 如果应用“需要分配新页”，UFFDIO_COPY 反而大约 快 20%。也就是说，单纯从空开始分配并填充，MOVE 不一定赢。
- 如果用户态已经有可回收页，典型是 heap compaction，UFFDIO_MOVE 可以避免 allocation + memcpy + madvise。提交说明说，在 Google Pixel 6 上的 compaction
  benchmark 里，compacting thread completion time 相比 UFFDIO_COPY 减少超过 40%。

所以它的性能模型是：

UFFDIO_COPY = allocate dst page + memcpy(src -> dst) + later release src
UFFDIO_MOVE = move mapping metadata + invalidate TLB/secondary MMU

当页内容已经在一个匿名页里，MOVE 很划算；当你还得先准备/分配那页，收益可能消失，甚至更慢。

为什么这不直接适合 slock 的 memfd swap

你现在的内存是 memfd MAP_SHARED，而 UFFDIO_MOVE 当前要求 src/dst 都是 private anonymous，目标还必须是当前 uffd ctx 注册区。代码限制在 mm/
userfaultfd.c:1951 和 mm/userfaultfd.c:1717。

所以 UFFDIO_MOVE 的原始收益点是“匿名页零拷贝搬迁”，不是“shared memfd page cache drop/swapout”。对 slock 当前路径，更贴近的内核接口还是 UFFDIO_DROP/
REMOVE 或直接基于 fd offset 的 punch hole，而不是复用 UFFDIO_MOVE。

## 可以尝试做一个 UFFDIO_MOVE 的反向操作，但是我确定需要考虑点有那些

## UFFDIO_MOVE 机制

很遗憾，这个功能在 5.10 内核中还没有。

https://lwn.net/Articles/947123/ commit messages 说，之前一直都没有找到应用场景，
这很奇怪，这个东西为什么会没有使用场景：

猜测是，如果知道了这个空间需要内存，就算从网卡或者存储系统获取到数据填充到内存中。
这些内存要在内核中拷贝一次。所以，正常来说，应该是直接填充到页面中，然后 move 过去.

似乎主要的问题在于，move 过去之后，还需要从内核中分配内存，这个分配过程不会比拷贝少花时间。
看来内存拷贝没有想象的花费时间。

pr 引用这里数据: https://lore.kernel.org/all/1425575884-2574-1-git-send-email-aarcange@redhat.com/

> The UFFDIO_REMAP method is still present in the patchset but it's
> provided primarily to remove (add not) memory from the userfault
> range. The addition of the UFFDIO_REMAP method is intentionally kept
> at the end of the patchset. The postcopy live migration qemu code will
> only use UFFDIO_COPY and UFFDIO_ZEROPAGE. UFFDIO_REMAP isn't intended
> to be merged upstream in the short term, and it can be dropped later
> if there's an agreement it's a bad idea to keep it around in the
> patchset.
>
> David run some KVM postcopy live migration benchmarks on a 8-way CPU
> system and he measured that using UFFDIO_COPY instead of UFFDIO_REMAP
> resulted in a roughly a -20% reduction in latency which is good. The
> standard deviation error on the latency measurement decreased
> significantly as well (because the number of CPUs that required IPI
> delivery was variable, while the copy always takes roughly the same
> time). A bigger improvement is expectable if measured on a larger host
> with more CPUs.

## vhost-user 如何支持 pagefault 机制

https://www.qemu.org/docs/master/interop/vhost-user.html#migrating-backend-state

从这个函数看起:
vhost_user_postcopy_notifier

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
