## 常用命令
1. showmount -e 10.0.0.2
检查当前暴露了那些:

```txt
Export list for 10.0.0.2:
/home/martins3/data     10.0.0.2/16
/home/martins3/data/mac 10.0.0.2/16
/home/martins3/data/vn  10.0.0.2/16
```

2. 检查结果
```sh
sudo exportfs -a
```
命令成功时通常没有任何输出，只是默默生效

使用 `*` 来到底谁可以访问:
```txt
/home/martins3/nfs         *(rw,fsid=0,no_subtree_check)
```

### 如果 server 退出，client 卡住了，如何解决
sudo umount -f -l /tmp/user-nfsd-mnt

-f : 强制退出
-l : 延迟退出

之后，client 会接受到 -EIO 的错误退出

## TODO

不知道为什么，nfs 的性能特别差

https://www.ibm.com/docs/en/aix/7.2?topic=management-nfs-performance

显然是被限制速度了:
```txt
➜  ~ fio test.fio
trash: (g=0): rw=write, bs=(R) 4096B-4096B, (W) 4096B-4096B, (T) 4096B-4096B, ioengine=libaio, iodepth=128
fio-3.34
Starting 1 process
trash: Laying out IO file (1 file / 10240MiB)
Jobs: 1 (f=1): [W(1)][0.6%][w=1944KiB/s][w=486 IOPS][eta 08h:17m:13s]
```
iops 限制了，但是拷贝大文件速度还可以

## 参考
- https://docs.fedoraproject.org/en-US/fedora-server/services/filesharing-nfs-installation/
- https://wiki.gentoo.org/wiki/Nfs-utils
- https://linux.die.net/man/5/exports
- https://man7.org/linux/man-pages/man5/nfs.5.html
- https://nixos.wiki/wiki/NFS
- https://unix.stackexchange.com/questions/122676/how-to-mount-an-remote-filesystem-with-specifying-a-port-number
- https://ubuntu.com/server/docs/service-nfs

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
