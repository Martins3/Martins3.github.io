| filename            | blank | comment | code |                                                                                                                           |
|---------------------|-------|---------|------|---------------------------------------------------------------------------------------------------------------------------|
| namei.c             | 501   | 965     | 3397 | pathname 处理 各种权限检查 实现 syscall such as : link symbol_link rename mknod    |
| namespace.c         | 440   | 651     | 2375 | @todo 各种mnt 函数，可能是处理namespace 的mt 机制                                                                         |
| buffer.c            | 378   | 905     | 2177 | page cache 用于在文件和 page 之间形成 cache，而 buffer cache 建立 block 和 page 之间的关系                                |
| dcache.c            | 321   | 888     | 1930 | dentry 和 inode cache，而且还和swap 机制搅在一起了                                                                        |
| locks.c             | 328   | 689     | 1800 | 为fs而生的lock，@todo 为什么会这里复杂                                                                                    |
| binfmt_elf.c        | 317   | 467     | 1644 | 处理elf 格式的，相关格式理解之后，参考plka 附录，这个比较简单                                                             |
| read_write.c        | 289   | 231     | 1576 | 实现read write 以及 lleek 等syscall                                                                                       |
| iomap.c             | 299   | 277     | 1534 | iomap                                                                                                                     |
| aio.c               | 353   | 334     | 1514 | 定义并且实现了各种aio syscall                                                                                             |
| block_dev.c         | 286   | 448     | 1438 | 和char_dev.c 对称，@todo 其实我有点怀疑，因为device 需要挂到文件系统中间，其实这只是inode 打交道的接口而已，              |
| fs-writeback.c      | 298   | 833     | 1361 | mark_inode_dirty 等函数实现，@todo 具体内容负责写会什么，不清楚                                                           |
| exec.c              | 302   | 344     | 1354 | do_execve 函数                                                                                                            |
| binfmt_elf_fdpic.c  | 275   | 218     | 1324 |                                                                                                                           |
| inode.c             | 223   | 639     | 1296 | 各种inode 操作，init free inode 之类的操作|
| eventpoll.c         | 331   | 732     | 1275 | epoll 实现                                                                                                                |
| userfaultfd.c       | 231   | 477     | 1234 |                                                                                                                           |
| compat_ioctl.c      | 132   | 120     | 1214 |                                                                                                                           |
| dax.c               | 219   | 402     | 1199 | https://lwn.net/Articles/717953/                                                                                          |
| splice.c            | 249   | 393     | 1116 | https://en.wikipedia.org/wiki/Splice_(system_call)                                                                        |
| super.c             | 188   | 396     | 998  |                                                                                                                           |
| select.c            | 213   | 220     | 991  | select poll 等syscall                                                                                                     |
| libfs.c             | 155   | 248     | 853  | libfs 定义大量的可复用的simple 和 empty                                                                                   |
| open.c              | 173   | 186     | 852  | 各种open access chmod                                                                                                     |
| pipe.c              | 162   | 178     | 847  | pipe 实现                                                                                                                 |
| fcntl.c             | 121   | 113     | 810  | @todo 感觉是一组接口                                                                                                      |
| direct-io.c         | 156   | 475     | 797  | 处理write 中间的 O_DIRECT，不经过缓存，直接到达disk @todo 和dax 的区分是什么                                              |
| xattr.c             | 116   | 133     | 742  |                                                                                                                           |
| posix_acl.c         | 122   | 99      | 733  |                                                                                                                           |
| seq_file.c          | 123   | 245     | 731  | https://lwn.net/Articles/22355/                                                                                           |
| binfmt_flat.c       | 133   | 172     | 709  |                                                                                                                           |
| file.c              | 131   | 162     | 689  | 处理文件描述符，包含 get_unused_fd_flags 之类的操作， 包括实现dup                                                                                               |
| binfmt_misc.c       | 133   | 100     | 640  |                                                                                                                           |
| coredump.c          | 91    | 171     | 589  | 实现coredump，但是@todo具体如何使用并不知道                                                                               |
| stat.c              | 101   | 100     | 534  | 处理文件的stat                                                                                                            |
| mpage.c             | 67    | 195     | 497  | 似乎是处理page cache的，mpage_readpage 之类的函数全部定义在此处 ，以及clean_page_buffers()                                |
| timerfd.c           | 77    | 46      | 466  | timerfd                                                                                                                   |
| ioctl.c             | 106   | 153     | 451  |                                                                                                                           |
| readdir.c           | 50    | 18      | 430  | 应该定义一些处理以前接口的内容                                                                                            |
| char_dev.c          | 88    | 175     | 422  | @todo 包含了各种`cdev_*`函数，让人意识到一个问题，driver 和 device 的关系是什么 ? 这一个文件为什么不放到 driver/base 下面 |
| pnode.c             | 63    | 140     | 406  |                                                                                                                           |
| statfs.c            | 40    | 18      | 338  | statfs                                                                                                                    |
| binfmt_aout.c       | 52    | 51      | 323  |                                                                                                                           |
| d_path.c            | 48    | 110     | 312  | prepend                                                                                                                   |
| mbcache.c           | 44    | 103     | 289  | Ext2 and ext4 use this cache for deduplication of extended attribute blocks.                                              |
| proc_namespace.c    | 43    | 18      | 278  | handling of /proc/<pid>/{mounts,mountinfo,mountstats}                                                                     |
| signalfd.c          | 44    | 55      | 262  |                                                                                                                           |
| eventfd.c           | 48    | 115     | 258  |                                                                                                                           |
| file_table.c        | 50    | 81      | 258  | file table 处理，不知道@todo 和 file.c 中间的关系                                                                         |
| nsfs.c              | 37    | 1       | 245  | https://unix.stackexchange.com/questions/465669/what-is-the-nsfs-filesystem                                               |
| dcookies.c          | 72    | 38      | 245  |                                                                                                                           |
| sync.c              | 44    | 120     | 211  | fsync                                                                                                                     |
| utimes.c            | 44    | 41      | 209  | 文件访问时间                                                                                                              |
| attr.c              | 35    | 107     | 204  |                                                                                                                           |
| fhandle.c           | 26    | 53      | 200  |                                                                                                                           |
| filesystems.c       | 38    | 48      | 194  | sysfs 系统调用处理                                                                                                        |
| bad_inode.c         | 35    | 43      | 165  |                                                                                                                           |
| fs_struct.c         | 20    | 10      | 137  |                                                                                                                           |
| mount.h             | 21    | 2       | 125  | 定义了mount所需要的各种结构体和简单的函数                                                                                 |
| Makefile            | 10    | 11      | 109  |                                                                                                                           |
| compat.c            | 16    | 17      | 102  | 给32bit系统的兼容性                                                                                                       |
| internal.h          | 31    | 65      | 96   | 一堆extern                                                                                                                |
| fs_pin.c            | 8     | 2       | 93   |                                                                                                                           |
| binfmt_script.c     | 13    | 29      | 88   |                                                                                                                           |
| anon_inodes.c       | 25    | 48      | 88   | https://unix.stackexchange.com/questions/463548/what-is-anon-inode-in-the-output-of-ls-l-proc-pid-fd                      |
| binfmt_em86.c       | 19    | 28      | 71   |                                                                                                                           |
| compat_binfmt_elf.c | 20    | 41      | 70   |                                                                                                                           |
| drop_caches.c       | 9     | 5       | 56   |                                                                                                                           |
| pnode.h             | 6     | 7       | 45   |                                                                                                                           |
| stack.c             | 5     | 36      | 35   |                                                                                                                           |
| no-block.c          | 3     | 10      | 10   |                                                                                                                           |

