## configfs
https://www.kernel.org/doc/html/latest/filesystems/configfs.html

```txt
Symbol: BLK_DEV_NULL_BLK [=n]
Type  : tristate
Defined at drivers/block/null_blk/Kconfig:6
  Prompt: Null test block driver
  Depends on: BLK_DEV [=y]
  Location:
    -> Device Drivers
      -> Block devices (BLK_DEV [=y])
(1)     -> Null test block driver (BLK_DEV_NULL_BLK [=n])
Selects: CONFIGFS_FS [=y]
```
mkdir /config/fakenbd/disk1
sudo modprobe null_blk 之后会出现 /sys/kernel/config/nullb

在 /sys/kernel/config/nullb 中 mkdir m ，然后在 /dev 中会出现一个 m

## 看看这个讨论
https://lore.kernel.org/all/Yl3aQQtPQvkskXcP@localhost.localdomain/

他吐槽，既然默认构建了一个 /dev/nullb0 ，那么为什么 /sys/kernel/config 是没有的

不过也算是知道了 lwn 中介绍的，为什么 configfs 是用户态来创建 kobject 了。

## samples/configfs/configfs_sample.c
先看看这个

## 了解下 configfs 的使用，在 /sys/kernel/config 中

https://lwn.net/Articles/148973/

> It provides a view into the kernel's data structures,
> and it can be used to cause things to happen with those structures.
> But sysfs cannot be used to create new objects - at least, not without distorting the interface somewhat.
> It is the wrong tool for this job.


有趣的
```txt
🤒  sudo targetcli
b'modprobe: FATAL: Module configfs not found in directory /lib/modules/6.13.2\n'
```

## 其实 configfs 的依赖不少
```txt
# CONFIG_OCFS2_FS is not set
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
