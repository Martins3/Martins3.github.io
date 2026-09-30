# Windows 内存管理
<!-- d983f046-516a-4b0c-be79-ce2b6869ba30 -->

Windows 的内存管理有不少 Linux 里没有的概念，比如“已提交（commit）”、分页/非分页池、
压缩内存，以及下面这个在虚拟化里非常烦人的行为。

- 任务管理器截图：![](./img/windows-memory.png)

> [!NOTE]
> 参考神奇海螺的意见，有待验证

## 为什么 Windows 开机后会“踩”满所有物理内存

### 现象

Windows guest 跑在 QEMU/KVM 上时，host 侧 qemu 进程的 RSS 会在开机后很快涨到 guest
配置的全部内存。哪怕 guest 内部可用内存还很充裕。

同样的现象在别人的环境里也被反复确认过：

- <https://serverfault.com/questions/1110430/qemu-process-allocates-all-of-memory-to-the-windows-vm>
- <https://learn.microsoft.com/en-us/answers/questions/1463079/how-to-disable-zero-page-thread>

### 直接原因：zero page thread

Windows 的 memory manager 把物理页按状态挂在不同的 page list 上，`free page list` 里的页
表示“可以拿来用，但内容不保证是 0”。而 Windows 承诺：进程通过 `VirtualAlloc` 拿到的新页
内容一定是 0（出于安全，不能把上一个进程的残留数据泄露给下一个进程）。
为了不在分配路径上同步清零，Windows 用了一个后台系统线程 **zero page thread**：
把 free page list 上的页清零后挂到 `zeroed page list`，分配时直接从 zeroed list 取。

Microsoft Learn 的 [Scheduling Priorities](https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities)
原话是：

> The zero-page thread is a system thread responsible for zeroing any free pages when there
> are no other threads that need to run.

也就是说它的优先级是 0，只有在没有任何其它线程需要运行时才会跑。开机阶段 guest 里几乎
全部物理内存都在 free list 上（内核和已加载模块只占一小部分），于是只要 CPU 一空闲，
这个线程就会把 free list 抽干，逐页写 0。在虚拟机里，每一页写 0 都会让 host 上对应的
匿名页发生第一次写 fault，RSS 随之上涨，直到接近 guest 配置的全部 RAM。

### 机制（对照 ReactOS ARM3 实现）

Windows 内部符号不公开，这里用 ReactOS 的同名实现来说明流程（命名与 Windows 内部一致）：

- 线程入口 `MmZeroPageThread()`，位于 `ntoskrnl/mm/ARM3/zeropage.c`。 线程起来第一件事是把 `Thread->BasePriority` 设成 0，即最低优先级。
- 它在 `MmZeroingPageEvent` 上等待。
- 任何页被归还到 free list 时，`MiInsertPageInFreeList()`（`ntoskrnl/mm/ARM3/pfnlist.c`） 会在 free list 页数达到阈值时 `KeSetEvent(&MmZeroingPageEvent, ...)` 唤醒它。
- 醒来后，它在 PFN 锁下从 `MmFreePageListHead` 取一批页（一次 `MI_ZERO_PTES` 个），
  用 `MiMapPagesInZeroSpace()` 映射到一段专门的“zero space”虚拟地址， 再用 `KeZeroPages()` 整块清零，最后挂到 `MmZeroedPageListHead`。
- free list 空了就 `KeClearEvent()`，继续回去等。

所以 free list 上的页在被回收进 zeroed list 之前，一定会被物理写一遍。CPU 越空闲，
清得越快，这正好解释了“开机后很快踩满”。

### 这不是 bug

- 安全：物理页在进程之间复用时不能被读到上一个所有者的数据。
- 语义：`VirtualAlloc` 保证返回零页，后台提前清零比在分配时同步清零延迟更低。
- 五种 page list（active / modified / standby / free / zeroed）是 Windows Internals 里
  PFN database 的标准划分，zeroed list 就是为这个承诺服务的。

### 和 Linux 的对比

Linux 没有后台线程去逐页清零：

- 读缺页时统一映射全局 `zero page`（`mm/memory.c` 的 `do_anonymous_page()`，
  `ZERO_PAGE()` / `empty_zero_page`），根本不分配物理页；
- 只有在写缺页时才分配物理页，并且分配路径上的清零是写时才发生（或直接用
  `__GFP_ZERO` 的页）。

因此 Linux guest 不会在开机后主动 touch 全部 RAM，这是 Windows guest 特有的开销。

### 如何观察

Windows 侧：

- RAMMap / System Informer 看 page list 分类，Zeroed 那一项会迅速吃掉 free 内存。
- 性能计数器 `\Memory\Free & Zero Page List Bytes`：

```powershell
Get-Counter '\Memory\Free & Zero Page List Bytes'
```

- WinDbg 内核调试：`!vm` 看 free / zeroed page list 的大小；用 `!stacks` 或 `!thread`
  可以找到 `MmZeroPageThread` 这个系统线程。

Host 侧：

```bash
# qemu 进程 RSS
ps -o pid,rss,cmd -p "$(pgrep -f 'qemu-system.*')"

# 更精确的 guest 统计
virsh dommemstat <domain>

# 直接看 RSS
awk '/VmRSS/ {print}' /proc/<qemu-pid>/status
```

### 缓解方法

1. **在 guest 里装 virtio-balloon 驱动（virtio-win）**，这是最直接的办法。balloon 驱动把
   guest 认为空闲的页交给 host，这些页会离开 guest 的 free list，zero page thread 不会再
   碰它们，host 就能回收内存。
2. **Hyper-V Dynamic Memory**：本质也是 balloon，Microsoft 官方的方案。
3. `VIRTIO_BALLOON_F_FREE_PAGE_HINT` / free page reporting：guest 主动把空闲页报告给 host。
   注意它和 zero page thread 有交互，被报告后 guest 不应再写这些页。
4. 不要试图禁掉 zero page thread：Microsoft 明确不建议，也没有官方开关，
   强行禁用会影响安全与稳定性。
5. 给 guest 配小内存并开启动态内存，而不是一次性把全部内存分给 guest。

## 参考

- <https://learn.microsoft.com/en-us/windows/win32/procthread/scheduling-priorities>
- <https://learn.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc>
- <https://learn.microsoft.com/en-us/answers/questions/1463079/how-to-disable-zero-page-thread>
- <https://serverfault.com/questions/1110430/qemu-process-allocates-all-of-memory-to-the-windows-vm>
- Windows Internals 7th edition, Chapter 5: Memory Management
- ReactOS ARM3: `ntoskrnl/mm/ARM3/zeropage.c`、`ntoskrnl/mm/ARM3/pfnlist.c`

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
