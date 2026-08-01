# GDB 读取未填充的 userfaultfd missing 页为何失败

## 结论

当一段尚未填充的匿名内存被注册为 `UFFDIO_REGISTER_MODE_MISSING` 后，当前 GDB
无法直接读取它。无论 userfaultfd 是否带有 `UFFD_USER_MODE_ONLY`，测试结果都是：

```text
Cannot access memory at address 0x...
```

两种配置分别命中 `handle_userfault()` 中的两道检查：

- 设置 `UFFD_USER_MODE_ONLY` 时，GDB 的访问不带 `FAULT_FLAG_USER`，因此被第一道
  检查拒绝。
- 不设置 `UFFD_USER_MODE_ONLY` 时，第一道检查不再拒绝；但 GDB 使用的 remote GUP
  路径不带 `FAULT_FLAG_ALLOW_RETRY`，因此仍被下一道检查拒绝。

两条路径都返回 `VM_FAULT_SIGBUS`，不会产生 userfaultfd 事件。远程内存读取最终
向 GDB 返回 `EIO`。

这里的“不能访问”不是因为地址没有对应的 VMA。VMA 存在并且具有读写权限，失败的
原因是该地址还没有 PTE，而 GDB 所走的内核远程访问路径不能等待 userfaultfd
handler 填充该页。

这个结论有以下前提：

1. 目标页仍处于 missing 状态。如果 fault handler 已经用 `UFFDIO_COPY` 等操作填充
   了 PTE，GDB 可以直接读取该页，不需要再次触发 missing fault。
2. GDB 通过 `/proc/PID/task/TID/mem` 或 ptrace 的 remote GUP 路径读取内存。未来
   如果内核或 GDB 更换为能够等待 userfaultfd 的访问方式，结果可能不同。

## 测试方法

`userfault.c` 包含两个 GDB 测试：

- `gdb-missing`：使用 `UFFD_USER_MODE_ONLY`。
- `gdb-missing-no-user-mode-only`：创建 userfaultfd 时不使用
  `UFFD_USER_MODE_ONLY`，允许 userfaultfd 处理具备其他必要条件的 kernel fault。

两个测试都会创建一个目标子进程。目标进程执行以下操作：

1. 按当前测试要求创建带或不带 `UFFD_USER_MODE_ONLY` 的 userfaultfd。
2. `mmap()` 一页匿名内存，但不访问它。
3. 将该页注册为 `UFFDIO_REGISTER_MODE_MISSING`。
4. 故意不启动 userfaultfd handler，保证该页一直没有被填充。
5. 临时设置 `PR_SET_PTRACER_ANY`，让同一测试创建的 GDB 进程可以通过 Yama 的
   `ptrace_scope` 检查。

测试随后执行真实的 GDB 命令：

```text
gdb --quiet --nx --batch --pid PID --ex 'x/1bx ADDRESS'
```

测试同时检查两件事：

- GDB 输出包含 `Cannot access memory at address`；
- GDB 访问结束后，userfaultfd 仍然没有 `POLLIN` 事件。

运行方法：

```bash
make
./userfault.out gdb-missing
./userfault.out gdb-missing-no-user-mode-only
```

如果系统没有安装 GDB，两个用例都返回 `SKIP`。如果系统权限不允许创建不带
`UFFD_USER_MODE_ONLY` 的 userfaultfd，`gdb-missing-no-user-mode-only` 返回
`SKIP`，不会把环境限制误报成内核行为失败。

## 从 GDB 到 userfaultfd 的内核调用链

当前 Linux native GDB 优先通过 `/proc/PID/task/TID/mem` 读取 inferior 内存。
`x/1bx ADDRESS` 的主要调用链如下：

```text
GDB x/1bx
  -> pread64(/proc/PID/task/TID/mem, ADDRESS)
  -> mem_read() / mem_rw()                    fs/proc/base.c
  -> access_remote_vm()
  -> __access_remote_vm()                     mm/memory.c
  -> get_user_page_vma_remote()               include/linux/mm.h
  -> get_user_pages_remote() / GUP            mm/gup.c
  -> faultin_page()                           mm/gup.c
  -> handle_mm_fault()                        mm/memory.c
  -> do_anonymous_page()                      mm/memory.c
  -> handle_userfault(..., VM_UFFD_MISSING)   fs/userfaultfd.c
```

