#include "internal.h"
#include <linux/bio.h>

// 主要测试一下 bio 的数据结构的遍历之类的
//
// bi_private;
// bio_init
// bio_alloc_bioset
//
// bio_chain
// blk_next_bio
//
// 参考 alloc_behind_master_bio
//
// submit_bio 还是需要到 virtio-dummy 中测试了
//
// bvec_alloc
//
// TODO 想不到 bio_dirty_fn 还有 dirty 的说法

// 参考一下: __blkdev_direct_IO  的实现，其中 bio_iov_iter_get_pages 是一个关键实现
// 将 iov 中的加入到 bio 中
static struct bio_set dummy_bio_pool;
static int basic_test(void)
{
	int ret;
	struct bio *bio;
	struct folio *folio;
	blk_opf_t opf = REQ_OP_READ;
	ret = bioset_init(&dummy_bio_pool, 4, 0,
			  BIOSET_NEED_BVECS | BIOSET_PERCPU_CACHE);
	if (ret)
		return ret;

	folio = folio_alloc(GFP_USER, 2);
	if (!folio)
		return -ENOMEM;

	// 从 bio 中分配一个， 从其中分配 10 个有 bvec ，一个 bvec 对应一个 page 才对的
	bio = bio_alloc_bioset(NULL, 10, opf, GFP_KERNEL, &dummy_bio_pool);
	bio_get(bio);
	// 默认的 bio_alloc
	if (!bio)
		return -ENOMEM;

	ret = bio_add_folio(bio, folio, 10, 0);
	if (!ret)
		return -EINVAL;

	return 0;
}

int test_bio(long action)
{
	switch (action) {
	case 0:
		return basic_test();
	case 1:
		break;
	}
	return 0;
}
