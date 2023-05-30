## 分析一下 struct block_device, struct hd_struct, and struct gendisk

将 lol/blockdev.md 中内容放到这里来吧!

```c
		struct gendisk *disk = bio->bi_bdev->bd_disk;
```
bio 初始化:
```txt
#0  bio_init (opf=<optimized out>, max_vecs=<optimized out>, table=<optimized out>, bdev=<optimized out>, bio=<optimized out>) at block/bio.c:284
#1  bio_alloc_bioset (bdev=bdev@entry=0xffff888105153c00, nr_vecs=<optimized out>, nr_vecs@entry=32, opf=opf@entry=0, gfp_mask=<optimized out>, gfp_mask@entry=3264, bs=0xffffffff83a1bba0 <fs_bio_set>) at block/bio.c:567
#2  0xffffffff81530a6e in bio_alloc (gfp_mask=3264, opf=0, nr_vecs=32, bdev=0xffff888105153c00) at ./include/linux/bio.h:427
#3  ext4_mpage_readpages (inode=0xffff888106af3c78, rac=<optimized out>, folio=<optimized out>) at fs/ext4/readpage.c:362
```

在 ext4_mpage_readpages 中:
```c
	    struct block_device *bdev = inode->i_sb->s_bdev;

			bio = bio_alloc(bdev, bio_max_segs(nr_pages),
					REQ_OP_READ, GFP_KERNEL);
```
通过 inode 找到 superblock，

## [ ] 确认一下，一个 partion 一个 blockdev 还是 一个 disk 一个 blockdev

感觉应该应该是一个 blockdev 一个
```c
struct block_device_operations {
	void (*submit_bio)(struct bio *bio);
	int (*poll_bio)(struct bio *bio, struct io_comp_batch *iob,
			unsigned int flags);
```
