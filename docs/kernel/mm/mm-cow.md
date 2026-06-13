# Copy-On-Write
- [ ]  https://dirtycow.ninja/
  - [ ] https://chao-tic.github.io/blog/2017/05/24/dirty-cow : and this one
- [ ] check the code related with copying page table when cow


- [ ] do_swap_page is used for read page from anonymous vma, check it's usage

不是太能理解，为什么在 do_swap_page 中存在这个?
```c
	if (vmf->flags & FAULT_FLAG_WRITE) {
		ret |= do_wp_page(vmf);
		if (ret & VM_FAULT_ERROR)
			ret &= VM_FAULT_ERROR;
		goto out;
	}
```
- at first glance, it's unreasonable, but what if a shared page is swapped out and a process it's trying to write to it.
  - 确定吗? 那么为什么不是首先 check 一下 FAULT_FLAG_WRITE 和 vma 的权限，而是在这里无条件的 do_wp_page 啊

```plain
handle_pte_fault ==>
                    ==> do_wp_page ==> wp_page_copy
do_swap_page     ==>
```

- [ ] do_wp_page
  - [ ] why we should check `PageAnon(vmf->page)` especially
  - [ ] `return VM_FAULT_WRITE;` check why it need return value
  - [ ] `(unlikely((vma->vm_flags & (VM_WRITE|VM_SHARED)) == (VM_WRITE|VM_SHARED))` if a write protection fault can be triggered on the writable page.


```c
static inline bool is_cow_mapping(vm_flags_t flags)
{
  return (flags & (VM_SHARED | VM_MAYWRITE)) == VM_MAYWRITE;
}
```
不是共享的，但是可以修改为写的

