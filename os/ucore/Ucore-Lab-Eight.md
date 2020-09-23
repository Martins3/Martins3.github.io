# 记录
从新实现基于文件系统的执行程序机制（即改写do_execve）

首先了解打开文件的处理流程，然后参考本实验后续的文件读写操作的过程分析，编写在sfs_inode.c中sfs_io_nolock读文件中数据的实现代码。
请在实验报告中给出设计实现”UNIX的PIPE机制“的概要设方案，鼓励给出详细设计方案

# 阅读代码

# 记录
1. 打开文件的处理流程
暂存 练习二 https://chyyuu.gitbooks.io/ucore_os_docs/content/lab8/lab8_2_1_exercises.html

3. mksys， 工具，是如何被编译的，如何使用的，还有那些工具，都是用来做什么的。

4. **重写了do_execve load_icode 等函数以支持执行文件系统中的文件**
do_execve 创建地址空间，并且调用load_icode 来实现 将image加载到虚拟地址空间中间。

5.  所以sfs 和 vfs 都是在搞什么, 为什么还是冒出来了一个devfs出来了

6. 文件系统是如何处理 目录的， 有什么特殊的地方吗?
目录项（dentry）：它主要从文件系统的文件路径的角度描述了文件路径中的一个特定的目录项（注：一系列目录项形成目录/文件路径）。
它的作用范围是整个OS空间。对于SFS而言，inode(具体为struct sfs_disk_inode)对应于物理磁盘上的具体对象，dentry（具体为struct sfs_disk_entry）是一个内存实体，
其中的ino成员指向对应的inode number，另外一个成员是file name(文件名).

8. copy_path中间的 lock_mm(mm)让我 我现在彻底不知道 锁 是做什么的，mm 不是每个进程私有的，为什么使用的时候还需要上锁。
```
copy_path(char **to, const char *from) {
```


9. sysfile_read 中间再次包含 lock_mm , 此外 copy_to_user 的 让人怀疑，为什么首先需要将数据读入到内容中间，然后复制到用户空间，效率不低吗？ 真实的机器上是如何实现的。

```
sysfile_read(int fd, void *base, size_t len) {
```
最后，空出来了部分的接口来实现

10. 实际上. sysfile的调用最后会全部都指向file 对应的实现，所以为什么添加sysfile 的实现.


11. file.c 主要处理的内容是

fd_array

fs.h 中间定义file_struct
file.h 中定义 struct file

process 和 file_struct 打交道, file_struct 控制一组struct file, 其中struct file和inode 相关联，相同的进程的struct file 可能会和同一个inode对应吧!

12. fd 进程相关的，和inode 是有区别的，fd 就是proess 打开文件的分配的数码号，并且是顺序分配的, 可以在自己的电脑上面测试一下。

13.  为什么inode 是抽象的 层次的代表?
```
 * A struct inode is an abstract representation of a file.
 *
 * It is an interface that allows the kernel's filesystem-independent
 * code to interact usefully with multiple sets of filesystem code.
```
14. file 层次会到达vfs 层次

15. 所以是如何实现链接文件的

16. 有必要测试一下，其中关于 inode 是不是放置到 一个磁盘 block 的
```
在sfs_fs.c文件中的sfs_do_mount函数中，完成了加载位于硬盘上的SFS文件系统的超级块superblock和freemap的工作。这样，在内存中就有了SFS文件系统的全局信息。
```
```
此外，和 inode 相似，每个 sfs_dirent_entry 也占用一个 block。
```

17. 
```
/* inode (on disk) */
struct sfs_disk_inode {
```

18. inode sfs_inode sfs_disk_inode 的区别是什么?

19. 为了方便实现上面提到的多级数据的访问以及目录中 entry 的操作，对 inode SFS实现了一些辅助的函数
```
sfs_bmap_load_nolock
sfs_bmap_truncate_nolock
sfs_dirent_read_nolock
sfs_dirent_search_nolock
```

20. 所以两个 是如何联系起来的，然后为什么需要将 inode 和 一个设备互相连接起来。
inode的成员变量in_info将成为一个device结构。这样inode就和一个设备建立了联系，这个inode就是一个设备文件。
struct device {

} vfs_dev_t;

```
struct inode {
  union {
    struct device __device_info; // inode 为什么需要包含这两个东西。
    struct sfs_inode __sfs_inode_info; // 分别对应文件和设备
  } in_info;
  enum {
    inode_type_device_info = 0x1234,
    inode_type_sfs_inode_info,
  } in_type;
  int ref_count;
  int open_count;
  struct fs *in_fs;
  const struct inode_ops *in_ops;
};
```

