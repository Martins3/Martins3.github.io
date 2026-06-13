# vhost

## vhost-net
![](https://www.redhat.com/cms/managed-files/2019-09-10-virtio-intro-fig2.jpg)

## vhost-user
virtio 如何 和 DPDK 工作:

- [ ] 和想想的不一样，应该是 vhost-user 应该在 qemu 中才对，dpdk 将数据组装为 virtio 格式，然后 qemu 作为 vhost 接受，最后发送给 Host 的

![](https://www.redhat.com/cms/managed-files/2019-09-20-virtio-and-dpdk-fig3.jpg)

这个图更加符合认识：
![](https://p3-juejin.byteimg.com/tos-cn-i-k3u1fbpfcp/00442fd6b19c4e9988c8e8e0c1c87914~tplv-k3u1fbpfcp-zoom-in-crop-mark:3024:0:0:0.awebp)

以及这个图:
![](https://img1.sdnlab.com/wp-content/uploads/2020/09/27/101.png)

## vDPA
![](https://www.redhat.com/cms/managed-files/2019-10-02-vdpa-figure5.jpg)

## VDUSE
![](https://www.redhat.com/cms/managed-files/styles/wysiwyg_full_width/s3/vduse-virtio-image4.png?itok=SXoYj-7L)

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
