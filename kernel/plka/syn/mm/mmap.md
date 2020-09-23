# mm/mmap.c
似乎 plka 的整个第四章的内容。

## 问题
1. kernel page fault
    1. 为什么 kernel 会发生 page fault, 不是实现分配好了吗 ?
2. file based 和 anon 的区别对照表
    1. vma_area 的 vm_operations_struct 不同吗 ?
    2. 创建 vma_area 的方法
3. stack 有什么什么需要特殊处理的吗 ?


## mmap_pgoff 和 munmap 
1. 根本的两个 syscall 机制
2. munmap 应该是需要调用 rmap 来释放一下相关的内容，但是目前没有找到证据(todo)

```c
SYSCALL_DEFINE6(mmap_pgoff, unsigned long, addr, unsigned long, len,
		unsigned long, prot, unsigned long, flags,
		unsigned long, fd, unsigned long, pgoff)
{
	return ksys_mmap_pgoff(addr, len, prot, flags, fd, pgoff);
}

SYSCALL_DEFINE2(munmap, unsigned long, addr, size_t, len)
{
	profile_munmap(addr);
	return vm_munmap(addr, len);
}
```



## vma_is_anonymous  

1. 最关键的区别
```c
static inline bool vma_is_anonymous(struct vm_area_struct *vma)
{
	return !vma->vm_ops;
}
```

1. vm_operations_struct 的存在 : 一共存在哪几种 vma_area, 居然是各种驱动会给 `vm_operations_struct->open` 赋值
    1. vm_operations_struct 各种成员的总用是什么 ? 
    2. 



## 和 rmap 的交互
1. find_mergeable_anon_vma
2. anon_vma_clone 的三个调用者:
    1. copy_vma
    2. `__split_vma` : 几乎靠 `__vma_adjust` 维持生活了
    3. `__vma_adjust`

3. vma_merge : 利用 can_vma_merge_after 和 can_vma_merge_before 等简单的辅助函数判断，然后调用 `__vma_adjust` 进行处理。

```c
  // 首先试图 vma_merge，如果 merge 不成功，那么就拷贝:
	if (find_vma_links(mm, addr, addr + len, &prev, &rb_link, &rb_parent))
		return NULL;	/* should never get here */
	new_vma = vma_merge(mm, prev, addr, addr + len, vma->vm_flags,
			    vma->anon_vma, vma->vm_file, pgoff, vma_policy(vma),
			    vma->vm_userfaultfd_ctx);


		new_vma = vm_area_dup(vma); // 浅拷贝，以及初始化 anon_vma_chain
		if (!new_vma)
			goto out;
		new_vma->vm_start = addr;
		new_vma->vm_end = addr + len;
		new_vma->vm_pgoff = pgoff;
		if (vma_dup_policy(vma, new_vma))
			goto out_free_vma;
		if (anon_vma_clone(new_vma, vma)) // 调用 context 和 anon_vma_fork 类似，拷贝而已。
			goto out_free_mempol;
		if (new_vma->vm_file)
			get_file(new_vma->vm_file); // 增加一个 ref count
		if (new_vma->vm_ops && new_vma->vm_ops->open)
			new_vma->vm_ops->open(new_vma);
		vma_link(mm, new_vma, prev, rb_link, rb_parent);
		*need_rmap_locks = false;
```






## special
> @maybe_todo
```c
static const struct vm_operations_struct special_mapping_vmops = {
	.close = special_mapping_close,
	.fault = special_mapping_fault,
	.mremap = special_mapping_mremap,
	.name = special_mapping_name,
};

static const struct vm_operations_struct legacy_special_mapping_vmops = {
	.close = special_mapping_close,
	.fault = special_mapping_fault,
};
```

## reserve
> @maybe_todo


```c
/*
 * Initialise sysctl_user_reserve_kbytes.
 *
 * This is intended to prevent a user from starting a single memory hogging
 * process, such that they cannot recover (kill the hog) in OVERCOMMIT_NEVER
 * mode.
 *
 * The default value is min(3% of free memory, 128MB)
 * 128MB is enough to recover with sshd/login, bash, and top/kill.
 */
static int init_user_reserve(void)
{
	unsigned long free_kbytes;

	free_kbytes = global_zone_page_state(NR_FREE_PAGES) << (PAGE_SHIFT - 10);

	sysctl_user_reserve_kbytes = min(free_kbytes / 32, 1UL << 17);
	return 0;
}
subsys_initcall(init_user_reserve);
```

# vm
> 主要在 mmap.c 中

# 问题
1. 各种region 的触发如何管理 ?
2. file anon 比对
3. brk 实现的方法
4. pagefault 实现

