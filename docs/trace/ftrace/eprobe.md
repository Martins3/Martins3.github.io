# eprobe - Event-based Probe Tracing

> [!NOTE]
> 参考神奇海螺的意见，有待验证

先仔细看看 Documentation/trace/eprobetrace.rst 文档吧

## 简介

eprobe（Event-based Probe Tracing，基于事件的探测跟踪）是 Linux ftrace 框架中的一种动态事件探测机制。它允许在现有的 trace event 上创建探测点，用于：

1. **解引用指针字段** - 访问事件字段中指针指向的数据
2. **限制记录字段** - 只记录感兴趣的字段，减少 ring buffer 开销
3. **类型转换** - 改变字段的显示类型（如指针显示为字符串）

eprobe 于 2021 年由 Steven Rostedt 和 Tzvetomir Stoyanov 引入内核（Linux 5.15+）。

## 快速开始

```bash
cd /sys/kernel/tracing

# 以下命令需要 root 权限，请使用 sudo 执行

# 1. 创建一个 eprobe：在 sched_switch 事件上只提取 pid 信息
echo 'e:switch sched.sched_switch prev=$prev_pid:u32 next=$next_pid:u32' >> dynamic_events

# 2. 启用原事件（必须启用原事件，eprobe 才能工作）
echo 1 > events/sched/sched_switch/enable

# 3. 启用 eprobe
echo 1 > events/eprobes/switch/enable

# 4. 查看输出（按 Ctrl+C 停止）
cat trace
```

输出示例：
```
            bash-1085    [001] d..4.  5041.240198: switch: (sched.sched_switch) prev=1085 next=141
```

**关键提醒**：eprobe 依赖原事件的触发，**必须启用原事件**才能看到输出。

## 主要使用场景

### 1. 精简事件数据

内核 trace event 通常包含很多字段，但用户可能只关心其中几个。例如 `sched_switch` 事件包含 60+ 字节的数据：

```
field:char prev_comm[16];   offset:8;   size:16;
field:pid_t prev_pid;       offset:24;  size:4;
field:int prev_prio;        offset:28;  size:4;
field:long prev_state;      offset:32;  size:8;
field:char next_comm[16];   offset:40;  size:16;
field:pid_t next_pid;       offset:56;  size:4;
...
```

如果只需要 pid 信息，使用 eprobe 可以只记录 `prev_pid` 和 `next_pid`：

```bash
cd /sys/kernel/tracing

# 查看事件字段定义（可选，用于确定字段名和类型）
cat events/sched/sched_switch/format

# 1. 创建 eprobe，只记录 pid
echo 'e:switch sched.sched_switch prev=$prev_pid:u32 next=$next_pid:u32' >> dynamic_events

# 2. 启用原事件（必须启用原事件，eprobe 才能工作）
echo 1 > events/sched/sched_switch/enable

# 3. 启用 eprobe
echo 1 > events/eprobes/switch/enable

# 4. 查看结果
cat trace
```

### 2. 解引用指针获取数据

系统调用事件中参数通常以指针形式存在，eprobe 可以解引用获取实际数据。

**⚠️ 重要前提**：eprobe 依赖原事件的触发，**必须启用原事件**才能看到输出。

```bash
cd /sys/kernel/tracing

# 1. 创建 eprobe
echo 'e:openat raw_syscalls.sys_enter nr=$id filename=+8($args):ustring' >> dynamic_events

# 验证创建成功
cat dynamic_events
# 输出示例: e:eprobes/openat raw_syscalls.sys_enter nr=$id filename=+8($args):ustring

# 2. 启用原事件 raw_syscalls/sys_enter（关键步骤！）
echo 1 > events/raw_syscalls/sys_enter/enable

# 3. 在 eprobe 上设置过滤器（只跟踪 openat 系统调用）
# 不同架构的系统调用号不同，请查阅下表：
#   x86_64:  257
#   aarch64: 56
#   riscv64: 56
#   i386:    295
echo 'nr == 257' > events/eprobes/openat/filter

# 4. 启用 eprobe
echo 1 > events/eprobes/openat/enable

# 5. 测试 - 运行一个会调用 openat 的命令
ls /tmp

# 6. 查看结果
cat trace
```

**参数说明**：
- `+8($args)`：从 `$args` 偏移 8 字节处读取
  - x86_64 上：`args` 数组每个元素 8 字节，+8 对应 `args[1]`（即 pathname）
  - aarch64 上：系统调用参数传递方式不同，可能需要调整偏移
