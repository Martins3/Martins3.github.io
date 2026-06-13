# 最小用户态 NFSv3 server

这是一个用于理解 NFSv3 基本数据路径的小型用户态 server，不是生产可用的
NFS server。

支持的 NFSv3 操作：

- `GETATTR`
- `SETATTR`
- `LOOKUP`
- `ACCESS`
- `FSINFO`
- `READ`
- `WRITE`
- `CREATE`
- `REMOVE`
- `READDIR`

其他操作会有意返回 `NFS3ERR_NOTSUPP`。

## 依赖

Fedora:

```sh
sudo dnf install -y gcc make rpcgen libtirpc-devel nfs-utils
```

## 构建

```sh
make -C docs/kernel/fs/nfs/user-nfsd
```

生成的二进制是 `user-nfsd.out`。`.out` 后缀会被仓库规则忽略。

修改 `nfs3.x` 或 `mount3.x` 之后，使用下面的命令重新生成 rpcgen 文件：

```sh
make -C docs/kernel/fs/nfs/user-nfsd regenerate
```

## 运行

```sh
mkdir -p /tmp/user-nfsd-root /tmp/user-nfsd-mnt
docs/kernel/fs/nfs/user-nfsd/user-nfsd.out /tmp/user-nfsd-root
```

在另一个 shell 中挂载：

```sh
sudo mount -t nfs \
  -o vers=3,proto=tcp,mountproto=tcp,port=3049,mountport=3050,nolock \
  127.0.0.1:/ /tmp/user-nfsd-mnt
```

测试基本文件 I/O：

```sh
echo hello > /tmp/user-nfsd-mnt/a
cat /tmp/user-nfsd-mnt/a
printf world >> /tmp/user-nfsd-mnt/a
rm /tmp/user-nfsd-mnt/a
```

卸载：

```sh
sudo umount /tmp/user-nfsd-mnt
```

## 限制

- 只支持 NFSv3。
- 只支持 TCP。
- 不注册 rpcbind，客户端需要显式指定 `port` 和 `mountport`。
- 不实现锁、认证、缓存一致性、恢复和 `READDIRPLUS`。
- file handle 直接编码相对路径，因此只适合较短路径。

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
