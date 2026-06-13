# Virtio Balloon (QEMU)
- update_balloon_stats

## 使用方法
- info balloon
- balloon N

- 基本的流程 : guest -> virtio device
  - 问题在于，guest 以为自己的内存是足够的，是不会换出的
  - 如果让 guest 还是以为自己持有很多内存，在 GVA -> GPA 的映射是存在的，实际上，Host 已经将内存换出，此时 GPA

lspci 可以得到:
```txt
00:06.0 Unclassified device [00ff]: Red Hat, Inc. Virtio memory balloon
```
- https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/7/html/virtualization_deployment_and_administration_guide/sect-manipulating_the_domain_xml-devices#sect-Devices-Memory_balloon_device

- [ ] virtio 驱动必然又一个设置中断的吧

```c
static void virtio_balloon_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

    device_class_set_props(dc, virtio_balloon_properties);
    dc->vmsd = &vmstate_virtio_balloon;
    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    vdc->realize = virtio_balloon_device_realize;
    vdc->unrealize = virtio_balloon_device_unrealize;
    vdc->reset = virtio_balloon_device_reset;
    vdc->get_config = virtio_balloon_get_config;
    vdc->set_config = virtio_balloon_set_config;
    vdc->get_features = virtio_balloon_get_features;
    vdc->set_status = virtio_balloon_set_status;
    vdc->vmsd = &vmstate_virtio_balloon_device;
}
```

- virtio 是无需映射 mmap 的吧，既然数据都是通过 vqs 传输的

- 这几个从来不会被调用:
```txt
Num     Type           Disp Enb Address            What
3       breakpoint     keep y   0x0000555555b350d0 in virtio_balloon_receive_stats at ../hw/virtio/virtio-balloon.c:450
        breakpoint already hit 1 time
4       breakpoint     keep y   0x0000555555b34d30 in virtio_ballloon_get_free_page_hints at ../hw/virtio/virtio-balloon.c:555
5       breakpoint     keep y   0x0000555555b34890 in virtio_balloon_handle_report at ../hw/virtio/virtio-balloon.c:330
```

- 使用 memory hotplug 来实现，我这是万万没有想到的。

一共可以配置的参数:
```c
static const Property virtio_balloon_properties[] = {
    DEFINE_PROP_BIT("deflate-on-oom", VirtIOBalloon, host_features,
                    VIRTIO_BALLOON_F_DEFLATE_ON_OOM, false),
    DEFINE_PROP_BIT("free-page-hint", VirtIOBalloon, host_features,
                    VIRTIO_BALLOON_F_FREE_PAGE_HINT, false),
    DEFINE_PROP_BIT("page-poison", VirtIOBalloon, host_features,
                    VIRTIO_BALLOON_F_PAGE_POISON, true),
    DEFINE_PROP_BIT("free-page-reporting", VirtIOBalloon, host_features,
                    VIRTIO_BALLOON_F_REPORTING, false),
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
