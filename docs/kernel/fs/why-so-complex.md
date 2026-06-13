# 为什么设计一个文件系统是很难的

## 基本考虑
锁机制
缓存
多进程

## 基本功能
1. O_DIRECT
2. fs freeze 一致性快照
3. journal 错误恢复
4. 容忍磁盘错误
5. 软硬链接
6. overlay

## 高级功能
1. bcache
2. raid : zfs 的 raid 相较于 linux 中 ext4 + raid 有什么好处吗?
3. cow
5. 压缩
6. 加密
8. snapshot
6. 动态扩容，缩容

## 操作手册
- https://bcachefs.org/bcachefs-principles-of-operation.pdf

## 想象
zfs 和 bcachefs 的分析中，才发现很多 block layer 的功能都是集成到文件系统中。

## 看看 https://bcachefs.org/ 的 feature 和 plans 就很牛逼了

1. reflink : 俗称 cow ，但是似乎也不是 cow
https://unix.stackexchange.com/questions/393305/does-any-file-system-implement-copy-on-write-mechanism-for-cp

- https://bcachefs.org/Wishlist/
- https://bcachefs.org/Roadmap/

man ext4(5) 也是不错的

## 有趣的 fs
- nilfs : 连续 https://news.ycombinator.com/item?id=33162259
  - https://dataswamp.org/~solene/2022-10-05-linux-nilfs-filesystem.html

## bcachefs 这也太慢了吧!

```txt
🧀  fio /home/martins3/core/vn/docs/kernel/code/aio/bcachefs.fio
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=io_uring, iodepth=128
fio-3.36
Starting 1 process
trash: Laying out IO file (1 file / 10240MiB)
Jobs: 1 (f=1): [r(1)][1.6%][r=1172KiB/s][r=293 IOPS][eta 16m:24s]
```

真实运行的时候也太慢了，哈哈!

```txt
🧀  fio /home/martins3/core/vn/docs/kernel/code/aio/bcachefs.fio
trash: (g=0): rw=randread, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=io_uring, iodepth=128
fio-3.36
Starting 1 process
kJobs: 1 (f=1): [r(1)][5.9%][r=504KiB/s][r=126 IOPS][eta 15m:41s]s]
```

但是，实际上 grep 之类的，速度还是很快的!


## 几个虚拟机文件系统让事情变的有趣

initramfs nofs 之类的

## 工业环境中几个文件系统让事情真的难受起来了

- fuse
	- virtiofs
- nfs
- overlayfs


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
