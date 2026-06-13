# mknod

## 为什么使用 major:minor
<!-- 9568289d-9df1-4a29-9d6c-f2e65d58944a -->

注意，/dev/sda1 不一定放到 /dev 下的，其实是可以放到其他的位置的

参考 [^1]，实际上是建立 `dev_t` 和路径的关系的，而这个路径具体是什么并不重要。

例如
```txt
ls -la /dev/sda
  brw-rw---- 8,0 root  6 Sep 18:30 /dev/sda
cd
sudo mknod m b 8 0
```
然后，对于 ~/m io 就是和对于 /dev/sda 进行 io 是一样的

## 原理
主要参考 [^2] [^3]

> 在写 bmbt 的时候，通过 cat /proc/partition 可以获取到 block 设备，需要 mknod 才可以将创建 /dev/nvme0n1，之后才可以将设备 mount 上去，
感觉根本没有必要，我的感觉是， /dev 这个目录就是多余的

其实这种想法是错误的，还是那个问题，unix 系统为了方便管理，需要一个文件，使用 /dev 目录在承载这个文件，是很方便的。


当调用 mknod 的时候，取决于该文件所在的文件系统，会调用不同的 `inode_operations::mknod`
创建在 /dev/x 中，最后调用的是 `shmem_mknod`，但是如果在 ~/ 中创建，那就是 `ext4_mknod` 。

- `shmem_mknod` : File creation. Allocate an inode, and we're done..
  - `shmem_get_inode`
```c
inode->i_op = &shmem_special_inode_operations;
init_special_inode(inode, mode, dev);
```

- `ext4_mknod`
```c
init_special_inode(inode, inode->i_mode, rdev);
inode->i_op = &ext4_special_inode_operations;
```
其中， 在 `init_special_inode` 的时候，设置 `inode::i_rdev`

如果是 chardev，那么注册给这个 inode 的 `file_operations` 是 `def_chr_fops`
```c
/*
 * Dummy default file-operations: the only thing this does
 * is contain the open that then fills in the correct operations
 * depending on the special file...
 */
const struct file_operations def_chr_fops = {
    .open = chrdev_open,
    .llseek = noop_llseek,
};
```

- 首先对于 chardev 注册一个 cdev
- `init_special_node` 将 inode 和 cdev 挂钩起来，但是只是告诉设备号和 `def_chr_fops`(`chrdev_open`)
- 当打开文件，也就会调用 inode 中间的 open 函数，也就是 `chrdev_open`, 此时将 struct cdev 和 inode 关联起来，并且替换 fops 然后调用这个设备的 inode 真正的 open, 例如 `memory_open`

如果启动内核之后，就是启动 bash ，虽然可以从 `/proc/partitions` 中可以看到有 blockdev，但是在 /dev 下依旧是什么都没有，
而在一般的 Linux distribution 中，在 /dev/ 下创建对应的文件的工作，应该是被 udev 完成了。

### 其实还是问题，如果 mknod 是 udev 之类的构建的，可以在任何地方，那么
内核是如何解析内核启动参数 console=/dev/ttyS0

因为，本来 /dev/ 目录是内核提供给用户态访问的，但是这么看，/dev/ttyS0 可以在 /tmpfs 都没有的时候，
就开始来访问了。

## 源码
处理 chardev 和 blockdev 的位置:
- chardev : fs/chardev.c
- blockdev : block/fops.c 和 block/bdev.c

主要是注册和管理 chardev 驱动，最后

## 测试
在 vn/code/src/m/dev.sh 中

## 问题

执行 lsinitrd 的时候，可以看到:
```txt
crw-r--r--   1 root     root       5,   1 May 23  2024 dev/console
crw-r--r--   1 root     root       1,  11 May 23  2024 dev/kmsg
crw-r--r--   1 root     root       1,   3 May 23  2024 dev/null
crw-r--r--   1 root     root       1,   8 May 23  2024 dev/random
crw-r--r--   1 root     root       1,   9 May 23  2024 dev/urandom
```

我估计只有有了这个 file ，fs mount 之后可以自动关联起来，
之后不需要

## 理解一个问题
为什么 kernel 的启动参数可以是 root=/dev/openeuler/root ?

root=/dev/mapper/fedora-root ro rd.lvm.lv=fedora/root nokaslr

这里的 rd.lvm.lv 都是在哪里解析的

除掉 root=PARTUUID root=UUID ，还有什么启动方式?
## 相关话题
- [ebbchar](http://derekmolloy.ie/writing-a-linux-kernel-module-part-2-a-character-device/) 的这个教程中，实际上是不使用 mknod 的
  - 因为其调用了 `device_create` 在内核模块中直接创建的，最后调用到 `devtmpfs_create_node`
- [/sys 和 /dev 的区别](https://unix.stackexchange.com/questions/176215/difference-between-dev-and-sys)
  - /dev 用于访问 dev 和内容，而 /sys 用于获取 dev 的基本信息以及管理之类的

[^1]: [What does mknod do?](https://unix.stackexchange.com/questions/562341/what-does-mknod-do)
[^2]: Professional Linux Kernel Architecture : Device Drivers
[^3]: Understand Linux Kernel : I/O Architecture and Device Drivers


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
