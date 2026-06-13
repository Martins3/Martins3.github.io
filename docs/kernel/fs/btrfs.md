# btrfs

## Readings
[^10] 的教程阅读，逐个分析:

btrfs overview:[^1]
More recent filesystems (including ext4) use pointers to "extents" instead; each extent is a group of contiguous blocks.

新特性:
1. Copy-on-rite
    1. snapshots
    2. does not need to implement a separate journal to provide crash resistance.
2. Another important Btrfs feature is its built-in volume manager.
A Btrfs filesystem can span multiple physical devices in a number of RAID configurations. Any given volume (collection of one or more physical drives) can also be split into "subvolumes," which can be thought of as independent filesystems sharing a single physical volume set. So Btrfs makes it possible to group part or all of a system's storage into a big pool, then share that pool among a set of filesystems, each with its own usage limits.

> TODO volume 是啥 ?
3. It can perform full checksummng of both data and metadata, making it robust in the face of data corruption by the hardware.
4. Data can be stored on-disk in compressed form.
5. The send/receive feature can be used as part of an incremental backup scheme, among other things.
6. The online defragmentation mechanism can fix up fragmented files in a running filesystem.
7. The 3.12 kernel saw the addition of an offline de-duplication feature; it scans for blocks containing duplicated data and collapses them down to a single, shared copy.

Cow 的问题:
Obviously, some sort of garbage collection is required or all those block copies will quickly eat up all of the available space on the filesystem. Copying blocks can take more time than simply overwriting them as well as significantly increasing the filesystem's memory requirements. COW operations will also have a tendency to fragment files, wrecking the nice, contiguous layout that the filesystem code put so much effort into creating.

btrfs on RAID : https://lwn.net/Articles/577961/

[^1]: https://lwn.net/Articles/576276/
[^10]: https://lwn.net/Kernel/Index/#Btrfs-LWNs_guide_to



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