## mmap实现细节

1. 如何分配内存的 ?
2. mmap 某一个文件之后，然后就可以直接访问，那么pgfault 如何配合使用 ?
https://stackoverflow.com/questions/26259421/use-mmap-in-c-to-write-into-memory

3. mmap 处理了filebased 的vma，实现anon 对应的版本vma 创建管理工作谁来完成 ?
4. mmap 之后的操作，对于内存的修改，最后结果会被自动写到文件系统中间吗 ?
5. 好像不只是含有 file anon 的vma !


syscall 的接口汇集到do_mmap,其中完成各种检查，查找合适的空间
mmap_region 中间实现:
> 主要，三种情况分别处理。@todo 既然在此处实现三种映射，除非mmap 不仅仅支持mmap

最后vmlink 之类的处理一下:
```c
		error = call_mmap(file, vma);
		if (error)
			goto unmap_and_free_vma;

		/* Can addr have changed??
		 *
		 * Answer: Yes, several device drivers can do it in their
		 *         f_op->mmap method. -DaveM
		 * Bug: If addr is changed, prev, rb_link, rb_parent should
		 *      be updated for vma_link()
		 */
		WARN_ON_ONCE(addr != vma->vm_start);

		addr = vma->vm_start;
		vm_flags = vma->vm_flags;
	} else if (vm_flags & VM_SHARED) {
		error = shmem_zero_setup(vma);
		if (error)
			goto free_vma;
	} else {
		vma_set_anonymous(vma);


// file based !
static inline int call_mmap(struct file *file, struct vm_area_struct *vma)
{
	return file->f_op->mmap(file, vma);
}


int generic_file_mmap(struct file * file, struct vm_area_struct * vma)
{
	struct address_space *mapping = file->f_mapping;

	if (!mapping->a_ops->readpage)
		return -ENOEXEC;
	file_accessed(file); // 添加一个文件访问时间而已
	vma->vm_ops = &generic_file_vm_ops; // 挂钩上关键的pagefault
	return 0;
}

static inline void file_accessed(struct file *file)
{
	if (!(file->f_flags & O_NOATIME))
		touch_atime(&file->f_path);
}

// shmem based
/**
 * shmem_zero_setup - setup a shared anonymous mapping
 * @vma: the vma to be mmapped is prepared by do_mmap_pgoff
 */
int shmem_zero_setup(struct vm_area_struct *vma)
{
	struct file *file;
	loff_t size = vma->vm_end - vma->vm_start;

	/*
	 * Cloning a new file under mmap_sem leads to a lock ordering conflict
	 * between XFS directory reading and selinux: since this file is only
	 * accessible to the user through its mapping, use S_PRIVATE flag to
	 * bypass file security, in the same way as shmem_kernel_file_setup().
	 */
	file = shmem_kernel_file_setup("dev/zero", size, vma->vm_flags);
	if (IS_ERR(file))
		return PTR_ERR(file);

	if (vma->vm_file)
		fput(vma->vm_file);
	vma->vm_file = file;
	vma->vm_ops = &shmem_vm_ops; // 关键操作

	if (IS_ENABLED(CONFIG_TRANSPARENT_HUGE_PAGECACHE) &&
			((vma->vm_start + ~HPAGE_PMD_MASK) & HPAGE_PMD_MASK) <
			(vma->vm_end & HPAGE_PMD_MASK)) {
		khugepaged_enter(vma, vma->vm_flags);
	}

	return 0;
}

static const struct vm_operations_struct shmem_vm_ops = {
	.fault		= shmem_fault,
	.map_pages	= filemap_map_pages,
#ifdef CONFIG_NUMA
	.set_policy     = shmem_set_policy,
	.get_policy     = shmem_get_policy,
#endif
};

// anon
static inline void vma_set_anonymous(struct vm_area_struct *vma)
{
	vma->vm_ops = NULL; // @todo 猜测这是默认操作方式!
}
```
> 猜测之后vm_fault会首先通过pgfault 所在的vma 中间获取。

@todo 这么说 anon page 是不用处理page fault的吗? 显然不是吧 !

mmap 如何给shmem 和 anon 使用 ? emmmmm 应该是mmap syscall 和 do_mmap_pgoff 不是等价的，
由于 do_mmap_pgoff 会进一步被其他位置调用，包括shmem

# 真正的核心page fault
> 检查ucore : ucore 不区分anon 和 file , 但是凭什么其可以不区分 ?
> ucore没有实现mmap

```c
// 玩美的配合!
static inline bool vma_is_anonymous(struct vm_area_struct *vma)
{
	return !vma->vm_ops;
}

```





