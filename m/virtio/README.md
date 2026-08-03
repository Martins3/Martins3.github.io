## virtio-dummy 说明

内核参考:

- Documentation/driver-api/virtio/writing_virtio_drivers.rst
- virtio-blk

QEMU 参考:

- drivers/virtio/virtio_balloon.c

## 我是没有想到，insmod 的过程中，就是需要自动读取 sector 啊

### 如果 qemu 没有携带参数 dummy 设备，insmod 会在哪里失败?

## virtqueue_kick 和 virtqueue_notify 有啥区别 ?

## 有点好奇，为什么什么都不修改，但是 write 和 read 都是可以正确返回的

bio 和 request 都是怎么维持生活的


## 修改 virtio-dummy 在 qemu 的实现，让起后端的确指向一个文件或者内存

从而 simplefs 的后端是 virtio-dummy ，那么整个链路都是清楚的

1. 使用 dev-dummy 测试 udev 是如何自动加载的。
2. 使用 perf 观测硬中断可以吗?
3. 让 qemu 支持延迟 reply ，测试 inflight

## tagset 的大小是 queue depth * queue 的数量决定的吗?

tagset 的大小如何看?

## 只有一件事情，就是 inflight 是不是就是
blk_mq_start_request 到 blk_mq_start_request 之前的

## 测试一下 bio_split 和 bio_chain 的功能

```txt
		struct bio *split = bio_split(bio, max_sectors,
					      gfp, &conf->bio_split);

		if (IS_ERR(split)) {
			error = PTR_ERR(split);
			goto err_handle;
		}
		bio_chain(split, bio);
```

## 还是需要支持 io 的，支持之后，
将盘都制作一下 make partition 试试
其实支持 io 之后，可以用内存，也可以使用文件，应该是很容易的了


## 逆天方法，看看如何新

```c
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/blkdev.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>

static int __init find_vda_init(void)
{
    struct class *block_class;
    struct device *dev;
    struct gendisk *disk = NULL;
    bool found = false;

    printk(KERN_INFO "Searching for gendisk of /dev/vda...\n");

    /* 获取 block 设备类 */
    block_class = class_find_by_name("block");
    if (!block_class) {
        printk(KERN_ERR "Cannot find block class\n");
        return -ENODEV;
    }

    /* 遍历 block 类下的所有设备 */
    spin_lock(&block_class->p->devices_kset->list_lock);
    list_for_each_entry(dev, &block_class->p->devices_kset->list, kobj.entry) {
        const char *devname = dev_name(dev);

        if (strcmp(devname, "vda") == 0) {
            /* 获取 gendisk 指针 */
            disk = dev_to_disk(dev);
            if (disk) {
                printk(KERN_INFO "Found gendisk for vda: major=%d, first_minor=%d, minors=%d, disk_name=%s\n",
                       disk->major, disk->first_minor, disk->minors, disk->disk_name);
                found = true;
                break;
            }
        }
    }
    spin_unlock(&block_class->p->devices_kset->list_lock);

    if (!found) {
        printk(KERN_WARNING "Device vda not found in block class\n");
        return -ENODEV;
    }

    return 0;
}

static void __exit find_vda_exit(void)
{
    printk(KERN_INFO "Module unloaded.\n");
}

module_init(find_vda_init);
module_exit(find_vda_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("A module to find gendisk of /dev/vda");
```


## 

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
