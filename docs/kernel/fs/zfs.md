# zfs
## https://github.com/openzfs/zfs

## 先用起来
- https://nixos.wiki/wiki/ZFS

参考:
- https://ubuntu.com/tutorials/setup-zfs-storage-pool#1-overview

这样的话就太酷了：
```txt
sudo zpool create new-pool /dev/vda /dev/vdb /dev/nvme1n1
```
原来 zfs 自带 raid 的功能，真的是全新的思考哇!

似乎 nixos 重启之后就消失了。

https://docs.freebsd.org/en/books/handbook/zfs/

https://news.ycombinator.com/item?id=37387392

https://news.ycombinator.com/item?id=41536088

## A detailed guide to OpenZFS - Understanding important ZFS concepts to help with system design and administration

very nice 的教程；
https://www.reddit.com/r/zfs/comments/107gz3u/a_detailed_guide_to_openzfs_understanding/

https://jro.io/truenas/openzfs/

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