20. 所以vfs sfs 和 dev 之间的关系是什么?
vfs 封装 sfs
sfs 封装 dev 由于dev 是包含io 和 磁盘的，
> 表示怀疑
```
void fs_init(void) {
    vfs_init(); // 似乎没有什么关键的内容
    dev_init();
    sfs_init();
}

void dev_init_stdout(void) {
    struct inode *node;
    if ((node = dev_create_inode()) == NULL) {
        panic("stdout: dev_create_node.\n");
    }
    stdout_device_init(vop_info(node, device));
    int ret;
    // 居然是添加到 vfs 中间
    if ((ret = vfs_add_dev("stdout", node, 0)) != 0) {
        panic("stdout: vfs_add_dev: %e.\n", ret);
    }
}

void sfs_init(void) {
    int ret;
    if ((ret = sfs_mount("disk0")) != 0) {
        panic("failed: sfs: sfs_mount: %e.\n", ret);
    }
}
```
关键问题fs:
* 如何管理磁盘块
* 如何实现设备虚拟化的接口统一的


22. 敲他妈，mount 到底是用于做什么的?

23. stdin 的中间的read 是理解等待队列的关键. sync/wait.c 
* wait 队列都是放在一起的吗?
* wait_state 和 wait_flag 都是做什么用的

24. 似乎dev这个还是很容易理解的
* 三个文件分别首先自己的初始化和一般操作
* dev 文件中间设置 inode 和 dev 的函数挂钩起来, 但是实现的方式简直令人窒息，全部都是一句话的函数
```
static int dev_write(struct inode *node, struct iobuf *iob) {
    struct device *dev = vop_info(node, device);
    return dop_io(dev, iob, 1);
}

#define vop_info(node, type) __vop_info(node, type)

#define __vop_info(node, type)                                                 \
  ({                                                                           \
    struct inode *__node = (node);                                             \
    assert(__node != NULL && check_inode_type(__node, type));                  \
    &(__node->in_info.__##type##_info);                                        \
  })
```
通过调用类型，实现了确定调用什么函数，具体在in.info 获取正确的结构体。

其中sys_inode 应该即使表示普通文件，由于sys_inode 中间包含了运行时信息，所以sys_inode 中间需要是包含 sys_disk_inode的

25. 所以为什么在 struct device中间已经包含了各种操作函数，struct inode 中间还是有函数操作信息，
一个是vop_open 一个是dev_open 函数， 两者之间的关系是什么呢?

26. 看一下打开文件的流程:
```
int fd1 = safe_open("sfs\_filetest1", O_RDONLY);
```
到了这里，需要把位于用户空间的字符串"sfs_filetest1"拷贝到内核空间中的字符串path中，并进入到文件系统抽象层的处理流程完成进一步的打开文件操作中。

> 所以，为什么连个文件名都是需要复制 到 进去。

为此需要进一步调用vfs_open函数来找到path指出的文件所对应的基于inode数据结构的VFS索引节点node。vfs_open函数需要完成两件事情：通过vfs_lookup找到path对应文件的inode；调用vop_open函数打开文件。
vfs_open vop_open


最后，打开文件需要将 file 和 inode 关联起来。

27. sfs 在操作文件的时候的封装了vfs 的内容，最后还是会调用到磁盘操作上，依赖于sfs 来实现磁盘块和读写。但是还是没有办法确定 vfs 和 dev 是有上下级 还是 并行的关系?

28. sfs_inode.c 中间的注册
```
// The sfs specific DIR operations correspond to the abstract operations on a inode.
static const struct inode_ops sfs_node_dirops = {
    .vop_magic                      = VOP_MAGIC,
    .vop_open                       = sfs_opendir,
    .vop_close                      = sfs_close,
    .vop_fstat                      = sfs_fstat,
    .vop_fsync                      = sfs_fsync,
    .vop_namefile                   = sfs_namefile,
    .vop_getdirentry                = sfs_getdirentry,
    .vop_reclaim                    = sfs_reclaim,
    .vop_gettype                    = sfs_gettype,
    .vop_lookup                     = sfs_lookup,
};
/// The sfs specific FILE operations correspond to the abstract operations on a inode.
static const struct inode_ops sfs_node_fileops = {
    .vop_magic                      = VOP_MAGIC,
    .vop_open                       = sfs_openfile,
    .vop_close                      = sfs_close,
    .vop_read                       = sfs_read,
    .vop_write                      = sfs_write,
    .vop_fstat                      = sfs_fstat,
    .vop_fsync                      = sfs_fsync,
    .vop_reclaim                    = sfs_reclaim,
    .vop_gettype                    = sfs_gettype,
    .vop_tryseek                    = sfs_tryseek,
    .vop_truncate                   = sfs_truncfile,
};
```
目录和文件的操作是分开的

