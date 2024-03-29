# seq_file.c

## [内核文档](https://www.kernel.org/doc/html/next/filesystems/seq_file.html)

文档写的有点复杂，简单来说:
1. next 移动指针，show 展示内容，让内核可以方面的保留出来一个数组或者链表之类的东西到用户态空间。


1. 这里各种东西的辅助函数，似乎划分为 file operations 和 seq hlist 两个类别的。
2. struct seq_file 如何初始化的 ? 谁来使用

## seq_read
1. seq_read is generic function which take advantage of seq_operations
    1. seq_operations is register at seq_open
    2. here is general calling graph:
        1. vfs -> debugfs_kprobes_operations::open -> seq_open -> register kprobes_seq_ops
        2. vfs -> debugfs_kprobes_operations::read -> seq_read -> kprobes_seq_ops::show_kprobe_addr

## seq_open : seq_open is simple, but it's comment describe how seq_operations function

```c
/**
 *	seq_open -	initialize sequential file
 *	@file: file we initialize
 *	@op: method table describing the sequence
 *
 *	seq_open() sets @file, associating it with a sequence described
 *	by @op.  @op->start() sets the iterator up and returns the first
 *	element of sequence. @op->stop() shuts it down.  @op->next()
 *	returns the next element of sequence.  @op->show() prints element
 *	into the buffer.  In case of error ->start() and ->next() return
 *	ERR_PTR(error).  In the end of sequence they return %NULL. ->show()
 *	returns 0 in case of success and negative number in case of error.
 *	Returning SEQ_SKIP means "discard this element and move on".
 *	Note: seq_open() will allocate a struct seq_file and store its
 *	pointer in @file->private_data. This pointer should not be modified.
 */
int seq_open(struct file *file, const struct seq_operations *op)
```

## core struct
```c
struct seq_file {
	char *buf;
	size_t size;
	size_t from;
	size_t count;
	size_t pad_until;
	loff_t index;
	loff_t read_pos;
	u64 version;
	struct mutex lock;
	const struct seq_operations *op;
	int poll_event;
	const struct file *file;
	void *private;
};

struct seq_operations {
	void * (*start) (struct seq_file *m, loff_t *pos);
	void (*stop) (struct seq_file *m, void *v);
	void * (*next) (struct seq_file *m, void *v, loff_t *pos);
	int (*show) (struct seq_file *m, void *v);
};
```

## seq_puts : seq_file maintains a buffer, seq_read will copy the buffer to userland !

```c
void seq_puts(struct seq_file *m, const char *s)
{
	int len = strlen(s);

	if (m->count + len >= m->size) {
		seq_set_overflow(m);
		return;
	}
	memcpy(m->buf + m->count, s, len);
	m->count += len;
}
EXPORT_SYMBOL(seq_puts);
```

## 记录一个小问题
```c
static int show_partition(struct seq_file *seqf, void *v)
{
	struct gendisk *sgp = v;
	struct block_device *part;
	unsigned long idx;

	if (!get_capacity(sgp) || (sgp->flags & GENHD_FL_HIDDEN))
		return 0;

	rcu_read_lock();
	xa_for_each(&sgp->part_tbl, idx, part) {
		if (!bdev_nr_sectors(part))
			continue;
		seq_printf(seqf, "%4d  %7d %10llu %pg\n",
			   MAJOR(part->bd_dev), MINOR(part->bd_dev),
			   bdev_nr_sectors(part) >> 1, part);
	}
	rcu_read_unlock();
	return 0;
}
```

为什么 %pg 是输出 partion 的 name ，而且 part 也不是 name 啊
```txt
major minor  #blocks  name

 259        0 1000204632 nvme1n1
 259        1 1000203264 nvme1n1p1
 259        2 1000204632 nvme0n1
 259        3  984080384 nvme0n1p1
 259        4   15624192 nvme0n1p2
 259        5     498688 nvme0n1p3
   8        0 1953514584 sda
   8        1 1953513472 sda1
```
