| filename            | blank | comment | code |                                                                                                                           |
|---------------------|-------|---------|------|---------------------------------------------------------------------------------------------------------------------------|
| namei.c             | 501   | 965     | 3397 | pathname 处理 各种权限检查 实现 syscall such as : link symbol_link rename mknod    |
| namespace.c         | 440   | 651     | 2375 | @todo 各种 mnt 函数，可能是处理 namespace 的 mt 机制                                                                         |
| buffer.c            | 378   | 905     | 2177 | page cache 用于在文件和 page 之间形成 cache，而 buffer cache 建立 block 和 page 之间的关系                                |
| dcache.c            | 321   | 888     | 1930 | dentry 和 inode cache，而且还和 swap 机制搅在一起了                                                                        |
| locks.c             | 328   | 689     | 1800 | 为 fs 而生的 lock，@todo 为什么会这里复杂                                                                                    |
| binfmt_elf.c        | 317   | 467     | 1644 | 处理 elf 格式的，相关格式理解之后，参考 plka 附录，这个比较简单                                                             |
| read_write.c        | 289   | 231     | 1576 | 实现 read write 以及 lleek 等 syscall                                                                                       |
| block_dev.c         | 286   | 448     | 1438 | 和 char_dev.c 对称，@todo 其实我有点怀疑，因为 device 需要挂到文件系统中间，其实这只是 inode 打交道的接口而已，              |
| fs-writeback.c      | 298   | 833     | 1361 | mark_inode_dirty 等函数实现，@todo 具体内容负责写会什么，不清楚                                                           |
| inode.c             | 223   | 639     | 1296 | 各种 inode 操作，init free inode 之类的操作|
| eventpoll.c         | 331   | 732     | 1275 | epoll 实现                                                                                                                |
| compat_ioctl.c      | 132   | 120     | 1214 |                                                                                                                           |
| dax.c               | 219   | 402     | 1199 | |
| super.c             | 188   | 396     | 998  |                                                                                                                           |
| select.c            | 213   | 220     | 991  | select poll 等 syscall                                                                                                     |
| libfs.c             | 155   | 248     | 853  | libfs 定义大量的可复用的 simple 和 empty                                                                                   |
| open.c              | 173   | 186     | 852  | 各种 open access chmod                                                                                                     |
| fcntl.c             | 121   | 113     | 810  | @todo 感觉是一组接口                                                                                                      |
| direct-io.c         | 156   | 475     | 797  | 处理 write 中间的 O_DIRECT，不经过缓存，直接到达 disk @todo 和 dax 的区分是什么                                              |
| xattr.c             | 116   | 133     | 742  |                                                                                                                           |
| posix_acl.c         | 122   | 99      | 733  |                                                                                                                           |
| binfmt_flat.c       | 133   | 172     | 709  |                                                                                                                           |
| file.c              | 131   | 162     | 689  | 处理文件描述符，包含 get_unused_fd_flags 之类的操作， 包括实现 dup                                                                                               |
| binfmt_misc.c       | 133   | 100     | 640  |                                                                                                                           |
| coredump.c          | 91    | 171     | 589  | 实现 coredump，但是@todo 具体如何使用并不知道                                                                               |
| stat.c              | 101   | 100     | 534  | 处理文件的 stat                                                                                                            |
| mpage.c             | 67    | 195     | 497  | 似乎是处理 page cache 的，mpage_readpage 之类的函数全部定义在此处 ，以及 clean_page_buffers()                                |
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
| mount.h             | 21    | 2       | 125  | 定义了 mount 所需要的各种结构体和简单的函数                                                                                 |
| Makefile            | 10    | 11      | 109  |                                                                                                                           |
| compat.c            | 16    | 17      | 102  | 给 32bit 系统的兼容性                                                                                                       |
| internal.h          | 31    | 65      | 96   | 一堆 extern                                                                                                                |
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
- [ ] get_next_ino()
  - [ ] what's relation with struct inode::i_ino and ext4 disk inode
