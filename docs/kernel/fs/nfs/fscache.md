# netfs

## 先看文档
- https://www.kernel.org/doc/html/latest/filesystems/netfs_library.html
- [netfs, afs, 9p: Delegate high-level I/O to netfslib](https://lwn.net/Articles/955944/)
- https://www.kernel.org/doc/html/latest/filesystems/caching/index.html

其实一共就没多少代码 fs/netfs/ ，其实基本上可以猜到，就是一些
网络上的原因让文件系统有一些共性。

## 基本测试

```txt
🧀  l /proc/fs/
Permissions Size User Date Modified Name
dr-xr-xr-x     - root 22 Jan 17:05   ext4
lrwxrwxrwx     - root 22 Jan 17:05   fscache -> netfs
dr-xr-xr-x     - root 22 Jan 17:05   jbd2
dr-xr-xr-x     - root 22 Jan 17:05   lockd
dr-xr-xr-x     - root 22 Jan 17:05   netfs
dr-xr-xr-x     - root 22 Jan 17:05   nfsd
dr-xr-xr-x     - root 22 Jan 17:05   nfsfs
```

```txt
🧀  tree /proc/fs/netfs
/proc/fs/netfs
├── caches
├── cookies
├── requests
├── stats
└── volumes
```

### 启动 cachefiles 后端并注册缓存

在 VM 中安装并启动 `cachefilesd`:

```sh
sudo dnf install -y cachefilesd
sudo sed -i 's/^secctx /# secctx /' /etc/cachefilesd.conf
sudo systemctl enable --now cachefilesd
```

（`secctx` 那行需要注释掉，因为本次内核启动时禁用了 SELinux。）

启动后 `/proc/fs/netfs/caches` 出现注册的缓存:

```txt
$ cat /proc/fs/netfs/caches
CACHE    REF   VOLS  OBJS  ACCES S NAME
======== ===== ===== ===== ===== = ===============
00000001     2     1     0     1 A mycache
```

结论: cachefiles 后端成功绑定到 fscache，`mycache` 被激活。

### 触发 `nfs_fscache_open_file`

先用 NFSv3 挂载一个带 `fsc` 选项的测试目录:

```sh
sudo mkdir -p /mnt/nfs-fsc-test
sudo mount -t nfs -o fsc,vers=3 10.0.0.2:/home/martins3/data /mnt/nfs-fsc-test
```

挂载选项里可以看到 `fsc`:

```txt
10.0.0.2:/home/martins3/data /mnt/nfs-fsc-test nfs ... vers=3 ... fsc ...
```

实验中发现 `mount -t nfs -o fsc,vers=4.1 ...` 时 `fsc` 没有写入 superblock 的挂载选项（`/proc/mounts` 不显示，也没有创建 cookie），而 `vers=3` 可以。当前分析尚未定位具体原因，已记录为待进一步调查的 NFSv4 fscache 路径问题。

### 观察 cookies / volumes / stats

读取文件前:

```txt
$ cat /proc/fs/netfs/cookies
COOKIE   VOLUME   REF ACT ACC S FL DEF
======== ======== === === === = == ================

$ cat /proc/fs/netfs/volumes
VOLUME   REF   nCOOK ACC FL CACHE           KEY
======== ===== ===== === == =============== ================
```

读取 `/mnt/nfs-fsc-test/vn/docs/kernel/fs/nfs/fscache.md` 后:

```txt
$ cat /proc/fs/netfs/cookies
COOKIE   VOLUME   REF ACT ACC S FL DEF
======== ======== === === === = == ================
00000002 00000010   1   0   0 - 602c 0100078134148076000000005cfef1739c664a12b67a668a9888855d9fa40eb6010000006353d3c3, ...

$ cat /proc/fs/netfs/volumes
VOLUME   REF   nCOOK ACC FL CACHE           KEY
======== ===== ===== === == =============== ================
00000010     2     1   1 00 mycache         nfs,3.0,2,,200000a,4fcfee04f99784ea,,,c0,100000,100000,bb8,ea60,7530,ea60,1
```

`netfs stats` 也出现了读缓存相关计数:

```txt
Reads  : DR=0 RA=1 RF=0 RS=0 WB=0 WBZ=0
Writes : BW=0 WT=0 DW=0 WP=0 2C=1
...
-- FS-Cache statistics --
Cookies: n=1 v=1 vcol=0 voom=0
Acquire: n=1 ok=1 oom=0
IO     : rd=0 wr=1 mis=0
```

结论: 带 `fsc` 的 NFS 挂载会触发 fscache 创建 cookie、volume，并把读取的数据写入本地 cachefiles 缓存。

## 经常观察到的日志
```txt
[   48.833775] netfs: FS-Cache loaded
```

fs/cachefiles

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
