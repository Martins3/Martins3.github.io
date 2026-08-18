# qemu-storage-daemon
https://www.qemu.org/docs/master/tools/qemu-storage-daemon.html

Export a qcow2 image file disk.qcow2 as a vhost-user-blk device over UNIX domain socket vhost-user-blk.sock:
```txt
qemu-storage-daemon \
    --blockdev driver=file,node-name=file,filename=disk.qcow2 \
    --blockdev driver=qcow2,node-name=qcow2,file=file \
    --export type=vhost-user-blk,id=export,addr.type=unix,addr.path=vhost-user-blk.sock,node-name=qcow2
```

```txt
qemu-storage-daemon \
    --blockdev driver=file,node-name=disk,filename=disk.img \
    --nbd-server addr.type=unix,addr.path=nbd.sock \
    --export type=nbd,id=export,node-name=disk,writable=on
```


想不到还可以设置 io thread ，这个实在是我没有想到的

## https://kvm-forum.qemu.org/2022/kvmforum2022_qsd_libblkio_v1.pdf

相当于，qemu-storage-daemon 把 qemu 中的 block layer 给分离开了

后面很多 libblkio 和 vDPA 的东西就看不懂了。



## 为什么 qsd 中不去显示 memfd 啊?
```txt
  l
Permissions Size User     Date Modified Name
lr-x------     - martins3 11 Jun 09:35   0 -> pipe:[315509]
l-wx------     - martins3 11 Jun 09:35   1 -> /home/martins3/.local/share/pueue/task_logs/535.log
l-wx------     - martins3 11 Jun 09:35   2 -> /home/martins3/.local/share/pueue/task_logs/535.log
lrwx------     - martins3 11 Jun 09:35   3 -> /home/martins3/hack/vm/2403-nix/img/img_qsd
lrwx------     - martins3 11 Jun 09:35   4 -> anon_inode:[eventfd]
lrwx------     - martins3 11 Jun 09:35   5 -> anon_inode:[signalfd]
lrwx------     - martins3 11 Jun 09:35   6 -> anon_inode:[eventfd]
lrwx------     - martins3 11 Jun 09:35   7 -> anon_inode:[eventfd]
lrwx------     - martins3 11 Jun 09:35   8 -> socket:[311558]
l-wx------     - martins3 11 Jun 09:35   9 -> /home/martins3/hack/vm/2403-nix/s/qsd.pid
lrwx------     - martins3 11 Jun 09:35   10 -> socket:[419878]
lrwx------     - martins3 11 Jun 09:35   11 -> socket:[418900]
lrwx------     - martins3 11 Jun 09:35   12 -> anon_inode:[eventfd]
lrwx------     - martins3 11 Jun 09:35   13 -> anon_inode:[eventfd]
lrwx------     - martins3 11 Jun 09:35   14 -> anon_inode:[eventfd]
```

测试 ms 的时候，qemu 都是可以知道 memfd 的

```txt
lrwx------     - martins3 11 Jun 09:40   34 -> '/memfd:memory-backend-memfd (deleted)'
```

## qsd 的 fuse 功能
<!-- acb9907c-5901-4054-8645-a751a0fba381 -->

这个非常能体现 qsd 的功能，如果外部想要打开一个 qcow2 ，如何复用 qemu 内部的代码，
可以通过 fuse 来暴露。

这个不是用来暴露一个文件系统的


核心场景是：需要让只认识“普通 raw 文件”的程序，访问 QEMU block layer 管理的镜像。

1. 让普通工具访问 qcow2

很多程序只会对普通文件做 pread/pwrite，不认识 qcow2、VMDK、加密层或 backing chain。
FUSE export 可以把这些 block node 呈现成 raw 文件：

qcow2 / backing chain / encryption
              ↓ QEMU block layer
         FUSE 普通文件
              ↓
       dd、文件系统工具、自研程序

例如：

dd if=disk.raw of=header.bin bs=1M count=1
file disk.raw
fsck.ext4 disk.raw

如果镜像本身就是无分区的 ext4 文件系统，还可以直接让文件系统工具操作。

4. 利用完整的 QEMU block graph

它导出的不一定只是简单 qcow2，也可以是由 QEMU block layer 组合出来的节点，例如：

- backing chain
- copy-on-write overlay
- 加密层
- filter node
- snapshot
- 远端存储协议
- 限速或调试节点

上层程序看到的仍然只是一个连续的 raw 文件。

5. 临时把 qcow2“伪装”为 raw

QEMU 官方示例直接把 FUSE 挂载到 qcow2 文件自身：

启动前：disk.qcow2 路径看到 qcow2 格式
启动后：disk.qcow2 路径看到虚拟磁盘的 raw 内容
退出后：disk.qcow2 路径恢复为原 qcow2 文件

这适合无法修改文件路径、但期望输入为 raw 的已有程序。

类似功能，可以利用 qemu-nbd 来导出

qemu-nbd --connect=/dev/nbd0 disk.qcow2

```sh
/home/martins3/data/qemu/build/storage-daemon/qemu-storage-daemon \
  --blockdev driver=file,node-name=file0,filename=img/boot1 \
  --blockdev driver=qcow2,node-name=qcow0,file=file0 \
  --export type=fuse,id=fuse0,node-name=qcow0,mountpoint=/tmp/fuseblk

```
```txt
qemu-storage-daemon: --export type=fuse,id=fuse0,node-name=qcow0,mountpoint=/tmp/fuseblk: Parameter 'type' does not accept value 'fuse'

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
