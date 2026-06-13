# vduse

https://www.redhat.com/en/blog/introducing-vduse-software-defined-datapath-virtio

- [ ] 为什么会和 vDPA 有关系啊?

https://www.phoronix.com/news/Linux-6.4-Faster-VDUSE
看看如何提升的?

https://docs.kernel.org/userspace-api/vduse.html : 用户态接口

## 看看这个如何使用了
https://stefano-garzarella.github.io/posts/2024-02-12-vdpa-blk/

使用这个 device 中的:
```txt
vdpa dev add name vduse0 mgmtdev vduse
driverctl -b vdpa set-override vduse0 virtio_vdpa
```

## 所谓的 vdpa-sim-blk 应该只是 vduse 的简单模拟吧


还是这些:
```c
static void vduse_dev_irq_inject(struct work_struct *work)
{
	struct vduse_dev *dev = container_of(work, struct vduse_dev, inject);

	spin_lock_bh(&dev->irq_lock);
	if (dev->config_cb.callback)
		dev->config_cb.callback(dev->config_cb.private);
	spin_unlock_bh(&dev->irq_lock);
}

static void vduse_vq_irq_inject(struct work_struct *work)
{
	struct vduse_virtqueue *vq = container_of(work,
					struct vduse_virtqueue, inject);

	spin_lock_bh(&vq->irq_lock);
	if (vq->ready && vq->cb.callback)
		vq->cb.callback(vq->cb.private);
	spin_unlock_bh(&vq->irq_lock);
}
```

## vduse 有 zero copy 吗?

应该是没有的，将 pages 拷贝到 vduse_dev_reg_umem 这里

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