29. fs.c 中间，只有一个关键函数，sys_mount, 似乎不是非常的清楚sfs 没有搞什么就完成了所有的磁盘IO

30. sys_inode 的函数中间全部含有参数: sys_fs
* 它是做什么用的
* 谁传递的参数

应该是超级块的填写的内容，可以实现包含有多个文件系统的。

dev 确实最下层的层次

31. sys_inode.c 中间仅仅处理了一件事情， 根据ino 返回 inode 结构体

打错特错， 虽然函数都是静态函数，但是都是通过函数指针得到了返回。这些函数都是读写文件实现

```
/*
 * sfs_load_inode - If the inode isn't existed, load inode related ino disk block data into a new created inode.
 *                  If the inode is in memory alreadily, then do nothing
 */
int
sfs_load_inode(struct sfs_fs *sfs, struct inode **node_store, uint32_t ino) {
```

32. 关键函数的理解
```
static int sfs_io_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, void *buf, off_t offset, size_t *alenp, bool write) {
```
sys_fs 参数的产生和传递:

## 逐个函数阅读
理清思路:
1. `struct proc` --->　`struct files_struct`(表示进程对于所有的文件的控制)

在kernel 中间，`struct files_struct` 中间的fils 是否区分　普通文件　和　设备文件，打开设备文件是不是也是获取其中的　设备号。

2. `struct file` 理论上，描述的文件都是　包含有进程属性的，而inode 描述的文件都是没有进程属性的. 实际上, `open_count` 没有办法解释。 `struct file` 中间持有inode 节点。

3. `struct file` 的四个状态的含义是什么: FD_NONE 和 FD_CLOSED 有什么区别? 为什么需要单独添加FD_INIT

```
    if (file->status == FD_CLOSED) {
        vfs_close(file->node);
    }
```
原因是file descriptor的关闭和　inode 的关闭并不是同时发生的。前者只表示该进程不在使用，而后者则是表示具体的文件不会被使用。

```
int
vfs_close(struct inode *node) {
    vop_open_dec(node);
    vop_ref_dec(node);
    return 0;
}
```

4. alloc free 和 acquire release 以及　open close　都有什么区别?

5. fs.c 中间的参数全部都是　`struct files_struct`

#### 进入到VFS 层次
1. vfslookup 包含库函数 vfs_lookup　

```
    vfs_lookup(char * path,struct inode ** node_store)
    vfs_lookup_parent(char * path,struct inode ** node_store,char ** endp)
```

2. VFS 进一步调用vop 函数，问题是vop 函数是如何实现　面向对象的?
```
#define __vop_op(node, sym)                                                    \
  ({                                                                           \
    struct inode *__node = (node);                                             \
    assert(__node != NULL && __node->in_ops != NULL &&                         \
           __node->in_ops->vop_##sym != NULL);                                 \
    inode_check(__node, #sym);                                                 \
    __node->in_ops->vop_##sym;                                                 \
  })
```
vop_lookup 首先解析获取当前的查询查询开始的inode, 最后调用`in_ops ->vop_lookup` 

get_device 中间的实现的　路径解析方式，似乎从来没有见到过.

根据实现的不同: 分别为sys_lookup 和 dev_lookup

dev_lookup 的实现非常的简单, 做出了一些假设;

sfs_lookup 的实现分析如下:
```
struct sfs_fs *sfs = fsop_info(vop_fs(node), sfs);

#define vop_fs(node) ((node)->in_fs)
```
首先获取fs 指针，然后在其中获取sfs_fs 文件系统:

sfs_lookup_once 中间　调用
```
        ret = sfs_dirent_search_nolock(sfs, sin, name, &ino, slot, NULL);
        ret = sfs_load_inode(sfs, node_store, ino);
```
实现查询和加载inode 节点。