- `ustring`：读取用户态字符串（以 null 结尾）

**常见故障排查**：

| 问题 | 检查方法 | 解决方案 |
|------|----------|----------|
| 无输出 | `cat events/raw_syscalls/sys_enter/enable` | 必须返回 `1`，表示原事件已启用 |
| eprobe 不存在 | `ls events/eprobes/` | 检查创建命令是否成功，查看 `cat dynamic_events` |
| 过滤器错误 | `cat events/eprobes/openat/filter` | 检查语法，或先不加过滤器测试 |
| 系统调用号不对 | `ausyscall openat` 或查头文件 | x86_64 上 openat 是 257，aarch64 上是 56 |
| 显示 `(fault)` | 用户态页未加载 | 使用 synthetic event 组合方案（见下文） |

### 3. 与 Synthetic Events 组合解决内存缺页问题

当访问用户态指针时，可能会遇到内存未加载（page fault）的情况。通过在 `sys_enter` 时保存地址，在 `sys_exit` 时读取（此时内存已加载），可以解决这个问题。

**注意**：此方案需要内核支持 histogram 的 `onmatch` 功能，较新的内核版本才可用。

```bash
cd /sys/kernel/tracing

# 1. 创建 eprobe 保存文件名地址（而不是直接读取字符串）
echo 'e:openat_entry raw_syscalls.sys_enter nr=$id filename_addr=+8($args):x64' >> dynamic_events

# 2. 创建 synthetic event 用于传递数据
echo 's:openat_info u64 pid; u64 filename_addr' >> dynamic_events

# 3. 在 sys_enter 处保存地址到 histogram
echo 'hist:keys=common_pid:pid=common_pid:addr=filename_addr if nr == 257' > events/eprobes/openat_entry/trigger

# 4. 在 sys_exit 处将地址传递给 synthetic event
# 注意：这里的语法可能因内核版本而异
# 方案 A: 使用 id 字段过滤
echo 'hist:keys=common_pid:pid=common_pid:addr=filename_addr:onmatch(eprobes.openat_entry).trace(synthetic.openat_info,common_pid,filename_addr) if id == 257' > events/raw_syscalls/sys_exit/trigger 2>/dev/null || \
# 方案 B: 不使用过滤器
echo 'hist:keys=common_pid:pid=common_pid:addr=filename_addr:onmatch(eprobes.openat_entry).trace(synthetic.openat_info,common_pid,filename_addr)' > events/raw_syscalls/sys_exit/trigger

# 5. 在 synthetic event 上创建 eprobe 读取字符串（此时内存已加载）
echo 'e:openat_final synthetic.openat_info filename=+0($filename_addr):ustring' >> dynamic_events

# 6. 启用事件
echo 1 > events/raw_syscalls/sys_enter/enable
echo 1 > events/raw_syscalls/sys_exit/enable
echo 1 > events/synthetic/openat_info/enable
echo 1 > events/eprobes/openat_final/enable

# 7. 测试
ls /tmp
cat trace | grep openat_final

# 8. 清理
echo '!hist' > events/eprobes/openat_entry/trigger
echo '!hist' > events/raw_syscalls/sys_exit/trigger
echo '-:openat_info' >> dynamic_events
echo '-:openat_entry' >> dynamic_events
echo '-:openat_final' >> dynamic_events
```

### 4. 修改字段显示类型

eprobe 可以将字段以不同类型显示：

```bash
# 将指针显示为十六进制
echo 'e:show_ptr some.event ptr=$field:x64' >> dynamic_events

# 将整数显示为有符号/无符号
echo 'e:show_int some.event val=$count:s32' >> dynamic_events

# 将整数显示为不同进制
echo 'e:show_hex sched.sched_switch pid=$prev_pid:x32' >> dynamic_events
```

支持的类型包括：

| 类型 | 说明 |
|------|------|
| `u8/u16/u32/u64` | 无符号整数（1/2/4/8 字节） |
| `s8/s16/s32/s64` | 有符号整数（1/2/4/8 字节） |
| `x8/x16/x32/x64` | 十六进制（1/2/4/8 字节） |
| `string` | 内核态字符串 |
| `ustring` | 用户态字符串 |
| `symbol` | 内核符号 |
| `symstr` | 符号字符串 |

### 5. 跟踪文件打开操作

结合 eprobe 和过滤器，可以跟踪特定文件的打开操作：

