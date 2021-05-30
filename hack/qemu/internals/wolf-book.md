## 代码分析

qemu/cpus.c : 
qemu/memory.c : 
qemu/net/tap.c : 

qemu/hw/virtio
qemu/linux-user
qemu/capstone/ : https://github.com/aquynh/capstone
- [ ] qemu/slirp : https://gitlab.freedesktop.org/slirp/libslirp 这个和普通的用户态网络库的关系是什么?

- [ ] qemu/stubs : 这个暂时搞不懂是什么意思 

- [ ] 可以通过 kernel-irqchip 来决定 irqchip 被谁模拟

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


- [x] 如果卡在这里死循环了，那么翻译工作是哪里完成的
  - 事件监听是放到一个特定的线程中间的

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

1. 类型的注册
  - type_init
  - register_module_init
  - type_register
2. 类型的初始化
  - type_initialize
3. 对象的初始化
  - object_new
  - object_initialize
  - object_initialize_with_type

类型的注册
，在 main 之前完成， 
类型的初始化, 在 main 中调用，全部初始化，
对象的初始化, 根据命令行参数选择进行初始化


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



## chapter 3 主板和固件模拟
> 草稿放到 qboot.md 和 kernel-img.md 中

## chapter 6

#### 6.2.2 PIC 中断模拟
- kvm_irqchip_create

kvm 模块在处理
KVM_CREATE_IRQCHIP
的时候，会调用
1. kvm_create_pic
2. kvm_ioapic_init
3. kvm_steup_default_irq_routing


## chapter 7 : 设备虚拟化

### 7.6 vhost net 简介
virtio 的问题在于为了将数据发送出去，需要切入到用户态，然后走 TAP 设备，vsock 直接走内核，从而减少一次用户态。

- [ ] 为什么 block io 不是类似的处理