# brk 机制

```c
/*
 *  this is really a simplified "do_mmap".  it only handles
 *  anonymous maps.  eventually we may be able to do some
 *  brk-specific accounting here.
 */
static int do_brk_flags(unsigned long addr, unsigned long len, unsigned long flags, struct list_head *uf)
```
> 并没有特别神奇的地方

```c
	/* Can we just expand an old private anonymous mapping? */
	vma = vma_merge(mm, prev, addr, addr + len, flags,
			NULL, NULL, pgoff, NULL, NULL_VM_UFFD_CTX);
	if (vma)
		goto out;

	/*
	 * create a vma struct for an anonymous mapping
	 */
	vma = vm_area_alloc(mm);
	if (!vma) {
		vm_unacct_memory(len >> PAGE_SHIFT);
		return -ENOMEM;
	}

	vma_set_anonymous(vma);
	vma->vm_start = addr;
	vma->vm_end = addr + len;
	vma->vm_pgoff = pgoff;
	vma->vm_flags = flags;
	vma->vm_page_prot = vm_get_page_prot(flags);
	vma_link(mm, vma, prev, rb_link, rb_parent);
out:
```
> @todo 可以分析一下vma_merge为什么会失败，难道heap不是一个连续的虚拟地址空间吗? 为什么反而可以出现空洞?


## vma_merge
1. do_brk_flags 调用
2. vma_merge 的参数为什么不是两个 vma ，而是一个prev 和 vma 的各种参数 ?


## `__vma_adjust`

1. `__vma_adjust` 被 vma_merge 和 vma_adjust 调用
```c
static inline int vma_adjust(struct vm_area_struct *vma, unsigned long start,
	unsigned long end, pgoff_t pgoff, struct vm_area_struct *insert)
{
	return __vma_adjust(vma, start, end, pgoff, insert, NULL);
}
```

2. 参数 vma insert expand 的含义:
    1. 被 vma_merge 调用时，参数 insert 总是 NULL
    2. vma_adjust 在 mmap.c 中间被 `__split_vma`
    3. 所以，`__vma_adjust` 函数表示调整 vma 的 start end 和 pgoff，insert 表示调整的时候拆出来的 vma，expand 表示用来 expand 的vma 


3. 问题 :
    1. 参数 vma 和 expand 可能是同一个变量，这对应于什么情况 ? (强行将 vma 展开吗 ?)
    2. 