```bash
cd /sys/kernel/tracing

# 创建 eprobe 跟踪 openat
echo 'e:file_open raw_syscalls.sys_enter nr=$id fd=$args[0]:s64 path=+8($args):ustring' >> dynamic_events

# 启用原事件
echo 1 > events/raw_syscalls/sys_enter/enable

# 设置过滤器：只跟踪 nr==257 (openat) 且路径包含特定字符串
# 注意：不能在 eprobe 上过滤 ustring 字段的内容
# 需要通过后续处理过滤
echo 'nr == 257' > events/eprobes/file_open/filter

# 启用 eprobe
echo 1 > events/eprobes/file_open/enable

# 查看结果
cat trace | grep file_open
```

## 实现原理

### 架构概述

```
┌─────────────────────────────────────────────────────────────┐
│                      eprobe 架构                             │
├─────────────────────────────────────────────────────────────┤
│  User Space                                                 │
│     └── echo "e:..." > /sys/kernel/tracing/dynamic_events   │
├─────────────────────────────────────────────────────────────┤
│  Kernel Space                                               │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐     │
│  │  trace_eprobe│    │ trace_probe │    │  target     │     │
│  │    .c       │───▶│   framework │───▶│  event      │     │
│  │             │    │             │    │  (tracepoint)│     │
│  └─────────────┘    └─────────────┘    └─────────────┘     │
│         │                    │              │               │
│         ▼                    ▼              ▼               │
│  ┌─────────────────────────────────────────────────────┐   │
│  │              dynamic_events 接口                     │   │
│  └─────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 核心数据结构

```c
// kernel/trace/trace_eprobe.c
struct trace_eprobe {
    /* 目标 tracepoint 的 system 名称 */
    const char *event_system;

    /* 目标 tracepoint 的 event 名称 */
    const char *event_name;

    /* 过滤条件字符串 */
    char *filter_str;

    /* 指向目标 trace_event_call 的指针 */
    struct trace_event_call *event;

    /* 动态事件管理结构 */
    struct dyn_event devent;

    /* 探测参数信息（与 kprobe 共享） */
    struct trace_probe tp;
};

struct eprobe_data {
    struct trace_event_file *file;
    struct trace_eprobe *ep;
};
```

### 创建流程

```
用户写入: e:switch sched.sched_switch prev=$prev_pid:u32
                │
                ▼
        eprobe_dyn_event_create()
                │
                ▼
        __trace_eprobe_create()
                │
        ┌───────┴───────┐
        ▼               ▼
  parse event      parse args
  (sched.sched_switch) (prev=$prev_pid:u32)
        │               │
        └───────┬───────┘
                ▼
        find_and_get_event()  // 查找目标 event
                │
                ▼
        alloc_event_probe()     // 分配 trace_eprobe
                │
                ▼
        traceprobe_parse_probe_arg()  // 解析参数
                │
                ▼
        trace_probe_register_event_call()  // 注册 event
                │
                ▼
        dyn_event_add()         // 添加到动态事件列表
```

### 触发流程

当目标事件触发时，eprobe 的执行流程：

```
tracepoint 触发
       │
       ▼
event_trigger_callback()  // 在目标 event 上注册的触发器
       │
       ▼
eprobe_trigger_func()     // eprobe 触发函数
       │
       ├──▶ 应用 filter（如果设置了过滤条件）
       │
       ├──▶ get_event_field()  // 获取字段值
       │      └── 使用 $FIELD 语法访问事件字段
       │
       ├──▶ process_fetch_insn()  // 处理 FETCHARGS
       │      └── 支持解引用、类型转换等
       │
       └──▶ trace_event_buffer_commit()  // 写入 ring buffer
```

### 关键实现细节

#### 1. 字段访问机制

eprobe 使用 `$FIELD` 语法访问目标事件的字段：

```c
// 获取事件字段值的代码片段
static nokprobe_inline unsigned long
get_event_field(struct fetch_insn *code, void *rec)
{
    struct ftrace_event_field *field = code->data;

    // rec 是目标事件的记录数据
    // 根据字段的 offset 和 size 读取值
    return fetch_kernel_var_arg(rec, field->offset,
                                field->size, field->is_signed);
}
```

#### 2. 与 kprobe 的关系

eprobe 依赖于 kprobe 事件的基础设施：

```
CONFIG_EPROBE_EVENTS 依赖于 CONFIG_KPROBE_EVENTS

