# audit 机制
<!-- 8c4683d4-19d8-45d3-b6ac-abec411c96b2 -->

Linux audit 不是一个访问控制子系统，它主要负责记录安全相关事件，
给 auditd/auditctl 这样的用户态工具提供可配置、可查询的审计日志。

## 大致总结下
- https://wiki.archlinux.org/title/Audit_framework
- https://github.com/linux-audit/audit-documentation/wiki

## 内核代码概览
内核侧可以拆成四块看：

1. `kernel/audit.c`: audit 控制面、netlink 协议、日志 buffer、队列和
   `kauditd` 发送线程。
2. `kernel/auditfilter.c`: 把用户态规则翻译成 `audit_krule`，维护规则链表，
   并提供部分简单过滤函数。
3. `kernel/auditsc.c`: syscall/io_uring 事件的上下文采集、规则匹配和最终记录输出。
4. `kernel/audit_watch.c`、`kernel/audit_tree.c`、`kernel/audit_fsnotify.c`:
   处理路径、目录树、可执行文件等需要跟踪 inode 变化的规则。

| Name                   | Blank | Comment | code |
| ---------------------- | ----- | ------- | ---- |
| audit_fsnotify.c       | 31    | 23      | 162  |
| auditfilter.c          | 153   | 136     | 1151 |
| auditsc.c              | 253   | 392     | 1884 |
| audit.c                | 300   | 544     | 1546 |
| audit_tree.c           | 138   | 84      | 812  |

在这里，我们可以看到，其实 filter 是 audit 实现的关键机制。
## 用户态消息和内核态事件是两条路径

`audit_receive_msg()` 只处理从 audit netlink 进来的用户态消息，典型来源是
`auditd`、`auditctl` 或其他持有对应 capability 的程序。

大致路径是：

```txt
auditctl/auditd
  -> NETLINK_AUDIT
  -> audit_receive()
  -> audit_receive_msg()
      -> AUDIT_GET/AUDIT_SET/AUDIT_SET_FEATURE
      -> AUDIT_ADD_RULE/AUDIT_DEL_RULE -> audit_rule_change()
      -> AUDIT_USER* -> audit_filter(..., AUDIT_FILTER_USER) -> audit_log_end()
```

syscall 审计不是从 `audit_receive_msg()` 进来的。syscall 路径来自 task 上的
`audit_context` 和 syscall work：

```txt
copy_process()
  -> audit_alloc()
      -> audit_filter_task()
      -> set_task_syscall_work(..., SYSCALL_AUDIT)

syscall enter
  -> syscall_enter_audit()
  -> audit_syscall_entry()
  -> __audit_syscall_entry()

syscall 内部 VFS/IPC/socket/exec 等 hook
  -> audit_getname()
  -> audit_inode()
  -> audit_inode_child()
  -> audit_sockaddr()
  -> audit_bprm()
  -> ...

syscall exit
  -> audit_syscall_exit()
  -> __audit_syscall_exit()
      -> audit_filter_syscall()
      -> audit_filter_inodes()
      -> audit_log_exit()
```

所以原笔记里的两个观察可以修正为：

1. 内核态 syscall 审计主要在 `auditsc.c`，但不是所有 audit 记录都来自
   `auditsc.c`。例如 netfilter、seccomp、module、fanotify 等也会直接调用
   audit logging API。
2. 用户态消息经 netlink 进入 `audit_receive_msg()`，其中用户态自报的
   `AUDIT_USER*` 会再经过 `AUDIT_FILTER_USER` 过滤。

## audit_log_* 做了什么

`audit_log_start()` 创建 `audit_buffer`，内部实际持有 netlink skb。之后
`audit_log_format()`、`audit_log_untrustedstring()` 等只是往 skb 里追加文本。

结束时 `audit_log_end()` 会把 skb 放入 `audit_queue`，唤醒 `kauditd`：

```txt
audit_log_start()
  -> audit_log_format()
  -> audit_log_end()
      -> skb_queue_tail(&audit_queue, skb)
      -> wake_up_interruptible(&kauditd_wait)

kauditd_thread()
  -> kauditd_send_queue()
      -> netlink unicast to auditd
      -> multicast
      -> retry/hold queue on failure
```

注意 `audit_log_start()` 会先调用：

```c
audit_filter(type, AUDIT_FILTER_EXCLUDE)
```

这意味着 `AUDIT_FILTER_EXCLUDE` 是记录创建前的全局排除规则，不是 syscall
exit 才生效。

## filter list 和 rules list 的区别

`auditfilter.c` 里有两个数组：

```c
struct list_head audit_filter_list[AUDIT_NR_FILTERS];
static struct list_head audit_rules_list[AUDIT_NR_FILTERS];
```

它们不是重复设计：

1. `audit_filter_list` 是运行时匹配用的链表。`audit_filter()`、
   `audit_filter_task()`、`audit_filter_syscall()` 等会遍历它。