2. get_next_ino : why we need get_next_ino ?

1. vfs 实现在哪几个函数中间 ? 实际上，感觉其实只是看到了一个非常小的皮毛，整个架构根本不是想象的那样的!
2. 和 ext4 使用的接口位置 ?
3. 和设备如何打交道的 ?


如果 block_dev.c 和 char_dev.c 是 device 挂到 fs 的接口，那么:
当访问文件系统的时候，最后到达 inode 然后找到 char_dev.c 中间，找到对应的 device


```c
// 使用的 rdev 从哪里来
// 初始化工作异常简单啊!
void init_special_inode(struct inode *inode, umode_t mode, dev_t rdev);

// 好吧，检查不下去了! 感觉还是文件打开的时候
struct inode *ext4_iget(struct super_block *sb, unsigned long ino)
```

namespace 中间居然都是各种 mount 实现。

buffer.c 在 16.5 中间含有详细的描述。
1. buffer.c 对应的 page.c 到底在哪里 ?
2. block_write_full_page 就是 ext2 的 writepage 实现的内容 ?

## (fs) file_operations 的主要作用是什么东西?
总结一下: inode_operation dir_operation 等操作的意图是什么 ?

# vfs
1. 各种 operation 函数指针接口
    1. 理一下 函数指针的接口的赋值的过程，显然 ext2 和 ext4 可以共存，但是一个 partion 被设置何种 ext 的标准是什么，一切的源头应该 mount 的过程

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
4. super_operations::statfs ? analyze the simple_statfs !
      1. in fact, we can't understand the `whole super_operations` at all !

## how to work with the `task_struct`

## 使用这个配置，可以迅速制作大量的 fio
但是不知道为什么，会出现所有的 page cache 瞬间被清理的现象。
这是 htop 的显示问题，还是真的存在这种代码。

```txt
[global]
time_based
runtime=30000
ioengine=libaio
iodepth=128
direct=1


[trash]
ioengine=sync
iodepth=1
direct=0

bs=4k
size=30G
rw=read
filename=/home/martins3/hack/vm/CentOS-7-x86_64-DVD-2207-02.qcow2
```

## 原来还有这种攻击方法

> 让文件系统在面对恶意创建的文件系统映像（filesystem images）时也能足够可靠（robust）是一项很有挑战性的任务，即使在文件系统的实现一直在积极维护时，许多文件系统也未能达到这一目标。然而，有一种方法可以使这项任务变得更加困难：在文件系统映像被挂载时还继续偷偷修改这个文件系统映像。

https://mp.weixin.qq.com/s/IMPyoLdw7xWJ6WrlYzje7Q

## 也许这个算作是 cxl 的一部分吧
https://lore.kernel.org/linux-fsdevel/ZjFcG9Q1CegMPj_7@casper.infradead.org/T/#t

## 有趣的讨论
https://mp.weixin.qq.com/s/VrkPPCs1p5OqNL7PI1eBfQ

# linux 文件系统

1. 似乎从来遇到过的 dentry 的 operation 的操作是做什么的 ? 还是不知道，甚至不知道 ext2 是否注册过


## fcntl
abbreviation for file control

1. lease ?
2. dnotify
3. seal

来源自 man fcntl(2)

主要的功能:
1. Duplicating a file descriptor
2. File descriptor flags

The principal difference between the two lock types is that whereas traditional record locks are associated with a process,
open file description locks are associated with the open file description on which they are acquired, much like locks acquired with flock(2).
Consequently (and unlike traditional advisory record locks), open file description locks are inherited across fork(2) (and clone(2) with CLONE_FILES),
and are only automatically released on the last close of the open file description, instead of being released on any close of the file.

