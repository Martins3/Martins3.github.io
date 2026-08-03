#include "internal.h"
#include <linux/file.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/pagemap.h>
#include <linux/pseudo_fs.h>

/**
 *  noop_dirty_folio 是给不会写回的，例如这个 aio 的这个场景，
 *  或者 dax 这种不会 page cache 的场景。
 */
static const struct address_space_operations aio_ctx_aops = {
	.dirty_folio = noop_dirty_folio,
};

static const struct vm_operations_struct aio_ring_vm_ops = {
	.fault = filemap_fault,
	.map_pages = filemap_map_pages,
	.page_mkwrite = filemap_page_mkwrite,
};

static int aio_ring_mmap(struct file *file, struct vm_area_struct *vma)
{
	vm_flags_set(vma, VM_DONTEXPAND);
	vma->vm_ops = &aio_ring_vm_ops;
	return 0;
}

static const struct file_operations aio_ring_fops = {
	.mmap = aio_ring_mmap,
};

static const struct file_operations aio_ring_fops;
static struct vfsmount *aio_mnt;

static struct file *aio_private_file(void)
{
	struct file *file;
	struct inode *inode = alloc_anon_inode(aio_mnt->mnt_sb);
	if (IS_ERR(inode))
		return ERR_CAST(inode);

	inode->i_mapping->a_ops = &aio_ctx_aops;
	inode->i_size = PAGE_SIZE;

	file = alloc_file_pseudo(inode, aio_mnt, "[aio]", O_RDWR,
				 &aio_ring_fops);
	if (IS_ERR(file)) {
		pr_err("failed to allocate file");
		iput(inode);
	}
	return file;
}

#define AIO_RING_MAGIC 0xa10a10a2

static int aio_init_fs_context(struct fs_context *fc)
{
	if (!init_pseudo(fc, AIO_RING_MAGIC))
		return -ENOMEM;
	fc->s_iflags |= SB_I_NOEXEC;
	return 0;
}

static void init_aio_test(void)
{
	static struct file_system_type aio_fs = {
		.name = "martins3-aio",
		.init_fs_context = aio_init_fs_context,
		.kill_sb = kill_anon_super,
	};
	aio_mnt = kern_mount(&aio_fs);
	if (IS_ERR(aio_mnt))
		pr_info("create fs failed\n");
}

int test_aio(long action)
{
	init_aio_test();
	struct file *file = aio_private_file();
	pr_info("[%s:%d] %px\n", __FUNCTION__, __LINE__, file);
	/*
   * 非常遗憾，我们无法测试出来 do_mmap 的场景，因为 do_mmap 并没有 export
   * 出来， 的确，真的不知道什么傻逼驱动会使用这种逆天的方法来共享内存。
   */
#ifdef HACKING_MMAP
	unsigned long unused;
	long mmap_base = do_mmap(file, 0, PAGE_SIZE, PROT_READ | PROT_WRITE,
				 MAP_SHARED, 0, 0, &unused, NULL);
	pr_info("mmap base : %lx\n", mmap_base);
#endif
	return 0;
}