2. `audit_rules_list` 是规则枚举和展示用的链表。`audit_list_rules()` 用它把
   当前规则转换回 `struct audit_rule_data` 发给用户态。
3. 带 inode/path 约束的规则不一定只在 `audit_filter_list` 上。`audit_find_rule()`
   对 `inode_f` 会走 `audit_inode_hash`，对 `watch` 会扫描 inode hash；
   syscall exit 阶段也会根据 `ctx->names_list` 里的 inode 去 hash bucket 匹配。

因此 `audit_add_rule()` 同时把规则挂入 `audit_rules_list` 和用于运行时匹配的
位置。`audit_rules_list` 负责“还能完整列出用户添加的规则”，
运行时匹配则可能为了性能放到 filter list 或 inode hash。

当前源码中：

```c
#define AUDIT_NR_FILTERS 8
```

除了原笔记中看到的 7 类，还有 `AUDIT_FILTER_URING_EXIT`，用于 io_uring op
结束时的规则匹配。

## 各类 filter 的含义

`AUDIT_FILTER_*` 不是“被监听的 event 列表”，而是规则在哪个阶段参与匹配：

1. `AUDIT_FILTER_USER`: 过滤用户态发来的 `AUDIT_USER*` 消息。
2. `AUDIT_FILTER_TASK`: task 创建时决定这个 task 是否需要 syscall audit
   context。
3. `AUDIT_FILTER_ENTRY`: syscall entry 旧接口，当前代码会提示 deprecated。
4. `AUDIT_FILTER_WATCH`: UAPI 中仍存在的旧 watch 类型标志，当前路径规则主要
   以 `AUDIT_FILTER_EXIT` 或 `AUDIT_FILTER_URING_EXIT` 形式进入。
5. `AUDIT_FILTER_EXIT`: syscall exit 时匹配 syscall number、返回值、uid/gid、
   path/inode、LSM label 等。
6. `AUDIT_FILTER_EXCLUDE`: 在创建 audit record 前排除某些 record type。
7. `AUDIT_FILTER_FS`: 在 `__audit_inode()` / `__audit_inode_child()` 采集路径
   信息时使用，当前有效字段主要是 `AUDIT_FSTYPE` 和 `AUDIT_FILTERKEY`。
8. `AUDIT_FILTER_URING_EXIT`: io_uring op exit 时匹配。

真正的 audit record type 是 `include/uapi/linux/audit.h` 中的 `AUDIT_*`
消息类型，例如：

```txt
AUDIT_SYSCALL
AUDIT_PATH
AUDIT_CWD
AUDIT_EXECVE
AUDIT_PROCTITLE
AUDIT_SECCOMP
AUDIT_NETFILTER_CFG
AUDIT_KERN_MODULE
AUDIT_USER*
AUDIT_CONFIG_CHANGE
```

所以“哪些 events 会被 audit”不能只看 `AUDIT_FILTER_*`。更准确的分类是：

1. syscall/io_uring 事件，由规则和 `auditsc.c` 决定是否输出。
2. 用户态可信消息，例如 `AUDIT_USER*`。
3. 内核安全事件，例如 seccomp、netfilter、module、fanotify、capability、
   ptrace、IPC、socket address 等。
4. audit 自身配置变化，例如开启/关闭、auditd pid、规则增删。

## watch、tree、fsnotify mark 的关系

这三个名字容易混在一起，但它们的语义不同。

### AUDIT_WATCH

`AUDIT_WATCH` 是单个路径 watch。规则解析时：

```txt
audit_data_to_entry()
  -> AUDIT_WATCH
  -> audit_to_watch()
```

`audit_to_watch()` 要求路径是绝对路径，不能以 `/` 结尾，并且规则 list 必须是
`AUDIT_FILTER_EXIT` 或 `AUDIT_FILTER_URING_EXIT`。添加规则时 `audit_add_watch()`
会把 watch 挂到父目录的 fsnotify mark 上。父目录发生 create/delete/move 时，
`audit_watch_handle_event()` 更新 watch 中记录的 dev/ino，必要时删除规则。

运行时匹配不是靠 fsnotify event 直接产生日志，而是 syscall 路径采集到
`audit_names` 后，在 syscall exit 用 inode hash 匹配规则。

### AUDIT_DIR / audit_tree

`AUDIT_DIR` 表示目录树规则。解析时：

```txt
audit_data_to_entry()
  -> AUDIT_DIR
  -> audit_make_tree()
```

`audit_tree` 用 fsnotify mark 维护目录树内 inode 到 tree 的关系。VFS hook
采集路径时，`handle_path()` 会沿 dentry 向上查找带 mark 的 inode，
把命中的 tree 引用放进当前 syscall 的 `audit_context`。

因此 tree 解决的是“整个目录子树”的规则维护问题，不是单个文件 watch。

### AUDIT_EXE / audit_fsnotify

`AUDIT_EXE` 是按当前进程可执行文件过滤。解析时：

