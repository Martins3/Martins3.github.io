# strace 基本使用

strace 可以监控的内容:
- 系统调用
- 信号
- process state



To trace child processes, -f option need to be specified [^1]
- `-b` syscall option can be used to instruct strace to detach process when specified syscall is executed. However, currently only execve is supported.
- `-D` option is used to detach strace and tracee processes. 使用此参数，tracee 不会作为 strace 的 children

```sh
strace -c ls # stat
strace -w -c ls # wall time
strace -c -S calls ls # sort by calls
```

-e expression 是一个强大的工具，可以控制输出的详细程度，约束到底跟踪哪一个东西，操控信号，注入 syscall 返回值

strace 执行的时候，可以修改执行的 trace 以及 修改指令执行的位置。

-I interruptible 用于屏蔽信号

## 常见操作

### 重定向到文件
```sh
strace -o a.log ls
```

### attach 到一个进程上

- `strace -p <PID>`
- `strace -p 2194,2195`  # 同时跟踪多个进程

### 过滤掉特定的 syscall
```sh
#!/usr/bin/env bash
strace -e 'trace=!read,writev' tcpdump -A -s0 port 80
```

### 跟踪和文件相关的调用
```sh
sudo strace -f -t -e trace=file ls
```
- -f 跟踪到 fork 出来的 child 里面
- -e traec=syscall_set
- -t Prefix each line of the trace with the wall clock time.

### 仅仅跟踪一个系统调用
strace -e trace=openat fio /home/martins3/core/vn/docs/kernel/code/aio/4k-read.fio

https://news.ycombinator.com/item?id=38908496

## 扩展阅读
graftcp 比 proxychain-ng 的好处在于，前者用的 ptrace 后者是截获动态库，
所以 graftcp 可以代理任何程序。
https://github.com/hmgle/graftcp


## 除了比 strace 好看，还有什么作用来着?
https://github.com/sectordistrict/intentrace

[^1]: [strace little book](https://github.com/NanXiao/strace-little-book)
[^2]: https://stackoverflow.com/questions/7482076/how-does-strace-connect-to-an-already-running-process

## strace 实现原理
http://arthurchiao.art/blog/how-does-strace-work-zh/

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
