# 下降到达最深层

其实就是第六章中间的block layer 就可以了

```c
/**
 * bdev_write_page() - Start writing a page to a block device
 * @bdev: The device to write the page to
 * @sector: The offset on the device to write the page to (need not be aligned)
 * @page: The page to write
 * @wbc: The writeback_control for the write
 *
 * On entry, the page should be locked and not currently under writeback.
 * On exit, if the write started successfully, the page will be unlocked and
 * under writeback.  If the write failed already (eg the driver failed to
 * queue the page to the device), the page will still be locked.  If the
 * caller is a ->writepage implementation, it will need to unlock the page.
 *
 * Errors returned by this function are usually "soft", eg out of memory, or
 * queue full; callers should try a different route to write this page rather
 * than propagate an error back up the stack.
 *
 * Return: negative errno if an error occurs, 0 if submission was successful.
 */
int bdev_write_page(struct block_device *bdev, sector_t sector,
			struct page *page, struct writeback_control *wbc)
{
	int result;
	const struct block_device_operations *ops = bdev->bd_disk->fops;

	if (!ops->rw_page || bdev_get_integrity(bdev))
		return -EOPNOTSUPP;
	result = blk_queue_enter(bdev->bd_queue, 0);
	if (result)
		return result;

	set_page_writeback(page);
	result = ops->rw_page(bdev, sector + get_start_sect(bdev), page,
			      REQ_OP_WRITE);
	if (result) {
		end_page_writeback(page);
	} else {
		clean_page_buffers(page);
		unlock_page(page);
	}
	blk_queue_exit(bdev->bd_queue);
	return result;
}
EXPORT_SYMBOL_GPL(bdev_write_page);


/**
 * submit_bio - submit a bio to the block device layer for I/O
 * @bio: The &struct bio which describes the I/O
 *
 * submit_bio() is very similar in purpose to generic_make_request(), and
 * uses that function to do most of the work. Both are fairly rough
 * interfaces; @bio must be presetup and ready for I/O.
 *
 */
blk_qc_t submit_bio(struct bio *bio)
{
	/*
	 * If it's a regular read/write or a barrier with data attached,
	 * go through the normal accounting stuff before submission.
	 */
	if (bio_has_data(bio)) {
		unsigned int count;

		if (unlikely(bio_op(bio) == REQ_OP_WRITE_SAME))
			count = queue_logical_block_size(bio->bi_disk->queue) >> 9;
		else
			count = bio_sectors(bio);

		if (op_is_write(bio_op(bio))) {
			count_vm_events(PGPGOUT, count);
		} else {
			task_io_account_read(bio->bi_iter.bi_size);
			count_vm_events(PGPGIN, count);
		}

		if (unlikely(block_dump)) {
			char b[BDEVNAME_SIZE];
			printk(KERN_DEBUG "%s(%d): %s block %Lu on %s (%u sectors)\n",
			current->comm, task_pid_nr(current),
				op_is_write(bio_op(bio)) ? "WRITE" : "READ",
				(unsigned long long)bio->bi_iter.bi_sector,
				bio_devname(bio, b), count);
		}
	}

	return generic_make_request(bio);
}
EXPORT_SYMBOL(submit_bio);
```

