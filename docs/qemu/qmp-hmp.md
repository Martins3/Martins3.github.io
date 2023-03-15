# qmp

```txt
hack/qemu/internals/e1000-2.md:#18 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
hack/qemu/internals/e1000.md:#16 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
hack/acpi/hack-with-qemu.md:#10 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2590
docs/kernel/mm-virtio-balloon.md:- qmp 对外仅仅提供两个功能
docs/qemu/block.md:2. 后面的就是各种 qmp 操作的
docs/qemu/block.md:在 `qmp_transaction` 中的，根据命令来调用这些内容:
docs/qemu/block.md:## qmp ：没办法，不搞的话，dirty bitmap 是没有办法维持生活的
docs/qemu/block.md:- [ ] grep 一下目前对于 qmp 的所有问题，尝试将 qmp 和 qemu option 融合一下
docs/qemu/reset.md:  - `qmp_x_exit_preconfig`
docs/qemu/reset.md:#6  0x0000555555c22788 in qmp_x_exit_preconfig (errp=0x5555567aa610 <error_fatal>) at ../softmmu/vl.c:2602
docs/qemu/migration/yank.md:instances can be called by the 'yank' out-of-band qmp command.
docs/qemu/migration/yank.md:# A yank instance can be yanked with the @yank qmp command to recover from a
docs/qemu/migration/multifd.md:    - `migrate_multifd_channels` : 这个数值是从 qmp 设置的
docs/qemu/migration/migration.md:- `qmp_migrate_incoming` / `qmp_migrate_recover`
docs/qemu/migration/migration.md:- `qmp_migrate`
docs/qemu/migration/migration.md:- `qmp_query_migrate_parameters`
docs/qemu/options.md:-qmp unix:/home/maritns3/core/vn/hack/qemu/x64-e1000/test.socket,server,nowait \
docs/qemu/options.md:[qmp] : [unix:/home/maritns3/core/vn/hack/qemu/x64-e1000/test.socket,server,nowait]
docs/qemu/sh/alpine.sh:  ${arg_qmp} ${arg_vfio} ${arg_smbios} ${arg_scsi}"
docs/qemu/todo-1.md:- [ ] docs/devel/qapi-code-gen.txt 和 qmp 如何工作的，是如何生成的。
docs/qemu/todo-1.md:## qmp
docs/qemu/todo-1.md:- [ ] `qmp_block_commit` 的唯一调用者是如何被生成的。
docs/qemu/todo-1.md:qmp 让 virsh 可以和 qemu 交互
docs/qemu/qom.md:#13 0x0000555555cdaf85 in qmp_x_exit_preconfig (errp=0x5555567a94b0 <error_fatal>) at ../softmmu/vl.c:2600
docs/qemu/qom.md:#18 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2689
docs/qemu/qom.md:#19 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2682
docs/qemu/seabios.md:#13 qmp_x_exit_preconfig (errp=<optimized out>) at ../softmmu/vl.c:2588
```

## 基本使用
- https://www.qemu.org/docs/master/devel/writing-monitor-commands.html
- https://wiki.qemu.org/Documentation/QMP
- https://www.qemu.org/docs/master/devel/writing-monitor-commands.html#writing-a-debugging-aid-returning-unstructured-text

每次必须首先执行:
```txt
{ "execute": "qmp_capabilities" }
```

{ "execute": "qom-set",
             "arguments": { "path": "/machine/peripheral/balloon0",
             "property": "guest-stats-polling-interval", "value": 2 } }

{ "execute": "qom-get",
             "arguments": { "path": "/machine/peripheral/balloon0",
             "property": "guest-stats" } }

- https://gist.github.com/rgl/dc38c6875a53469fdebb2e9c0a220c6c

## get

```json
{ "execute": "qom-get",
             "arguments": { "path": "/machine/peripheral/balloon0",
             "property": "guest-stats" } }
```