> Warning: the Linux implementation of mandatory locking is unreliable.  See BUGS below.  Because of these bugs, and the fact that the feature is believed to be little used, since Linux 4.5, mandatory locking has been made an optional feature, governed by a configuration option (CONFIG_MANDATORY_FILE_LOCKING).  This is an initial step toward removing this feature completely.


## anon_inodes
- [ ] 很短的一个代码 : /home/maritns3/core/linux/fs/anon_inodes.c
    - [ ]  kmv 用于创建 kvm-vm 的方法

## flock
首先，区分一件事情 :
https://www.kernel.org/doc/html/latest/filesystems/locking.html : 说明几乎 VFS api 调用的时候需要持有的锁

主要内容在 : fs/locks.c

下面仅仅阅读一下 fcntl:
- `__do_sys_fcntl`
  - do_fcntl
    - fcntl_setlk
      - mandatory_lock : 对于 fs 和 inode 进行检查, 看是不是 mandatory lock，和 tlpi 上描述完全一致
      - do_lock_file_wait
        - vfs_lock_file
          - filp->f_op->lock(filp, cmd, fl); : 一些 nfs 使用自定义的
          - posix_lock_file
            - posix_lock_inode : fcntl 上锁是对于 inode 实施的, 在这里对于 file_lock_context 的链表进行遍历，决定是否上锁了之类的
              -

```c
struct file_lock_context {
  spinlock_t    flc_lock;
  struct list_head  flc_flock;
  struct list_head  flc_posix;
  struct list_head  flc_lease;
};
```

还有关于 lease 相关的实现，close file 和 exec 对于锁的释放，flock 的实现，都在 fs/locks.c 中间占据了不少篇幅。

## helper
1. `inode_init_owner` :

3. `d_make_root` : 通过 root inode 创建出来对应的 dentry，所有的 inode 都是存在对应的 dentry 并且存放在 parent direcory file 中间，但是唯独 root 不行。

```c
struct dentry *d_make_root(struct inode *root_inode)
{
  struct dentry *res = NULL;

  if (root_inode) {
    res = d_alloc_anon(root_inode->i_sb);
    if (res) {
      res->d_flags |= DCACHE_RCUACCESS;
      d_instantiate(res, root_inode);
    } else {
      iput(root_inode);
    }
  }
  return res;
}
```

4. `inode_init_always` : 基本的初始化

5. `inode_init_once` : 所以，inode_init_always 和 inode_init_once 存在什么区别吗?
inode_init_once 看来作用更像是更加基本的初始化，在 minfs 中间之所以需要单独调用 init_once，
因为其使用的是通用的 kmalloc 机制。

```c
static int __init init_inodecache(void)
{
  ext4_inode_cachep = kmem_cache_create_usercopy("ext4_inode_cache",
        sizeof(struct ext4_inode_info), 0,
        (SLAB_RECLAIM_ACCOUNT|SLAB_MEM_SPREAD|
          SLAB_ACCOUNT),
        offsetof(struct ext4_inode_info, i_data),
        sizeof_field(struct ext4_inode_info, i_data),
        init_once); //
  if (ext4_inode_cachep == NULL)
    return -ENOMEM;
  return 0;
}
```


## fifo
https://unix.stackexchange.com/questions/433488/what-is-the-purpose-of-using-a-fifo-vs-a-temporary-file-or-a-pipe

## close
- `__do_sys_close`
  - close_fd
    - pick_file : 通过 fd 获取 file
    - filp_close
      - fput
        - fput_many : 使用 task_work 来封装延迟
          - `____fput`
            - __fput : 真正完成工作的位置
              - file->f_op->release


```c
static struct file_operations kvm_vcpu_fops = {
  .release        = kvm_vcpu_release,
  .unlocked_ioctl = kvm_vcpu_ioctl,
  .mmap           = kvm_vcpu_mmap,
  .llseek   = noop_llseek,
  KVM_COMPAT(kvm_vcpu_compat_ioctl),
};
```

