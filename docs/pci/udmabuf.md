## qemu 中就是支持 udmabuf 的
hw/display/virtio-gpu-udmabuf.c
contrib/vhost-user-gpu/vugbm.c
hw/display/virtio-dmabuf.c

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```txt
# 传统路径
用户态 malloc() → 拷贝 → 内核 buffer → DMA → 设备

# udmabuf 的路径
用户态 malloc() → udmabuf → dma-buf → 设备 DMA
                ↑          ↑
            无拷贝     跨子系统共享
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
