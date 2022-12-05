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

而 balloon 的大小是从:
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
