# Qemu

## env
一般的编译方法
```
mkdir build
cd build
./configure --target-list=x86_64-softmmu,aarch64-softmmu,aarch64-linux-user
make
```

使用 ../configure --help 查看支持的系统

### 使用 Qemu 的参数
https://blahcat.github.io/2018/01/07/building-a-debian-stretch-qemu-image-for-aarch64/
https://kennedy-han.github.io/2015/06/15/QEMU-arm64-guide.html

### 经典配置方案
1. 自己编译内核 + minimal 镜像: 使用 https://linux-kernel-labs.github.io/
2. 自己编译内核 + ubuntun 镜像:
3. ubuntun 镜像和内核:

## related project
- [Unicorn](https://github.com/unicorn-engine/unicorn) is a lightweight, multi-platform, multi-architecture CPU emulator framework based on QEMU.

## 代码分析

qemu/cpus.c : 
qemu/memory.c : 
qemu/net/tap.c : 

qemu/hw/virtio
qemu/linux-user
qemu/capstone/ : https://github.com/aquynh/capstone
- [ ] qemu/slirp : https://gitlab.freedesktop.org/slirp/libslirp 这个和普通的用户态网络库的关系是什么?

- [ ] qemu/stubs : 这个暂时搞不懂是什么意思 
qemu/roms : 各种 BIOS，其中包括 qboot

## chapter2

### glib
- GMainLoop 表示事件循环
- 每一个 GMainLoop 都有一个上下文 GMainContext
- GMainContext 可以关联多个 GSource
- 事件源使用 GSource 表示, GSource 可以和多个文件描述符关联

g_main_context_new
g_main_context_prepare
g_main_context_query
g_main_context_dispatch


- [ ] 如果卡在这里死循环了，那么翻译工作是哪里完成的

- main
  - main_loop
    - main_loop_should_exit
    - main_loop_wait
      - os_host_main_loop_wait
        - qemu_poll_ns
          - ppoll
        - glib_pollfds_poll : 获取所需要监听的 fd，并且计算一个最小的超时时间
          - g_main_context_check
          - g_main_context_dispatch

AioContext

- aio_context_new
  - g_source_new
  - aio_set_event_notifier
    - aio_set_fd_handler 

使用 signalfd 作为例子来分析事件处理过程:
- qemu_init_main_loop
  - qemu_signal_init
    - qemu_signalfd : 调用 syscall 获取一个 sysfd 啊
  - aio_context_new

- [ ] 必须找一个具体一点的例子，太无聊了

### Qemu 线程模型
每一个 CPU 都会有一个叫做 vcpu 的线程用于执行代码。

### QOM

从 edu.c 中间分析:

```c
typedef enum {
    MODULE_INIT_BLOCK,
    MODULE_INIT_OPTS,
    MODULE_INIT_QOM,
    MODULE_INIT_TRACE,
    MODULE_INIT_XEN_BACKEND,
    MODULE_INIT_LIBQOS,
    MODULE_INIT_MAX
} module_init_type;

void module_call_init(module_init_type type)
{
    ModuleTypeList *l;
    ModuleEntry *e;

    l = find_type(type);

    QTAILQ_FOREACH(e, l, node) {
        e->init();
    }
}
```
- [ ] 从 module_init_type 看，似乎一个类型(module_init_type)的都是在一起初始

## chapter 3
