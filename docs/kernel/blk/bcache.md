## bcache
- https://docs.kernel.org/admin-guide/bcache.html

## 先使用起来
https://wiki.archlinux.org/title/Bcache

echo 1 | sudo tee /sys/block/sda/bcache/stop # 重新转回来
echo 1 | sudo tee /sys/block/sdb/bcache/stop # 重新转回来
sudo wipefs -a /dev/sda
sudo wipefs -a /dev/sdb

sudo make-bcache --block 4k -B /dev/sdb --wipe-bcache
sudo make-bcache -C  /dev/sda --wipe-bcache

sudo bcache-super-show /dev/sda | grep cset.uuid

echo 87e5a730-11be-41d8-a2a7-29f7df10f97b  | sudo tee /sys/block/bcache0/bcache/attach
sudo mkfs.xfs /dev/bcache0

修改默认的模式:
echo writeback | sudo tee /sys/block/bcache0/bcache/cache_mode


1. /sys/block/sda/bcache/stop 只是 bcache 的状态
cache 似乎是存在自己的 stop 的方法的。

```txt
[259908.001200] bcache: bcache_device_free() bcache0 stopped
[259950.530302] bcache: register_bdev() registered backing device sdb
[260030.869901] bcache: __cached_dev_store() Can't attach 66384716-2bc3-404b-bbee-a8211a713380
                : cache set not found
[260042.477131] bcache: __cached_dev_store() Can't attach 66384716-2bc3-404b-bbee-a8211a713380
                : cache set not found
[260072.234224] bcache: __cached_dev_store() Can't attach 66384716-2bc3-404b-bbee-a8211a713380
                : cache set not found
[260105.878972] bcache: __cached_dev_store() Can't attach 66384716-2bc3-404b-bbee-a8211a713380
                : cache set not found
[260133.290381] bcache: __cached_dev_store() Can't attach 66384716-2bc3-404b-bbee-a8211a713380
                : cache set not found
[260168.929476] bcache: __cached_dev_store() Can't attach 66384716-2bc3-404b-bbee-a8211a713380
                : cache set not found
[260178.013369] bcache: __cached_dev_store() Can't attach 66384716-2bc3-404b-bbee-a8211a713380
                : cache set not found
```

sudo make-bcache --block 4k --bucket 2M -C  /dev/sda
```txt
[260640.597322] bcache: bch_cached_dev_attach() Couldn't attach sdb: block size less than set's block size
[260656.040193] bcache: bch_cached_dev_attach() Couldn't attach sdb: block size less than set's block size
[260665.535687] bcache: bch_cached_dev_attach() Couldn't attach sdb: block size less than set's block size
```

## 基本流程


## 关闭 bcache 并不容易哇
- https://unix.stackexchange.com/questions/225017/how-to-remove-bcache0-volume

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
