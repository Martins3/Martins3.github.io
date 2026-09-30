## 为什么在 systemd 中，需要将日志设置为这个东西

```txt
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
```

想不到标准输出都替换为了这个:
```txt
root@localhost:/proc/7416/fd# ls -la
total 0
dr-x------ 2 root root  3 Nov  6 14:22 .
dr-xr-xr-x 9 root root  0 Nov  6 14:20 ..
lr-x------ 1 root root 64 Nov  6 14:22 0 -> /dev/null
lrwx------ 1 root root 64 Nov  6 14:22 1 -> 'socket:[38538]'
lrwx------ 1 root root 64 Nov  6 14:22 2 -> 'socket:[38538]'
```

> [!NOTE]
> 参考 Deepseeek ，有待验证

- stdout：如果连接到终端（TTY），默认是 行缓冲（line-buffered）。
- 但在 systemd service 中，stdout/stderr 被重定向到 /dev/null 或 journal，不再是终端。
- 此时，stdout 变为 全缓冲（fully buffered），默认缓冲区大小通常是 4KB 或 8KB。
- stderr 默认是 无缓冲 或 行缓冲（取决于实现），但在某些环境下也可能被缓冲。

qemu 有这个参数基本上应该是相同的
或者说，这个是如何实现的
-daemonize

## 容器

### docker -it 和 docker -dt 中 -t

```txt
🧀  podman run -it --rm fedora:latest
[root@3d3194f9c422 /]# tty
/dev/pts/0
[root@3d3194f9c422 /]#
```

```txt
podman run -it --rm fedora:latest
  │      │   │  │    │
  │      │   │  │    └── 镜像名:标签
  │      │   │  └─────── 容器退出后自动删除
  │      │   └────────── 分配一个伪终端 (tty)
  │      └────────────── 保持标准输入打开 (interactive)
  └───────────────────── Podman 的子命令，创建并启动一个新容器
```

当然也可以 detach
```txt
podman run -dt --rm fedora:latest bash
podman attach <name>
```

类似的我们也可以注意到:
```txt
🧀  podman top genshin

USER        PID         PPID        %CPU        ELAPSED        TTY         TIME        COMMAND
root        1           0           0.000       44.729600291s  pts/0       0s          /bin/bash
```


我们是可以这种方法运行的:
```txt
podman run -i --rm fedora:latest
```
这种时候输入 tty 结果就是 not a tty 了。

### 如果是 libkrun 机制

```txt
podman run -it --rm --runtime=krun fedora:latest
```

宿主机终端 → Podman/krun → 虚拟机的 virtio console → /dev/hvc0 → bash

我在本机复现时，虚拟机将 /dev/hvc0 注册为控制台，而 shell 的标准输入显示为
/dev/console。所以它能接收键盘输入，也能处理 Ctrl-C；只是它不像普通 crun 容器那样使用
/dev/pts/0。libkrun 的控制台接口也将默认控制台端口标为 hvc0。libkrun 控制台接口
(https://github.com/libkrun/libkrun/blob/main/include/libkrun.h)

```txt
[root@7a3b9bc575c4 /]# tty
not a tty
[root@7a3b9bc575c4 /]# echo 1 > /dev/hvc0
1
```

所以，这里是 tty 这个程序有问题，其实当前交互的是 hvc0 ，就是 tty

## windows 中的这个东西
https://cmder.app/

在例如这个例子，也就是 windows 也有 console 的概念:
https://github.com/gammasoft71/Examples_Win32/blob/master/Win32.System/Console/ConsoleColor/ConsoleColor.cpp

windows 也是有 pty 的
https://learn.microsoft.com/en-us/windows/console/creating-a-pseudoconsole-session

可运行的 [Windows Console 与 ConPTY 小实验](windows-conpty-demo/README.md) 使用
MSVC 构建，比较普通管道与 ConPTY 的标准句柄、Console API、颜色、输入和尺寸变化。

虽然 tmux 没有， wezterm 又是可以在 windows 下有的。

https://www.reddit.com/r/tmux/comments/l580mi/is_there_a_tmuxlike_equivalent_for_windows/

### windows 下的 serial 驱动
https://github.com/microsoft/Windows-driver-samples/tree/main/serial
https://github.com/raphamorim/rio

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
