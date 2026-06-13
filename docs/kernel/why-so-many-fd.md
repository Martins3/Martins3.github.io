# 为什么 Linux 内核中有如此多 fd

## 引言

如果你翻阅 Linux
内核的演进历史，会注意到一个明显的趋势：越来越多的内核子系统开始以**文件描述符（file
descriptor, fd）**作为其核心抽象接口。从早期的
`eventfd`、`signalfd`、`timerfd`，到后来的 `memfd`、`pidfd`，再到 `io_uring`
和虚拟机相关的 `guest_memfd`，内核似乎对 fd 这种抽象情有独钟。

一位 Hacker News 用户曾这样感叹：

> "There are so many features in the Linux kernel it sometimes blows my mind.
> eventfd, signalfd, timerfd, memfd, pidfd. The whole fricking tc/qdisc
> featureset (OMG). netlink. io_uring. criu. SO_REUSEPORT. Teaming. Namespaces.
> veths. vsocks. Dpdk/netmap/af_packet. XDP ! Seccomp."
>
> — [Hacker News](https://news.ycombinator.com/item?id=27328285)

本文试图回答两个问题：Linux 内核中到底有哪些 fd？以及，为什么内核如此偏爱 fd
这种抽象？

## Linux 内核中的 fd 家族

1. 事件与信号通知类

| 机制               | 用途                                                          |
| ------------------ | ------------------------------------------------------------- |
| `eventfd`          | 用户空间事件通知，常用于线程/进程间传递 "事件发生" 信号       |
| `signalfd`         | 将信号（signal）转换为文件描述符上的可读事件                  |
| `timerfd`          | 将定时器到期事件转换为 fd 上的可读事件                        |
| `memfd`            | 创建匿名内存文件，驻留在 tmpfs 中，无持久化路径，支持 sealing |
| `hugetlbfs`        | 大页内存的虚拟机文件系统，通过 mmap 获得大页支持              |
| `pidfd`            | 稳定的进程引用，解决 PID 重用（PID reuse）问题                |
| `guest_memfd`      | 用于虚拟化场景                                                |
| `/dev/userfaultfd` | 解决 userfaultfd() 权限问题                                   |

`memfd` 是一个特别值得关注的设计。它本质上是创建了一块有 fd
引用的匿名共享内存，相比传统的 `/dev/shm` 方式，`memfd` 有以下优势：

- 自动清理：fd 关闭或进程退出后自动释放，不会像 `/dev/shm` 下的文件那样残留
- 无路径暴露：不需要在文件系统中创建可见路径
- 支持 sealing：可以设置只读或不可再调整大小，增强安全性

`pidfd` 提供了基于文件描述符的进程引用，配合 `pidfd_open`、`pidfd_send_signal`
等系统调用，可以解决 目标进程退出后，PID 可能被新进程重用的问题发:

在 Linux 中可以通过 mmap 的方法使用大页，也可以通过 hugetlbfs 来创建。

从 pipe 到 `socketpair(AF_UNIX, SOCK_STREAM, 0, fd)`

类似 msharefs 也是一个经典案例，2026-06-09 暂时没有合并到内核中:

- https://mp.weixin.qq.com/s/OavFbBFanLrLiHQI3aAGow
- https://www.phoronix.com/news/Linux-Sharing-PTEs-Processes

使用 `/dev/userfaultfd` 而不是 userfaultfd() syscall ，具体可以看看
https://lwn.net/Articles/897260/

fexecve 也算是一个小花样了。

## 为什么 fd 是内核的宠儿？

将各种资源抽象为 fd，并非偶然的设计偏好，而是工程上的深思熟虑。

1. 全局唯一标识

fd
是进程级别的整数句柄，内核通过它就能唯一标识一个打开的文件或资源。这种统一的命名空间大大简化了内核与用户空间的接口设计。

2. 统一的操作接口

一旦变成 fd，就可以复用已有的系统调用族：

- `read()` / `write()`：数据读写
- `mmap()`：内存映射，实现共享内存
- `fcntl()`：获取/设置资源属性
- `ioctl()`：设备特定控制
- `close()`：资源释放

这意味着新增一种 fd 类型时，不需要设计全新的系统调用接口，用户空间也无需学习新的
API 范式。

3. 事件驱动的天然适配：`epoll` / `poll` / `select`

现代高并发程序普遍基于事件循环（event loop）。fd 可以被注册到 `epoll`
中，使得各种异构资源（网络
socket、定时器、信号、文件变更、进程退出）**在一个统一的事件循环中被等待和处理**。这是
signalfd、timerfd、eventfd 等机制最核心的价值。

4. 进程间传递（FD Passing）

通过 Unix Domain Socket 的 `SCM_RIGHTS` 辅助消息，fd
可以在进程间传递。这一能力极其强大：

- 特权进程（如 libvirt）可以打开受保护的资源，然后将 fd 传递给非特权进程（如
  QEMU）
- 实现资源的 **capability
  委托**：接收方不需要重新打开文件，直接继承发送方的访问权
- 容器环境中避免路径权限问题

从 `/dev/shm` 到 `memfd` 的演进就是一个例子：`memfd` 创建的匿名内存可以通过 fd
passing 在进程间共享，而不需要依赖全局可见的文件路径。

5. 引用计数与生命周期管理

fd 在内核中天然具有引用计数。当所有指向某个内核对象的 fd 都被 `close()`
后，对象自动释放。这解决了共享资源的棘手生命周期问题。

例如：为了共享内存给 vhost（SPDK / virtiofsd），原本使用 `/dev/shm`
下的共享文件。虽然配置了 `discard-data=on`，正常退出时会自动清理，但如果 QEMU 被
`kill -9`，这些文件不会自动删除。改用 `memfd` 后，fd
关闭即释放，完美解决了残留问题。

```bash
# 过去需要的清理脚本
shopt -s nullglob
find /dev/shm/ -name "qemu*" -print0 -delete
```

## 实际案例：QEMU 的 fd 传递机制

QEMU 的 `-add-fd` 参数是 fd 传递在真实生产环境中的经典应用。

假设你需要让 QEMU 将 guest 的串口输出写入一个日志文件。普通做法：

```bash
-chardev file,path=/tmp/xxx.log
```

实际上，libvirt
先以高权限身份打开日志文件：`open("/var/log/libvirt/qemu/vm-serial0.log", O_CREAT|O_WRONLY|O_APPEND)`，得到
fd=108 然后可以通过 Unix Domain Socket 将 fd 传输给 QEMU ，具体看
monitor/fds.c:qmp_add_fd

```bash
# 1. 将外部已打开的 fd=108 注册到 QEMU 的 fdset 33 中
-add-fd set=33,fd=108

# 2. 让 chardev 使用这个 fdset，而不是自己 open 文件
# /dev/fdset/33 不是 Linux 真实的设备节点，而是 QEMU 内部实现的"伪路径协议"
-chardev file,id=charserial1,path=/dev/fdset/33,append=on

# 3. 将这个 chardev 作为 guest 串口
-serial chardev:charserial1
```

## QEMU 到底打开了那些文件
<!-- c7e86aa2-6f9b-40a1-9c42-06af5a7768df -->

cd /proc/$qemu_pid/fd 下，可以看到:
```txt
0 -> 'pipe:[10816829]'
1 -> /home/martins3/.local/share/pueue/task_logs/28.log
2 -> /home/martins3/.local/share/pueue/task_logs/28.log

14 -> /home/martins3/data/hack/vm/fedora/s/debugcon.log
16 -> /home/martins3/data/hack/vm/fedora/1.qcow2
5 -> /home/martins3/data/hack/vm/fedora/s/pid
7 -> 'socket:[11012803]'
10 -> 'socket:[10826967]'
11 -> 'socket:[10826968]'
12 -> 'socket:[10826969]'
13 -> 'socket:[10826970]'
21 -> 'socket:[10827048]'
15 -> 'socket:[10992463]'
158 -> 'socket:[10794653]'
159 -> 'socket:[10794654]'
498 -> 'socket:[11012804]'
17 -> /dev/kvm
18 -> anon_inode:kvm-vm
19 -> /dev/net/tun
20 -> /dev/net/tun
22 -> '/memfd:memory-backend-memfd (deleted)'
154 -> '/memfd:displaysurface (deleted)'
156 -> '/memfd:displaysurface (deleted)'
157 -> '/memfd:displaysurface (deleted)'
23 -> 'anon_inode:[eventpoll]'
3 -> 'anon_inode:[eventpoll]'
6 -> 'anon_inode:[signalfd]'

# 下面这些重复太多了，所以就移除
84 -> anon_inode:kvm-vcpu:29
99 -> anon_inode:kvm-vcpu-stats:36
286 -> 'anon_inode:[eventfd]'
```

由于这些都是显示为 symbolic ，但是实际上没有这个路径，如果执行 ls ，这里就全部都是红色
例如:
```txt
[root@localhost fd]# file 99
99: symbolic link to anon_inode:kvm-vcpu-stats:36
```

vfs 很有趣软连接的名称设置

以 kvm-vcpu-stats 为例子:

- kvm_vcpu_ioctl_get_stats_fd

可以支持的命令为:
```txt
(qemu) info stats vm
(qemu) info stats vcpu
```
不过实现 vcpu 细化查询，这就是 debugfs 中的内容啊

## fd 抽象有代价吗？

有时候，有点语义过载，例如 mmap 一个 signalfd 的含义是什么呢？

对于 memfd 的 inotify 是什么含义?

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