共享的组件：
- trace_probe.c/h: 通用探测框架
- FETCHARGS 解析器
- 参数获取机制（process_fetch_insn）
```

主要区别在于：
- **kprobe**: 基于指令地址，可以访问寄存器、栈、函数参数
- **eprobe**: 基于 tracepoint，只能访问事件字段（`$FIELD`）

#### 3. 动态事件管理

eprobe 通过 `dyn_event` 框架管理：

```c
static struct dyn_event_operations eprobe_dyn_event_ops = {
    .create = eprobe_dyn_event_create,      // 创建
    .show = eprobe_dyn_event_show,          // 显示
    .is_busy = eprobe_dyn_event_is_busy,    // 检查是否繁忙
    .free = eprobe_dyn_event_release,       // 释放
    .match = eprobe_dyn_event_match,        // 匹配
};
```

### 内核配置

启用 eprobe 需要以下配置：

```
CONFIG_TRACING=y
CONFIG_KPROBE_EVENTS=y      # eprobe 依赖 kprobe 基础设施
CONFIG_EPROBE_EVENTS=y      # 启用 eprobe 功能
CONFIG_DYNAMIC_EVENTS=y     # 动态事件支持
```

## 命令语法

```
e[:[EGRP/][EEVENT]] GRP.EVENT [FETCHARGS] [if FILTER]  : 创建探测
-:[EGRP/][EEVENT]                                        : 删除探测
```

参数说明：
- `EGRP`: 新事件的组名，默认为 `eprobes`
- `EEVENT`: 新事件的事件名，默认与目标事件相同
- `GRP.EVENT`: 要附加的目标事件（格式：组名.事件名）
- `FETCHARGS`: 要获取的参数（最多 128 个）
- `FILTER`: 过滤条件

**命名规则**：
- `e:myevent sched.sched_switch ...` - 创建名为 `eprobes/myevent` 的事件
- `e:mygroup/myevent sched.sched_switch ...` - 创建名为 `mygroup/myevent` 的事件
- `e:sched/sched_switch sched.sched_switch ...` - 创建名为 `sched/sched_switch` 的事件

FETCHARGS 语法：
- `$FIELD`: 获取事件字段的值
- `$comm`: 获取当前任务名
- `@ADDR`: 获取内核地址处的内存
- `+OFFSET($FIELD)`: 从字段值偏移处获取
- `NAME=VALUE`: 设置参数名
- `ARG:TYPE`: 设置参数类型

## 使用示例汇总

```bash
# 进入 tracing 目录
cd /sys/kernel/tracing

# ========== 示例 1: 精简 sched_switch 事件 ==========
# 1. 创建 eprobe
echo 'e:switch sched.sched_switch prev=$prev_pid next=$next_pid' >> dynamic_events

# 2. 启用原事件（重要！）
echo 1 > events/sched/sched_switch/enable

# 3. 启用 eprobe
echo 1 > events/eprobes/switch/enable

# 4. 查看
cat trace

# ========== 示例 2: 跟踪 openat 系统调用 ==========
# 1. 创建 eprobe
echo 'e:openat raw_syscalls.sys_enter nr=$id path=+8($args):ustring' >> dynamic_events

# 2. 启用原事件（重要！）
echo 1 > events/raw_syscalls/sys_enter/enable

# 3. 添加过滤器（根据架构选择正确的系统调用号）
# x86_64:  nr == 257
# aarch64: nr == 56
# i386:    nr == 295
echo 'nr == 257' > events/eprobes/openat/filter

# 4. 启用 eprobe
echo 1 > events/eprobes/openat/enable

# 5. 测试并查看
ls /tmp
cat trace

# ========== 常用管理命令 ==========
# 查看已创建的 eprobe
cat dynamic_events

# 删除指定 eprobe
echo '-:openat' >> dynamic_events

# 删除所有 eprobe
echo '-:*' >> dynamic_events

# 禁用 eprobe
echo 0 > events/eprobes/openat/enable

# 禁用原事件
echo 0 > events/raw_syscalls/sys_enter/enable

# 清空 trace buffer
echo > trace
```

## 注意事项

1. **必须启用原事件**：eprobe 依赖目标事件的触发，因此原事件必须启用
2. **性能影响**：虽然 eprobe 可以减少 ring buffer 的数据量，但目标事件的所有字段仍会被记录到临时缓冲区中处理
3. **内存缺页**：访问用户态指针时可能遇到 page fault，建议使用 synthetic event 组合
4. **eprobe 可以附加到动态事件**：包括 kprobe、synthetic event、fprobe 等
5. **事件命名**：如果不指定组名，默认组名为 `eprobes`；如果不指定事件名，默认与目标事件同名

## 故障排查

### 问题：eprobe 创建后没有输出

**排查步骤**：

```bash
cd /sys/kernel/tracing

