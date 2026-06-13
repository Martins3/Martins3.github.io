## 先不搞那些虚的东西，分析清楚下面这个问题
- Documentation/trace/
  - Documentation/trace/events.rst : /sys/kernel/debug/tracing/events 中的使用，包括 filter 和 trigger
    - Documentation/trace/tracepoint-analysis.rst : 重复了 ?
  - Documentation/trace/ftrace-design.rst
  - Documentation/trace/timerlat-tracer.rst

## 总结
- 所以 ftrace 的 kprobe 和 ftrace 的 function tracer 有什么区别吗?
  - ftrace 可以帮助 kprobe 的实现
  - ftrace function tracer 无法增加额外的代码，但是 kprobe 可以增加额外的代码
- 相对于 kprobe 的功能，ftrace 提供了
  - function graph
  - stack trace
  - profile

## 更加好用的前端分析工具

- [hotspot](https://github.com/KDAB/hotspot) :star:

- https://github.com/cyring/CoreFreq

- [sysdig](https://github.com/draios/sysdig)

- [kernelshark 使用介绍](https://elinux.org/images/6/64/Elc2011_rostedt.pdf)
- [kernelshark](https://www.cnblogs.com/arnoldlu/p/9014365.html)

使用 csysdig 可以得到一些类似 htop 的界面
```sh
csysdig -l
```

- sysdig 可以监控整个系统中发生的所有正在发生那些
  - 系统调用 : 我们发现很多都是 futex
  - 所有的 IO : pipe unix netlink
    - 并且检查所有在使用 pipe 的进程
      - 而且可以将这些进程的 backtrace 打印出来

- [ ] https://github.com/aquasecurity/tracee : nixos 上可以安装，但是无法使用，docker 中执行失败

- [ ] https://perfetto.dev/docs/ :  android 上的分析工具

- [uftrace](https://github.com/namhyung/uftrace)
仅仅支持 c / c++，可以非常精准将函数的调用图生成出来，编译代码需要增加额外的 flags


## [ ] cflow
- https://graphviz.org/
- https://graphviz.org/pdf/gvpr.1.pdf
- https://www.gnu.org/software/cflow/manual/cflow.html : 可以绘制整个图形的

## rtla
- https://lwn.net/Articles/869563/
- https://bristot.me/and-now-linux-has-a-real-time-linux-analysis-rtla-tool/

Linux调度延迟调试分析利器：深度解析rtla工具 - 超龄码农的文章 - 知乎
https://zhuanlan.zhihu.com/p/1931440485681049801

原来 rtla 是做这个东西

## 针对于特定语言的
- python : https://github.com/benfred/py-spy
- [memray](https://github.com/bloomberg/memray) : python 的内存使用

- https://github.com/tikv/pprof-rs : 给 rs 来实现 perf 的 ?

- https://github.com/felixge/fgprof : pprof-rs 对应的 go 的实现的版本

只有 python 需要针对的分析工具吗?

- [pprof](https://github.com/google/pprof)
  - 和这个东西是什么关系? https://github.com/gperftools/gperftools
  - [ ] 如果 pprof 似乎是可以 C 语言工作的，但是 gperf 据说已经很好用了
  - [ ] https://github.com/jrfonseca/gprof2dot
    - 这个工具是被我们使用上了，但是本身是一个将各种 perf 结果生成新的结果的工具，可以看看原来的结果的位置

- https://github.com/javamelody/javamelody : JavaMelody : monitoring of JavaEE applications


## 看看邹大哥的文章
https://lawrencezx.github.io/blogs/2022-3-Linux-Dynamic-Tracing.html


## 写一个更新版本的
[Linux tracing systems & how they fit together](https://jvns.ca/blog/2017/07/05/linux-tracing-systems/)

基本没有问题

## 其他项目
- https://github.com/koute/bytehound

- https://gitee.com/anolis/sysak : 里面有大量的脚本， 也许可以参考下
  - [surftrace](https://gitee.com/anolis/surftrace) : ftrace 封装以及两个编译器，但是我不知道相对于 bpftrace 有啥优势
- [linux tracing workshop](https://github.com/goldshtn/linux-tracing-workshop) : 教程，但是没有维护了

## 常看长新的
- [Linux Performance](http://www.brendangregg.com/linuxperf.html)

## 应该已经没有人用了

3. https://oprofile.sourceforge.io/about/
4. dtrace


## 想法
- 实际上，通过 function trace 可以制作函数级别的 code coverage
- 借鉴 kernel_visualization 中的案例，实际上可以利用 function graph trace 制作一个函数调用图出来

## trace 的实现

1. 解析 stack 绝对是一个复杂的事情
-  https://news.ycombinator.com/item?id=35592446
2. CONFIG_TASKS_RCU 中看，ftrace 居然是和 rcu 有关的
- https://docs.kernel.org/RCU/Design/Requirements/Requirements.html#tasks-rcu

## TODO

1. 如果想要获取 softirq_raise 的参数是 RCU_SOFTIRQ 时候的 backtrace ，暂时不知道怎么处理
```txt
sudo bpftrace -e "tracepoint:irq:softirq_raise { @[kstack] = count(); }"
```
  - 只是有点复杂，但是肯定是可以的
3. 需要等待 bpf 理解之后重新翻回来看看，理解 bpf 需要首先理解 k8s

4. 整理一个观察，bpftrace 的 kfunc 是无法在 trace_softirq_raise 上设置的


5. https://lists.gnu.org/archive/html/qemu-devel/2013-04/msg00505.html
  - qemu 为什么会存在 ftrace 的后端?

6. bcc 似乎还可以直接操作用户态

## TODO 最近又增加了一些 trace 技术
https://lore.kernel.org/all/170952359657.229804.14867636035660590574.stgit@devnote2/

## TODO user stack 的 trace 也需要分析
https://lwn.net/Articles/889607/

## 对比 tracepoint 在 gcc 和 llvm 的实现差别

## uprobe 使用
objdump -T /usr/libexec/qemu-kvm | grep -w x86_cpu_dump_state
```txt
0000000000644c00 g    DF .text  00000000000009e0  Base        x86_cpu_dump_state
```

cat /proc/`pgrep qemu`/maps | grep qemu | grep r-xp
```txt
55a0d25b8000-55a0d2aed000 r-xp 003df000 fd:03 3181654                    /usr/libexec/qemu-kvm
```

echo 'p:myprobe /usr/libexec/qemu-kvm:0x55a0d2bfcc00 %ip' > /sys/kernel/debug/tracing/uprobe_events
echo 1 > /sys/kernel/debug/tracing/events/uprobes/myprobe/enable

https://blog.quarkslab.com/defeating-ebpf-uprobe-monitoring.html

不知道为什么我们自己构建的 qemu 使用 objdump -T /usr/libexec/qemu-kvm 几乎没有输出什么，
似乎是后来的 qemu 修改了构建方法，尤其是 nixos 自带的 qemu-kvm 只有不到 10 个符号。

## kprobe 获取函数参数

问题是这里的 offset 是很难获取的:

echo 'p:myprobe vmx_get_msr vcpu=$arg1' > /sys/kernel/debug/tracing/kprobe_events

echo 1 > /sys/kernel/debug/tracing/tracing_on # 虚拟机中为什么是默认打开的
```sh
echo 'p:myprobe vmx_get_msr msr=+8000($arg1)' > /sys/kernel/debug/tracing/kprobe_events
echo 1 > /sys/kernel/debug/tracing/events/kprobes/myprobe/enable
sleep 10
cat /sys/kernel/debug/tracing/trace
echo 0 > /sys/kernel/debug/tracing/events/kprobes/myprobe/enable
echo -:myprobe3 >> /sys/kernel/debug/tracing/kprobe_events
```

删除:
```sh
echo 0 > /sys/kernel/debug/tracing/events/kprobes/myprobe/enable
echo -:myprobe3 >> /sys/kernel/debug/tracing/kprobe_events
```

```sh
echo 'p:myprobe3 qi_flush_piotlb iommu=$arg1 did=$arg2 addr=$arg4 npages=$arg5 ih=$arg6' > /sys/kernel/debug/tracing/kprobe_events
echo 1 > /sys/kernel/debug/tracing/events/kprobes/myprobe3/enable
```

## 为什么说增加 tracepoint 要比增加 printk 更好

就算是使用重新构建，tracepoint 也要比 printk 好很多。


## QEMU 的这个 trace 文档还是需要看看的
docs/devel/tracing.rst

## 看看这个项目
https://github.com/kernel-cyrus/tracecat?tab=readme-ov-file


## trace 的内容放到 userspace
- https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/commit/?id=53683e408013

## 在 terminal 中检查 flamegraph
https://github.com/jonhoo/inferno
https://github.com/YS-L/flamelens
https://github.com/davidmarkclements/0x

## 这个应该如何归类?
https://github.com/plasma-umass/coz

https://github.com/Linaro/OpenCSD

## 尝试下这个工具
https://0x.tools/ : 等 nixos 上有打包之后再说吧

https://github.com/plasma-umass/scalene

## trace
- [Debug Hacks](https://book.douban.com/subject/6799412/) 内核调试的老技术


## 经典
- https://mp.weixin.qq.com/s/28MPKE5Er77hSf-HsyEu6g
  - 如何处理中断
  - 分析了 off cpu 的跟踪场景

## 计划一下
https://www.brendangregg.com/FlameGraphs/cpuflamegraphs.html



## A Top-Down Method for Performance Analysis and Counters Architecture
<!-- 39478226-34df-4f6f-8415-03750ba2314b -->

https://rcs.uwaterloo.ca/~ali/cs854-f23/papers/topdown.pdf

先看四个顶层问题:
- Retiring（有效干活）
- Bad Speculation（白跑）
- Frontend Bound（前端卡住）
- Backend Bound（后端卡住）

https://cloud.tencent.com/developer/article/1844992

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