[Memory region flags in Linux: why both VM_WRITE and VM_MAYWRITE are needed?](https://stackoverflow.com/questions/48241187/memory-region-flags-in-linux-why-both-vm-write-and-vm-maywrite-are-needed) 解释了为什么需要 VM_MAYWRITE 和 VM_WRITE 两个 flag 的，
一般来说，因为有的 vma 是可以通过 mprotect 修改是否为 readonly 的，而有的 vma 不可以，例如映射的是一个文件。

看 `do_mmap` 的实现:

默认是含有 VM_MAYWRITE 的
```c
	vm_flags |= calc_vm_prot_bits(prot, pkey) | calc_vm_flag_bits(flags) |
			mm->def_flags | VM_MAYREAD | VM_MAYWRITE | VM_MAYEXEC;
```
特殊情况下才会去掉:
```c
			if (!(file->f_mode & FMODE_WRITE))
				vm_flags &= ~(VM_MAYWRITE | VM_SHARED);
			fallthrough;
```

This is a interesting question, if we want to protect a page being written by cow,
- if the page is writable, so we should clear the writable flags of it ?
- but if the page is not writable, so we should fail the cow page fault ?

[内存在父子进程间的共享时间及范围](https://www.cnblogs.com/tsecer/p/10487840.html)

> mmap 时它 mmap 的是私有的，这一点就导致 is_cow_mapping 中 VM_SHARED 是没有置位，因此函数返回值为 true；对于代码段中的空间，它的 VM_SHARED 是满足的，所以函数返回 false，进而导致父进程和子进程直接共享页面，不会设置 COW 属性。

### cow pte 的产生

当进行 fork/clone/vfork 之类的系统调用的时候，其处理内存拷贝的过程大致如下:


注意 ``copy_one_pte`` 其中的一段:

```c
    /*
     * If it's a COW mapping, write protect it both
     * in the parent and the child
     */
    if (is_cow_mapping(vm_flags) && pte_write(pte)) {
        ptep_set_wrprotect(src_mm, addr, src_pte);
        pte = pte_wrprotect(pte);
    }
```

此时，如果 page fault 的过程发现 pte 的 flags 是 write proect 的，但是 vma 是 is_cow_mapping ，
那么就会发生 copy on write 。

### cow page 的产生

在 ``handle_pte_fault`` 中间，当检查到 vmf->flags 中间存在 FAULT_FLAG_WRITE，表示该 page fault 需要进行写操作，但是，pte 权限并不存在写权限，那么就会进行 ``do_wp_page`` 来创建 cow page。

```c
    if (vmf->flags & FAULT_FLAG_WRITE) {
        if (!pte_write(entry))
            return do_wp_page(vmf);
        entry = pte_mkdirty(entry);
    }
```

- [ ]  这个 page 现在只读了，导致 parent 写和 child 都会产生一个新的 page ，那么原来的 page 咋办 ?


### Dirty Cow
https://github.com/dirtycow/dirtycow.github.io/blob/master/dirtyc0w.c

1. 主进程将一个以只读的方式将目标文件 mmap 到自己的进程地址空间。
2. 创建两个 thread，第一个循环调用 madvise，叫做 madviseThread，而另一个则是打开 /proc/self/mmap，对于目标文件映射到的虚拟地址空间进行写操作，叫做 procselfmemThread

通过查询 man proc(5)

```txt

   /proc/[pid]/mem
          This file can be used to access the pages of a process's memory through open(2), read(2), and lseek(2).

          Permission to access this file is governed by a ptrace access mode PTRACE_MODE_ATTACH_FSCREDS check; see ptrace(2).
```

检查其权限:

```txt
  $  ~ l /proc/self/mem
  .rw------- shen shen 0 B Sun Jun  7 07:23:01 2020 mem
```

```c
  static const struct file_operations proc_mem_operations = {
    .llseek     = mem_lseek,
    .read       = mem_read,
    .write      = mem_write,
    .open       = mem_open,
    .release    = mem_release,
  };
```

显然， ``mem_write`` 就是对于 /proc/self/mem 进行写的实际调用对象。

```c
  static ssize_t mem_write(struct file *file, const char __user *buf,
         size_t count, loff_t *ppos)
  {
    return mem_rw(file, (char __user*)buf, count, ppos, 1);
  }
```

- mem_rw
  - copy_from_user
  - access_remote_vm
    - `__access_remote_vm`
      - get_user_page_vma_remote
        - get_user_page_vma_remote
          - __get_user_pages_locked
            - __get_user_pages
              - follow_page_mask


1. `__get_user_pages` 中，如果 faultin_page 的返回值为 0 ，那么将会通过 retry 重新获取到 page 。
```c
retry:
		/*
		 * If we have a pending SIGKILL, don't keep faulting pages and
		 * potentially allocating memory.
		 */
		if (fatal_signal_pending(current)) {
			ret = -EINTR;
			goto out;
		}
		cond_resched();

		page = follow_page_mask(vma, start, foll_flags, &ctx);
		if (!page || PTR_ERR(page) == -EMLINK) {
			ret = faultin_page(vma, start, &foll_flags,
					   PTR_ERR(page) == -EMLINK, locked);
			switch (ret) {
			case 0:
				goto retry;
```

- [ ] 这里有点不懂，为什么 faultin_page 成功之后，不直接向上返回 page ，而是重新 follow_page_mask 一次


2. faultin_page 中，如果发现 handle_mm_fault 返回值中含有 VM_FAULT_WRITE，说明创建了 page tale
```c
	  ret = handle_mm_fault(vma, address, fault_flags);

    // ...

    /*
     * The VM_FAULT_WRITE bit tells us that do_wp_page has broken COW when
     * necessary, even if maybe_mkwrite decided not to set pte_write. We
     * can thus safely do subsequent page lookups as if they were reads.
     * But only do so when looping for pte_write is futile: in some cases
     * userspace may also be wanting to write to the gotten user page,
     * which a read fault here might prevent (a readonly page might get
     * reCOWed by userspace write).
     */
    if ((ret & VM_FAULT_WRITE) && !(vma->vm_flags & VM_WRITE))
        *flags &= ~FOLL_WRITE;
```


- [ ] 为什么第一个方法可以修复?

## 修复
### 第一个 19be0eaffa3ac7d8eb6784ad9bdbc7d67ed8e619
```diff
commit 19be0eaffa3ac7d8eb6784ad9bdbc7d67ed8e619
Author: Linus Torvalds <torvalds@linux-foundation.org>
Date:   Thu Oct 13 13:07:36 2016 -0700

    mm: remove gup_flags FOLL_WRITE games from __get_user_pages()

    This is an ancient bug that was actually attempted to be fixed once
    (badly) by me eleven years ago in commit 4ceb5db9757a ("Fix
    get_user_pages() race for write access") but that was then undone due to
    problems on s390 by commit f33ea7f404e5 ("fix get_user_pages bug").

    In the meantime, the s390 situation has long been fixed, and we can now
    fix it by checking the pte_dirty() bit properly (and do it better).  The
    s390 dirty bit was implemented in abf09bed3cce ("s390/mm: implement
    software dirty bits") which made it into v3.9.  Earlier kernels will
    have to look at the page state itself.

    Also, the VM has become more scalable, and what used a purely
    theoretical race back then has become easier to trigger.

    To fix it, we introduce a new internal FOLL_COW flag to mark the "yes,
    we already did a COW" rather than play racy games with FOLL_WRITE that
    is very fundamental, and then use the pte dirty flag to validate that
    the FOLL_COW flag is still valid.

    Reported-and-tested-by: Phil "not Paul" Oester <kernel@linuxace.com>
    Acked-by: Hugh Dickins <hughd@google.com>
    Reviewed-by: Michal Hocko <mhocko@suse.com>
    Cc: Andy Lutomirski <luto@kernel.org>
    Cc: Kees Cook <keescook@chromium.org>
    Cc: Oleg Nesterov <oleg@redhat.com>
    Cc: Willy Tarreau <w@1wt.eu>
    Cc: Nick Piggin <npiggin@gmail.com>
    Cc: Greg Thelen <gthelen@google.com>
    Cc: stable@vger.kernel.org
    Signed-off-by: Linus Torvalds <torvalds@linux-foundation.org>

diff --git a/include/linux/mm.h b/include/linux/mm.h
index e9caec6a51e9..ed85879f47f5 100644
--- a/include/linux/mm.h
+++ b/include/linux/mm.h
@@ -2232,6 +2232,7 @@ static inline struct page *follow_page(struct vm_area_struct *vma,
 #define FOLL_TRIED	0x800	/* a retry, previous pass started an IO */
 #define FOLL_MLOCK	0x1000	/* lock present pages */
 #define FOLL_REMOTE	0x2000	/* we are working on non-current tsk/mm */
+#define FOLL_COW	0x4000	/* internal GUP flag */

 typedef int (*pte_fn_t)(pte_t *pte, pgtable_t token, unsigned long addr,
 			void *data);
diff --git a/mm/gup.c b/mm/gup.c
index 96b2b2fd0fbd..22cc22e7432f 100644
--- a/mm/gup.c
+++ b/mm/gup.c
@@ -60,6 +60,16 @@ static int follow_pfn_pte(struct vm_area_struct *vma, unsigned long address,
 	return -EEXIST;
 }

+/*
+ * FOLL_FORCE can write to even unwritable pte's, but only
+ * after we've gone through a COW cycle and they are dirty.
+ */
+static inline bool can_follow_write_pte(pte_t pte, unsigned int flags)
+{
+	return pte_write(pte) ||
+		((flags & FOLL_FORCE) && (flags & FOLL_COW) && pte_dirty(pte));
+}
+
 static struct page *follow_page_pte(struct vm_area_struct *vma,
 		unsigned long address, pmd_t *pmd, unsigned int flags)
 {
@@ -95,7 +105,7 @@ static struct page *follow_page_pte(struct vm_area_struct *vma,
 	}
 	if ((flags & FOLL_NUMA) && pte_protnone(pte))
 		goto no_page;
-	if ((flags & FOLL_WRITE) && !pte_write(pte)) {
+	if ((flags & FOLL_WRITE) && !can_follow_write_pte(pte, flags)) {
 		pte_unmap_unlock(ptep, ptl);
 		return NULL;
 	}
@@ -412,7 +422,7 @@ static int faultin_page(struct task_struct *tsk, struct vm_area_struct *vma,
 	 * reCOWed by userspace write).
 	 */
 	if ((ret & VM_FAULT_WRITE) && !(vma->vm_flags & VM_WRITE))
-		*flags &= ~FOLL_WRITE;
+	        *flags |= FOLL_COW;
 	return 0;
 }
```

### 第二个 5535be3099717646781ce1540cf725965d680e7b

```diff
commit 5535be3099717646781ce1540cf725965d680e7b
Author: David Hildenbrand <david@redhat.com>
Date:   Tue Aug 9 22:56:40 2022 +0200

    mm/gup: fix FOLL_FORCE COW security issue and remove FOLL_COW

    Ever since the Dirty COW (CVE-2016-5195) security issue happened, we know
    that FOLL_FORCE can be possibly dangerous, especially if there are races
    that can be exploited by user space.

    Right now, it would be sufficient to have some code that sets a PTE of a
    R/O-mapped shared page dirty, in order for it to erroneously become
    writable by FOLL_FORCE.  The implications of setting a write-protected PTE
    dirty might not be immediately obvious to everyone.

    And in fact ever since commit 9ae0f87d009c ("mm/shmem: unconditionally set
    pte dirty in mfill_atomic_install_pte"), we can use UFFDIO_CONTINUE to map
    a shmem page R/O while marking the pte dirty.  This can be used by
    unprivileged user space to modify tmpfs/shmem file content even if the
    user does not have write permissions to the file, and to bypass memfd
    write sealing -- Dirty COW restricted to tmpfs/shmem (CVE-2022-2590).

    To fix such security issues for good, the insight is that we really only
    need that fancy retry logic (FOLL_COW) for COW mappings that are not
    writable (!VM_WRITE).  And in a COW mapping, we really only broke COW if
    we have an exclusive anonymous page mapped.  If we have something else
    mapped, or the mapped anonymous page might be shared (!PageAnonExclusive),
    we have to trigger a write fault to break COW.  If we don't find an
    exclusive anonymous page when we retry, we have to trigger COW breaking
    once again because something intervened.

    Let's move away from this mandatory-retry + dirty handling and rely on our
    PageAnonExclusive() flag for making a similar decision, to use the same
    COW logic as in other kernel parts here as well.  In case we stumble over
    a PTE in a COW mapping that does not map an exclusive anonymous page, COW
    was not properly broken and we have to trigger a fake write-fault to break
    COW.

    Just like we do in can_change_pte_writable() added via commit 64fe24a3e05e
    ("mm/mprotect: try avoiding write faults for exclusive anonymous pages
    when changing protection") and commit 76aefad628aa ("mm/mprotect: fix
    soft-dirty check in can_change_pte_writable()"), take care of softdirty
    and uffd-wp manually.

    For example, a write() via /proc/self/mem to a uffd-wp-protected range has
    to fail instead of silently granting write access and bypassing the
    userspace fault handler.  Note that FOLL_FORCE is not only used for debug
    access, but also triggered by applications without debug intentions, for
    example, when pinning pages via RDMA.

    This fixes CVE-2022-2590. Note that only x86_64 and aarch64 are
    affected, because only those support CONFIG_HAVE_ARCH_USERFAULTFD_MINOR.

    Fortunately, FOLL_COW is no longer required to handle FOLL_FORCE. So
    let's just get rid of it.

    Thanks to Nadav Amit for pointing out that the pte_dirty() check in
    FOLL_FORCE code is problematic and might be exploitable.

    Note 1: We don't check for the PTE being dirty because it doesn't matter
            for making a "was COWed" decision anymore, and whoever modifies the
            page has to set the page dirty either way.

    Note 2: Kernels before extended uffd-wp support and before
            PageAnonExclusive (< 5.19) can simply revert the problematic
            commit instead and be safe regarding UFFDIO_CONTINUE. A backport to
            v5.19 requires minor adjustments due to lack of
            vma_soft_dirty_enabled().

    Link: https://lkml.kernel.org/r/20220809205640.70916-1-david@redhat.com
    Fixes: 9ae0f87d009c ("mm/shmem: unconditionally set pte dirty in mfill_atomic_install_pte")
    Signed-off-by: David Hildenbrand <david@redhat.com>
    Cc: Greg Kroah-Hartman <gregkh@linuxfoundation.org>
    Cc: Axel Rasmussen <axelrasmussen@google.com>
    Cc: Nadav Amit <nadav.amit@gmail.com>
    Cc: Peter Xu <peterx@redhat.com>
    Cc: Hugh Dickins <hughd@google.com>
    Cc: Andrea Arcangeli <aarcange@redhat.com>
    Cc: Matthew Wilcox <willy@infradead.org>
    Cc: Vlastimil Babka <vbabka@suse.cz>
    Cc: John Hubbard <jhubbard@nvidia.com>
    Cc: Jason Gunthorpe <jgg@nvidia.com>
    Cc: David Laight <David.Laight@ACULAB.COM>
    Cc: <stable@vger.kernel.org>    [5.16]
    Signed-off-by: Andrew Morton <akpm@linux-foundation.org>

diff --git a/include/linux/mm.h b/include/linux/mm.h
index 3bedc449c14d..982f2607180b 100644
--- a/include/linux/mm.h
+++ b/include/linux/mm.h
@@ -2885,7 +2885,6 @@ struct page *follow_page(struct vm_area_struct *vma, unsigned long address,
 #define FOLL_MIGRATION	0x400	/* wait for page to replace migration entry */
 #define FOLL_TRIED	0x800	/* a retry, previous pass started an IO */
 #define FOLL_REMOTE	0x2000	/* we are working on non-current tsk/mm */
-#define FOLL_COW	0x4000	/* internal GUP flag */
 #define FOLL_ANON	0x8000	/* don't do file mappings */
 #define FOLL_LONGTERM	0x10000	/* mapping lifetime is indefinite: see below */
 #define FOLL_SPLIT_PMD	0x20000	/* split huge pmd before returning */
diff --git a/mm/gup.c b/mm/gup.c
index 732825157430..5abdaf487460 100644
--- a/mm/gup.c
+++ b/mm/gup.c
@@ -478,14 +478,42 @@ static int follow_pfn_pte(struct vm_area_struct *vma, unsigned long address,
 	return -EEXIST;
 }

-/*
- * FOLL_FORCE can write to even unwritable pte's, but only
- * after we've gone through a COW cycle and they are dirty.
- */
-static inline bool can_follow_write_pte(pte_t pte, unsigned int flags)
+/* FOLL_FORCE can write to even unwritable PTEs in COW mappings. */
+static inline bool can_follow_write_pte(pte_t pte, struct page *page,
+					struct vm_area_struct *vma,
+					unsigned int flags)
 {
-	return pte_write(pte) ||
-		((flags & FOLL_FORCE) && (flags & FOLL_COW) && pte_dirty(pte));
+	/* If the pte is writable, we can write to the page. */
+	if (pte_write(pte))
+		return true;
+
+	/* Maybe FOLL_FORCE is set to override it? */
+	if (!(flags & FOLL_FORCE))
+		return false;
+
+	/* But FOLL_FORCE has no effect on shared mappings */
+	if (vma->vm_flags & (VM_MAYSHARE | VM_SHARED))
+		return false;
+
+	/* ... or read-only private ones */
+	if (!(vma->vm_flags & VM_MAYWRITE))
+		return false;
+
+	/* ... or already writable ones that just need to take a write fault */
+	if (vma->vm_flags & VM_WRITE)
+		return false;
+
+	/*
+	 * See can_change_pte_writable(): we broke COW and could map the page
+	 * writable if we have an exclusive anonymous page ...
+	 */
+	if (!page || !PageAnon(page) || !PageAnonExclusive(page))
+		return false;
+
+	/* ... and a write-fault isn't required for other reasons. */
+	if (vma_soft_dirty_enabled(vma) && !pte_soft_dirty(pte))
+		return false;
+	return !userfaultfd_pte_wp(vma, pte);
 }

 static struct page *follow_page_pte(struct vm_area_struct *vma,
@@ -528,12 +556,19 @@ static struct page *follow_page_pte(struct vm_area_struct *vma,
 	}
 	if ((flags & FOLL_NUMA) && pte_protnone(pte))
 		goto no_page;
-	if ((flags & FOLL_WRITE) && !can_follow_write_pte(pte, flags)) {
-		pte_unmap_unlock(ptep, ptl);
-		return NULL;
-	}

 	page = vm_normal_page(vma, address, pte);
+
+	/*
+	 * We only care about anon pages in can_follow_write_pte() and don't
+	 * have to worry about pte_devmap() because they are never anon.
+	 */
+	if ((flags & FOLL_WRITE) &&
+	    !can_follow_write_pte(pte, page, vma, flags)) {
+		page = NULL;
+		goto out;
+	}
+
 	if (!page && pte_devmap(pte) && (flags & (FOLL_GET | FOLL_PIN))) {
 		/*
 		 * Only return device mapping pages in the FOLL_GET or FOLL_PIN
@@ -986,17 +1021,6 @@ static int faultin_page(struct vm_area_struct *vma,
 		return -EBUSY;
 	}

-	/*
-	 * The VM_FAULT_WRITE bit tells us that do_wp_page has broken COW when
-	 * necessary, even if maybe_mkwrite decided not to set pte_write. We
-	 * can thus safely do subsequent page lookups as if they were reads.
-	 * But only do so when looping for pte_write is futile: in some cases
-	 * userspace may also be wanting to write to the gotten user page,
-	 * which a read fault here might prevent (a readonly page might get
-	 * reCOWed by userspace write).
-	 */
-	if ((ret & VM_FAULT_WRITE) && !(vma->vm_flags & VM_WRITE))
-		*flags |= FOLL_COW;
 	return 0;
 }

diff --git a/mm/huge_memory.c b/mm/huge_memory.c
index 8a7c1b344abe..e9414ee57c5b 100644
--- a/mm/huge_memory.c
+++ b/mm/huge_memory.c
@@ -1040,12 +1040,6 @@ struct page *follow_devmap_pmd(struct vm_area_struct *vma, unsigned long addr,

 	assert_spin_locked(pmd_lockptr(mm, pmd));

-	/*
-	 * When we COW a devmap PMD entry, we split it into PTEs, so we should
-	 * not be in this function with `flags & FOLL_COW` set.
-	 */
-	WARN_ONCE(flags & FOLL_COW, "mm: In follow_devmap_pmd with FOLL_COW set");
-
 	/* FOLL_GET and FOLL_PIN are mutually exclusive. */
 	if (WARN_ON_ONCE((flags & (FOLL_PIN | FOLL_GET)) ==
 			 (FOLL_PIN | FOLL_GET)))
@@ -1395,14 +1389,42 @@ vm_fault_t do_huge_pmd_wp_page(struct vm_fault *vmf)
 	return VM_FAULT_FALLBACK;
 }

-/*
- * FOLL_FORCE can write to even unwritable pmd's, but only
- * after we've gone through a COW cycle and they are dirty.
- */
-static inline bool can_follow_write_pmd(pmd_t pmd, unsigned int flags)
+/* FOLL_FORCE can write to even unwritable PMDs in COW mappings. */
+static inline bool can_follow_write_pmd(pmd_t pmd, struct page *page,
+					struct vm_area_struct *vma,
+					unsigned int flags)
 {
-	return pmd_write(pmd) ||
-	       ((flags & FOLL_FORCE) && (flags & FOLL_COW) && pmd_dirty(pmd));
+	/* If the pmd is writable, we can write to the page. */
+	if (pmd_write(pmd))
+		return true;
+
+	/* Maybe FOLL_FORCE is set to override it? */
+	if (!(flags & FOLL_FORCE))
+		return false;
+
+	/* But FOLL_FORCE has no effect on shared mappings */
+	if (vma->vm_flags & (VM_MAYSHARE | VM_SHARED))
+		return false;
+
+	/* ... or read-only private ones */
+	if (!(vma->vm_flags & VM_MAYWRITE))
+		return false;
+
+	/* ... or already writable ones that just need to take a write fault */
+	if (vma->vm_flags & VM_WRITE)
+		return false;
+
+	/*
+	 * See can_change_pte_writable(): we broke COW and could map the page
+	 * writable if we have an exclusive anonymous page ...
+	 */
+	if (!page || !PageAnon(page) || !PageAnonExclusive(page))
+		return false;
+
+	/* ... and a write-fault isn't required for other reasons. */
+	if (vma_soft_dirty_enabled(vma) && !pmd_soft_dirty(pmd))
+		return false;
+	return !userfaultfd_huge_pmd_wp(vma, pmd);
 }

 struct page *follow_trans_huge_pmd(struct vm_area_struct *vma,
@@ -1411,12 +1433,16 @@ struct page *follow_trans_huge_pmd(struct vm_area_struct *vma,
 				   unsigned int flags)
 {
 	struct mm_struct *mm = vma->vm_mm;
-	struct page *page = NULL;
+	struct page *page;

 	assert_spin_locked(pmd_lockptr(mm, pmd));

-	if (flags & FOLL_WRITE && !can_follow_write_pmd(*pmd, flags))
-		goto out;
+	page = pmd_page(*pmd);
+	VM_BUG_ON_PAGE(!PageHead(page) && !is_zone_device_page(page), page);
+
+	if ((flags & FOLL_WRITE) &&
+	    !can_follow_write_pmd(*pmd, page, vma, flags))
+		return NULL;

 	/* Avoid dumping huge zero page */
 	if ((flags & FOLL_DUMP) && is_huge_zero_pmd(*pmd))
@@ -1424,10 +1450,7 @@ struct page *follow_trans_huge_pmd(struct vm_area_struct *vma,

 	/* Full NUMA hinting faults to serialise migration in fault paths */
 	if ((flags & FOLL_NUMA) && pmd_protnone(*pmd))
-		goto out;
-
-	page = pmd_page(*pmd);
-	VM_BUG_ON_PAGE(!PageHead(page) && !is_zone_device_page(page), page);
+		return NULL;

 	if (!pmd_write(*pmd) && gup_must_unshare(flags, page))
 		return ERR_PTR(-EMLINK);
@@ -1444,7 +1467,6 @@ struct page *follow_trans_huge_pmd(struct vm_area_struct *vma,
 	page += (addr & ~HPAGE_PMD_MASK) >> PAGE_SHIFT;
 	VM_BUG_ON_PAGE(!PageCompound(page) && !is_zone_device_page(page), page);

-out:
 	return page;
 }


```

## 扩展
连 page table 都要 copy on write 的设计:
- https://www.cs.purdue.edu/homes/pfonseca/papers/eurosys21-odf.pdf
- https://lore.kernel.org/lkml/20220521040301.GA1508050@strix-laptop/T/#m78a5ce920c0bacde410f38768f4892eb0bb4f818

## do_wp_page 是做啥的

A further potential case arises if the region grants write permission for the page
but the *access mechanisms of the hardware do not
(thus triggering the fault)*. Notice that since the page is present in this case,
the above if case is executed and the kernel drops right through to the following code:
```c
	if (flags & FAULT_FLAG_WRITE) {
		if (!pte_write(entry))
			return do_wp_page(mm, vma, address,
					pte, pmd, ptl, entry);
		entry = pte_mkdirty(entry);
	}
```

## do_wp_page 和 do_cow_fault 是啥关系?


```txt
@[
    do_wp_page+1
    __handle_mm_fault+2066
    handle_mm_fault+341
    do_user_addr_fault+351
    exc_page_fault+109
    asm_exc_page_fault+38
]: 21072
```



- do_pte_missing
  - do_fault
    - do_cow_fault : 如果不存在 page table 的时候，应该是 unshared 的映射一个文件

do_wp_page 是在还存在 page table 的时候调用

dup_mmap
  copy_page_range
    copy_one_pte
       if (is_cow_mapping(vm_flags) && pte_write(pte)) {
               /父子页表都清除 RW 标记，置为只读，但 P 依然存在/
               ptep_set_wrprotect(src_mm, addr, src_pte);
               pte = pte_wrprotect(pte);
        }

- [ ] 关键问题，为什么 fork 的过程中，需要将 page table 拷贝一次

```diff
History:   #0
Commit:    c89357e27f20dda3fff6791d27bb6c91eae99f4a
Author:    David Hildenbrand <david@redhat.com>
Committer: Andrew Morton <akpm@linux-foundation.org>
Date:      Tue 10 May 2022 09:20:45 AM CST

mm: support GUP-triggered unsharing of anonymous pages

Whenever GUP currently ends up taking a R/O pin on an anonymous page that
might be shared -- mapped R/O and !PageAnonExclusive() -- any write fault
on the page table entry will end up replacing the mapped anonymous page
due to COW, resulting in the GUP pin no longer being consistent with the
page actually mapped into the page table.

The possible ways to deal with this situation are:
 (1) Ignore and pin -- what we do right now.
 (2) Fail to pin -- which would be rather surprising to callers and
     could break user space.
 (3) Trigger unsharing and pin the now exclusive page -- reliable R/O
     pins.

We want to implement 3) because it provides the clearest semantics and
allows for checking in unpin_user_pages() and friends for possible BUGs:
when trying to unpin a page that's no longer exclusive, clearly something
went very wrong and might result in memory corruptions that might be hard
to debug.  So we better have a nice way to spot such issues.

To implement 3), we need a way for GUP to trigger unsharing:
FAULT_FLAG_UNSHARE.  FAULT_FLAG_UNSHARE is only applicable to R/O mapped
anonymous pages and resembles COW logic during a write fault.  However, in
contrast to a write fault, GUP-triggered unsharing will, for example,
still maintain the write protection.

Let's implement FAULT_FLAG_UNSHARE by hooking into the existing write
fault handlers for all applicable anonymous page types: ordinary pages,
THP and hugetlb.

* If FAULT_FLAG_UNSHARE finds a R/O-mapped anonymous page that has been
  marked exclusive in the meantime by someone else, there is nothing to do.
* If FAULT_FLAG_UNSHARE finds a R/O-mapped anonymous page that's not
  marked exclusive, it will try detecting if the process is the exclusive
  owner. If exclusive, it can be set exclusive similar to reuse logic
  during write faults via page_move_anon_rmap() and there is nothing
  else to do; otherwise, we either have to copy and map a fresh,
  anonymous exclusive page R/O (ordinary pages, hugetlb), or split the
  THP.

This commit is heavily based on patches by Andrea.
```

## 扩展内容

- 理解一下文件系统的 cow 的实现 e.g., btrfs


## 如何实现 page table 的 copy on write ？

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
