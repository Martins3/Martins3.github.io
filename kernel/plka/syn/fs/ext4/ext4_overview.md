| File             | blank | comment | code | explanation                                                                                                                             |
|------------------|-------|---------|------|-----------------------------------------------------------------------------------------------------------------------------------------|
| super.c          | 632   | 692     | 4695 |                                                                                                                                         |
| inode.c          | 671   | 1525    | 4067 | @todo 看似熟悉的文件名，其实完全不知道在说什么!                                                                                         |
| extents.c        | 648   | 1319    | 3993 | 使用首尾顺序direct indirect 逐个分析描述                                                                                                |
| mballoc.c        | 639   | 1139    | 3577 | http://oenhan.com/ext4-mballoc @todo 还是非常难以理解!                                                                                  |
| namei.c          | 369   | 432     | 3059 | ext4_dir_inode_operations 和　ext4_special_inode_operations  定义，还是处理link path 之类的问题                                         |
| xattr.c          | 362   | 395     | 2380 |                                                                                                                                         |
| ext4.h           | 340   | 611     | 2294 |                                                                                                                                         |
| inline.c         | 294   | 189     | 1546 | 似乎inline 含义?                                                                                                                        |
| resize.c         | 260   | 315     | 1497 | Support for resizing an ext4 filesystem while it is mounted. This could probably be made into a module, because it is not often in use. |
| ialloc.c         | 151   | 243     | 1051 |                                                                                                                                         |
| ioctl.c          | 152   | 88      | 872  |                                                                                                                                         |
| extents_status.c | 152   | 251     | 851  | extents_status 为 extent 服务，不是很懂为什么会出现extent 内容                                                                          |
| indirect.c       | 116   | 534     | 806  | 处理indirect的内容                                                                                                                      |
| balloc.c         | 90    | 207     | 605  | 处理bitmap的相关的内容，@todo 但是具体作用不知道                                                                                        |
| fsmap.c          | 96    | 122     | 495  |                                                                                                                                         |
| dir.c            | 63    | 116     | 485  |                                                                                                                                         |
| move_extent.c    | 59    | 157     | 479  |                                                                                                                                         |
| migrate.c        | 68    | 143     | 462  |                                                                                                                                         |
| page-io.c        | 54    | 68      | 415  |                                                                                                                                         |
| sysfs.c          | 62    | 10      | 378  |                                                                                                                                         |
| file.c           | 66    | 87      | 371  | ext4_file_operations 和 ext4_file_inode_operations 两个结构体的定义                                                                     |
| mmp.c            | 57    | 64      | 275  |                                                                                                                                         |
| ext4_jbd2.h      | 68    | 123     | 271  |                                                                                                                                         |
| ext4_jbd2.c      | 53    | 35      | 244  |                                                                                                                                         |
| acl.c            | 30    | 28      | 237  |                                                                                                                                         |
| readpage.c       | 23    | 55      | 217  |                                                                                                                                         |
| hash.c           | 29    | 35      | 204  |                                                                                                                                         |
| block_validity.c | 26    | 23      | 194  |                                                                                                                                         |
| xattr.h          | 34    | 32      | 146  |                                                                                                                                         |
| mballoc.h        | 32    | 53      | 133  |                                                                                                                                         |
| ext4_extents.h   | 36    | 95      | 132  |                                                                                                                                         |
| extents_status.h | 30    | 20      | 128  |                                                                                                                                         |
| fsync.c          | 14    | 65      | 85   |                                                                                                                                         |
| bitmap.c         | 17    | 9       | 72   |                                                                                                                                         |
| acl.h            | 13    | 6       | 54   |                                                                                                                                         |
| xattr_security.c | 7     | 5       | 53   |                                                                                                                                         |
| symlink.c        | 8     | 19      | 47   |                                                                                                                                         |
| xattr_user.c     | 5     | 7       | 37   |                                                                                                                                         |
| fsmap.h          | 10    | 9       | 37   |                                                                                                                                         |
| xattr_trusted.c  | 5     | 7       | 34   |                                                                                                                                         |
| truncate.h       | 7     | 26      | 17   |                                                                                                                                         |
| Makefile         | 3     | 4       | 8    |                                                                                                                                         |

> 感觉vfs 中间实现的和此处耦合的很重

> 曾经的一个巨大的错觉，其他的fs 实现都是对称的，其实 sysfs 等其实也很重要啊 !

> simple 从database的角度看 linux io stack
https://www.postgresql.eu/events/fosdem2019/sessions/session/2346/slides/159/fosdem_linux_io.pdf

1. 总结一下 ext4 和 blk layer 以及 vfs layer 之间的关系是什么 ?
2. chicken eggs(显然ucore 已经帮我们处理好了)
    1. 通过文件系统找到对应的设备
    2. 文件系统首先需要挂载到设备上去


## http://oenhan.com/ext4-mballoc
buddy 什么情况 ?

## https://opensource.com/article/17/5/introduction-ext4-filesystem
In EXT4, data allocation was changed from fixed blocks to extents.
An extent is described by its starting and ending place on the hard drive.
This makes it possible to describe very long, physically contiguous files in a single inode pointer entry, which can significantly reduce the number of pointers required to describe the location of all the data in larger files.

EXT4 reduces fragmentation by scattering newly created files across the disk so that they are not bunched up in one location at the beginning of the disk, as many early PC filesystems did. 

Aside from the actual location of the data on the disk, EXT4 uses functional strategies, such as delayed allocation, to allow the filesystem to collect all the data being written to the disk before allocating space to it. This can improve the likelihood that the data space will be contiguous.