```txt
#0  balloon_stats_get_all (obj=0x55555790d780, v=0x555557b205a0, name=0x55555790e430 "guest-stats", opaque=0x0, errp=0x7fffffff5da0) at ../hw/virtio/virtio-balloon.c:243
#1  0x0000555555c12de8 in object_property_get (obj=0x55555790d780, name=0x55555790e430 "guest-stats", v=v@entry=0x555557b205a0, errp=errp@entry=0x7fffffff5e00) at ../qom/object.c:1400
#2  0x0000555555c12e9b in property_get_alias (obj=<optimized out>, v=<optimized out>, name=<optimized out>, opaque=0x55555790e410, errp=0x7fffffff5e00) at ../qom/object.c:2716
#3  0x0000555555c12de8 in object_property_get (obj=obj@entry=0x5555579053c0, name=name@entry=0x555556d13d20 "guest-stats", v=v@entry=0x555556922af0, errp=errp@entry=0x7fffffff5e98) at ../qom/object.c:1400
#4  0x0000555555c1632d in object_property_get_qobject (obj=0x5555579053c0, name=0x555556d13d20 "guest-stats", errp=errp@entry=0x7fffffff5e98) at ../qom/qom-qobject.c:40
#5  0x0000555555cdc8db in qmp_qom_get (path=<optimized out>, property=<optimized out>, errp=errp@entry=0x7fffffff5e98) at ../qom/qom-qmp-cmds.c:89
#6  0x0000555555d3b19b in qmp_marshal_qom_get (args=<optimized out>, ret=0x7ffff6dbfe98, errp=0x7ffff6dbfe90) at qapi/qapi-commands-qom.c:131
#7  0x0000555555d62f39 in do_qmp_dispatch_bh (opaque=0x7ffff6dbfea0) at ../qapi/qmp-dispatch.c:128
#8  0x0000555555d7ff24 in aio_bh_call (bh=0x5555568304b0) at ../util/async.c:150
#9  aio_bh_poll (ctx=ctx@entry=0x55555662e020) at ../util/async.c:178
#10 0x0000555555d6c9ae in aio_dispatch (ctx=0x55555662e020) at ../util/aio-posix.c:421
#11 0x0000555555d7fb6e in aio_ctx_dispatch (source=<optimized out>, callback=<optimized out>, user_data=<optimized out>) at ../util/async.c:320
#12 0x00007ffff7bd5dfb in g_main_context_dispatch () from /nix/store/iw2sf51fa42kd61qpmppi5h1yj675982-glib-2.72.3/lib/libglib-2.0.so.0
#13 0x0000555555d820a8 in glib_pollfds_poll () at ../util/main-loop.c:297
#14 os_host_main_loop_wait (timeout=0) at ../util/main-loop.c:320
#15 main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:606
#16 0x00005555559e09e7 in qemu_main_loop () at ../softmmu/runstate.c:739
#17 0x0000555555831317 in qemu_default_main () at ../softmmu/main.c:37
#18 0x00007ffff7814237 in __libc_start_call_main () from /nix/store/v6szn6fczjbn54h7y40aj7qjijq7j6dc-glibc-2.34-210/lib/libc.so.6
#19 0x00007ffff78142f5 in __libc_start_main_impl () from /nix/store/v6szn6fczjbn54h7y40aj7qjijq7j6dc-glibc-2.34-210/lib/libc.so.6
#20 0x0000555555831241 in _start () at ../sysdeps/x86_64/start.S:116
```

# hmp

- [ ] 可以阅读的文档:
这里描述在 graphic 和 non-graphic 的模式下访问 HMI 的方法，并且说明了从 HMI 中间如何获取各种信息
https://web.archive.org/web/20180104171638/http://nairobi-embedded.org/qemu_monitor_console.html

## 一些尝试
自己尝试的效果:
```txt
(qemu) info chardev
virtiocon0: filename=pty:/dev/pts/6
serial1: filename=pipe
parallel0: filename=vc
gdb: filename=disconnected:tcp:0.0.0.0:1234,server
compat_monitor0: filename=stdio
serial0: filename=pipe
```
如果什么都不配置，结果如下:
```txt
(qemu) info chardev
parallel0: filename=vc
compat_monitor0: filename=stdio
serial0: filename=vc
```

## 源码分析
- hmp_info_balloon

