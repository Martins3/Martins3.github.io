.. _perf:

=============
perf
=============

总结
----

perf可以做很多事情，但是最关键的功能利用采样的方法来分析CPU性能。当提到perf的时候，有时候是指用户层的 `perf-tools <https://github.com/brendangregg/perf-tools>`_ ,有的时候指的内核态的perf_events，两者的关系可以用 perf_tools 的自我介绍说明清楚:

**Performance analysis tools based on Linux perf_events (aka perf) and ftrace.**

也就是说，perf-tools是perf_events和ftrace的封装工具，perf到底指的是什么，可以很容易从上下文中间判断。下面总结perf的基本使用方法，详细的内容参看 :ref:`perf`

.. todo::

  - 说明 flamegraph 和 https://github.com/KDAB/hotspot 如何进一步简化工作
  - https://github.com/google/pprof

首先区分两个东西:
- https://github.com/brendangregg/perf-tools 内部只是各种bash脚本，
- tools/perf : 可以从 perf.c 开始分析

.. todo::
     1. perf-tools 和 perf 的功能的区别和实现的差异是什么 ?
     2. 是不是 perf-tools 调用了 perf ?


最核心的功能 : samplings 体现在哪里 ? 应该是 stat 和 record，但是这两个都是可以指定 event 的，非常不理解!

不核心的功能:
1. strace
2. event
3. probe

各种数据操作:
1. top
2. stat

perf
----
perf record 和 perf report

.. list-table:: perf 功能简介
   :widths: 25 25
   :header-rows: 0

   * - perf stat
     - wow

   * - perf top
     - wow

perf list
*********
- 功能 : List all symbolic event types


PMU 的功能是 : performance monitor unit

.. code-block:: sh

    perf list | fzf

.. todo::
  1. event modified 一共存在那些 ?
  2. 什么叫做 symbolic event types ?
  4. 一些奇怪的和处理器 specific 的东西
  5. event group 
  6. leader samplings

perf bench
**********
对于 sched mem numa futex epoll 几个子系统进行测试

.. todo::
  岂不是可以采用这个作为比对标准吗 ?

perf ftrace
***********
其实是对于 debugfs 中间的 ftrace 的简单封装，控制cpu，pid，func 和 function_graph 的

.. todo::
  - 所以 ftrace 是如何实现 pid 区分
  - 这个东西到底跟踪的是啥呀 ? 该函数是否被调用吗 ?

perf annotate/record/report
***************************

perf-annotate - Read perf.data (created by perf record) and display annotated code
perf-report - Read perf.data (created by perf record) and display the profile


perf record 和 perf stat 很相似，但是 perf stat 利用 perf stat report 进行输出。


.. todo::
  1. annotate 和 report 似乎都是将数据输出，然后加以分析，选项太多了，懒得看了。
  2. report 分析了一下 overhead calculation


perf archive/buildid-list/buildid-cache
***************************************
.. todo::
  1. 什么是 buildid ?
  2. This command displays the buildids found in a perf.data file, so that other tools can be used to fetch packages with matching symbol tables for use by perf report. 中间的 other tools 指的是 ?
  3. buildid-cache 和 perf-probe 是什么关系呀 ?

perf probe
**********
kprobe 和 uprobe 的工具

.. todo::
  细节，插入的语法

perf data
*********
转化为 `ctf <http://www.efficios.com/ctf>`_ 格式

perf kallsyms
*************
This command searches the running kernel kallsyms file for the given symbol(s) and prints information about it, including the DSO, the kallsyms begin/end addresses and the addresses in the ELF kallsyms symbol table (for symbols in modules).

.. todo::
   1. 和普通的读取 kallsyms 有区别吗 ?
   2. kallsyms 文件在哪里 ?

perf mem sched
**************

.. todo::
   1. 似乎存在非常多的位置对于 sched 和 mem 进行分析



perf-tools
----------
.. todo::
  1. 似乎其中所有的脚本都是利用 ftrace 实现
  2. 找到其中和 perf 相关的内容，试图证明 perf 可以利用 ftrace 的脚本实现


perf_event_open
---------------
这应该是操作系统唯一提供用于获取 perf 性能的内容:

.. note::

  A call to perf_event_open() creates a file descriptor that allows measuring performance information.  Each file descriptor corresponds to one event that is measured; these can be grouped together to measure multiple events simultaneously.

  Events come in two flavors: counting and sampled.  A counting event is one that is used for counting the aggregate number of events that occur.  In general, counting event results are gathered with a read(2) call.  A sampling event periodically writes measurements to a buffer that can then be accessed via mmap(2).


.. todo::
  1. prtcl 系统调用的含义
  2. 参数 group_id 的作用


理解 struct perf_event_open
****************************
- type 和 contig : 说明到底可能跟踪什么，其中 type 指明大类，contig 说明具体类型
- perf_event_open 也是可以用于获取 tracepoints, kprobe，uprobe

.. todo::
  1. 性能计数器总是打开的吗 ? 还是 perf_event_open 来打开的
  2. 为什么可以读取 tracepoints 的数据，是因为其进入到 ftrace 的缓冲区中间读取的吗 ?

参考资料
-------
