# fuse-bench

`fuse-bench` 是一个小型 FUSE 协议实验项目，用来测量最小用户态文件系统路径的开销。它在初始 `FUSE_INIT` 握手之后使用 FUSE-over-io_uring 传递请求。文件系统只暴露一个文件：

```text
/
`-- file
```

daemon 直接通过 `include/uapi/linux/fuse.h` 和 `/dev/fuse` 通信，不使用 libfuse，也不使用 liburing。初始 `FUSE_INIT` 请求仍然使用传统 `/dev/fuse` read/write 协议处理，因为连接初始化完成之前，内核会拒绝 FUSE io_uring command。初始化之后，daemon 会按 possible CPU 数量注册 io_uring entry，并通过 `FUSE_IO_URING_CMD_REGISTER` 和 `FUSE_IO_URING_CMD_COMMIT_AND_FETCH` 处理请求。

## 构建

```sh
make
make test
```

生成的二进制是 `fuse-bench.out`、`bench.out` 和 `test_common.out`。仓库已经忽略 `*.out`。

## 运行

daemon 使用 `fusermount3` 挂载，所以只要 `fusermount3` 按常见发行版方式安装为 setuid root，就可以用普通用户启动。内核模块必须启用 FUSE-over-io_uring：

```sh
cat /sys/module/fuse/parameters/enable_uring
echo Y | sudo tee /sys/module/fuse/parameters/enable_uring
```

启动 daemon：

```sh
mkdir -p /tmp/fuse-bench-mnt
truncate -s 1G /tmp/fuse-bench.img
./fuse-bench.out --backing /tmp/fuse-bench.img --mountpoint /tmp/fuse-bench-mnt --uring --debug
```

在另一个 shell 中运行 benchmark：

```sh
./bench.out --path /tmp/fuse-bench-mnt/file --size 256M --bs 128k --mode rw
./bench.out --path /tmp/fuse-bench.img --size 256M --bs 128k --mode rw
```

第二条命令是直接访问 backing file 的基线。把它和挂载路径的结果对比，可以估算 FUSE 路径的额外开销。

如果 daemon 被中断但没有完成清理，可以手动卸载：

```sh
fusermount3 -u -z /tmp/fuse-bench-mnt
```

## 参数

`fuse-bench.out`：

- `--backing FILE`：作为 `/file` 后端存储的 backing file。
- `--mountpoint DIR`：已经存在的挂载点目录。
- `--uring`：显式说明使用 io_uring；当前 io_uring 是强制路径。
- `--direct-io`：在 `OPEN` reply 中返回 `FOPEN_DIRECT_IO`。
- `--writeback-cache`：如果内核提供 `FUSE_WRITEBACK_CACHE`，则请求启用它。
- `--debug`：打印处理过的 opcode。

`bench.out`：

- `--path FILE`：需要测试的文件。
- `--size N[k|m|g]`：总读写大小。
- `--bs N[k|m|g]`：块大小。
- `--mode read|write|rw`：顺序读、顺序写或读写混合。
- `--direct`：使用 `O_DIRECT` 打开文件；块大小必须按 4096 字节对齐。

## TODO

当前环境有 `/dev/fuse` 和 setuid 的 `fusermount3`，但规划时没有发现 `fuse3.pc` 或 `liburing.pc`。因此 v1 直接使用内核 UAPI，只把需要特权的挂载和卸载操作委托给 `fusermount3`。

系统头文件支持 FUSE protocol 7.45，包括 `FUSE_PASSTHROUGH` 和 `FUSE_OVER_IO_URING`。daemon 强制要求 `FUSE_OVER_IO_URING`；如果内核在 `FUSE_INIT` 中没有提供这个能力，启动会失败，不会退回传统 `/dev/fuse` 请求循环。`FUSE_PASSTHROUGH` 当前只会作为内核提供的能力被打印出来，本实验还没有启用它。

(我的总结:)

看来一直捣鼓的 fuse iouring 只是在接受 fd 上的变化，而不是在于 fd 的变化。
我希望的功能在 FUSE_PASSTHROUGH 中了。

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
