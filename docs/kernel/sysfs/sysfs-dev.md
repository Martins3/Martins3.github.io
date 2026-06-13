# sysfs dev

总体来说，其中的内容很少。

实现的位置 drivers/base/devtmpfs.c

## /dev/shm 以及 /dev/mqueue/ 是共享队列
https://unix.stackexchange.com/questions/534287/interprocess-message-queues-dev-shm-vs-dev-mqueue

### /dev/disk 是 sysmted 创建的

```txt
#0  shmem_mkdir (idmap=0xffffffff82fade60 <nop_mnt_idmap>, dir=0xffff8881008f03b0, dentry=0xffff888105e2d240, mode=511)
    at mm/shmem.c:3276
#1  0xffffffff8146f5ef in vfs_mkdir (idmap=0xffffffff82fade60 <nop_mnt_idmap>, dir=0xffff8881008f03b0,
    dentry=dentry@entry=0xffff888105e2d240, mode=<optimized out>) at fs/namei.c:4106
#2  0xffffffff814748da in do_mkdirat (dfd=dfd@entry=-100, name=0xffff88810b17a000, mode=mode@entry=511)
    at ./include/linux/mount.h:80
#3  0xffffffff81474b49 in __do_sys_mkdir (mode=<optimized out>, pathname=0x7ffeaf22d6d5 "/dev/abcadfa") at fs/namei.c:4149
#4  __se_sys_mkdir (mode=<optimized out>, pathname=<optimized out>) at fs/namei.c:4147
#5  __x64_sys_mkdir (regs=<optimized out>) at fs/namei.c:4147
#6  0xffffffff8222ac23 in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001313f58) at arch/x86/entry/common.c:51
#7  do_syscall_64 (regs=0xffffc90001313f58, nr=<optimized out>) at arch/x86/entry/common.c:82
#8  0xffffffff824000eb in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
```

破案了，就是在 systemd-udevd 创建的:
```txt
[    1.349248] [martins3:shmem_mkdir:3280] systemd-udevd
```

## devpts
https://www.kernel.org/doc/html/latest/filesystems/devpts.html

才意识到，在 /dev 下是可以构建任何文件的，systemd 做了这个软连接，让其他的程序的日志
通过 uds 来发送过来:
```txt
🧀  l /dev/log
Permissions Size User Date Modified Name
lrwxrwxrwx     - root 10 Jun 20:11   /dev/log -> /run/systemd/journal/dev-log
```

## /dev/char 和 /dev/block 是如何实现的

好吧，也是用户态来创建的:

内核创建的目录目前只有这些:
```txt
[    0.557437] devtmpfs: [martins3:dev_mkdir:165] cpu
[    0.557737] devtmpfs: [martins3:dev_mkdir:165] cpu/28
[    0.558245] devtmpfs: [martins3:dev_mkdir:165] cpu
[    0.558535] devtmpfs: [martins3:dev_mkdir:165] cpu/29
[    0.558921] devtmpfs: [martins3:dev_mkdir:165] cpu
[    0.559212] devtmpfs: [martins3:dev_mkdir:165] cpu/30
[    0.559766] devtmpfs: [martins3:dev_mkdir:165] cpu
[    0.560093] devtmpfs: [martins3:dev_mkdir:165] cpu/31
[    0.665680] devtmpfs: [martins3:dev_mkdir:165] vduse
[    0.710657] devtmpfs: [martins3:dev_mkdir:165] bsg
[    0.743536] devtmpfs: [martins3:dev_mkdir:165] net
[    0.753853] devtmpfs: [martins3:dev_mkdir:165] vfio
[    0.758352] devtmpfs: [martins3:dev_mkdir:165] input
[    0.762881] devtmpfs: [martins3:dev_mkdir:165] mapper
[    0.766333] devtmpfs: [martins3:dev_mkdir:165] snd
```

不是，/dev/char 是内核创建的
```txt
[   10.987146] sysfs: cannot create duplicate filename '/dev/char/5:0'
```

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