```txt
#0  hmp_info_balloon (mon=0x5555568abe50, qdict=0x555556a05150) at ../monitor/hmp-cmds.c:690
#1  0x0000555555a2b369 in handle_hmp_command_exec (cmd=<optimized out>, cmd=<optimized out>, qdict=0x555556a05150, mon=0x5555568abe50) at ../monitor/hmp.c:1108
#2  handle_hmp_command_exec (qdict=0x555556a05150, cmd=0x5555564c8280 <hmp_info_cmds>, mon=0x5555568abe50) at ../monitor/hmp.c:1100
#3  handle_hmp_command (mon=mon@entry=0x5555568abe50, cmdline=<optimized out>, cmdline@entry=0x5555568c5790 "info balloon ") at ../monitor/hmp.c:1160
#4  0x0000555555a2b4ad in monitor_command_cb (opaque=0x5555568abe50, cmdline=0x5555568c5790 "info balloon ", readline_opaque=<optimized out>) at ../monitor/hmp.c:49
#5  0x0000555555d90bc6 in readline_handle_byte (rs=0x5555568c5790, ch=<optimized out>) at ../util/readline.c:411
#6  0x0000555555a2b4fb in monitor_read (opaque=0x5555568abe50, buf=<optimized out>, size=<optimized out>) at ../monitor/hmp.c:1398
#7  0x0000555555cd696a in tcp_chr_read (chan=<optimized out>, cond=<optimized out>, opaque=<optimized out>) at ../chardev/char-socket.c:508
#8  0x00007ffff7bd5d04 in g_main_context_dispatch () from /nix/store/iw2sf51fa42kd61qpmppi5h1yj675982-glib-2.72.3/lib/libglib-2.0.so.0
#9  0x0000555555d820a8 in glib_pollfds_poll () at ../util/main-loop.c:297
#10 os_host_main_loop_wait (timeout=499000000) at ../util/main-loop.c:320
#11 main_loop_wait (nonblocking=nonblocking@entry=0) at ../util/main-loop.c:606
#12 0x00005555559e09e7 in qemu_main_loop () at ../softmmu/runstate.c:739
#13 0x0000555555831317 in qemu_default_main () at ../softmmu/main.c:37
#14 0x00007ffff7814237 in __libc_start_call_main () from /nix/store/v6szn6fczjbn54h7y40aj7qjijq7j6dc-glibc-2.34-210/lib/libc.so.6
#15 0x00007ffff78142f5 in __libc_start_main_impl () from /nix/store/v6szn6fczjbn54h7y40aj7qjijq7j6dc-glibc-2.34-210/lib/libc.so.6
#16 0x0000555555831241 in _start () at ../sysdeps/x86_64/start.S:116
```

```c
void hmp_info_balloon(Monitor *mon, const QDict *qdict)
{
    BalloonInfo *info;
    Error *err = NULL;

    info = qmp_query_balloon(&err);
    if (hmp_handle_error(mon, err)) {
        return;
    }

    monitor_printf(mon, "balloon: actual=%" PRId64 "\n", info->actual >> 20);

    qapi_free_BalloonInfo(info);
}
```

- [ ] /home/martins3/core/qemu/build/hmp-commands-info.h 是如何生成的
- [ ] /home/martins3/core/qemu/build/qapi/qapi-commands-machine.h 中包含了 qmp_query_balloon

## 总结一些和 qobject 纠缠在一起的功能

- object_register_sugar_prop

```c
static void qemu_process_sugar_options(void)
{
    if (mem_prealloc) {
        QObject *smp = qdict_get(machine_opts_dict, "smp");
        if (smp && qobject_type(smp) == QTYPE_QDICT) {
            QObject *cpus = qdict_get(qobject_to(QDict, smp), "cpus");
            if (cpus && qobject_type(cpus) == QTYPE_QSTRING) {
                const char *val = qstring_get_str(qobject_to(QString, cpus));
                object_register_sugar_prop("memory-backend", "prealloc-threads",
                                           val, false);
            }
        }
        object_register_sugar_prop("memory-backend", "prealloc", "on", false);
    }
}
```
## 才意识到 qmp 也是可以使用 shell 交互的
- https://wiki.qemu.org/Documentation/QMP

### 分析下 qmp shell 中常用命令
1. query-cpu-definitions

## 为什么 libvirt 是需要 -pidfile，不是

/run/current-system/sw/bin/qemu-system-x86_64 -S -no-user-config -nodefaults -nographic -machine none,accel=kvm:tcg
-qmp unix:/home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.monitor,server=on,wait=off
-pidfile /home/martins3/.config/libvirt/qemu/lib/qmp-LEY2Z1/qmp.pid

## qemu 中的 -object 参数
```txt
       -object typename[,prop1=value1,...]
              Create  a  new object of type typename setting properties in the order they are specified. Note that the 'id'
              property must be set. These objects are placed in the '/objects' path.
```

- [ ] 这里的 typename 是什么，有没有什么方便的方法实现查询一个 typename 的 property 机制

## 如何快速定位到代码
hmp 中提供了一个 mce 命令，如何找到对应的实现:

hmp_mce 直接搜索 hmp_mce 即可
