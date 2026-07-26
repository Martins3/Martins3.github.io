# kernfs
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


# 补充资料
https://en.wikipedia.org/wiki/NVM_Express

https://nvmexpress.org/education/drivers/linux-driver-information/

[^1]:SATA uses the Advanced Host Controller Interface (AHCI) to access data. One of the most significant features brought by AHCI is Native Command Queuing (NCQ), explicitly designed to speed up mechanical hard drives and enable hot-swapping.

M.2 is a physical standard that defines the shape, dimensions, and the physical connector itself.

nvme-cli 使用的教程:
https://www.nvmedeveloperdays.com/English/Collaterals/Proceedings/2018/20181204_PRECON2_Hands.pdf

http://blog.coderhuo.tech/2020/07/18/flash_basics/ : 讲解的不错

- https://pages.cs.wisc.edu/~remzi/OSTEP/file-ssd.pdf : 似乎是一个教程中的一部分

- https://www.jinbuguo.com/storage/ssd_intro.html

https://www.youtube.com/watch?v=NtkKHhXf3V4

[^1]: https://phoenixnap.com/kb/nvme-vs-sata-vs-m-2-comparison

## sysfs 和 procfs 是使用的 kernfs 吗?

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
