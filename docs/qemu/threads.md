# QEMU 到底有那些线程

QEMU 的执行流程大致来说是分为 io thread 和 vCPU thread 的。

![https://martins3.github.io/ppt/repo/2021-8-24/index.html](https://github.com/Martins3/ppt/blob/master/images/QEMU-ARCH.svg)

**一般来说**:
- 这个 io thread 就是指的是 main-loop.c 中 `qemu_main_loop` 执行的循环。
- vCPU 的取决于具体的 accel 是什么，`AccelOpsClass::create_vcpu_thread` 上会注册具体的 hook, 例如 kvm 注册的 kvm_start_vcpu_thread

对于双核配置，使用 gdb 的 `info thread` [^8][^9] 
```plain
  Id   Target Id                                             Frame
* 1    Thread 0x7fffeb1d4300 (LWP 1389363) "qemu-system-x86" 0x00007ffff61a6bf6 in __ppoll (fds=0x555556ad10f0, nfds=8, timeout=<optimized out>, timeout@entry=0x7fffffffd2c0, sigmask=sigmask@entry=0x0) at ../sysdeps/unix/sysv/linux/ppoll.c:44
  2    Thread 0x7fffeb073700 (LWP 1389367) "qemu-system-x86" syscall () at ../sysdeps/unix/sysv/linux/x86_64/syscall.S:38
  3    Thread 0x7fffea5fb700 (LWP 1389373) "gmain"           0x00007ffff61a6aff in __GI___poll (fds=0x5555569c41e0, nfds=1, timeout=-1) at ../sysdeps/unix/sysv/linux/poll.c:29
  4    Thread 0x7fffe9dfa700 (LWP 1389374) "gdbus"           0x00007ffff61a6aff in __GI___poll (fds=0x5555569cfe40, nfds=2, timeout=-1) at ../sysdeps/unix/sysv/linux/poll.c:29
  5    Thread 0x7fffe93f6700 (LWP 1389377) "qemu-system-x86" 0x00007ffff61a6bf6 in __ppoll (fds=0x7fffd4001ff0, nfds=1, timeout=<optimized out>, timeout@entry=0x0, sigmask=sigmask@entry=0x0) at ../sysdeps/unix/sysv/linux/ppoll.c:44
  6    Thread 0x7fffe8af4700 (LWP 1389378) "qemu-system-x86" 0x00007ffff6296618 in futex_abstimed_wait_cancelable (private=0, abstime=0x7fffe8af0220, clockid=0, expected=0, futex_word=0x555556730c78) at ../sysdeps/nptl/futex-internal.h:320
  7    Thread 0x7ffe51dff700 (LWP 1389381) "qemu-system-x86" 0x00007ffff61a850b in ioctl () at ../sysdeps/unix/syscall-template.S:78
  8    Thread 0x7ffe515fe700 (LWP 1389382) "qemu-system-x86" 0x00007ffff61a850b in ioctl () at ../sysdeps/unix/syscall-template.S:78
  9    Thread 0x7ffe48e29700 (LWP 1389385) "threaded-ml"     0x00007ffff61a6aff in __GI___poll (fds=0x7ffe38007170, nfds=3, timeout=-1) at ../sysdeps/unix/sysv/linux/poll.c:29
  10   Thread 0x7ffe29ddd700 (LWP 1389387) "qemu-system-x86" 0x00007ffff6296618 in futex_abstimed_wait_cancelable (private=0, abstime=0x7ffe29dd9220, clockid=0, expected=0, futex_word=0x555556730c78) at ../sysdeps/nptl/futex-internal.h:320
```

在 `qemu_thread_create` 中添加调试语句:
```plain
call_rcu
IO io0
worker
CPU 0/KVM
CPU 1/KVM
worker
```

分析上面的内容，可以看到:
- 加上 main loop 线程，`qemu_thread_create` 创建的线程是和 `info thread` 中叫做 "qemu-system-x86" 的线程数目完全对应的。
- 两个 vCPU 分别对应一个线程
- 有一些看不懂的 gmain / gdbus / threaded-ml


通过 `thread ${pid_num}` 和 `backtrace` 可以获取这几个 thread 的内部的执行流程。

比如 threaded-ml 的，gmain 和 gdbus 和这个类似，不列举了。
总之就是这些线程会调用到 poll 系统调用上, 来监听一些事情。
```c
/*
>>> thread 8
[Switching to thread 8 (Thread 0x7ffe51629700 (LWP 1186997))]
#0  0x00007ffff61a6aff in __GI___poll (fds=0x7ffe3c007170, nfds=3, timeout=-1) at ../sysdeps/unix/sysv/linux/poll.c:29
29      ../sysdeps/unix/sysv/linux/poll.c: No such file or directory.
>>> bt
#0  0x00007ffff61a6aff in __GI___poll (fds=0x7ffe3c007170, nfds=3, timeout=-1) at ../sysdeps/unix/sysv/linux/poll.c:29
#1  0x00007ffff6df31d6 in  () at /lib/x86_64-linux-gnu/libpulse.so.0
#2  0x00007ffff6de4841 in pa_mainloop_poll () at /lib/x86_64-linux-gnu/libpulse.so.0
#3  0x00007ffff6de4ec3 in pa_mainloop_iterate () at /lib/x86_64-linux-gnu/libpulse.so.0
#4  0x00007ffff6de4f70 in pa_mainloop_run () at /lib/x86_64-linux-gnu/libpulse.so.0
#5  0x00007ffff6df311d in  () at /lib/x86_64-linux-gnu/libpulse.so.0
#6  0x00007ffff56f272c in  () at /usr/lib/x86_64-linux-gnu/pulseaudio/libpulsecommon-13.99.so
#7  0x00007ffff628c609 in start_thread (arg=<optimized out>) at pthread_create.c:477
#8  0x00007ffff61b3293 in clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:95
```

这几个 thread 不是通过 `qemu_thread_create` 创建的，使用 gdb 在 `clone` 地方打断点，然后逐个 `backtrace` 可以看到所有的 thread 是如何创建的。
下面是 threaded-ml 的创建的过程:
```c
/*
#0  clone () at ../sysdeps/unix/sysv/linux/x86_64/clone.S:50
#1  0x00007ffff628b2ec in create_thread (pd=pd@entry=0x7ffe48e29700, attr=attr@entry=0x7fffffffcf10, stopped_start=stopped_start@entry=0x7fffffffcf0e, stackaddr=stackad
dr@entry=0x7ffe48e25400, thread_ran=thread_ran@entry=0x7fffffffcf0f) at ../sysdeps/unix/sysv/linux/createthread.c:101
#2  0x00007ffff628ce10 in __pthread_create_2_1 (newthread=<optimized out>, attr=<optimized out>, start_routine=<optimized out>, arg=<optimized out>) at pthread_create.c
:817
#3  0x00007ffff56f3eb4 in pa_thread_new () at /usr/lib/x86_64-linux-gnu/pulseaudio/libpulsecommon-13.99.so
#4  0x00007ffff6dc639b in pa_threaded_mainloop_start () at /lib/x86_64-linux-gnu/libpulse.so.0
#5  0x00007ffe51226e33 in pulse_driver_open () at /usr/lib/x86_64-linux-gnu/libcanberra-0.30/libcanberra-pulse.so
#6  0x00007fffe957219b in  () at /lib/x86_64-linux-gnu/libcanberra.so.0
#7  0x00007fffe9569538 in  () at /lib/x86_64-linux-gnu/libcanberra.so.0
#8  0x00007fffe956a1ef in ca_context_play_full () at /lib/x86_64-linux-gnu/libcanberra.so.0
#9  0x00007ffff7fb5648 in ca_gtk_play_for_widget () at /lib/x86_64-linux-gnu/libcanberra-gtk3.so.0
#10 0x00007ffff7fbcc6e in  () at /usr/lib/x86_64-linux-gnu/gtk-3.0/modules/libcanberra-gtk-module.so
#11 0x00007ffff6fa0f4d in  () at /lib/x86_64-linux-gnu/libgdk-3.so.0
#12 0x00007ffff787a04e in g_main_context_dispatch () at /lib/x86_64-linux-gnu/libglib-2.0.so.0
#13 0x0000555555e6d1a8 in glib_pollfds_poll () at ../util/main-loop.c:232
#14 os_host_main_loop_wait (timeout=<optimized out>) at ../util/main-loop.c:255
#15 main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:531
#16 0x0000555555c190d1 in qemu_main_loop () at ../softmmu/runstate.c:726
#17 0x0000555555940ab2 in main (argc=<optimized out>, argv=<optimized out>, envp=<optimized out>) at ../softmmu/main.c:50
```

gmain 和 gdbus 类似，只是从 `early_gtk_display_init` 开始，然后经过层层的在 gtk 库函数的调用

所以，现在可以基本确定一个事情，那就是这几个与众不同的 thread 是 gtk 处理图形界面和音频创建的出来的。
这些东西的处理都是被 glib 库封装好了，之后没有必要关注了。

之后我们逐个分析 worker thread / IOThtread 的

[^1]: https://github.com/chiehmin/gdbus_test
[^8]: https://stackoverflow.com/questions/21926549/get-thread-name-in-gdb
[^9]: https://stackoverflow.com/questions/8944236/gdb-how-to-get-thread-name-displayed