## ext4_fiemap && do_vfs_ioctl 是做啥的

## 更多的问题

1. dirty page 除了 address_space 的跟踪，还有什么位置？
    - 用什么方法将 dirty page 链接起来?
2. 每一个 process 对应一个 address_space 还是，一类对应一个?
3. 控制 page 是不是 dirty 和 page belong to which process 的实现是不是含有重叠的位置？
4. address_space 同时在支持 swap file , page cache 和　swap 如此相似，难道代码实现没有公用的部分吗 ?
5. radix_tree 进行管理，整个内核中间其实只有一个
6. 应该并不是每一个进程管理一个 address_space ,但是实际上，每一个 inode 对应一个 address_space
7. 通过 mmap 的实现，所有的 vm_struct 都是和　inod 对应的吗，当为匿名映射的时候，虽然没有 inode, 但是含有 swap 的管理工作

todo:
1. radix_tree 的工作原理: radix_tree 的键和值是什么 ?



address_space 仅仅在 page　cache 含有作用，实际上反向映射没有什么关系(不对吧，fs.txt 中间已经说明了含有连个，而且 rmap 中间也是大量的使用了该文件)

> Of course, I have shown the situation in simplified form because file data are not generally stored contiguously on hard disk but are distributed over several smaller areas (this is discussed in Chapter 9). The
kernel makes use of the address_space data structure4 to provide a set of methods to read data from
a backing store — from a filesystem, for example. address_spaces therefore form an auxiliary layer to
represent the mapped data as a contiguous linear area to memory management.
是不是说: 如何将一个 disk 文件 mapped 内存中间，由于 disk 本身不是连续的，而且
而且此时 address_space 的功能变化成为了，读写磁盘中间商（是 virtual process　和 ***?***)

其中定义了 readpage 和 writepage 的功能，aops 的赋值在
```c
void ext4_set_aops(struct inode *inode)
```
但是其中的函数进一步被谁调用目前不清楚啊!

@todo 据说 ext2_readpage 的内容非常简单。也说明了 address_space 的内容穿透到了 vfs 层次。

> **Filesystem and block layers are linked by the address_space_operations discussed in Chapter 4.** In the
> Ext2 filesystem, these operations are filled with the following entries
```c
static int ext2_readpage(struct file *file, struct page *page) {
  return mpage_readpage(page, ext2_get_block);
}

static int ext2_writepage(struct page *page, struct writeback_control *wbc) {
  return block_write_full_page(page, ext2_get_block, wbc);
}
```
> 这就是一直想知道的 fs 通向底层的道路啊，再一次 ext_readpage 很麻烦，被单独放到 ext4/readpage 中间了。


```c
    /*
     * For areas with an address space and backing store,
     * linkage into the address_space->i_mmap interval tree.
     */
    struct {
        struct rb_node rb;
        unsigned long rb_subtree_last;
    } shared;

    /*
     * A file's MAP_PRIVATE vma can be in both i_mmap tree and anon_vma
     * list, after a COW of one of the file pages.  A MAP_SHARED vma
     * can only be in the i_mmap tree.  An anonymous MAP_PRIVATE, stack
     * or brk vma (with NULL file) can only be in an anon_vma list.
   * @todo 这J8 英语说的是什么东西 ? 到底含有多少种类FLAGS, 分别表示什么含义 ?
     */
    struct list_head anon_vma_chain; /* Serialized by mmap_sem &
                      * page_table_lock */
    struct anon_vma *anon_vma;  /* Serialized by page_table_lock */
```
> mm_area_struct 中间的内容，注释说: 对于含有 backing store 的 area, 那么放到 i_mmap

i_mmap 在 address_space 中间定义
```c
    struct rb_root_cached   i_mmap;     /* tree of private and shared mappings */
```
@todo i_mmap 的具体使用规则也是不清楚的