```
sfs_dirent_search_nolock(struct sfs_fs *sfs, struct sfs_inode *sin, const char *name, uint32_t *ino_store, int *slot, int *empty_slot) {
```
> ucore 没有实现递归查找
注意：在lab8中，为简化代码，sfs_lookup函数中并没有实现能够对多级目录进行查找的控制逻辑（在ucore_plus中有实现）。

**所有的文件都是通过inode 沟通的，现在inode 如何　文件系统　或者　stdin 沟通起来**

3. 列举其中各种 不同的实现的实现的位置:

3. inode 中间 fs 指针的用途:
---> 发现fs.h 文件，几乎不知道在搞什么鬼东西?

#### Mount 在搞什么
vfsdev.c 中间定义:
vfs_add_fs 调用vfs_do_add 函数:
mount 似乎和添加一个设备区别不大。
做的事情，kmalloc 一个　vfs_dev，　然后添加到
```
    list_add(&vdev_list, &(vdev->vdev_link));
```

又找到了一个关键的函数:
```
#define fsop_cleanup(fs) ((fs)->fs_cleanup(fs))
```
然后fs中间本身就有这些函数: 比如sfs 中间


sys_fs.c
```
int sfs_mount(const char *devname) {
    return vfs_mount(devname, sfs_do_mount);
}
```

vfs_add_dev 和 vfs_mount 有什么区别吗?
mount 的过程: 将dev 和 路径对应起来

https://unix.stackexchange.com/questions/3192/what-is-meant-by-mounting-a-device-in-linux
Mounting is the act of associating a storage device to a particular location in the directory tree
For example, when the system boots, a particular storage device (commonly called the root partition) is associated with the root of the directory tree, i.e., that storage device is mounted on / (the root directory).
Mounting applies to anything that is made accessible as files, not just actual storage devices. 


下面只分析: sfs_do_mount

```
    struct sfs_fs *sfs = fsop_info(fs, sfs);


#define fsop_info(fs, type) __fsop_info(fs, type)


#define __fsop_info(_fs, type)                                                 \
  ({                                                                           \
    struct fs *__fs = (_fs);                                                   \
    assert(__fs != NULL && check_fs_type(__fs, type));                         \
    &(__fs->fs_info.__##type##_info);                                          \
  })
```
似乎当不同的文件系统的实现的时候，比如, sfs_fs, 都是按照union 的形式放入到 fs 中间的

struct fs 是通用的模板，但是不同的fs 各自含有自己的特定实现。

通过fsop_info 获取对应的私有数据。

mount 的过程:
初始化　superblock bitmap sem 以及 fs的通用操作函数。

所以　sfs的 sfs_do_mount 的参数device 什么时候添加的 ?

fs_init() 时候就会mount , 所以 dev_init() 的时候就会init disk0 而创建的。

如何理解设备: 定义操作底层硬件的接口
mount 的过程，将路径和设备关联起来，实现设备中间的操作

设备文件的inode 是关联 文件系统的

disk0 的设备初始化，将设备的各种参数初始化，然后给其中的函数指针赋值


#### 下面三者之间的关系是什么
vfs_dev_t 

struct inode

struct device

struct fs

struct sfs_fs : 似乎对于所有函数，sfs都是含有自己的实现 super inode disk_inode disk_entry

https://chyyuu.gitbooks.io/ucore_os_docs/content/lab8/lab8_3_5_1_data_structure.html
解释了设备和inode 是如何链接起来的:

此处inode --> sys_inode & device 
和之前的fs ----> sfs_fs 
采用的关系相同:


#### 分析一下iobuf 的作用
也是在file 层次中间的

sysfile.c 是向上层 提供接口的最终位置。

#### bootfs的初始化的过程是什么,到底是搞什么的

#### sys_inode 是sfs的所有的关键
> 所以printf 的整个过程是什么

#### 磁盘缓冲区的实现 
对于磁盘的读写 最终将会调用到sfs_rwblock_nolock 中间，
1. 为什么每次都初始化io_buf

## 神奇的操作
https://docs.oracle.com/cd/E19120-01/open.solaris/817-5477/esqaq/index.html

## 文档阅读
1. unix 提出了四个文件系统抽象概念：文件(file)、目录项(dentry)、索引节点(inode)和安装点(mount point)
2. ucore目前支持如下几种类型的文件: regular file,dentry,link file, dev file, pipeline


## 代码分析

#### mksfs.c

#### 如何才能索引到sfs_disk_inode

#### fs_struct的作用

