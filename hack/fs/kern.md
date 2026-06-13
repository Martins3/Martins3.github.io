- [ ] it's user : cgroup

```c
struct kernfs_ops {
	/*
	 * Optional open/release methods.  Both are called with
	 * @of->seq_file populated.
	 */
	int (*open)(struct kernfs_open_file *of);
	void (*release)(struct kernfs_open_file *of);

	/*
	 * Read is handled by either seq_file or raw_read().
	 *
	 * If seq_show() is present, seq_file path is active.  Other seq
	 * operations are optional and if not implemented, the behavior is
	 * equivalent to single_open().  @sf->private points to the
	 * associated kernfs_open_file.
	 *
	 * read() is bounced through kernel buffer and a read larger than
	 * PAGE_SIZE results in partial operation of PAGE_SIZE.
	 */
	int (*seq_show)(struct seq_file *sf, void *v);

	void *(*seq_start)(struct seq_file *sf, loff_t *ppos);
	void *(*seq_next)(struct seq_file *sf, void *v, loff_t *ppos);
	void (*seq_stop)(struct seq_file *sf, void *v);

```

- [ ] It seems there is another libfs provide `seq_next`
  - Oh yes, it's /home/maritns3/core/linux/fs/seq_file.c


- [ ] kernfs_open_file : oh shit, the fucking kernfs
