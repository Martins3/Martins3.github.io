## 同步模型

### 常见的 lock 收集下


```c
struct mddev {

	/* 'open_mutex' avoids races between 'md_open' and 'do_md_stop', so
	 * that we are never stopping an array while it is open.
	 * 'reconfig_mutex' protects all other reconfiguration.
	 * These locks are separate due to conflicting interactions
	 * with disk->open_mutex.
	 * Lock ordering is:
	 *  reconfig_mutex -> disk->open_mutex
	 *  disk->open_mutex -> open_mutex:  e.g. __blkdev_get -> md_open
	 */
	struct mutex			open_mutex;
	struct mutex			reconfig_mutex;


	/* "lock" protects:
	 *   flush_bio transition from NULL to !NULL
	 *   rdev superblocks, events
	 *   clearing MD_CHANGE_*
	 *   in_sync - and related safemode and MD_CHANGE changes
	 *   pers (also protected by reconfig_mutex and pending IO).
	 *   clearing ->bitmap
	 *   clearing ->bitmap_info.file
	 *   changing ->resync_{min,max}
	 *   setting MD_RECOVERY_RUNNING (which interacts with resync_{min,max})
	 */
	spinlock_t			lock;
	wait_queue_head_t		sb_wait;	/* for waiting on superblock updates */
	atomic_t			pending_writes;	/* number of active superblock writes */
```

此外，raid1 中也是存在 lock 的

```c
struct r1conf {

	spinlock_t		device_lock;

	spinlock_t		resync_lock;
```

### reconfig_mutex

```c
static inline int __must_check mddev_lock(struct mddev *mddev)
{
	return mutex_lock_interruptible(&mddev->reconfig_mutex);
}

/* Sometimes we need to take the lock in a situation where
 * failure due to interrupts is not acceptable.
 */
static inline void mddev_lock_nointr(struct mddev *mddev)
{
	mutex_lock(&mddev->reconfig_mutex);
}

static inline int mddev_trylock(struct mddev *mddev)
{
	return mutex_trylock(&mddev->reconfig_mutex);
}
```

我是真的不知道这种错误是怎么被找出来的:
https://git.kernel.org/pub/scm/linux/kernel/git/song/md.git/commit/?h=md-next&id=c4fe7edfc73f750574ef0ec3eee8c2de95324463

1. normal io is waiting for updating superblock
```c
	/* A separate list of r1bio which just need raid_end_bio_io called.
	 * This mustn't happen for writes which had any errors if the superblock
	 * needs to be written.
	 */
	struct list_head	bio_end_io_list;
```
当存在 normal io 挂到了 bio_end_io_list，然后又在修改 r1bio ，那么就需要一直等待，直到 superblock 被修改了

2. sync thread is waiting for normal io : 通过 barrier 机制
3. drop 'reconfig_mutex' is waiting for sync thread : 如果向 sync_action 接口写入数值，那么就开始需要等待 resync 结束了，此时持有了 reconfig_mutex
如果 rsync 无法结束，那么 sync_action 总是无法结束。
4. md thread can't update superblock : 但是发现 md thread 如果无法持有 reconfig_mutex ，那么就导致无法更新 superblock

这种死锁也能发生，真的 nb 哇。

1. 持有 reconfig_mutex ，等待 fail io
	1. 其实是在等待 resync
	2. resync 等待 normal io
	3. normal io 在 raid1d() 中根本无法修改

### mddev_trylock
存在特别多的地方，如果 mddev_trylock 失败，那么直接什么都不做，这不会导致问题吗？

## rcu 是在保护谁的?

`raid1_status`

`print_conf`

似乎是这里的:
```c
rcu_assign_pointer(p->rdev, rdev);
```
`raid1_spare_active`

`raid1_remove_disk`

保护的 rdev ，而不是 mirror ，那么很遗憾

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
