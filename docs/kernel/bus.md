https://www.bilibili.com/video/BV1im411Q73m


bus_add_device 中 pr_debug 打开，
可以看到这些，workqueue 也是 bus ，只能说 bus 是一个通用概念


```txt
[    1.090460] bus: 'virtio': add device virtio0
[    1.094849] bus: 'virtio': add device virtio1
[    1.099812] bus: 'virtio': add device virtio2
[    1.104121] bus: 'virtio': add device virtio3
[    1.108670] bus: 'virtio': add device virtio4
[    1.112337] bus: 'virtio': add device virtio5
[    1.114722] bus: 'virtio': add device virtio6
[    1.119198] bus: 'virtio': add device virtio7
[    1.123044] bus: 'virtio': add device virtio8
[    3.405868] bus: 'workqueue': add device nvme-wq
[    3.406305] bus: 'workqueue': add device nvme-reset-wq
[    3.426116] bus: 'workqueue': add device nvme-delete-wq
[    3.721514] bus: 'workqueue': add device scsi_tmf_0
[    3.731699] bus: 'scsi': add device host0
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