#### 设备文件如何实现
目前实现了stdin 设备文件文件、stdout设备文件、disk0设备。stdin设备就是键盘，stdout设备就是CONSOLE（串口、并口和文本显示
器），而disk0设备是承载SFS文件系统的磁盘设备。

但这个设备描述没有与文件系统以及表示一个文件的inode数据结构建立关系，为此，还需要另外一个数据结构把device和
inode联通起来，这就是vfs_dev_t数据结构。

#### 试验执行流程是什么
初始化流程

```c
fs_init(void)
    vfs_init();
      sem_init(&bootfs_sem, 1); => A
      vfs_devlist_init();
    dev_init();
    sfs_init();
```

* **A**: 1. bootfs 的作用?


#### fs 如何和 dev 关联起来
1. fs 和 dev 相互支持的，fs 需要 dev 的下层支持，dev 需要 fs 查找路径
2. 


#### init_main 中间的fs 初始化和清空
```c
if ((ret = vfs_set_bootfs("disk0:")) != 0) // 在此之前，创建dev 和 fs 之前的关联
    vfs_chdir
    vfs_get_curdir // 

fs_cleanup();
```

```c
vfs_lookup
  get_device 获取根节点对应的inode，但在目前的设置中间，根节点对应必定为root节点的内容。根节点的根据参数，返回的可能是dev 也可能是 文件的
  vop_lookup 实现真正的查询，具体会调用node 自己的 lookup 方法。

sfs_get_root(struct fs *fs)
    if ((ret = sfs_load_inode(fsop_info(fs, sfs), &node, SFS_BLKN_ROOT)) != 0) {
    // 根据约定，第一个blk 存放root_inode 节点
```

#### 当用户程序初始化的时候，其根目录为什么

#### 为什么mount 之后，访问的就是特定的设备
> vfs_lookup 首先会get_device => vfs_get_root 

`sfs_load_inode(struct sfs_fs *sfs, struct inode **node_store, uint32_t ino)` 之类的函数总是携带了 sfs_fs，而sfs_fs 在mount 的时候会和具体的dev 挂钩。

> 文件系统 : 操作的软件 + disk 中间的数据排布(magic)
> inode 的union 部分放置一个fs 的额外信息 : 窒息的操作
> 所以文件系统上挂载文件系统 ?

#### file 如何和vfs 关联起来的
1. 定义file_open 等 `file_`
2. 然后调用 `vfs_open`

inode 在它们之间的关系 ? inode 在进程中间共享 ?  : file 中间含有inode 指针，同一个inode 多个进程共享，但是每一个process 含有自己的file

```c
static void
fd_array_acquire(struct file *file) {
    assert(file->status == FD_OPENED);
    fopen_count_inc(file);
}

// fd_array_open - file's open_count++, set status to FD_OPENED
void
fd_array_open(struct file *file) {
    assert(file->status == FD_INIT && file->node != NULL);
    file->status = FD_OPENED;
    fopen_count_inc(file);
}
```
> 两者什么区别 ? 作用 ?

1. array_open 和 array_close 构成一对 分别为file_open 和 file_close 所用的
2. acquire 和 release 构成file 的文件操作的首尾

#### 文件系统如何和proc 管理
```c
/*
 * get_cwd_nolock - retrieve current process's working directory. without lock protect
 */
static struct inode *
get_cwd_nolock(void) {
    return current->filesp->pwd;
}

/*
 * process's file related informaction
 */
struct files_struct {
  struct inode *pwd;     // inode of present working directory
  struct file *fd_array; // opened files array
  int files_count;       // the number of opened files
  semaphore_t files_sem; // lock protect sem
};
```
> 其实很简单，维护一些最简单的内容!


#### vfs_lookup 函数分析
1. 内核中间对应的函数在哪里 ?
2. 为什么需要查询设备? get_device

3. vfs_get_root : 通过路径获取inode
    1. inode 是该设备下所有的文件的根节点吗 ?
    2. 所以为什么不返回 superblock 呢 ?

## dev 如何挂到vfs上
```c
// device info entry in vdev_list 
typedef struct {
    const char *devname;
    struct inode *devnode;
    struct fs *fs; // 当该设备上没有挂载文件系统的时候，为NULL
    bool mountable;
    list_entry_t vdev_link;
} vfs_dev_t;
```

vfs_add_fs vfs_add_dev => vfs_do_add 区别是什么:
1. vfs_add_dev 三个关键设备
2. vfs_add_fs 无人调用 **kernel** 或者说，kernel 如何添加文件系统

