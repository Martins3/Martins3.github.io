# [linux tracing workshop](https://github.com/goldshtn/linux-tracing-workshop) 阅读笔记

## 1
1. /sys/kernel/debug/ 和 /sys/kernel/tracing 是什么关系呀 ?
2. uprobe 的能力让人感到震惊

> TODO bounus 的部分没有完成，而且有一部分完全没有思路

## 2
只是说明 perf 可以用于原生的，jvm 以及 v8 的性能监控
```c
gcc -g -fno-omit-frame-pointer -fopenmp primes.c -o primes
perf record -g -F 997 -- ./primes
perf report --stdio
perf annotate
perf script > primes.stacks
FlameGraph/stackcollapse-perf.pl primes.stacks > primes.collapsed
FlameGraph/flamegraph.pl primes.collapsed > primes.svg
```

1. Java 的内容操作上有点不对啊 !
2. bonus 部分没有操作，似乎需要 debuginfo，似乎不需要重新编译就可以得到 debuginfo

## 3
pidstat -u -p $(pidof server) 1
sudo syscount  -p $(pidof server)

// 似乎需要重新编译内核，提供 BTF 支持才可以
sudo opensnoop -p $(pidof server)

// 似乎也是不可以实现
```plain
argdist -p $(pidof server) -H 'p::SyS_nanosleep(struct timespec *time):u64:time->tv_nsec'
```
// 访问 bcc，中间的例子，发现根本不能使用使用


## 废话一些安装的事情
1. setproxy 真的有效 git clone 的时候

2. BCC 安装:
按照这里
https://github.com/goldshtn/linux-tracing-workshop/blob/master/setup-fedora.sh
然后添加上:
MANPATH=/usr/share/bcc/man:$MANPATH
PATH=/usr/share/bcc/tools:$PATH

# [perf-little-book](https://nanxiao.gitbooks.io/perf-little-book/content/)

## 问题
- [x] perf_events 在内核中间的位置在哪里 ? kernel/events 中间
- [ ] perf 可以实现什么，不可以实现什么东西 ? 从 perf man 的最下面的几个方向。
- [ ] 为什么可以定位符号，gcc -g 到底生成了什么东西
- [ ] ftrace 和 perf 是什么关系呀 ? 至少应该是功能不同的东西吧，如果 perf 采用 sampling 的技术，而 ftrace 可以知道其中
- [ ] hardware breakpoint 是什么东西 ?


## 笔记
```plain
/proc/sys/kernel/perf_event_paranoid
```

`Skid` is the distance between events which trigger the issue and the events which are actually caught by perf

每一个 thread:
```plain
perf record -s ./sum
perf report -T
```

profile system in realtime:
```plain
perf top
```

trace 命令的 syscall:
```plain
perf trace ls
```

measure scheduler latency:
```plain
perf sched record sleep 10
perf sched latency
```

也可以作为 ftrace 使用:
perf ftrace is a simple *wrapper* for kernel's ftrace functionality, and only supports single thread tracing now.
```plain
perf ftrace -T __kmalloc ./add_vec
perf ftrace ./add_vec
```

`perf diff` command is used to displays the performance difference amongst two or more perf.data files captured via perf record command.

## [strace little book](https://github.com/NanXiao/strace-little-book)

## 问题
- [ ] strace 可以监控 signal 和 process state 的变化

## 笔记
It is used to monitor and tamper with interactions between processes and the Linux kernel, which **include system calls, signal deliveries, and changes of process state.**

```plain
strace -A -o a.log ls
strace -p 2194,2195  # 同时跟踪多个进程
strace -p $(pidof dead_loop) # 利用 pidof 或者 pgrep
```

To trace child processes, -f option need to be specified
`-b` syscall option can be used to instruct strace to detach process when specified syscall is executed. However, currently only execve is supported.
`-D` option is used to detach strace and tracee processes. 使用此参数，tracee 不会作为 strace 的 children
```plain
strace -c ls # stat
strace -w -c ls # wall time
strace -c -S calls ls # sort by calls
```

-e expression 是一个强大的工具，可以控制输出的详细程度，约束到底跟踪哪一个东西，操控信号，注入 syscall 返回值

strace 执行的时候，可以修改执行的 trace 以及 修改指令执行的位置。

-I interruptible 用于屏蔽信号

算是有新的收获了，syscall 和 信号相关的都可以操作，甚至是屏蔽信号机制。

## https://jvns.ca/blog/2016/03/12/how-does-perf-work-and-some-questions/
1. /home/shen/Core/linux/kernel/events/core.c 中间定义了 perf_event_open 的内容
2. 输出放到 buffer ring 中间
```c
/*
 * Callers need to ensure there can be no nesting of this function, otherwise
 * the seqlock logic goes bad. We can not serialize this because the arch
 * code calls this from NMI context.
 */
void perf_event_update_userpage(struct perf_event *event)
```

## kdump 是怎么回事啊