如果 `/proc/PID/task/TID/mem` 不可用，GDB 可以回退到
`ptrace(PTRACE_PEEKDATA)`。它经过 `ptrace_request()`、
`generic_ptrace_peekdata()` 和 `ptrace_access_vm()`，随后同样进入
`access_remote_vm()`。因此两种 GDB 读取路径在 userfaultfd fault 处理部分没有
本质区别。

关键点在 `faultin_page()`。GUP 根据 `FOLL_WRITE`、`FOLL_REMOTE`、
`FOLL_UNLOCKABLE` 等标志构造 `fault_flags`：

- 它不会设置 `FAULT_FLAG_USER`。该标志表示 fault 是由 CPU 在用户态执行指令时
  产生的；GDB 是在内核中替另一个进程访问地址，因此属于 remote kernel fault。
- 只有 GUP 带 `FOLL_UNLOCKABLE` 时，它才设置 `FAULT_FLAG_ALLOW_RETRY`。GDB 经
  `access_remote_vm()` 调用 `get_user_page_vma_remote()`，后者调用
  `get_user_pages_remote()` 时没有传入 `locked` 参数，因此不会获得
  `FOLL_UNLOCKABLE`，最终也没有 `FAULT_FLAG_ALLOW_RETRY`。

`handle_userfault()` 的返回值初始为 `VM_FAULT_SIGBUS`。它取得 VMA 上的
userfaultfd context 后依次执行以下判断：

```c
if (!(vmf->flags & FAULT_FLAG_USER) &&
    (ctx->flags & UFFD_USER_MODE_ONLY))
	goto out;

if (!(vmf->flags & FAULT_FLAG_ALLOW_RETRY))
	goto out;
```

带 `UFFD_USER_MODE_ONLY` 的测试命中第一项，不带该标志的测试命中第二项。两种
情况下函数都直接返回 `VM_FAULT_SIGBUS`，不会把 `UFFD_EVENT_PAGEFAULT` 放入
userfaultfd 队列，也不会等待用户态 handler。这就是两个测试都要断言
userfaultfd 没有 `POLLIN` 的原因。

## 错误如何变成 GDB 的提示

在当前 GDB 优先使用的 `/proc/PID/task/TID/mem` 路径中，错误转换过程如下：

```text
handle_userfault(): VM_FAULT_SIGBUS
  -> faultin_page(): vm_fault_to_errno(...): -EFAULT
  -> get_user_pages_remote(): 无法取得目标页
  -> __access_remote_vm(): 复制 0 字节
  -> fs/proc/base.c:mem_rw(): -EIO
  -> pread64(/proc/PID/task/TID/mem): errno = EIO
  -> GDB: Cannot access memory at address 0x...
```

`mem_rw()` 在尚未复制任何数据时发现 `access_remote_vm()` 返回 0，会返回
`-EIO`。ptrace 回退路径的结果相同：`generic_ptrace_peekdata()` 必须成功读到完整的
一个 `unsigned long`，否则也返回 `-EIO`。所以 GDB 最终看到的是远程内存读取的
`EIO`，而不是直接看到内部 GUP 使用的 `EFAULT`。

目标进程不会真的收到一个 `SIGBUS` 信号；`VM_FAULT_SIGBUS` 在这里是内核 fault
处理结果位，随后被 GUP 转换成错误返回。

## 不带 UFFD_USER_MODE_ONLY 的实验

在允许非特权 kernel fault 的测试环境中，第二个用例确实成功创建了不带
`UFFD_USER_MODE_ONLY` 的 userfaultfd，而不是因为权限问题跳过。结果仍然是：

```text
TEST gdb-missing-no-user-mode-only
GDB: Cannot access memory at address 0x...
PASS gdb-missing-no-user-mode-only
```

测试随后轮询 userfaultfd，确认没有收到 `UFFD_EVENT_PAGEFAULT`。这证明仅仅关闭
`UFFD_USER_MODE_ONLY` 不足以使 GDB 访问进入 userfaultfd 等待路径。

不带 `UFFD_USER_MODE_ONLY` 的 userfaultfd 仍然可以处理一部分 kernel fault，但
具体调用路径必须允许释放锁、等待 handler 并重试，也就是进入 `handle_userfault()`
时必须带有 `FAULT_FLAG_ALLOW_RETRY`。GDB 当前的 remote GUP 路径不满足该条件。

因此，让 GDB 读取这段地址的可靠方法是提前用 `UFFDIO_COPY`、
`UFFDIO_ZEROPAGE` 等操作填充目标页，或者先取消该范围的 userfaultfd missing
注册；仅关闭 `UFFD_USER_MODE_ONLY` 不会改变 GDB 的读取结果。

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
