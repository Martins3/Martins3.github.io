## tick 模式
<!-- 31828d65-5b5f-4828-9157-381d230e5af5 -->

## 三种模式
目前的 tick 一共三个模式:
- HZ_PERIODIC
- NO_HZ_IDLE : 默认模式，只有 idle 的时候才会关闭
- NO_HZ_FULL

一般配置为:
```txt
CONFIG_NO_HZ_COMMON=y
CONFIG_NO_HZ_IDLE=y
# CONFIG_NO_HZ_FULL is not set
CONFIG_NO_HZ=y
```

这三个模式为什么不可以类似 preemption 一样，可以动态切换的?

## 关键数据结构
tick-sched.c
核心结构体 `struct tick_sched`


## 实验
如果没有负载，那么 /proc/interrupts 中的 Local timer interrupts 的增加很缓慢，但是如果有负载，
那么按照 CONFIG_HZ 增加。 即便是只有一个 core ，也是如此。

而且 CPU 0 也不会一直保持 CONFIG_HZ 增加。

配合命令:
```sh
viddy -n 1 'cat /proc/interrupts | grep "Local timer interrupts"'

stress-ng --vm-bytes 200M --vm-keep --vm 1
```

## 问题
### 如何决定停止 tick 的
什么地方会停止 tick ？

重启 tick : tick_nohz_restart_sched_tick

```txt
@[
    tick_nohz_restart_sched_tick+1
    tick_nohz_idle_exit+162
    do_idle+322
    cpu_startup_entry+41
    rest_init+193
    start_kernel+1659
    x86_64_start_reservations+24
    x86_64_start_kernel+197
    common_startup_64+318
]: 48
@[
    tick_nohz_restart_sched_tick+1
    tick_nohz_idle_exit+162
    do_idle+322
    cpu_startup_entry+41
    start_secondary+286
    common_startup_64+318
]: 55

```

### 为什么这个没有触发?
```sh
sudo bpftrace -e 'kprobe:tick_handle_periodic_broadcast { @[kstack(bpftrace)] = count(); }'
```

### 具体在 idle 的位置如何实现的?
- do_idle
  - tick_nohz_idle_enter : 如果配置为 HZ_PERIODIC，这个函数是空函数，如果额外配置了 NO_HZ_FULL，那么其对应的一些系列函数都会展开
  - idle 子系统
  - tick_nohz_idle_exit

## 似乎这个问题关系了好几个问题

1. 似乎和 softlocup 有关
2. 似乎和 csatte 有关
3. 显然和 kernel/sched/loadavg.c 有关的

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
