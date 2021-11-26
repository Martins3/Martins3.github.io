# QEMU 字符设备模拟

<!-- vim-markdown-toc GitLab -->

- [Architecture](#architecture)
  - [Chardev](#chardev)
  - [CharBackend](#charbackend)
- [init](#init)
  - [Chardev 的创建](#chardev-的创建)
  - [SerialState](#serialstate)
- [附录](#附录)
  - [后端的一点抽象](#后端的一点抽象)
  - [mux](#mux)

<!-- vim-markdown-toc -->

如果想要大致了解 printf 的实现，从上到下参考如下内容:
1. 用户态 : [musl](https://www.musl-libc.org/) 和 《程序员的自我修养》
2. 内核态 : [TTY 到底是什么？](https://www.kawabangga.com/posts/4515) 和 Linux Device Driver 可以参考
3. 硬件: 参考 serial parallel 等字符设备的手册

当然这这是考虑一些很粗略的情况了，今天这些分析的是，QEMU 是如何模拟字符设备的，当 guest 读写 pio / mmio 导致 vmexit
出来，QEMU 进行一系列的操作。

## Architecture
Guest 需要使用各种设备，比如 serial virtio-console，当 guest 在对应的 pio / mmio 上操作的时候，QEMU 正确的模拟出来。
这些设备模拟的部分被成为 Frontend，主要的代码出现在 /hw/char。

QEMU 的参数 `-nographic` 可以让 guest 是在 terminal 中运行还是在图形化的界面中运行，其实这是因为对于一个 guest 设备的输出，
QEMU 可以将数据导入到不同的 host 载体中，比如 serial, file 或者 tcp，这种和 host 的载体打交道的部分被成为 Backend，主要的代码出现在 /chardev 上。

各种后端都是 Chardev 的子类，各种前端的共性较小，无法创建一个公共的 parent, 但是 QEMU 提供了一个 ChardevBackend 嵌入到结构体用于和 Backend 打交道。

### Chardev

```c
typedef struct ChardevClass {
    ObjectClass parent_class;

    bool internal; /* TODO: eventually use TYPE_USER_CREATABLE */
    void (*parse)(QemuOpts *opts, ChardevBackend *backend, Error **errp);

    void (*open)(Chardev *chr, ChardevBackend *backend,
                 bool *be_opened, Error **errp);

    int (*chr_write)(Chardev *s, const uint8_t *buf, int len);
    int (*chr_sync_read)(Chardev *s, const uint8_t *buf, int len);
    GSource *(*chr_add_watch)(Chardev *s, GIOCondition cond);
    void (*chr_update_read_handler)(Chardev *s);
    int (*chr_ioctl)(Chardev *s, int cmd, void *arg);
    int (*get_msgfds)(Chardev *s, int* fds, int num);
    int (*set_msgfds)(Chardev *s, int *fds, int num);
    int (*chr_add_client)(Chardev *chr, int fd);
    int (*chr_wait_connected)(Chardev *chr, Error **errp);
    void (*chr_disconnect)(Chardev *chr);
    void (*chr_accept_input)(Chardev *chr);
    void (*chr_set_echo)(Chardev *chr, bool echo);
    void (*chr_set_fe_open)(Chardev *chr, int fe_open);
    void (*chr_be_event)(Chardev *s, int event);
    /* Return 0 if succeeded, 1 if failed */
    int (*chr_machine_done)(Chardev *chr);
} ChardevClass;
```

使用 debugcon 作为一个例子:
- debugcon_ioport_write
  - qemu_chr_fe_write_all
    - qemu_chr_write
      - qemu_chr_write_buffer
        - ChardevClass::chr_write : debugcon 关联的 Chardev 不同，其最后的写入位置也不同

### CharBackend

```c
/* This is the backend as seen by frontend, the actual backend is
 * Chardev */
struct CharBackend {
    Chardev *chr;
    IOEventHandler *chr_event;
    IOCanReadHandler *chr_can_read;
    IOReadHandler *chr_read;
    BackendChangeHandler *chr_be_change;
    void *opaque;
    int tag;
    int fe_open;
};
```
CharBackend 中四个 hook 都是前端注册上的
- chr_event : 因为 backend 收到一些特殊信息需要 frontend 来采取特殊操作，比如 OPEN CLOSE
- chr_be_change : 当 backend 发生变化的时候采取的东西
- chr_can_read / chr_read
  - read 模拟过程是: 如果 host 的"设备" ready 了，比如标准输入中有数据了，然后 backend 读去数据，最后发送到 frontend，frontend 处理完成之后将通过中断的方法告诉 vCPU
  - 显然不能使用阻塞的方式等待 host 的"设备" ready, QEMU 已经有了一套完整的事件监听机制来实现异步的等待。
  - 当 ready 之后，serial 在 serial_realize_core 中调用 qemu_chr_fe_set_handlers 注册的 serial_can_receive1 就可以被调用

```c
/*
#4  0x0000555555a208c7 in serial_can_receive1 (opaque=<optimized out>) at /home/maritns3/core/xqm/hw/char/serial.c:609
#5  0x0000555555c4cca0 in mux_chr_read (opaque=<optimized out>, buf=<optimized out>, size=<optimized out>) at /home/maritns3/core/xqm/chardev/char-mux.c:223
#6  0x0000555555c4ac3d in fd_chr_read (chan=0x55555649e810, cond=<optimized out>, opaque=<optimized out>) at /home/maritns3/core/xqm/chardev/char-fd.c:68
#7  0x00007ffff704704e in g_main_context_dispatch () at /lib/x86_64-linux-gnu/libglib-2.0.so.0
#8  0x0000555555caf228 in glib_pollfds_poll () at /home/maritns3/core/xqm/util/main-loop.c:219
#9  os_host_main_loop_wait (timeout=<optimized out>) at /home/maritns3/core/xqm/util/main-loop.c:242
#10 main_loop_wait (nonblocking=<optimized out>) at /home/maritns3/core/xqm/util/main-loop.c:518
```
暂时不用看 *fd_chr_read* 和 *mux_chr_read*，下面会分析的。

## init
下面我们使用 serial 和 stdio 作为前端和后端来分析初始化的过程。

### Chardev 的创建
在 vl.c:main 中间，当采用 -nographic 的时候，serial 是默认被导入到 stdio 中间的，
也就是执行 add_device_config(DEV_SERIAL, "stdio");
```c
    if (nographic) {
        if (default_parallel)
            add_device_config(DEV_PARALLEL, "null");
        if (default_serial && default_monitor) {
            add_device_config(DEV_SERIAL, "mon:stdio");
        } else {
            if (default_serial)
                add_device_config(DEV_SERIAL, "stdio");
            if (default_monitor)
                monitor_parse("stdio", "readline", false);
        }
    } else {
        if (default_serial)
            add_device_config(DEV_SERIAL, "vc:80Cx24C");
        if (default_parallel)
            add_device_config(DEV_PARALLEL, "vc:80Cx24C");
        if (default_monitor)
            monitor_parse("vc:80Cx24C", "readline", false);
    }
```
然后 在 foreach_device_config 中执行 hook serial_parse 来解析 "stdio"

- serial_parse : 通过 qemu_chr_new_mux_mon 创建的 Chardev 存储在 `serial_hds` 中
  - qemu_chr_new_mux_mon
    - qemu_chr_new_permit_mux_mon
      - qemu_chr_new_noreplay
        - qemu_chr_parse_compat
        - qemu_chr_new_from_opts
          - qemu_chr_new_from_opts
            - qemu_chardev_new
              - 创建具体的 Chardev，比如 stdio
              - qemu_char_open : 调用 ChardevClass::open

### SerialState
入口在 serial_hds_isa_init 中

```c
void serial_hds_isa_init(ISABus *bus, int from, int to)
{
    int i;

    assert(from >= 0);
    assert(to <= MAX_ISA_SERIAL_PORTS);

    for (i = from; i < to; ++i) {
        if (serial_hd(i)) {
            serial_isa_init(bus, i, serial_hd(i));
        }
    }
}
```
当 serial_isa_init 中的参数 serial_hd(i) 就是之前创建的 Chardev

- serial_isa_init
  - qdev_prop_set_chr : 这里的 qom property 的操作最后会调用到
    - set_chr
      - qemu_chr_find : 获取 chardev 的名字 "serial0"
      - qemu_chr_fe_init : 初始化 SerialState::CharBackend

```c
static Property serial_isa_properties[] = {
    // ...
    DEFINE_PROP_CHR("chardev",   ISASerialState, state.chr),
    // ...
    DEFINE_PROP_END_OF_LIST(),
};

const PropertyInfo qdev_prop_chr = {
    .name  = "str",
    .description = "ID of a chardev to use as a backend",
    .get   = get_chr,
    .set   = set_chr,
    .release = release_chr,
};
```

## 附录
### 后端的一点抽象
在 chardev 下除了每一个后端一个文件描述，还有
- chardev
  - char-io.c : 主要处理 epoll 等机制
  - char-fe.c : 主要是 CharBackend 的进一步的进一步封装，正如其文件名，处理前端的
  - char-fd.c : 因为有好几个后端比如 file stdio 都是使用 fd 来索引，这些后端有一些通用属性，所以抽象出来 TYPE_CHARDEV 的子类 TYPE_CHARDEV_FD

```c
static void char_fd_class_init(ObjectClass *oc, void *data)
{
    ChardevClass *cc = CHARDEV_CLASS(oc);

    cc->chr_add_watch = fd_chr_add_watch;
    cc->chr_write = fd_chr_write;
    cc->chr_update_read_handler = fd_chr_update_read_handler;
}
```

### mux
可以找到 qemu-options.hx 中关于 char mux 的介绍:
- QEMU 支持多个 front-end 的内容导入一个 backend 的情况
- QEMU 不支持一个 front-end 的内容导入到多个 backend 的情况，这个比较显然

在 [patch 的讨论](https://patchwork.kernel.org/project/qemu-devel/patch/1455638581-5912-1-git-send-email-peter.maydell@linaro.org/) 还有一些补充信息