## 路线
1. ramfs 的代码阅读一下
2. get_next_ino : why we need get_next_ino ?
3. https://zhuanlan.zhihu.com/zorrolang 关于mount 写了一系列的文章，水平应该不错。


1. aio syscall 处理
2. epoll
3. blk_dev.c 和 char_dev.c 其中的内容
4. 各种fd，比如event timer 需要测试

> 首先理解这些api的含义吧!

> 感觉，和mm 很容易，但是fs 根本不知道在说什么J8东西啊!

1. vfs 实现在哪几个函数中间 ? 实际上，感觉其实只是看到了一个非常小的皮毛，整个架构根本不是想象的那样的!
2. 和 ext4 使用的接口位置 ?
3. 和设备如何打交道的 ?


如果block_dev.c 和 char_dev.c 是device 挂到fs 的接口，那么:
当访问文件系统的时候，最后到达inode 然后找到 char_dev.c 中间，找到对应的device


```c
// 使用的 rdev 从哪里来
// 初始化工作异常简单啊!
void init_special_inode(struct inode *inode, umode_t mode, dev_t rdev);

// 好吧，检查不下去了! 感觉还是文件打开的时候
struct inode *ext4_iget(struct super_block *sb, unsigned long ino)
```

chrdev_open :
实现一个非常诡异的操作，虽然所有的char 开始的时候，def_char_ops ，但是最后在首次打开的时候会在此处进行替换
完成 i_cdev 的赋值
dev_t 可以查询对应的 struct cdev 通过kobject 机制


