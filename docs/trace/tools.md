## ps 常见用法

### ps -elf

- ps aux : 和 ps -elf 等价
> a = show processes for all users
> u = display the process's user/owner
> x = also show processes not attached to a terminal

[What does aux mean in `ps aux`?](https://unix.stackexchange.com/questions/106847/what-does-aux-mean-in-ps-aux)

### ps f

### pstree

## vmstat

各种基础功能:
```txt
🧀  vmstat -h

Usage:
 vmstat [options] [delay [count]]

Options:
 -a, --active           active/inactive memory
 -f, --forks            number of forks since boot
 -m, --slabs            slabinfo
 -n, --one-header       do not redisplay header
 -s, --stats            event counter statistics
 -d, --disk             disk statistics
 -D, --disk-sum         summarize disk statistics
 -p, --partition <dev>  partition specific statistics
 -S, --unit <char>      define display unit
 -w, --wide             wide output
 -t, --timestamp        show timestamp

 -h, --help     display this help and exit
 -V, --version  output version information and exit

For more details see vmstat(8).
```
- [ ] 这里统计了从开机到现在的 fork 数量吗？

## ipcs

## htop
- [ ] htop 中，如何控制 CPU 条的大小，在核心很多的位置上，这个 CPU 条被严重压缩了。

checksheet : https://wangchujiang.com/reference/docs/htop.html

### 操作
- u : 按照 user 显示
- k : 给 process 发送信号
- l : 显示打开的文件
- s : 对于 process attach 上 strace
- Space : 把当前所在行的进程进行标记，U 则是取消标记
- K : 显示 kernel thread
- h : help

- H : 没有看懂 man


### 界面
- VIRT 是虚拟内存大小
- RES 是实际使用的物理内存大小
- SHR 是使用的共享页的大小

### processs 的状态

```txt
R    running or runnable (on run queue)
S    interruptible sleep (waiting for an event to complete)
D    uninterruptible sleep (usually IO)
Z    defunct ("zombie") process, terminated but not reaped by its parent
T    stopped by job control signal
t    stopped by debugger during the tracing
X    dead (should never be seen)
```

### top
- [ ] 打开 top, 按数值 1 的时候，可以观测那个 CPU 上的 softirq 发生的频率
- [ ] top 是如何统计 usr 和 sys 的

## 文摘

- [htop](https://peteris.rocks/blog/htop/)

## 源码位置
- https://github.com/util-linux/util-linux
  - https://en.wikipedia.org/wiki/Util-linux
- https://gitlab.com/procps-ng/procps
