## QEMU Event Loop

和 QEMU Event Loop 关联的文件

- util/async.c : AioContext 处理 bh 和 coroutine 相关的操作
- util/aio-posix.c : 定义了 aio_dispatch_handler, aio_poll, aio_set_fd_handler 等核心函数
- util/thread-pool.c
- main-loop.c : main loop thread
- iothread.c : IOThread thread

在 QEMU 中，用于 event loop 的线程为 main loop thread 和 IOThread，其中 IOThread 需要 explicit 的配置才可以被使用。
也就是说，默认情况下就是 main loop thread 和 vCPU thread 相互交互。

### main loop thread
QEMU 的第一个 thread 启动了各个 vCPU 之后，然后就会调用 ppoll 来进行实践监听。

相关代码在 main-loop.c 中间，下面是 main loop

- main
  - qemu_main_loop
    - main_loop_wait
      - os_host_main_loop_wait
        - qemu_poll_ns
          - ppoll

在 glib 中进行 event loop 是通过调用 g_main_loop_run 来进行进行的，但是 QEMU 的 main loop thread 存在更强的自定义，也就是
os_host_main_loop_wait

- os_host_main_loop_wait
  - glib_pollfds_fill
    - g_main_context_query : 调用 glib 的库，将需要监听的 fd 取出来，放到 gpollfds 中
  - qemu_poll_ns : 调用 poll 来监听保存到 gpollfds 中的 fd
  - glib_pollfds_poll
    - g_main_context_dispatch : 调用监听的 fd 的 callback 函数

当存在 fd ready 之后，其执行流程为:
- g_main_context_dispatch
  - aio_ctx_dispatch
    - aio_dispatch
        - aio_dispatch_handlers
            - aio_dispatch_handler
              -  qemu_luring_completion_cb

这就是 aio_set_fd_handler 的任务，对于一个监听的 fd, 会创建 `AioHandler` 来保存这个 fd 关联的 hook 函数

需要指出的是，AioHandler::io_poll 用于用户态的 poll 操作，找到其注册的三个 hook 函数，都是简单查询一下一个变量，
如果发现已经存在 fd ready 了，那么就可以直接返回。io_poll 注册的 hook 为:
- aio_context_notifier_poll
- qemu_luring_poll_cb : Returns how many unconsumed entries are ready in the CQ ring
- virtio_queue_host_notifier_aio_poll

实现真的非常的平易近人:
```c
/* Returns true if aio_notify() was called (e.g. a BH was scheduled) */
static bool aio_context_notifier_poll(void *opaque)
{
    EventNotifier *e = opaque;
    AioContext *ctx = container_of(e, AioContext, notifier);

    return qatomic_read(&ctx->notified);
}
```

- main
  - `main_loop`
    - `main_loop_should_exit`
    - `main_loop_wait`
      - `os_host_main_loop_wait`
        - `qemu_poll_ns`
          - ppoll
        - `glib_pollfds_poll` : 获取所需要监听的 fd，并且计算一个最小的超时时间
          - `g_main_context_check`
          - `g_main_context_dispatch`


使用 signalfd 作为例子来分析事件处理过程:
- `qemu_init_main_loop`
  - `qemu_signal_init`
    - `qemu_signalfd` : 调用 syscall 获取一个 sysfd 啊
  - `aio_context_new`

使用 signalfd 作为例子的确是不错的


## 所以，glib + 两个 context

1. 一个循环中，如何保证不会出现互相的阻塞
2. 都是分别监听那些 fd 的，可以列出来吗?

## 现在依赖的 gtk 中，一定存在这个，所以现在就是需要这个

