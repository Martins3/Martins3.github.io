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

# [perf-little-book](https://nanxiao.gitbooks.io/perf-little-book/content/)

## 问题
- [x] perf_events 在内核中间的位置在哪里 ? kernel/events 中间
- [ ] perf 可以实现什么，不可以实现什么东西 ? 从 perf man 的最下面的几个方向。
- [ ] 为什么可以定位符号，gcc -g 到底生成了什么东西
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

`perf diff` command is used to displays the performance difference amongst two or more perf.data files captured via perf record command.
