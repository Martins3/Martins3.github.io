# fs

## virtual fs
各种 fs 的区别内核文档[^1]，这个 blog 已经分析过一次[^2]，我就不重复了，下面是我的总结。

| fs        | explanation                                                                       |
|-----------|-----------------------------------------------------------------------------------|
| ramdisk   | 使用 ram 模拟 disk                                                                |
| ramfs     | ramdisk 的改进，大小可以伸缩，无需 page cache 缓存                                |
| tmpfs     | 基于 ramfs 的 virtual fs, fs 一般的存储介质是 disk / ssd，tmpfs 的存储介质是内存  |
| rootfs    | 系统启动的时候的临时 fs, 用于挂载 real root fs                                    |
| initrd    | kernel 启动的时候，可以指定 rootfs 中存储的内容，从而在 mount real root fs 之前搞 |
| initramfs | initrd 的改进版本                                                                 |

## 这里空说，实际上难以感受在说啥
将 hack/qemu/bare-metal/ 下的内容整理一下吧！

[^1]: http://junyelee.blogspot.com/2020/03/ramfs-rootfs-and-initramfs.html
[^2]: https://docs.kernel.org/filesystems/ramfs-rootfs-initramfs.html