doc:
1. 观察 page 的定义，page 中间的定义支持各种类型，
其中，包括:  *page cache and anonymous pages*
> 1. 从 buddy 系统中间分配的 page 含有确定的类型吗, 除了上面的两个类型还有什么类型
> 2. ucore 如何管理 page cache
> 3. 其他类型的 page 都没有含有 address_space, 是不是意味着这些 page 永远不会被刷新出去，只是被内核使用的

在 16 章才是对于 address_space 的终极描述:
1. host  page_tree 的作用:
The link with the areas managed by an address space is established by means of a pointer to
an inode instance (of type struct inode) to specify the backing store and a root radix tree
(page_tree) with a list of all physical memory pages in the address space.(@question 这是说明一个 backstore (disk ssd partition)对应一个文档，还是说仅仅对应一个 file)
@question 所以什么时候创建的 address_space

* **filep**
1. filep 出现的位置在什么层次 ?
2. filep 包含的内容是什么 ?
3. filep 的出现就是为了支持 process 可以访问同一个文件，谁持有文件，也就是持有 inode, 在 inode 中间包含进程需要的信息，比如引用计数不就可以了吗,
为什么需要单独独立出来信息 ?
5. file_operations 什么时候赋值 ?
6. 问题是 inode 中间也包含 file_operations 结构体，所以和 struct file　的 file_operations 有什么区别 ?


## TODO
- 一个 a.out 正在被调试，但是此时 gcc 同时生成一个新的 a.out，似乎两者不会产生任何干扰.
  - 文件可以被一个进程使用的过程中删除
  - 一个目录似乎也是可以在正在使用的过程中删除
- [ ] https://news.ycombinator.com/item?id=24758024 : O_SYNC O_DIRECT and many other semantic of posix interface
- [ ] 内部的 lock 的设计
- [ ] 了解一下 squashfs 的设计

- [ ] 想不到 mountpoint 可以跟着被 move ，一个 /dev/sdb 本来是 mount 到  /home/martins3/a/b 可以拷贝 /home/martins3/a 到 /tmp ，
那么 mountpoint 就进入到 /tmp/a/b 下了


