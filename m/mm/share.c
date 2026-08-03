#include "internal.h"
#include <linux/gfp.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/miscdevice.h>
#include <linux/pagemap.h>
#include <linux/migrate.h>
/*
 * 分析和 userspace 共享内存:
 * 1. 一共方法是类似 qemu kvm 共享内存，qemu mmap 映射空间，然后通过 KVM_SET_USER_MEMORY_REGION 传递给 kvm
 * 2. 一种是驱动注册一个文件，在 file_operations::mmap 上注册函数
 *
 * 总之，无论如何，需要用户态 mmap 起来才可以
 */
static struct folio *folio = NULL;
#define USE_HACKING_MMAP 1
#ifdef USE_HACKING_MMAP

static void show_attr(void)
{
	struct address_space *m;
	const struct movable_operations *mo;
	if (!folio)
		return;
	// TODO page_movable_ops 什么时候会注册上
	m = folio_mapping(folio);
	if (m)
		pr_info("address_space : %px\n", m);
	mo = page_movable_ops(folio_page(folio, 0));
	if (mo)
		pr_info("page_movable_ops : %px\n", mo);
	pr_info("ref : %d\n", folio_ref_count(folio));
	pr_info("map : %d\n", folio_mapped(folio));
	// 这个符号没有导出
	// pr_info("movable : %d\n", folio_test_movable(folio));
}

static int alloc(struct vm_area_struct *vma)
{
	size_t sz = vma->vm_end - vma->vm_start;
	if (sz != PAGE_SIZE)
		return -EINVAL;
	if (folio == NULL) {
		folio = folio_alloc(GFP_USER, 1);

		if (folio == NULL)
			return -ENOMEM;

		pr_info("alloc\n");
		show_attr();
	}
	return 0;
}

#define USE_VM_INSERT_PAGES

#ifdef USE_REMAP_PFN_RANGE
// 这个实现 mapcount 始终为 1 ，即便是 page 已经到进程的地址空间中
// 但是 drivers/usb/core/devio.c 的确用的这个方法
static int ring_mmap(struct file *filp, struct vm_area_struct *vma)
{
	int ret = alloc(vma);
	if (ret)
		return ret;
	return remap_pfn_range(vma, vma->vm_start, folio_pfn(folio), PAGE_SIZE,
			       vma->vm_page_prot);
}
#endif

#ifdef USE_VM_INSERT_PAGES
/*
 * 从 io_uring_mmap 中抄过来的，通过这种方法，可以看到 page 的 mapcount 和 refcount 都增加了
 * 是因为调用过 folio_add_file_rmap_pte
 * */
static int ring_mmap(struct file *filp, struct vm_area_struct *vma)
{
	struct address_space *as;
	int ret = alloc(vma);
	struct page *pages = folio_page(folio, 0);
	if (ret)
		return ret;

	/* folio 和 address_space 的关系构建其实将 folio 加入到 page cache
	 * 例如在 __filemap_add_folio 中
	 *
	 * @[
	 *   __filemap_add_folio+5
    	 *   filemap_add_folio+137
    	 *   __filemap_get_folio+352
    	 *   ext4_da_write_begin+221
    	 *   generic_perform_write+238
    	 *   ext4_buffered_write_iter+103
    	 *   vfs_write+673
    	 *   __x64_sys_pwrite64+152
    	 *   do_syscall_64+193
    	 *   entry_SYSCALL_64_after_hwframe+119
	 * ]: 49
	 *
	 * 所以，这里还是默认的 0
	 */
	pr_info("before address space %lx", (unsigned long)folio->mapping); // 0
	/* 插入多个 folio 的 api */
	/* return vm_insert_pages(vma, vma->vm_start, &pages, &nr_pages); */
	ret = vm_insert_page(vma, vma->vm_start, pages);
	as = folio_mapping(folio);
	pr_info("after address space %lx", (unsigned long)folio->mapping); // 0

	return ret;
}
#endif
#endif

static ssize_t amsg_read(struct file *file, char __user *buf, size_t len,
			 loff_t *ppos)
{
	struct address_space *as;
	if (!folio)
		return -ENOSPC;

	as = folio_mapping(folio);
	pr_info("after address space %lx", (unsigned long)folio->mapping); // 0
		//
	pr_info("file mapping : %px\n", file->f_mapping);
	pr_info("file mapping aops : %px\n", file->f_mapping->a_ops);
	/*
	 * gdb ，你令我欢喜
	 *
	 * p *((struct address_space *)(0xffff88812b5ad638))
	 *
	 * 其中可以看到:
	 *   a_ops = 0xffffffff82e6a300 <empty_aops>,
	 *
	 * p *((struct address_space_operations *)(0xffffffff82e6a300))
	 *
	 * 得到都是 0
	 *
	 * 所以，这种设备虽然可以打开文件，但是 mmap 的实话，注册的是 empty_aops ，所以
	 */

	return simple_read_from_buffer(buf, len, ppos, folio_address(folio),
				       PAGE_SIZE);
}

static struct file_operations kvm_chardev_ops = {
	.llseek = noop_llseek,
	.read = amsg_read,
#ifdef USE_HACKING_MMAP
	.mmap = ring_mmap,
#else
	/*
	 * 好吧，我理解错误了，generic_file_mmap 是 disk 用的，其 aops 必须包含了
	 * mapping->a_ops->read_folio 才可以，否则 mmap 会错误的
	 */
	.mmap = generic_file_mmap,
#endif
};

static struct miscdevice kvm_dev = {
	MISC_DYNAMIC_MINOR,
	"folio",
	&kvm_chardev_ops,
};

int test_share_init(void)
{
	return misc_register(&kvm_dev);
}

int test_share_exit(void)
{
	misc_deregister(&kvm_dev);
	return 0;
}

static void test_refcount(void)
{
	pr_info("test \n");
	show_attr();
}

int test_share(long action)
{
	switch (action) {
	case 1:
		test_refcount();
		break;
	case 2:
		break;
	}
	return 0;
}
