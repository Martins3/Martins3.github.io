# 为什么 Linux Kernel 的代码质量比 QEMU 更高

## hmp 和 qmp 的混合使用

例如 balloon 中，统计采集数据必须使用:
```c
static void virtio_balloon_instance_init(Object *obj)
{
    VirtIOBalloon *s = VIRTIO_BALLOON(obj);

    qemu_mutex_init(&s->free_page_lock);
    qemu_cond_init(&s->free_page_cond);
    s->free_page_hint_cmd_id = VIRTIO_BALLOON_FREE_PAGE_HINT_CMD_ID_MIN;
    s->free_page_hint_notify.notify = virtio_balloon_free_page_hint_notify;

    object_property_add(obj, "guest-stats", "guest statistics",
                        balloon_stats_get_all, NULL, NULL, NULL);

    object_property_add(obj, "guest-stats-polling-interval", "int",
                        balloon_stats_get_poll_interval,
                        balloon_stats_set_poll_interval,
                        NULL, NULL);
}
```

而 balloon 的大小是使用 hmp:
```c
void hmp_info_balloon(Monitor *mon, const QDict *qdict)
{
    BalloonInfo *info;
    Error *err = NULL;

    info = qmp_query_balloon(&err);
    if (hmp_handle_error(mon, err)) {
        return;
    }

    monitor_printf(mon, "balloon: actual=%" PRId64 "\n", info->actual >> 20);

    qapi_free_BalloonInfo(info);
}
```

## qobj 正确声明居然依赖头文件的顺序，而且格式化之后，头文件顺序被修改，编译直接出错


## 这是错觉
因为阅读的代码只是 kernel 中的核心部分的代码，
那些被 Linus 称之为需要被 rust 重写的代码，我的妈呀，
辣眼睛。

### raid 1

struct md_rdev::sb_loaded 被赋值为 0 1 2 ，
直接看代码可以知道是什么吗?

0 : 未初始化
1 : clean
2 : dirty

大量的这种多个 if else 的判断堆在一起，根本搞不清楚在搞什么
```c
static void sync_sbs(struct mddev *mddev, int nospares)
{
	/* Update each superblock (in-memory image), but
	 * if we are allowed to, skip spares which already
	 * have the right event counter, or have one earlier
	 * (which would mean they aren't being marked as dirty
	 * with the rest of the array)
	 */
	struct md_rdev *rdev;
	rdev_for_each(rdev, mddev) {
		if (rdev->sb_events == mddev->events ||
		    (nospares &&
		     rdev->raid_disk < 0 &&
		     rdev->sb_events+1 == mddev->events)) {
			/* Don't update this superblock */
			rdev->sb_loaded = 2;
		} else {
			sync_super(mddev, rdev);
			rdev->sb_loaded = 1;
		}
	}
}
```

基本上是完全重复的:
```txt
	/* not spare disk */
	if (rdev->desc_nr >= 0 && rdev->desc_nr < le32_to_cpu(sb->max_dev) &&
	    (le16_to_cpu(sb->dev_roles[rdev->desc_nr]) < MD_DISK_ROLE_MAX ||
	     le16_to_cpu(sb->dev_roles[rdev->desc_nr]) == MD_DISK_ROLE_JOURNAL))
		spare_disk = false;
```

```txt
		/* Insist of good event counter while assembling, except for
		 * spares (which don't need an event count).
		 * Similar to mdadm, we allow event counter difference of 1
		 * from the freshest device.
		 */
		if (rdev->desc_nr >= 0 &&
		    rdev->desc_nr < le32_to_cpu(sb->max_dev) &&
		    (le16_to_cpu(sb->dev_roles[rdev->desc_nr]) < MD_DISK_ROLE_MAX ||
		     le16_to_cpu(sb->dev_roles[rdev->desc_nr]) == MD_DISK_ROLE_JOURNAL))
			if (ev1 + 1 < mddev->events)
				return -EINVAL;
```

诡异的硬编码:
```txt
static int super_1_validate(struct mddev *mddev, struct md_rdev *freshest, struct md_rdev *rdev)
		mddev->max_disks =  (4096-256)/2;
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
