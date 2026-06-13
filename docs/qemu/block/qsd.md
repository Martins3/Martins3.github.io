## qemu-storage-daemon
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

## qsd 的 fuse 功能如何理解
<!-- acb9907c-5901-4054-8645-a751a0fba381 -->

测试失败，不过，我认为这就是切入 qsd 的最佳入口:
```sh
/home/martins3/data/qemu/build/storage-daemon/qemu-storage-daemon \
  --blockdev driver=file,node-name=file0,filename=img/boot1 \
  --blockdev driver=qcow2,node-name=qcow0,file=file0 \
  --export type=fuse,id=fuse0,node-name=qcow0,mountpoint=/tmp/fuseblk

```
```txt
qemu-storage-daemon: --export type=fuse,id=fuse0,node-name=qcow0,mountpoint=/tmp/fuseblk: Parameter 'type' does not accept value 'fuse'

```

> [!NOTE]
> 参考神奇海螺的意见，有待验证

```
+------------------+
| qemu-storage-    |
| daemon / QEMU    |
|                  |
|  block backend   |  (raw / qcow2 / rbd / nbd / etc)
|        │
|        ▼
|   FUSE export
+--------│---------+
         │ /dev/fuse
         ▼
+------------------+
| 用户态程序 /     |
| QEMU / vhost-    |
| user-blk client  |
+------------------+
```

```bash
qemu-storage-daemon \
  --blockdev driver=file,node-name=file0,filename=boot1 \
  --blockdev driver=qcow2,node-name=qcow0,file=file0 \
  --export type=fuse,id=fuse0,node-name=qcow0,mountpoint=/tmp/fuseblk
```

```bash
ls -l /tmp/fuseblk
```

如果 QEMU 支持 `fuse-lseek`：

```bash
filefrag -v /tmp/fuseblk
```
```bash
python3 - << 'EOF'
import os
fd = os.open("/tmp/fuseblk", os.O_RDONLY)
print(os.lseek(fd, 0, os.SEEK_HOLE))
EOF
```

真的吗?
```bash
qemu-system-x86_64 \
  -drive file=/tmp/fuseblk,if=virtio,format=raw
```

```bash
-device vhost-user-blk-pci,chardev=char0 \
-chardev socket,id=char0,path=/tmp/vhost-blk.sock
```

FUSE 后端通常作为 **vhost-user-blk 的存储提供者**。

### FUSE ≠ 高性能默认方案

* 每个 I/O 都有用户态切换
* 不适合高 IOPS 场景，除非：
  * 大块 I/O
  * vhost-user + 多队列

### `fuse-lseek` 的意义

支持：

* 稀疏文件识别
* discard / trim 优化
* qcow2 / thin-provisioning backend

如果没有该能力：

* guest 的 discard 可能退化为全 0 写

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