```c
/*
 * We cannot adjust vm_start, vm_end, vm_pgoff fields of a vma that
 * is already present in an i_mmap tree without adjusting the tree.
 * The following helper function should be used when such adjustments
 * are necessary.  The "insert" vma (if any) is to be inserted
 * before we drop the necessary locks.
 */
int __vma_adjust(struct vm_area_struct *vma, unsigned long start,
	unsigned long end, pgoff_t pgoff, struct vm_area_struct *insert,
	struct vm_area_struct *expand)
{
	struct mm_struct *mm = vma->vm_mm;
	struct vm_area_struct *next = vma->vm_next, *orig_vma = vma;
	struct address_space *mapping = NULL;
	struct rb_root_cached *root = NULL;
	struct anon_vma *anon_vma = NULL;
	struct file *file = vma->vm_file;
	bool start_changed = false, end_changed = false;
	long adjust_next = 0;
	int remove_next = 0;

  // 合并的情况: 阅读一下 vma_merge 关于 mprotect 带来的影响 todo
	if (next && !insert) {
		struct vm_area_struct *exporter = NULL, *importer = NULL;

    // 考虑自己的边界 和 next 的三个返回
		if (end >= next->vm_end) {
			/*
			 * vma expands, overlapping all the next, and
			 * perhaps the one after too (mprotect case 6).
			 * The only other cases that gets here are
			 * case 1, case 7 and case 8.
			 */
			if (next == expand) {
				/*
				 * The only case where we don't expand "vma"
				 * and we expand "next" instead is case 8.
				 */
				VM_WARN_ON(end != next->vm_end);
				/*
				 * remove_next == 3 means we're
				 * removing "vma" and that to do so we
				 * swapped "vma" and "next".
				 */
				remove_next = 3;
				VM_WARN_ON(file != next->vm_file);
				swap(vma, next);
			} else {
				VM_WARN_ON(expand != vma);
				/*
				 * case 1, 6, 7, remove_next == 2 is case 6,
				 * remove_next == 1 is case 1 or 7.
				 */
				remove_next = 1 + (end > next->vm_end);
				VM_WARN_ON(remove_next == 2 &&
					   end != next->vm_next->vm_end);
				VM_WARN_ON(remove_next == 1 &&
					   end != next->vm_end);
				/* trim end to next, for case 6 first pass */
				end = next->vm_end;
			}

			exporter = next;
			importer = vma;

			/*
			 * If next doesn't have anon_vma, import from vma after
			 * next, if the vma overlaps with it.
			 */
			if (remove_next == 2 && !next->anon_vma)
				exporter = next->vm_next;

		} else if (end > next->vm_start) {
			/*
			 * vma expands, overlapping part of the next:
			 * mprotect case 5 shifting the boundary up.
			 */
			adjust_next = (end - next->vm_start) >> PAGE_SHIFT;
			exporter = next;
			importer = vma;
			VM_WARN_ON(expand != importer);
		} else if (end < vma->vm_end) {
			/*
			 * vma shrinks, and !insert tells it's not
			 * split_vma inserting another: so it must be
			 * mprotect case 4 shifting the boundary down.
			 */
			adjust_next = -((vma->vm_end - end) >> PAGE_SHIFT);
			exporter = vma;
			importer = next;
			VM_WARN_ON(expand != importer);
		}


    // 上面三种情况分析，得到 importer 和 exporter 之后
    // importer 表示吸收空间的，exporter 表示释放空间的部分
    // 然后进入到 again 中间，开始分析:

		/*
		 * Easily overlooked: when mprotect shifts the boundary,
		 * make sure the expanding vma has anon_vma set if the
		 * shrinking vma had, to cover any anon pages imported.
		 */

    // 当 importer 没有持有 anon_vma 的时候，那么公用一下
    // 这里提供了 : anon_vma_clone 的情况不仅仅是 fork 
		if (exporter && exporter->anon_vma && !importer->anon_vma) {
			int error;

			importer->anon_vma = exporter->anon_vma;
			error = anon_vma_clone(importer, exporter);
			if (error)
				return error;
		}
	}
again:
	vma_adjust_trans_huge(orig_vma, start, end, adjust_next);

  // 对于文件更新应该很简单，但是此处并不是更新的位置
	if (file) {
		mapping = file->f_mapping;
		root = &mapping->i_mmap;
		uprobe_munmap(vma, vma->vm_start, vma->vm_end);

		if (adjust_next)
			uprobe_munmap(next, next->vm_start, next->vm_end);

		i_mmap_lock_write(mapping);
		if (insert) {
			/*
			 * Put into interval tree now, so instantiated pages
			 * are visible to arm/parisc __flush_dcache_page
			 * throughout; but we cannot insert into address
			 * space until vma start or end is updated.
			 */
			__vma_link_file(insert);
		}
	}

  // 更新一下 vma 之前，将其从 interval tree 中间删除
  // 更新我们需要的三个数值，然后将重新插入
  // 对于 mapping->i_mmap 为 NULL 和 非 NULL 都是可以处理的
	anon_vma = vma->anon_vma;
	if (!anon_vma && adjust_next)
		anon_vma = next->anon_vma;

	if (anon_vma) {
		VM_WARN_ON(adjust_next && next->anon_vma &&
			   anon_vma != next->anon_vma);
		anon_vma_lock_write(anon_vma);
		anon_vma_interval_tree_pre_update_vma(vma);
		if (adjust_next)
			anon_vma_interval_tree_pre_update_vma(next);
	}

	if (root) {
		flush_dcache_mmap_lock(mapping);
		vma_interval_tree_remove(vma, root);
		if (adjust_next)
			vma_interval_tree_remove(next, root);
	}

	if (start != vma->vm_start) {
		vma->vm_start = start;
		start_changed = true;
	}
	if (end != vma->vm_end) {
		vma->vm_end = end;
		end_changed = true;
	}
	vma->vm_pgoff = pgoff;
	if (adjust_next) {
		next->vm_start += adjust_next << PAGE_SHIFT;
		next->vm_pgoff += adjust_next;
	}

	if (root) {
		if (adjust_next)
			vma_interval_tree_insert(next, root);
		vma_interval_tree_insert(vma, root);
		flush_dcache_mmap_unlock(mapping);
  }

  // 如果 next 需要被删除掉
	if (remove_next) {
		/*
		 * vma_merge has merged next into vma, and needs
		 * us to remove next before dropping the locks.
		 */
		if (remove_next != 3)
			__vma_unlink_prev(mm, next, vma);
		else
			/*
			 * vma is not before next if they've been
			 * swapped.
			 *
			 * pre-swap() next->vm_start was reduced so
			 * tell validate_mm_rb to ignore pre-swap()
			 * "next" (which is stored in post-swap()
			 * "vma").
			 */
			__vma_unlink_common(mm, next, NULL, false, vma);
		if (file)
			__remove_shared_vm_struct(next, file, mapping);
	} else if (insert) {
		/*
		 * split_vma has split insert from vma, and needs
		 * us to insert it before dropping the locks
		 * (it may either follow vma or precede it).
		 */
		__insert_vm_struct(mm, insert);
	} else {
		if (start_changed)
			vma_gap_update(vma);
		if (end_changed) {
			if (!next)
				mm->highest_vm_end = vm_end_gap(vma);
			else if (!adjust_next)
				vma_gap_update(next);
		}
	}

	if (anon_vma) {
		anon_vma_interval_tree_post_update_vma(vma);
		if (adjust_next)
			anon_vma_interval_tree_post_update_vma(next);
		anon_vma_unlock_write(anon_vma);
	}
  // 算是更新结束了吧 !


	if (mapping)
		i_mmap_unlock_write(mapping);

	if (root) {
		uprobe_mmap(vma);

		if (adjust_next)
			uprobe_mmap(next);
	}

	if (remove_next) {
		if (file) {
			uprobe_munmap(next, next->vm_start, next->vm_end);
			fput(file);
		}
		if (next->anon_vma)
			anon_vma_merge(vma, next);
		mm->map_count--;
		mpol_put(vma_policy(next));
		vm_area_free(next);
		/*
		 * In mprotect's case 6 (see comments on vma_merge),
		 * we must remove another next too. It would clutter
		 * up the code too much to do both in one go.
		 */
		if (remove_next != 3) {
			/*
			 * If "next" was removed and vma->vm_end was
			 * expanded (up) over it, in turn
			 * "next->vm_prev->vm_end" changed and the
			 * "vma->vm_next" gap must be updated.
			 */
			next = vma->vm_next;
		} else {
			/*
			 * For the scope of the comment "next" and
			 * "vma" considered pre-swap(): if "vma" was
			 * removed, next->vm_start was expanded (down)
			 * over it and the "next" gap must be updated.
			 * Because of the swap() the post-swap() "vma"
			 * actually points to pre-swap() "next"
			 * (post-swap() "next" as opposed is now a
			 * dangling pointer).
			 */
			next = vma;
		}
		if (remove_next == 2) {
			remove_next = 1;
			end = next->vm_end;
			goto again;
		}
		else if (next)
			vma_gap_update(next);
		else {
			/*
			 * If remove_next == 2 we obviously can't
			 * reach this path.
			 *
			 * If remove_next == 3 we can't reach this
			 * path because pre-swap() next is always not
			 * NULL. pre-swap() "next" is not being
			 * removed and its next->vm_end is not altered
			 * (and furthermore "end" already matches
			 * next->vm_end in remove_next == 3).
			 *
			 * We reach this only in the remove_next == 1
			 * case if the "next" vma that was removed was
			 * the highest vma of the mm. However in such
			 * case next->vm_end == "end" and the extended
			 * "vma" has vma->vm_end == next->vm_end so
			 * mm->highest_vm_end doesn't need any update
			 * in remove_next == 1 case.
			 */
			VM_WARN_ON(mm->highest_vm_end != vm_end_gap(vma));
		}
	}
	if (insert && file)
		uprobe_mmap(insert);

	validate_mm(mm);

	return 0;
}



/*
 * vma has some anon_vma assigned, and is already inserted on that
 * anon_vma's interval trees.
 *
 * Before updating the vma's vm_start / vm_end / vm_pgoff fields, the
 * vma must be removed from the anon_vma's interval trees using
 * anon_vma_interval_tree_pre_update_vma().
 *
 * After the update, the vma will be reinserted using
 * anon_vma_interval_tree_post_update_vma().
 *
 * The entire update must be protected by exclusive mmap_sem and by
 * the root anon_vma's mutex.
 */
static inline void
anon_vma_interval_tree_pre_update_vma(struct vm_area_struct *vma)
{
	struct anon_vma_chain *avc;

	list_for_each_entry(avc, &vma->anon_vma_chain, same_vma)
		anon_vma_interval_tree_remove(avc, &avc->anon_vma->rb_root);
}

static inline void
anon_vma_interval_tree_post_update_vma(struct vm_area_struct *vma)
{
	struct anon_vma_chain *avc;

	list_for_each_entry(avc, &vma->anon_vma_chain, same_vma)
		anon_vma_interval_tree_insert(avc, &avc->anon_vma->rb_root);
}
```

## get_unmapped_area