在上面操作进行之前，然后就是需要 dev 和 cdev 准备好，**还有对应的path** 
> 关联路径 : mknod 的含义 ?
```c
int cdev_add(struct cdev *p, dev_t dev, unsigned count) 

// dir 和 rdev 关联
static int ext4_mknod(struct inode *dir, struct dentry *dentry,
		      umode_t mode, dev_t rdev)
```

namespace 中间居然都是各种mount 实现。

buffer.c 在16.5 中间含有详细的描述。
1. buffer.c 对应的 page.c 到底在哪里 ?
2. block_write_full_page 就是ext2 的 writepage 实现的内容 ?

## todo
1. seq_file 的实现方法和作用是什么 ?
2. ext2_get_block 的原理(ucore 了解一下)

# vfs
1. 各种operation 函数指针接口
    1. 理一下 函数指针的接口的赋值的过程，显然ext2 和 ext4 可以共存，但是一个partion 被设置何种ext 的标准是什么，一切的源头应该mount 的过程

address_space_operations
file
file_inode
dir
dir_inode
super_block

2. 公共接口
    1. namei
    2. page io

## Question
1. `inode->i_ino` 的作用是什么 ? it seems that it's generated dynamically ?
2. iomap 和 ioremap 的关系是什么 ?
3. 为什么fs 的 locking 都可以单独做成一个 : Filesystem locking is described in the document Documentation/filesystems/locking.rst.
4. super_operations::statfs ? analyze the simple_statfs !
      1. in fact, we can't understand the `whole super_operations` at all !
5. how to work with the task_struct ?

```c
struct fs_struct {
	int users;
	spinlock_t lock;
	seqcount_t seq;
	int umask;
	int in_exec;
	struct path root, pwd;
} __randomize_layout;

struct path {
	struct vfsmount *mnt;
	struct dentry *dentry;
} __randomize_layout;
```
4. /etc/fstab ?
5. uuid
6. sudo umount -l /tmp : why it can clear anything in it ? check the routine that free memory !

## Documentation
An individual dentry usually has a pointer to an inode.

Opening a file requires another operation: allocation of a file structure (this is the kernel-side implementation of file descriptors).
The freshly allocated file structure is initialized with a pointer to the dentry and a set of file operation member functions.
> 也就是说 : file struct 指向 dentry ?

New vfsmount referring to the tree returned by `->mount()` will be attached to the mountpoint, so that when pathname resolution reaches the mountpoint it will jump into the root of that vfsmount.
> 为什么感觉 mount 就像是 symbol link, 比如 mount /dev/sda1 ~/WanShe


Usually, a filesystem uses one of the generic `mount()` implementations and provides a `fill_super()` callback instead. The generic variants are:

- mount_bdev

  mount a filesystem residing on a block device

- mount_nodev

  mount a filesystem that is not backed by a device

- mount_single

  mount a filesystem which shares the instance between all mounts

```c
static struct dentry *ext2_mount(struct file_system_type *fs_type,
	int flags, const char *dev_name, void *data)
{
	return mount_bdev(fs_type, flags, dev_name, data, ext2_fill_super);
}
```
> @todo file_system_type::mount 这个指针的调用位置，非常的令人窒息，fs_context 什么鬼东西啊 ?

Well before we get to symlinks we have another major division based on the VFS’s approach to locking which will allow us to review “REF-walk” and “RCU-walk” separately. But we are getting ahead of ourselves. There are some important low level distinctions we need to clarify first.

#### Documentation/filesystems/relay.txt
If `CONFIG_TMPFS` is enabled, rootfs will use tmpfs instead of ramfs by
default.  To force ramfs, add "rootfstype=ramfs" to the kernel command
line.

Rootfs is a special instance of ramfs (or tmpfs, if that's enabled), which is
always present in 2.6 systems.  You can't unmount rootfs for approximately the
same reason you can't kill the init process;