# 1. 检查 eprobe 是否创建成功
cat dynamic_events
# 应该显示: e:eprobes/openat raw_syscalls.sys_enter nr=$id path=+8($args):ustring

# 2. 检查 eprobe 目录是否存在
ls events/eprobes/
# 应该看到 openat 目录

# 3. 检查原事件是否启用
cat events/raw_syscalls/sys_enter/enable
# 必须返回 1

# 4. 检查 eprobe 是否启用
cat events/eprobes/openat/enable
# 应该返回 1

# 5. 检查过滤器是否正确（如果有）
cat events/eprobes/openat/filter
# 应该显示: nr == 257

# 6. 直接测试原事件是否有输出
echo 0 > events/eprobes/openat/enable  # 先禁用 eprobe
cat trace | head -20
# 应该看到 raw_syscalls.sys_enter 的输出

# 7. 确认系统调用号
ausyscall $(uname -m) openat
# 或使用: grep __NR_openat /usr/include/asm/unistd_$(uname -m | sed 's/x86_64/64/;s/aarch64//;s/riscv64//').h 2>/dev/null || grep __NR_openat /usr/include/asm-generic/unistd.h
```

### 问题：显示 (fault)

当读取用户态字符串时显示 `(fault)`，表示内存页未加载：

```
cat-1331 [001] ...5. 2944.787977: openat: (raw_syscalls.sys_enter) filename=(fault)
```

**原因**：在 `sys_enter` 时，用户态传入的字符串指针可能指向的内存页尚未加载到物理内存中。

**解决方案**：
1. 使用 synthetic event 组合（详见示例 3）
2. 改用 `sys_exit` 时机读取（如果内核版本支持）
3. 对于内核态指针，使用 `:string` 而不是 `:ustring`

### 问题：过滤器语法错误

如果过滤器语法错误，写入时会报错：

```bash
# 错误的过滤器（使用 = 而不是 ==）
echo 'nr=257' > events/eprobes/openat/filter
# bash: echo: write error: Invalid argument

# 正确的过滤器（使用 ==）
echo 'nr == 257' > events/eprobes/openat/filter
```

### 问题：eprobe 创建失败

```bash
# 目标事件不存在
echo 'e:test nonexistent.event field=$value' >> dynamic_events
# bash: echo: write error: No such file or directory

# 字段名错误
echo 'e:test sched.sched_switch field=$nonexistent' >> dynamic_events
# bash: echo: write error: Invalid argument

# 类型错误
echo 'e:test sched.sched_switch pid=$prev_pid:invalid_type' >> dynamic_events
# bash: echo: write error: Invalid argument
```

### 问题：权限不足

```bash
# 普通用户无法写入 tracing 文件
echo 'e:test sched.sched_switch pid=$prev_pid' >> dynamic_events
# bash: /sys/kernel/tracing/dynamic_events: Permission denied

# 解决方案：使用 sudo
sudo bash -c 'echo "e:test sched.sched_switch pid=\$prev_pid" >> /sys/kernel/tracing/dynamic_events'
```

## 系统调用号参考表

常见系统调用在不同架构的编号：

| 系统调用 | x86_64 | aarch64 | riscv64 | i386 |
|----------|--------|---------|---------|------|
| read     | 0      | 63      | 63      | 3    |
| write    | 1      | 64      | 64      | 4    |
| open     | 2      | -       | -       | 5    |
| close    | 3      | 57      | 57      | 6    |
| openat   | 257    | 56      | 56      | 295  |
| readlink | 89     | -       | -       | 85   |
| readlinkat | 267  | 78      | 78      | 305  |
| execve   | 59     | 221     | 221     | 11   |
| execveat | 322    | 281     | 281     | 358  |
| clone    | 56     | 220     | 220     | 120  |
| clone3   | 435    | 435     | 435     | 435  |

查询当前架构的系统调用号：
```bash
# 方法 1：使用 ausyscall
ausyscall $(uname -m) openat

# 方法 2：查看头文件
grep __NR_openat /usr/include/asm/unistd_64.h 2>/dev/null || \
grep __NR_openat /usr/include/asm-generic/unistd.h
```

## 参考

- [内核文档 - eprobetrace](https://docs.kernel.org/trace/eprobetrace.html)
- [LWN - Creation of event probe](https://lwn.net/Articles/866765/)
- `kernel/trace/trace_eprobe.c` - 内核实现源码
- `kernel/trace/trace_probe.c` - 通用探测框架

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