3. device < inode < vfs_dev_t


#### sys_do_mount
1. 各种各样的文件系统mount到达各种各样的设备上去。
```c
    /* allocate fs structure */
    struct fs *fs;
    if ((fs = alloc_fs(sfs)) == NULL) {
        return -E_NO_MEM;

#define alloc_fs(type) __alloc_fs(__fs_type(type))

struct fs *__alloc_fs(int type);


// __alloc_fs - allocate memory for fs, and set fs type
struct fs *
__alloc_fs(int type) {
    struct fs *fs;
    if ((fs = kmalloc(sizeof(struct fs))) != NULL) {
        fs->fs_type = type;
    }
    return fs;
}

#define __fs_type(type) fs_type_##type##_info

// 这些蛇皮等价于 malloc 和 设置一个enum 类型
```

2. sys_fs 中间的hash_list 的作用

```c
/*
 * sfs_hash_list - return inode entry in sfs->hash_list
 */
static list_entry_t *
sfs_hash_list(struct sfs_fs *sfs, uint32_t ino) {
    return sfs->hash_list + sin_hashfn(ino);
}

#define sin_hashfn(x) (hash32(x, SFS_HLIST_SHIFT))

/* *
 * hash32 - generate a hash value in the range [0, 2^@bits - 1]
 * @val:    the input value
 * @bits:   the number of bits in a return value
 *
 * High bits are more random, so we use them.
 * */
uint32_t
hash32(uint32_t val, unsigned int bits) {
    uint32_t hash = val * GOLDEN_RATIO_PRIME_32;
    return (hash >> (32 - bits));
}


// 莫名其妙
/*
 * sfs_set_links - link inode sin in sfs->linked-list AND sfs->hash_link
 */
static void
sfs_set_links(struct sfs_fs *sfs, struct sfs_inode *sin) {
    list_add(&(sfs->inode_list), &(sin->inode_link));
    list_add(sfs_hash_list(sfs, sin->ino), &(sin->hash_link)); // 理解错误了，其实hast list 并不会限制在1 << 10 之类，只是hash 的入口如此而已。
}


// 使用缓冲区，实现快速查询吗 ?
/*
 * lookup_sfs_nolock - according ino, find related inode
 *
 * NOTICE: le2sin, info2node MACRO
 */
static struct inode *
lookup_sfs_nolock(struct sfs_fs *sfs, uint32_t ino) {
    struct inode *node;
    list_entry_t *list = sfs_hash_list(sfs, ino), *le = list;
    while ((le = list_next(le)) != list) {
        struct sfs_inode *sin = le2sin(le, hash_link);
        if (sin->ino == ino) {
            node = info2node(sin, sfs_inode);
            if (vop_ref_inc(node) == 1) {
                sin->reclaim_count ++;
            }
            return node;
        }
    }
    return NULL;
}

// 最终都会使用的位置
sfs_load_inode()


//
// fs需要使用的关键函数
/*
 * sfs_get_root - get the root directory inode  from disk (SFS_BLKN_ROOT,1)
 */
static struct inode *
sfs_get_root(struct fs *fs) {
    struct inode *node;
    int ret;
    if ((ret = sfs_load_inode(fsop_info(fs, sfs), &node, SFS_BLKN_ROOT)) != 0) {  // 看来，inode 第一个数值居然是固定的
        panic("load sfs root failed: %e", ret);
    }
    return node;
}


sfs_lookup_once(struct sfs_fs *sfs, struct sfs_inode *sin, const char *name, struct inode **node_store, int *slot) {

sfs_dirent_search_nolock // 实现查找ino 的函数


// 除非ino 就是磁盘偏移量，那么inode 和 磁盘号码必须一一对应的!
// 最多支持1000个disk 就是 4M 的空间!

/* file entry (on disk) */
struct sfs_disk_entry {
  uint32_t ino;                     /* inode number */
  char name[SFS_MAX_FNAME_LEN + 1]; /* file name */
};
```

3. freemap 的作用: 所有的alloc 和 free 都需要使用的内容。

#### 关键问题:fread 如何实现 ?
1. 我们如何处理重定向的 ? 定位到特定的设备文件中。(在进程的眼中只有一个数值，当其读写的时候，并不做区分)
2. fread 实现 : 从系统调用到inb


#### feather : fifo link 如何实现

#### 添加文件系统，新的内容需要复制了!


## 内核对照阅读
1. 似乎其地址格式是自己设置的?
