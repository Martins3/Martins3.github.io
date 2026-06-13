# Zoned block devices

https://zonedstorage.io/introduction/zoned-storage/
## https://zonedstorage.io/
看上去是西部数据在推动这个事情


- https://www.kernel.org/doc/html/latest/filesystems/zonefs.html
- zoned-block-devices.rst

## 可以看看的文章: Advanced Zoned Namespace Interface for Supporting In-Storage Zone Compaction

## https://www.qemu.org/2022/11/17/zoned-emulation/ : 只是一个尝试

## 看看内核中关联的 config

```txt
CONFIG_BLK_DEV_ZONED=y
CONFIG_BLK_DEBUG_FS_ZONED=y
CONFIG_ZONEFS_FS is not set
CONFIG_ZONE_DMA=y
CONFIG_ZONE_DMA32=y
CONFIG_ZONE_DEVICE
```

## 关联的文件
- drivers/scsi/sd_zbc.c
- block/blk-zoned.c
- drivers/nvme/host/zns.c
- drivers/md/dm-zoned 之类的好几个文件

## 为什么 zone device 不需要 plug 机制

```c
/*
 * blk_mq_plug() - Get caller context plug
 * @bio : the bio being submitted by the caller context
 *
 * Plugging, by design, may delay the insertion of BIOs into the elevator in
 * order to increase BIO merging opportunities. This however can cause BIO
 * insertion order to change from the order in which submit_bio() is being
 * executed in the case of multiple contexts concurrently issuing BIOs to a
 * device, even if these context are synchronized to tightly control BIO issuing
 * order. While this is not a problem with regular block devices, this ordering
 * change can cause write BIO failures with zoned block devices as these
 * require sequential write patterns to zones. Prevent this from happening by
 * ignoring the plug state of a BIO issuing context if it is for a zoned block
 * device and the BIO to plug is a write operation.
 *
 * Return current->plug if the bio can be plugged and NULL otherwise
 */
static inline struct blk_plug *blk_mq_plug( struct bio *bio)
{
	/* Zoned block device write operation case: do not plug the BIO */
	if (IS_ENABLED(CONFIG_BLK_DEV_ZONED) &&
	    bdev_op_is_zoned_write(bio->bi_bdev, bio_op(bio)))
		return NULL;

	/*
	 * For regular block devices or read operations, use the context plug
	 * which may be NULL if blk_start_plug() was not executed.
	 */
	return current->plug;
}
```

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
