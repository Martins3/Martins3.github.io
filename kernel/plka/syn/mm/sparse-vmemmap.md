# mm/sparse-vmemmap.c

1. 使用 vmemmap ，浪费大量的虚拟地址空间, 可以实现 pfn_to_page page_to_pfn 之间的快速装换。
2. 此处完成的是将那些存在物理内存的区间　对应的 page descriptor 所在的虚拟地址对应的物理内存建立一个对应关系。


```c
// fpn : section 对应的第一个 fpn
// nr_pages : 一个 section 持有 page 数量
// vmem_altmap : NULL
struct page * __meminit __populate_section_memmap(unsigned long pfn,
		unsigned long nr_pages, int nid, struct vmem_altmap *altmap)
{
	unsigned long start;
	unsigned long end;

	/*
	 * The minimum granularity of memmap extensions is
	 * PAGES_PER_SUBSECTION as allocations are tracked in the
	 * 'subsection_map' bitmap of the section.
	 */
	end = ALIGN(pfn + nr_pages, PAGES_PER_SUBSECTION);
  // 一个 subsection 可以容纳的 page 的数量，pfn + nr_pages 表示其中的结束位置的 pfn
	pfn &= PAGE_SUBSECTION_MASK; // 将起点位置进行对其，向左扩充，但是根据参数，这一个操作没有意义，因为 fpn 是 section 对应的第一个 fpn
	nr_pages = end - pfn;

	start = (unsigned long) pfn_to_page(pfn); // 这是虚拟地址
  // 没有填充就是已经可以使用 ? 所以其实 vmemmap 的存在，是不需要建立 page table 的，所以此时也是不可以访问该位置。
  // 所以此处进行的工作是 : start 到 end 之间的位置进行建立
  // todo 但是，此时还是不知道使用 mem_section 的原理 !
	end = start + nr_pages * sizeof(struct page); // 计算填充的两个虚拟地址的开始结束的分布

	if (vmemmap_populate(start, end, nid, altmap))
		return NULL;

	return pfn_to_page(pfn); // pfn 对应的 page descriptor 所在虚拟地址
}
```

arch/x86/mm/init_64.c
```c
int __meminit vmemmap_populate(unsigned long start, unsigned long end, int node,
		struct vmem_altmap *altmap)
{
	int err;

	if (end - start < PAGES_PER_SECTION * sizeof(struct page))
		err = vmemmap_populate_basepages(start, end, node);
	else if (boot_cpu_has(X86_FEATURE_PSE))
		err = vmemmap_populate_hugepages(start, end, node, altmap); // 假设 CPU 不支持
	else if (altmap) {
		pr_err_once("%s: no cpu support for altmap allocations\n",
				__func__);
		err = -ENOMEM;
	} else
		err = vmemmap_populate_basepages(start, end, node); // 那么会进入到此处
	if (!err)
		sync_global_pgds(start, end - 1);
	return err;
}
```

## vmemmap_populate_basepages : 填充函数

1. 对于 start 到 end 之间地址进行填充, 逐级向下进行 !
2. 操作方法有点类似于 pagefault 的感觉: 当需要page frame 的时候就进行分配而已。
```c
int __meminit vmemmap_populate_basepages(unsigned long start,
					 unsigned long end, int node)
{
	unsigned long addr = start;
	pgd_t *pgd;
	p4d_t *p4d;
	pud_t *pud;
	pmd_t *pmd;
	pte_t *pte;

	for (; addr < end; addr += PAGE_SIZE) {
		pgd = vmemmap_pgd_populate(addr, node);
		if (!pgd)
			return -ENOMEM;
		p4d = vmemmap_p4d_populate(pgd, addr, node);
		if (!p4d)
			return -ENOMEM;
		pud = vmemmap_pud_populate(p4d, addr, node);
		if (!pud)
			return -ENOMEM;
		pmd = vmemmap_pmd_populate(pud, addr, node);
		if (!pmd)
			return -ENOMEM;
		pte = vmemmap_pte_populate(pmd, addr, node);
		if (!pte)
			return -ENOMEM;
		vmemmap_verify(pte, node, addr, addr + PAGE_SIZE);
	}

	return 0;
}
```

```c
pte_t * __meminit vmemmap_pte_populate(pmd_t *pmd, unsigned long addr, int node)
{
	pte_t *pte = pte_offset_kernel(pmd, addr);
	if (pte_none(*pte)) {
		pte_t entry;
		void *p = vmemmap_alloc_block_buf(PAGE_SIZE, node);
		if (!p)
			return NULL;
		entry = pfn_pte(__pa(p) >> PAGE_SHIFT, PAGE_KERNEL);
		set_pte_at(&init_mm, addr, pte, entry);
	}
	return pte;
}

static void * __meminit vmemmap_alloc_block_zero(unsigned long size, int node)
{
	void *p = vmemmap_alloc_block(size, node);

	if (!p)
		return NULL;
	memset(p, 0, size);

	return p;
}

pmd_t * __meminit vmemmap_pmd_populate(pud_t *pud, unsigned long addr, int node)
{
	pmd_t *pmd = pmd_offset(pud, addr);
	if (pmd_none(*pmd)) {
		void *p = vmemmap_alloc_block_zero(PAGE_SIZE, node);
		if (!p)
			return NULL;
		pmd_populate_kernel(&init_mm, pmd, p);
	}
	return pmd;
}

pud_t * __meminit vmemmap_pud_populate(p4d_t *p4d, unsigned long addr, int node)
{
	pud_t *pud = pud_offset(p4d, addr);
	if (pud_none(*pud)) {
		void *p = vmemmap_alloc_block_zero(PAGE_SIZE, node);
		if (!p)
			return NULL;
		pud_populate(&init_mm, pud, p);
	}
	return pud;
}

p4d_t * __meminit vmemmap_p4d_populate(pgd_t *pgd, unsigned long addr, int node)
{
	p4d_t *p4d = p4d_offset(pgd, addr);
	if (p4d_none(*p4d)) {
		void *p = vmemmap_alloc_block_zero(PAGE_SIZE, node);
		if (!p)
			return NULL;
		p4d_populate(&init_mm, p4d, p);
	}
	return p4d;
}

pgd_t * __meminit vmemmap_pgd_populate(unsigned long addr, int node)
{
	pgd_t *pgd = pgd_offset_k(addr);
	if (pgd_none(*pgd)) {
		void *p = vmemmap_alloc_block_zero(PAGE_SIZE, node);
		if (!p)
			return NULL;
		pgd_populate(&init_mm, pgd, p);
	}
	return pgd;
}
```