类似的 backtrace 很多，就不去罗列了:
```tx
#0  0x00007ffff7911629 in g_source_attach () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libglib-2.0.so.0
#1  0x00007ffff76cb130 in g_task_get_type_once () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#2  0x00007ffff76cb54d in g_task_get_type () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#3  0x00007ffff77440c6 in _g_dbus_initialize.part.0 () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#4  0x00007ffff77428e0 in g_dbus_proxy_new_for_bus () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#5  0x00007ffff71e8baa in _gdk_wayland_screen_new () at /nix/store/v7fvwaf6j1hwvb9dmwfa97nzmrlshqai-gtk+3-3.24.52/lib/libgdk-3.so.0
#6  0x00007ffff71e0c43 in _gdk_wayland_display_open () at /nix/store/v7fvwaf6j1hwvb9dmwfa97nzmrlshqai-gtk+3-3.24.52/lib/libgdk-3.so.0
#7  0x00007ffff71a7607 in gdk_display_manager_open_display () at /nix/store/v7fvwaf6j1hwvb9dmwfa97nzmrlshqai-gtk+3-3.24.52/lib/libgdk-3.so.0
#8  0x00007ffff6c2875a in gtk_init_check () at /nix/store/v7fvwaf6j1hwvb9dmwfa97nzmrlshqai-gtk+3-3.24.52/lib/libgtk-3.so.0
#9  0x0000555555ca7088 in early_gtk_display_init (opts=0x555557273ee8 <dpy>) at ../ui/gtk.c:2844
#10 0x0000555555de6dfa in qemu_display_early_init (opts=0x5555575a9a40, opts@entry=0x555557273ee8 <dpy>) at ../ui/console.c:1444
#11 0x0000555555c73807 in qemu_setup_display () at ../system/vl.c:1392
#12 qemu_init (argc=<optimized out>, argv=0x7ffffffe7358) at ../system/vl.c:3792
#13 0x0000555555eeceda in main (argc=1465555520, argv=0x5555575aca40) at ../system/main.c:71
```

```txt
#0  0x00007ffff7911629 in g_source_attach () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libglib-2.0.so.0
#1  0x00007ffff7776777 in _ik_startup () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#2  0x00007ffff77756c4 in _ip_startup () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#3  0x00007ffff7774df9 in _ih_startup () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#4  0x00007ffff769449f in _g_io_module_get_default_type () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#5  0x00007ffff776636a in g_local_file_monitor_new () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#6  0x00007ffff77674a9 in g_local_file_monitor_new_for_path () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#7  0x00007ffff767a791 in g_file_monitor_file () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#8  0x00007ffff77135db in g_keyfile_settings_backend_constructed () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#9  0x00007ffff7e3c10a in g_object_new_internal.part () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgobject-2.0.so.0
#10 0x00007ffff7e3d67c in g_object_new_with_properties () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgobject-2.0.so.0
#11 0x00007ffff7e3e699 in g_object_new () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgobject-2.0.so.0
#12 0x00007ffff76940fc in try_implementation () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#13 0x00007ffff7694746 in _g_io_module_get_default () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#14 0x00007ffff7715392 in g_settings_backend_get_default () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#15 0x00007ffff7719edd in g_settings_constructed () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#16 0x00007ffff7e3c10a in g_object_new_internal.part () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgobject-2.0.so.0
#17 0x00007ffff7e3e2bb in g_object_new_valist () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgobject-2.0.so.0
#18 0x00007ffff7e3e66b in g_object_new () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgobject-2.0.so.0
#19 0x00007ffff771b8f8 in g_settings_new_full () at /nix/store/jlyahda14aya375lv7k9fsin2zk90nxz-glib-2.88.1/lib/libgio-2.0.so.0
#20 0x00007ffff71e8c53 in _gdk_wayland_screen_new () at /nix/store/v7fvwaf6j1hwvb9dmwfa97nzmrlshqai-gtk+3-3.24.52/lib/libgdk-3.so.0
#21 0x00007ffff71e0c43 in _gdk_wayland_display_open () at /nix/store/v7fvwaf6j1hwvb9dmwfa97nzmrlshqai-gtk+3-3.24.52/lib/libgdk-3.so.0
#22 0x00007ffff71a7607 in gdk_display_manager_open_display () at /nix/store/v7fvwaf6j1hwvb9dmwfa97nzmrlshqai-gtk+3-3.24.52/lib/libgdk-3.so.0
#23 0x00007ffff6c2875a in gtk_init_check () at /nix/store/v7fvwaf6j1hwvb9dmwfa97nzmrlshqai-gtk+3-3.24.52/lib/libgtk-3.so.0
#24 0x0000555555ca7088 in early_gtk_display_init (opts=0x555557273ee8 <dpy>) at ../ui/gtk.c:2844
#25 0x0000555555de6dfa in qemu_display_early_init (opts=0x5555575c02e0, opts@entry=0x555557273ee8 <dpy>) at ../ui/console.c:1444
#26 0x0000555555c73807 in qemu_setup_display () at ../system/vl.c:1392
#27 qemu_init (argc=<optimized out>, argv=0x7ffffffe7358) at ../system/vl.c:3792
#28 0x0000555555eeceda in main (argc=1465647840, argv=0x5555575aca40) at ../system/main.c:71
```

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
