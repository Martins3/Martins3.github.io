===========
ftrace
===========
The trace-cmd(1) record command will set up the Ftrace Linux kernel tracer to record the specified plugins or events that happen while the command executes. If no command is given, then it will record until the user hits Ctrl-C.

.. todo::
  1. 和 perf trace 的功能重复了，或者说想必，强化是什么 ?
  2. trace-cmd 可以 record 特定的 plugins 和 events，其中的 plugins 指的是 ?

trace-cmd 安装
--------------

::

  ➜  trace-cmd git:(master) ✗ pkg-config --cflags python
  Package python was not found in the pkg-config search path.
  Perhaps you should add the directory containing `python.pc'
  to the PKG_CONFIG_PATH environment variable
  No package 'python' found

pkg-config 是个好东西，虽然不知道是什么原理。

对于的修改为:

.. code:: diff

  -PYTHON_VERS ?= python
  +PYTHON_VERS ?= python2

存在类似的错误，似乎无人解决:
https://bugzilla.redhat.com/show_bug.cgi?id=1322287

trace-cmd 用法
-------------

trace-cmd record
****************


.. todo::
  1. start 和 record 的唯一区别就是两者记录文件的区别吗 ?
  2. 

trace-cmd list
**************

显示的内容存在三种 : event trace-cmd option

trace-cmd start/stop/extract
****************************

1. 利用 trace-cmd-extract

.. note::
  The trace-cmd(1) extract is usually used after trace-cmd-start(1) and trace-cmd-stop(1). It can be used after the Ftrace tracer has been started manually through the Ftrace pseudo file system.

  The extract command creates a trace.dat file that can be used by trace-cmd-report(1) to read from. It reads the kernel internal ring buffer to produce the trace.dat file.


2. trace-cmd-reset

.. note::
   The trace-cmd(1) reset command turns off all tracing of Ftrace. This will bring back the performance of the system before tracing was enabled. This is necessary since trace-cmd-record(1), trace-cmd-stop(1) and trace-cmd-extract(1) do not disable the tracer, event after the data has been pulled from the buffers. The rational is that the user may want to manually enable the tracer with the Ftrace pseudo file system, or examine other parts of Ftrace to see what trace-cmd did. After the reset command happens, the data in the ring buffer, and the options that were used are all lost.


3. trace-cmd-stop

.. note::
       The trace-cmd(1) stop is a complement to trace-cmd-start(1). This will disable Ftrace from writing to the ring buffer. This does not stop the overhead that the tracing may incur. Only the updating of the ring buffer is disabled, the Ftrace tracing may still be inducing overhead.

       After stopping the trace, the trace-cmd-extract(1) may strip out the data from the ring buffer and create a trace.dat file. The Ftrace pseudo file system may also be examined.

       To disable the tracing completely to remove the overhead it causes, use trace-cmd-reset(1). But after a reset is performed, the data that has been recorded is lost.

4. trace-cmd-restart

.. note::
  trace-cmd restart [-B buf [-B buf]..] [-a] [-t]
          Restarts recording after a trace-cmd stop.
          Used in conjunction with stop
          -B restart a given buffer (more than one may be specified)
          -a restart all buffers (except top one)
          -t restart the top level buffer (useful with -B or -a)

.. todo::
  为什么存在用户需要手动解析内容的需求


trace-cmd options
*****************
.. todo::
  完全无法理解其输出是什么

trace-cmd stat
**************
trace-cmd stat 不是用于实现统计的，而是类似于文件系统中间的 stat 的功能，或者说，其作用是查看当前 ftrace-cmd 的功能

.. note::
       The trace-cmd(1) stat displays the various status of the tracing (ftrace) system. The status that it shows is:

       - Instances: List all configured ftrace instances.
       - Tracer: if one of the tracers (like function_graph) is active. Otherwise nothing is displayed.
       - Events: Lists the events that are enable.
       - Event filters: Shows any filters that are set for any events
       - Function filters: Shows any filters for the function tracers
       - Graph functions: Shows any functions that the function graph tracer should graph
       - Buffers: Shows the trace buffer size if they have been expanded. By default, tracing buffers are in a compressed format until they are used. If they are compressed, the buffer display will not be shown.
       - Trace clock: If the tracing clock is anything other than the default "local" it will be displayed.
       - Trace CPU mask: If not all available CPUs are in the tracing CPU mask, then the tracing CPU mask will be displayed.
       - Trace max latency: Shows the value of the trace max latency if it is other than zero.
       - Kprobes: Shows any kprobes that are defined for tracing.
       - Uprobes: Shows any uprobes that are defined for tracing.
       - Error log: Dump the content of ftrace error_log file.

.. todo::
  1. instances 是什么
  2. trace clock ?
  3. trace max latency ?

trace-cmd hist
**************

trace-cmd profile
*****************

.. note::
       The trace-cmd(1) profile will start tracing just like trace-cmd-record(1), with the --profile option, except that it does not write to a file, but instead, it will read the events as they happen and will update the accounting of the events. When the trace is finished, it will report the results just like trace-cmd-report(1) would do with its --profile option. In other words, the profile command does the work of trace-cmd record --profile, and trace-cmd report --profile without having to record the data to disk, in between.

       The advantage of using the profile command is that the profiling can be done over a long period of time where recording all events would take up too much disk space.

       This will enable several events as well as the function graph tracer with a depth of one (if the kernel supports it). This is to show where tasks enter and exit the kernel and how long they were in the kernel.

       To disable calling function graph, use the -p option to enable another tracer. To not enable any tracer, use -p nop.

       All timings are currently in nanoseconds.

好吧，出现如下错误了，而且还是无法理解 profile 的作用

.. error::
  trace-cmd: No such file or directory
    [xhci-hcd:xhci_setup_device_slot] function xhci_decode_slot_context not defined

.. todo::
   1. perf 存在类似的功能吗 ?

trace-cmd stack
***************

.. todo::
  1. 好吧，我没有办法正确的使用这个东西
  2. 曾经以为 profile 就是分析 stack

trace-cmd check-events
**********************
会产生和 profile 完全相同的错误，用于检查 event 的格式

kernel shark
------------
kernel shark 是 trace-cmd 的封装
