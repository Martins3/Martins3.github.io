## fsfreeze
https://man7.org/linux/man-pages/man8/fsfreeze.8.html

> fsfreeze is unnecessary for device-mapper devices. The device-mapper (and LVM) automatically freezes a filesystem on the
device when a snapshot creation is requested.

这个如何理解 lvm 可以实现文件系统的 freeze 啊

```sh
sudo fsfreeze -f /mnt
sudo fsfreeze -u /mnt
```

如果只是用的 sb_start_write
这么看，fsfreeze 是不需要特殊支持的，只是阻塞所有的 io ，
但是需要等到 fs 的操作

fsfreeze 需要注意一下顺序啊，例如这样的结果，这样的顺序也是很难构造了
```sh
mkfs.xfs /dev/vdb
sudo mount /dev/vdb /mnt2

img=/mnt2/loop.img
dd if=/dev/zero of=$img bs=1M count=50
mkfs.ext4 $img

mkdir -p /mnt2/share
sudo mount -o loop,rw  $img /mnt2/share
sudo chown martins3 /mnt2/share

sudo fsfreeze -f /mnt2
sudo fsfreeze -f /mnt2/share
```



## xfs 存在 fsfreeze 的问题
- https://bugzilla.kernel.org/show_bug.cgi?id=205833

- https://news.ycombinator.com/item?id=16494370

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