[^1]: [kernel doc : Overview of the Linux Virtual File System](https://www.kernel.org/doc/html/latest/filesystems/vfs.html)
[^3]: [stackexchange : loop device](https://unix.stackexchange.com/questions/66075/when-mounting-when-should-i-use-a-loop-device)
[^6]: [lwn : Dentry negativity](https://lwn.net/Articles/814535/)
[^11]: [usenix : Filebench a flexible framework for fs benchmark](https://www.usenix.org/system/files/login/articles/login_spring16_02_tarasov.pdf)
[^12]: [sun : Filebench tutorial](http://www.nfsv4bat.org/Documents/nasconf/2005/mcdougall.pdf)

#### (fs) 文件系统和设备的链接处在哪里 ?
从 inode 中间的 dev_t 到 file_system_type 中间的 struct module * owner。
为什么两者之间的 connection 如此之多，如果 dev 和 fs 只有一个沟通环节不是更好吗 ?
文件系统是和具体的设备挂钩的，superblock 中间持有一个 dev_t 就完全足够，但是实际上，整个文件系统的中间的所有的 inode 都是需要只有 dev_t(也许)
这很不优雅!


## flock 和 nfs 的关系
https://man7.org/linux/man-pages/man2/fcntl.2.html

不明觉厉啊:
```txt
   Record locking and NFS
       Before Linux 3.12, if an NFSv4 client loses contact with the
       server for a period of time (defined as more than 90 seconds with
       no communication), it might lose and regain a lock without ever
       being aware of the fact.  (The period of time after which contact
       is assumed lost is known as the NFSv4 leasetime.  On a Linux NFS
       server, this can be determined by looking at
       /proc/fs/nfsd/nfsv4leasetime, which expresses the period in
       seconds.  The default value for this file is 90.)  This scenario
       potentially risks data corruption, since another process might
       acquire a lock in the intervening period and perform file I/O.

       Since Linux 3.12, if an NFSv4 client loses contact with the
       server, any I/O to the file by a process which "thinks" it holds
       a lock will fail until that process closes and reopens the file.
       A kernel parameter, nfs.recover_lost_locks, can be set to 1 to
       obtain the pre-3.12 behavior, whereby the client will attempt to
       recover lost locks when contact is reestablished with the server.
       Because of the attendant risk of data corruption, this parameter
       defaults to 0 (disabled).
```

那么 virtio-fs 也有类似的问题吗?

从 nfs_flock 开始看起来吧，类似的 hook 有好几个

而且很奇怪，既然都实现了，这 qemu 做的基本都是完全没办法用，这真的好吗?


## 数值溢出
https://lore.kernel.org/all/20240215155009.94493-1-mheyne@amazon.de/

## 重点突破
1. negative dentry
  - https://lore.kernel.org/all/20240929122831.92515-1-laoar.shao@gmail.com/

## 这里的代码都可以整理掉
kernel/plka/syn/fs/

## cpio 压缩的文件系统是什么格式的？

找到内核解析 cpio 的地方


## 两个问题
- https://github.com/deepseek-ai/3FS
  - https://news.ycombinator.com/item?id=43716058

## 不错的总结
https://news.ycombinator.com/item?id=43283498

## 用 GPU 显存来存储文件
- https://news.ycombinator.com/item?id=43517234


## 观察 qemu 的一个文件
```txt
🧀  stat LICENSE
  File: LICENSE
  Size: 1177            Blocks: 8          IO Block: 4096   regular file
Device: 8,16    Inode: 834040544   Links: 1
Access: (0644/-rw-r--r--)  Uid: ( 1000/martins3)   Gid: (  100/   users)
Access: 2025-06-22 21:53:04.515223601 +0800
Modify: 2022-11-27 10:59:40.776859792 +0800
Change: 2025-03-20 11:47:56.355272471 +0800
 Birth: 2025-03-20 11:47:56.354997585 +0800
```
也就是 2022-11-27 clone 过，2025-03-20 的时候 mv 到不同的路径中的。
也就是 Birth 比 Modify 还要老，有趣

## 之前都没有注意到，writeback 居然是串行的
https://mp.weixin.qq.com/s/bb1tDhEVwQtsB7yi1gBm4w

# 分析常见的 vfs syscall 的流程都放到这里吧

- do_sys_openat2
  - getname : read the file pathname from the process address space.
  - [ ] do_filp_open
  - get_unused_fd_flags :  find an empty slot in current->files->fd. The corresponding index (the new file descriptor) is stored in the fd local variable.
  - fd_install : insert the file to slot

## 仔细分析下这个吧
Documentation/filesystems/vfs.rst

is_dirty_writeback

## 数据库和文件系统基本总是一起的
https://www.sqlite.org/fasterthanfs.html

## 看看 nfs 如何实现 aio 的

## 如果一个文件中的内存修改了，那么上层目录的时间都需要更新吗?

如果不支持，岂不是 rsync 需要检查所有的目录么:
```txt
rsync -avzh  --filter='dir-merge,- .gitignore' $(pwd) ~/data/q
```
## dirlock
https://mp.weixin.qq.com/s/_giKUQa9b0C1vQt15hjJKw
https://www.youtube.com/watch?v=Rjcd7zonx7Q

可以先观察下吧

## p2p fs 如何理解?
https://github.com/dragonflyoss/dragonfly


## GPU fs ，终于来了
https://mp.weixin.qq.com/s/2wwOW0BpFX5QqZiBJtNoGQ

## 把这个东西搞清楚吧
```txt
overlayfs as the kernel interface
EROFS for a mountable metadata tree
fs-verity (optional) from the lower filesystem
```
不然这个三个机制在搞什么一直都没有搞清楚

https://github.com/composefs/composefs?tab=readme-ov-file


https://mp.weixin.qq.com/s/wMxHrzwV2JCJDLgznY-s5Q

## fscrypt

Documentation/filesystems/fscrypt.rst

代码量还不小:
```txt
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Language              Files        Lines         Code     Comments       Blanks
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 C                        10         5685         3375         1662          648
 C Header                  1          793          395          280          118
 Makefile                  1           14           11            1            2
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Total                    12         6492         3781         1943          768
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```

但是这个文件系统的功能不是所有人都使用的
```txt
 rg FS_ENCRYPTION  --glob "Kconfig"
fs/ubifs/Kconfig
14:     select UBIFS_FS_XATTR if FS_ENCRYPTION
15:     select FS_ENCRYPTION_ALGS if FS_ENCRYPTION

fs/f2fs/Kconfig
8:      select F2FS_FS_XATTR if FS_ENCRYPTION
9:      select FS_ENCRYPTION_ALGS if FS_ENCRYPTION

fs/ext4/Kconfig
36:     select FS_ENCRYPTION_ALGS if FS_ENCRYPTION

fs/crypto/Kconfig
2:config FS_ENCRYPTION
17:# Filesystems supporting encryption must select this if FS_ENCRYPTION.  This
19:# whereas selecting them from FS_ENCRYPTION would force them to be built-in.
29:config FS_ENCRYPTION_ALGS
39:config FS_ENCRYPTION_INLINE_CRYPT
41:     depends on FS_ENCRYPTION && BLK_INLINE_ENCRYPTION

fs/ceph/Kconfig
10:     select FS_ENCRYPTION_ALGS if FS_ENCRYPTION
```

## 一些有趣的思考
https://mp.weixin.qq.com/s/8SqEr_Yv1SzIvtWU4KQIoQ

https://mp.weixin.qq.com/s/bILdHM9P1M1fwDSdWN63Sg


https://mp.weixin.qq.com/s/qRKiBgAEtZNTjVvxk3-C2g

## 通过 fincore 来检查一个文件有多少页在 page cache
<!-- 380bda09-c79d-44f7-a9ab-7f794cc38edf -->

https://man7.org/linux/man-pages/man1/fincore.1.html

fincore 是 https://github.com/util-linux/util-linux 提供的工具
这个工具需要一个 syscall 的支持

fincore /path/to/file

## 看看这个
https://mp.weixin.qq.com/s/G2k3e56D1RJWjzobDRlUvw

## 测试 api 内容
```txt
kern_path : 这个是内核文件查询的入口

这个用起来:

Documentation/core-api/printk-formats.rst

```txt
dentry names
------------

::

	%pd{,2,3,4}
	%pD{,2,3,4}

For printing dentry name; if we race with :c:func:`d_move`, the name might
be a mix of old and new ones, but it won't oops.  %pd dentry is a safer
equivalent of %s dentry->d_name.name we used to use, %pd<n> prints ``n``
last components.  %pD does the same thing for struct file.

Passed by reference.
```

## 这个真不错啊
https://www.reddit.com/r/bcachefs/comments/1rntted/new_principles_of_operation_preview/

## nfs 的 swap 和普通的 fs 的 swap 有什么区别吗?

## 似乎不止一次疑惑他们到底有没有关系
```txt
CONFIG_DEVTMPFS=y
CONFIG_TMPFS=y
```

## 看看这个
https://mp.weixin.qq.com/s/pIl-yVemNfins8FLMmt9zw

## 原来很多时候，.nfs 临时文件是这样产生的

• 进程还在，但可执行文件已经变成 NFS 的 .nfs... 临时名，说明原来的 binary 被删除或替换了，内核还持有旧 inode。它的 stdout/stderr 都写到 mmoc-test-dir/
  mmoc-server.log，我继续看命令行、线程状态和网络端口。


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