```txt
audit_data_to_entry()
  -> AUDIT_EXE
  -> audit_alloc_mark()
```

`audit_fsnotify.c` 里的 `audit_fsnotify_mark` 保存 executable path 对应的
dev/ino，并在 create/delete/move/self-delete 等 fsnotify 事件中更新或自动移除。
匹配时 `audit_exe_compare()` 会取 `current->mm` 的 exe file，再比较 dev/ino。

所以 `audit_fsnotify.c` 更准确地说是服务 `AUDIT_EXE` 这类 inode mark 规则，
不是所有 audit watch 的统一入口。

## 为什么 watch/tree/fsnotify 都有全局 group

`audit_watch_group`、`audit_tree_group`、`audit_fsnotify_group` 都是
`fsnotify_group`。fsnotify 的 mark 需要隶属于某个 group，事件回调也挂在 group
的 `fsnotify_ops` 上。

使用全局 group 的原因是：

1. audit 子系统需要一个统一的 fsnotify 身份来注册 mark。
2. 回调里要确认事件是否来自自己的 group，例如 `audit_mark_handle_event()`
   会检查 `inode_mark->group == audit_fsnotify_group`。
3. 同类 audit mark 共用同一套回调和生命周期管理。
4. group 生命周期等同 audit 子系统生命周期，适合在 init 阶段创建一次。

这不是为了“保存所有规则”，规则仍在 `audit_krule`、watch/tree 对象、
filter list 或 inode hash 中。

## LSM rule、tree、watch 分别是什么

`LSM rule` 指的是 audit 规则中的 subject/object label 条件，例如
`AUDIT_SUBJ_TYPE`、`AUDIT_OBJ_TYPE`。这些字段通过：

```txt
security_audit_rule_init()
security_audit_rule_match()
```

交给当前 LSM 解释。它和 `audit_tree` 没有直接关系。

`audit_tree` 是目录树路径匹配结构。

`audit_watch` 是单路径 watch 结构。

它们都可能作为同一个 `audit_krule` 的字段或辅助对象存在，但含义完全不同：
LSM 负责安全标签匹配，tree/watch 负责路径和 inode 关系维护。

## 实验

实验环境：

```txt
oe2403
Linux 6.6.0-28.0.0.34.oe2403.x86_64
auditd active
audit enabled 1
```

临时目录为 `/tmp/audit-lab`。测试前 `auditctl -l` 为 `No rules`，测试后已清理
回 `No rules`。

验证过两类规则：

```bash
auditctl -a always,exit -F arch=b64 \
  -S openat -S openat2 -S creat -S unlink -S rename -S renameat -S renameat2 \
  -F dir=/tmp/audit-lab -F perm=wa -k audit_lab_syscall

auditctl -w /tmp/audit-lab -p wa -k audit_lab_watch
```

触发 `create -> rename -> unlink` 后，日志中可以看到：

1. 规则增删产生 `CONFIG_CHANGE`，说明配置面走 netlink 到内核。
2. 文件操作产生 `SYSCALL`、`CWD`、`PATH`、`PROCTITLE` 组合记录。
3. `PATH` 记录包含 `nametype=CREATE`、`nametype=DELETE`、`nametype=PARENT`。
4. watch 规则最终也表现为 syscall audit 事件，而不是 fsnotify event 直接输出日志。
5. 查询历史日志需要用 `ausearch --input-logs -k <key>`，单独 `ausearch -k <key>`
   在这个环境没有查到旧日志。

这个实验支持前面的判断：watch/tree/fsnotify 主要维护 inode/path 关系，最终记录
仍由 syscall exit 路径输出。

## 具体问题
### 内核日志

原因未知，我估计还是由于 auditd 没有接受，发所以临时内核中:
```txt
[1379552.118802] audit: type=1400 audit(1757558606.315:14364): avc:  denied  { map } for  pid=4112132 comm="libuv-worker" path="anon_inode:[io_uring]" dev="anon_inodefs" ino=20572134 scontext=unconfined_u:unconfined_r:unconfined_t:s0-s0:c0.c1023 tcontext=unconfined_u:object_r:unconfined_t:s0 tclass=anon_inode permissive=0
```

```txt
[   54.461015] audit: type=1400 audit(1757341608.061:6): avc:  denied  { checkpoint_restore } for  pid=6892 comm="agetty" capability=40  scontext=system_u:system_r:getty_t:s0-s0:c0.c1023 tcontext=system_u:system_r:getty_t:s0-s0:c0.c1023 tclass=capability2 permissive=0
[   54.489236] audit: type=1400 audit(1757341608.089:7): avc:  denied  { checkpoint_restore } for  pid=6893 comm="agetty" capability=40  scontext=system_u:system_r:getty_t:s0-s0:c0.c1023 tcontext=system_u:system_r:getty_t:s0-s0:c0.c1023 tclass=capability2 permissive=0
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
