# qmp 和 hmp

## 基本使用
- https://www.qemu.org/docs/master/devel/writing-monitor-commands.html
- https://wiki.qemu.org/Documentation/QMP
- https://www.qemu.org/docs/master/devel/writing-monitor-commands.html#writing-a-debugging-aid-returning-unstructured-text
- https://gist.github.com/rgl/dc38c6875a53469fdebb2e9c0a220c6c

## qmp shell
- https://wiki.qemu.org/Documentation/QMP

qmp shell 常见命令:
1. query-cpu-definitions

## qom-get

```json
{ "execute": "qom-get",
             "arguments": { "path": "/machine/peripheral/balloon0",
             "property": "guest-stats" } }
```

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_default_main
        - qemu_main_loop
          - main_loop_wait
            - os_host_main_loop_wait
              - glib_pollfds_poll
                - g_main_context_dispatch
                  - aio_ctx_dispatch
                    - aio_dispatch
                      - aio_bh_poll
                        - aio_bh_call
                          - do_qmp_dispatch_bh
                            - qmp_marshal_qom_get
                              - qmp_qom_get
                                - object_property_get_qobject
                                  - object_property_get
                                    - property_get_alias
                                      - object_property_get
                                        - balloon_stats_get_all

# hmp

- [ ] 可以阅读的文档:
这里描述在 graphic 和 non-graphic 的模式下访问 HMI 的方法，并且说明了从 HMI 中间如何获取各种信息
https://web.archive.org/web/20180104171638/http://nairobi-embedded.org/qemu_monitor_console.html


## 源码分析
- hmp_info_balloon

- _start
  - __libc_start_main_impl
    - __libc_start_call_main
      - qemu_default_main
        - qemu_main_loop
          - main_loop_wait
            - os_host_main_loop_wait
              - glib_pollfds_poll
                - g_main_context_dispatch
                  - tcp_chr_read
                    - monitor_read
                      - readline_handle_byte
                        - monitor_command_cb
                          - handle_hmp_command
                            - handle_hmp_command_exec
                              - handle_hmp_command_exec
                                - hmp_info_balloon

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

## 如何快速定位到代码
hmp 中提供了一个 mce 命令，如何找到对应的实现:

hmp_mce 直接搜索 hmp_mce 即可

## 分析下
https://qemu.readthedocs.io/en/v8.1.5/interop/qemu-qmp-ref.html

## hmp 中的 sendkey 是如何实现的

此外，相关配置:
https://vncdotool.readthedocs.io/en/latest/usage.html

arm 上面没办法用，难道是这个问题?
https://lists.gnu.org/archive/html/qemu-devel/2018-02/msg06218.html


## 也许有用
给Qemu虚拟机“打信号”：自定义QMP注入SCI中断 - MyStackTrace的文章 - 知乎
https://zhuanlan.zhihu.com/p/1943785816179607321


## qmp ：没办法，不搞的话，dirty bitmap 是没有办法维持生活的
- [ ] grep 一下目前对于 qmp 的所有问题，尝试将 qmp 和 qemu option 融合一下
- [ ] https://gist.github.com/rgl/dc38c6875a53469fdebb2e9c0a220c6c
- [ ] https://wiki.qemu.org/Documentation/QMP

## qmp

- [ ] `qmp_block_commit` 的唯一调用者是如何被生成的。
- [ ] `OBJECT_DECLARE_SIMPLE_TYPE` 是什么意思，和类似的 macro 有什么区别
- [ ] docs/devel/qapi-code-gen.txt 和 qmp 如何工作的，是如何生成的。

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
