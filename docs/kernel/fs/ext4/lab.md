## dumpe2fs

```txt
🧀  sudo dumpe2fs -h  /dev/mapper/openeuler-home
dumpe2fs 1.47.0 (5-Feb-2023)
Filesystem volume name:   <none>
Last mounted on:          /home
Filesystem UUID:          8f296976-c0a9-4d9b-9bcf-87adbf31702c
Filesystem magic number:  0xEF53
Filesystem revision #:    1 (dynamic)
Filesystem features:      has_journal ext_attr resize_inode dir_index filetype needs_recovery extent 64bit flex_bg sparse_super large_file huge_file dir_nlink extra_isize metadata_csum
Filesystem flags:         signed_directory_hash
Default mount options:    user_xattr acl
Filesystem state:         clean
Errors behavior:          Continue
Filesystem OS type:       Linux
Inode count:              7905280
Block count:              31596544
Reserved block count:     1579827
Overhead clusters:        642697
Free blocks:              25268284
Free inodes:              7841947
First block:              0
Block size:               4096
Fragment size:            4096
Group descriptor size:    64
Reserved GDT blocks:      1024
Blocks per group:         32768
Fragments per group:      32768
Inodes per group:         8192
Inode blocks per group:   512
Flex block group size:    16
Filesystem created:       Fri Jan 10 12:57:09 2025
Last mount time:          Thu Jan  1 08:00:10 1970
Last write time:          Thu Jan  1 08:00:10 1970
Mount count:              328
Maximum mount count:      -1
Last checked:             Fri Jan 10 12:57:09 2025
Check interval:           0 (<none>)
Lifetime writes:          5662 GB
Reserved blocks uid:      0 (user root)
Reserved blocks gid:      0 (group root)
First inode:              11
Inode size:               256
Required extra isize:     32
Desired extra isize:      32
Journal inode:            8
Default directory hash:   half_md4
Directory Hash Seed:      a29de8e6-8986-4228-bb69-fdf86e8be41e
Journal backup:           inode blocks
Checksum type:            crc32c
Checksum:                 0x904b214f
Journal features:         journal_incompat_revoke journal_64bit journal_checksum_v3
Total journal size:       512M
Total journal blocks:     131072
Max transaction length:   131072
Fast commit length:       0
Journal sequence:         0x00005dd9
Journal start:            119226
Journal checksum type:    crc32c
Journal checksum:         0x02a3fad1
```

类似的:
sudo tune2fs -l /dev/mapper/openeuler-home

## 使用文档
Documentation/admin-guide/ext4.rst
  - https://www.kernel.org/doc/html/latest/admin-guide/ext4.html


## https://github.com/tytso/e2fsprogs
中的 fuse 如何理解?

如何，原来现在就是有这个工具了
fuse2fs

## 这个提升都是在 buffer io ，为什么啊?
https://www.phoronix.com/news/EXT4-BS-Greater-Than-PS

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
