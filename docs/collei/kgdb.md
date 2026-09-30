# kgdb 支持

- https://docs.kernel.org/process/debugging/kgdb.html
- [通过 kgdb 调试 pixel 8 内核](https://xairy.io/articles/pixel-kgdb)
- [vscode + gdb + qemu 调试 arm64 linux内核](https://zhuanlan.zhihu.com/p/510289859)

## 基本操作
```ini
cmdline = console=hvc0 kgdboc=ttyS0,115200 kgdbwait rodata=off
```

```bash
echo ttyS1,115200 | sudo tee /sys/module/kgdboc/parameters/kgdboc
echo g | sudo tee /proc/sysrq-trigger
```

先启动 VM  ，然后在另一个终端执行：

```bash
./collei/scripts/collei-action.py -a kgdb -n kgdb
```

默认连接第一个 `serial socket` 通道， 也可以显式传入 `serial` 或 `hvc`。
使用 `kgdbwait` 时，guest 会在启动阶段等待 GDB；

## kgdb break point 是如何实现的?

> [!NOTE]
> 参考神奇海螺的意见，有待验证

2026-09-29 : 我感觉说的很清晰了，有时候可以简化一下细节。


KGDB 的普通断点通过修改内核指令实现：把目标位置改成断点指令，CPU 执行到这里触发异常，再进入 KGDB。串口只负责传输 GDB 命令和结果。

以你当前源码的 x86 实现为例。

1. 设置断点：把指令首字节替换为 0xCC

执行 break foo 后，GDB 根据符号信息确定地址，在需要安装断点时发送 Z0,地址,长度 请求。KGDB 的 gdb_cmd_break() 接收请求，并记录软件断点。GDB 协议
(https://sourceware.org/gdb/current/onlinedocs/gdb.html/Packets.html)

恢复执行前，KGDB 激活断点：

dbg_activate_sw_breakpoints()
  → kgdb_arch_set_breakpoint()
    → 保存原来的 1 字节
    → 写入 0xCC，即 INT3

例如：

原始代码：  48 89 e5 ...     mov %rsp, %rbp
安装断点：  cc 89 e5 ...     int3

只覆盖第一个字节，原字节保存在断点记录中。当前 x86 实现还会在普通写入失败时尝试 text_poke_kgdb()，以修改只读内核代码页。源码 (arch/x86/kernel/
kgdb.c:731)

2. 命中断点：CPU 的异常机制接管

执行 INT3 触发 #BP，即 3 号异常，通过 x86 异常处理路径进入 KGDB：

执行 INT3
  → #BP 异常
  → KGDB 的异常处理钩子
  → kgdb_handle_exception()
  → 停住其他 CPU
  → gdb_serial_stub() 等待 GDB 命令

正常情况下，其他 CPU 停在 KGDB 的等待循环里，当前 CPU 执行调试器代码。因此“内核停住”时，KGDB 仍能通过串口 polling 收发数据。调试核心 (kernel/debug/
debug_core.c:690)

3. 继续执行：恢复原指令，单步越过，再装回断点

这里不能直接在原地址放着 INT3 继续，否则会立即再次命中。

典型流程由 GDB 与 KGDB 配合完成：

恢复原始指令，并将执行地址修正到断点位置
  → 暂时移除该位置的断点
  → 单步执行原指令
  → 重新安装断点
  → 继续运行

x86 执行 INT3 后保存的 RIP 指向断点后一个字节，所以还需要处理 RIP 回退 1 字节；当前 x86 KGDB 声明由 GDB 调整断点偏移。

单步则使用 RFLAGS.TF（Trap Flag）：设置 TF 后，CPU 在执行一条指令后触发 #DB，即 1 号异常，重新进入调试器。单步实现 (arch/x86/kernel/kgdb.c:440)

还有一个源码细节：KGDB 进入调试循环时会撤下所有已激活的软件断点，退出前再激活仍保留的断点；GDB 负责协调当前断点的移除、单步和重装。

硬件断点是另一条路径：hbreak 使用 x86 的 DR0–DR3、DR7 等调试寄存器，由 CPU 匹配执行地址并触发 #DB，不修改代码，但槽位有限。ARM64 的软件断点原理类似，只
是使用 BRK 指令，而不是 x86 的 INT3。


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
