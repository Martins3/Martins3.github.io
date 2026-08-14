# 测试一些东西
https://mp.weixin.qq.com/s/yLM5FlYxT06axDoyTeECKg

- lru_lock
- page_lock
- mmap_lock

吸收一下这个函数:
```c
struct page *get_page_from_vaddr(struct mm_struct *mm, unsigned long vaddr)
{
	struct page *page;
	struct vm_area_struct *vma;
	unsigned int follflags;

	down_read(&mm->mmap_lock); // 为什么要 lock

	vma = find_vma(mm, vaddr);
	if (!vma || vaddr < vma->vm_start || vma->vm_flags & VM_LOCKED) {
		up_read(&mm->mmap_lock);
		return NULL;
	}

	follflags = FOLL_GET | FOLL_DUMP;
	page = follow_page(vma, vaddr, follflags);
    // 参考这个例子， 研究下 gup 的各种函数的使用接口
	if (IS_ERR(page) || !page) {
		up_read(&mm->mmap_lock);
		return NULL;
	}

	up_read(&mm->mmap_lock);
	return page;
}
```

测试一下 reclaim_pages 可以直接从外部调用吗?


## 添加一个测试，按道理说，但是估计是有问题的

MemTotal = Cached + Buffers + AnonPages + KernelStack + PageTables + MemFree + Slab + Hugetlb

找 dogfood 中的机器都看看

写一个内核模块，如果调用 gfp 之后，通过 /proc/meminfo 和 /proc/vmstat 中那些会有变化?


