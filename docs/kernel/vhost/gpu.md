## vhost gpu 的实现
<!-- ccd0d5a6-ca36-4c99-9a03-1fead9b36e27 -->

vhost/vhost-user 图形后端里，最经典、历史上最有代表性的项目是 virglrenderer。
在 QEMU 语境里，典型组合是：

- 前端：virtio-gpu / vhost-user-gpu
- 后端渲染库：virglrenderer
- 一个经典后端实现：contrib/vhost-user-gpu (太强了，才发现这个目录，里面有很多东西的经典实现)


rutabaga_gfx + gfxstream 之类的东西，可以仔细看看 QEMU 的文档:
docs/system/devices/virtio/virtio-gpu.rst

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
