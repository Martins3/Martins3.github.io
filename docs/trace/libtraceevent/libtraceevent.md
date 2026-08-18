## libtraceevent

libtraceevent 是 Linux 内核跟踪事件解析库，用于解析和处理 ftrace 产生的跟踪数据。

- 官方仓库：https://git.kernel.org/pub/scm/libs/libtrace/libtraceevent.git/
- 文档：https://www.trace-cmd.org/Documentation/libtraceevent/libtraceevent.html

### 简介

libtraceevent 提供了对内核跟踪事件的解析功能，包括：
- 解析内核跟踪格式文件（`/sys/kernel/debug/tracing/events/*`）
- 解析二进制跟踪数据
- 插件系统支持各种事件类型（sched、kvm、kmem 等）
- 与 libtracefs 配合实现完整的跟踪流程

### 插件目录

```
/usr/lib64/traceevent/plugins
/usr/lib64/traceevent/plugins/plugin_cfg80211.so
/usr/lib64/traceevent/plugins/plugin_function.so
/usr/lib64/traceevent/plugins/plugin_futex.so
/usr/lib64/traceevent/plugins/plugin_hrtimer.so
/usr/lib64/traceevent/plugins/plugin_jbd2.so
/usr/lib64/traceevent/plugins/plugin_kmem.so
/usr/lib64/traceevent/plugins/plugin_kvm.so
/usr/lib64/traceevent/plugins/plugin_mac80211.so
/usr/lib64/traceevent/plugins/plugin_sched_switch.so
/usr/lib64/traceevent/plugins/plugin_scsi.so
/usr/lib64/traceevent/plugins/plugin_tlb.so
/usr/lib64/traceevent/plugins/plugin_xen.so
```

### Nix 环境安装

在 Nix/NixOS 环境中，可以通过以下方式安装：

```bash
# 临时安装
nix-shell -p libtraceevent libtracefs

# 或者添加到 home.packages (home-manager)
# home.nix
home.packages = with pkgs; [
  libtraceevent
  libtracefs
  trace-cmd  # 依赖 libtraceevent
];
```

内核源码中相关工具位置：`kernel/tools/perf/builtin-kvm.c`

### trace-cmd 依赖关系

trace-cmd 依赖 libtraceevent 和 libtracefs：

```txt
🧀  ldd /home/martins3/.nix-profile/bin/trace-cmd
 linux-vdso.so.1 (0x00007ff90494e000)
 librt.so.1 => /nix/store/wn7v2vhyyyi6clcyn0s9ixvl7d4d87ic-glibc-2.40-36/lib/librt.so.1 (0x00007ff904943000)
 libpthread.so.0 => /nix/store/wn7v2vhyyyi6clcyn0s9ixvl7d4d87ic-glibc-2.40-36/lib/libpthread.so.0 (0x00007ff90493e000)
 libtraceevent.so.1 => /nix/store/nv12vywgl8b0xm5xhnl3c56yjgx0liwf-libtraceevent-1.8.4/lib/libtraceevent.so.1 (0x00007ff90491b000)
 libtracefs.so.1 => /nix/store/c2dl1yc3q7xqxll6w9hn00gwbaqiir6d-libtracefs-1.8.1/lib/libtracefs.so.1 (0x00007ff9048f4000)
 libzstd.so.1 => /nix/store/8pys6a47askf0g75a1k73p3rx2wim7m6-zstd-1.5.6/lib/libzstd.so.1 (0x00007ff90481e000)
 libdl.so.2 => /nix/store/wn7v2vhyyyi6clcyn0s9ixvl7d4d87ic-glibc-2.40-36/lib/libdl.so.2 (0x00007ff904819000)
 libc.so.6 => /nix/store/wn7v2vhyyyi6clcyn0s9ixvl7d4d87ic-glibc-2.40-36/lib/libc.so.6 (0x00007ff904620000)
 /nix/store/wn7v2vhyyyi6clcyn0s9ixvl7d4d87ic-glibc-2.40-36/lib/ld-linux-x86-64.so.2 => /nix/store/wn7v2vhyyyi6clcyn0s9ixvl7d4d87ic-glibc-2.40-36/lib64/ld-linux-x86-64.so.2 (0x00007ff904950000)
```

### 主要 API

#### libtraceevent

| 函数 | 说明 |
|------|------|
| `tep_alloc()` | 分配 tep 句柄 |
| `tep_free()` | 释放 tep 句柄 |
| `tep_parse_event()` | 从字符串解析事件格式 |
| `tep_find_event_by_name()` | 按名称查找事件 |
| `tep_get_event()` | 按索引获取事件 |
| `tep_get_events_count()` | 获取事件数量 |
| `tep_list_events()` | 获取排序后的事件列表 |
| `tep_event_fields()` | 获取事件的字段列表 |
| `tep_find_field()` | 按名称查找字段 |
| `tep_set_page_size()` | 设置 page size |
| `tep_set_long_size()` | 设置 long 类型大小 |
| `tep_print_plugins()` | 打印已加载插件 |

#### libtracefs (配合 libtraceevent 使用)

| 函数 | 说明 |
|------|------|
| `tracefs_tracing_dir()` | 获取 tracing 目录路径 |
| `tracefs_fill_local_events()` | 从系统加载事件到 tep_handle |
| `tracefs_load_headers()` | 加载 tracing header 信息 |
| `tracefs_load_cmdlines()` | 加载进程命令行信息 |

### 相关工具

- **trace-cmd**: 命令行跟踪工具，底层使用 libtraceevent
- **kernelshark**: GUI 跟踪数据查看器
- **perf**: 内核 perf 工具也使用相关库

### 参考

- [libtraceevent 官方文档](https://www.trace-cmd.org/Documentation/libtraceevent/libtraceevent.html)
- [内核源码 libtraceevent](https://git.kernel.org/pub/scm/libs/libtrace/libtraceevent.git/)

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
