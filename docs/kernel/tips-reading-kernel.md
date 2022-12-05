# 使用 QEMU 和 ebpf 阅读内核

在[上一篇文章](https://martins3.github.io/kernel/learn-linux-kernel.html)中，提出了学习内核的目的，工作的方向以及大致的学习路径，下面谈一下可以提高分析效率的工具。

## QEMU
使用 QEMU 调试内核网上已经有很多的文章, 比如 [Booting a Custom Linux Kernel in QEMU and Debugging It With GDB](http://nickdesaulniers.github.io/blog/2018/10/24/booting-a-custom-linux-kernel-in-qemu-and-debugging-it-with-gdb/)

QEMU 很强大，但是 QEMU 的参数对于新手不是很友好，我写了一个[QEMU 的基本使用方法](https://martins3.github.io/qemu/manual.html)，
关于调试可以参考[这个脚本](https://github.com/Martins3/Martins3.github.io/blob/master/docs/qemu/sh/alpine.sh)

## FlameGraph
使用 FlameGraph 可以很快的定位出来函数的大致执行的流程，无需使用编辑器一个个的跳转，非常好用。
其局限在于，似乎总是只能从用户态系统调用触发。

具体细节参考 https://www.brendangregg.com/FlameGraphs/cpuflamegraphs.html

perf 可能需要安装:
```sh
sudo apt install linux-tools-common linux-tools-generic linux-tools-`uname -r`
```

最终效果如下，可以在新的窗口中打开从而可以动态交互。
![](./img/dd.svg)

这个工具我使用的非常频繁，所以构建了简单的[一个脚本](https://github.com/Martins3/Martins3.github.io/blob/master/docs/kernel/code/flamegraph.sh)，例如:

```sh
./flamegraph.sh -c 'iperf -c localhost' -g iperf
```

## bpftrace
使用 bpftrace 的 kprobe 可以很容易的动态的获取内核函数的 backtrace

首先可以使用 sudo bpftrace -l | fzf 检查可以观测的点

脚本：
```bt
#!/usr/bin/bpftrace
kprobe:task_tick_fair
{
  @[kstack] = count();
}
```

结果:
```txt
@[
    task_tick_fair+1
    update_process_times+187
    tick_sched_handle.isra.0+37
    tick_sched_timer+109
    __hrtimer_run_queues+251
    hrtimer_interrupt+265
    __sysvec_apic_timer_interrupt+100
    sysvec_apic_timer_interrupt+56
    asm_sysvec_apic_timer_interrupt+18
]: 171
```

<!-- ## [ ] gcov -->

<!-- ## TODO -->
<!-- - [ ] 将 QEMU 的基本使用变为一个单独的文章分析一下 -->
<!--   - 分析各种常用的技术 -->
