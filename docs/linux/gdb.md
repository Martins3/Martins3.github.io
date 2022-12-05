# Debugger 的理念，原理和使用

## 理念

关于 debugger 的理念，我的想法和这篇 blog 完全相同:

> [I do not use a debugger](https://lemire.me/blog/2016/06/21/i-do-not-use-a-debugger/)

如果出现 bug ，你应该想到的是:
- 重构代码，让代码更佳清晰，bug 不容易发生
- 增加更多的测试，让 bug 更加容易显形


而不是使用 debugger 让自己陷入到细节中。

当然也有例外，如果你刚刚接手陌生的一个大项目，debugger 是分析代码流程的好工具。

## [ ] gdb 的原理

## gdb 的使用
首先强烈推荐 [gdb-dashboard](https://github.com/cyrus-and/gdb-dashboard)

### Basic
- break
- continue
- backtrace
- `next` line
- `step` in
- gdb -ex "handle SIG127 nostop noprint" -ex "run" --args ./a.out 1 2 3
  - 启动 gdb 之后首先执行 `handle SIG127 nostop noprint` 和 `run`，然后调试 a.out ，而且 a.out 的参数为 1 2 3

### breakpoints
> - To list current breakpoints: "info break"
> - To delete a breakpoint: "del [breakpointnumber]"
> - To temporarily disable a breakpoint: "dis [breakpointnumber]"
> - To enable a breakpoint: "en [breakpointnumber]"
> - To ignore a breakpoint until it has been crossed x times:"ignore [breakpointnumber] [x]"
> - To save breakpoints: save breakpoints [filename]
> - To set a temporary breakpoint: tbreak
> cited from [^1]

### watch
- watch variable
- `watch *(data type*)address` : 监控一个地址上的变化[^1]
- watch expr thread threadnum : 只是监控特定线程的
- rwatch / awatch : 设置只读 watchpoint 和读写 watchpoint

### catch
- catch syscall : 可以用于替代 strace 分析

### backtrace
- bt full : 在 backtrace 的基础上显示每一个 frame 的信息

### print
- p array[index]@num : 打印数组 array 从 index 开始的 num 个成员
- 打印内存
  - x/nfs addr : nfs=(number format size) 例如 x/2xb 0x401234
  - f : 和 printf 相同
  - s : b (byte，1 个字节) h (half，两个字节) w (word，四个字节) g (gaint，四个字节)

例如打印内核中的变量: jiffies
```txt
>>> x/xw &jiffies
0xffffffff82807980 <jiffies_64>:        0xfffb6c20
>>> x/xg &jiffies
0xffffffff82807980 <jiffies_64>:        0x00000000fffb6c20
```

### thread
- thread apply all bt

### signal
- handle SIG127 nostop noprint pass : gdb 是否停止，是否打印信息，是否将信号传递给程序
- signal SIG127 : 给被调试的程序发送
- 可以通过 `$_siginfo` 变量读取一些额外的有关当前信号的信息

### info
- info functions
- info proc mapppins
- info frame
- info files
- info signal
- info sharedlibrary
- info threads
- info args
- info variables
- info locals

### [ ] gdb script

### 高级
- 条件 break
- continue 多次

给定一个虚拟地址，获取其所在源代码地址，一种方法是通过 add2line

```plain
➜  test git:(fix-kern) ✗ addr2line -e posted_ipi 0x406aec
/home/maritns3/core/dune/libdune/apic.c:53
```
但是 gdb 也可以，使用 disassemble 0x406aec 来查看
   - 直接反汇编一个函数 : disassemble `do_dentry_open`
   - disassemble /m 0x406aec 可以显示源码和汇编
   - disassemble /r 0x406aec 可以显示 hex 和汇编

### 小技巧
- gdb 退出时不显示提示信息
  - set confirm off
- 退出正在调试的函数
  - finish
  - return
- 直接执行函数
  - call
- 检查变量
  - whatis var
  - i variables var
  - ptype var
- 将汇编格式改为 intel 格式 set disassembly-flavor intel
- 如果要把断点设置在汇编指令层次函数的开头，要使用如下命令：`b *func`
- [清屏](https://stackoverflow.com/questions/12938067/how-clear-gdb-command-screen) : Ctrl-l

### gdb 封装工具
2. https://www.gdbgui.com/
3. https://github.com/rohanrhu/gdb-frontend

## 其他工具
这里列举了 Linux 平台上的各种 debugger，https://scattered-thoughts.net/writing/the-state-of-linux-debuggers/

### record debugger
记录下一个程序所有的状态，从而可以切入到任何一个时间点来调试

- [qita](https://github.com/geohot/qira)
- [rr](https://github.com/rr-debugger/rr)

### 内存泄露调试技术
- valgrind
- [heaptrack](https://github.com/KDE/heaptrack) : a heap memory profiler for Linux

### 代码覆盖率工具
- [syzkaller](https://github.com/google/syzkaller)

### 静态检查工具

### misc
- [DynamoRIO](https://github.com/DynamoRIO/dynamorio) : 通过动态插桩来调试
- [mull](https://github.com/mull-project/mull) is a practical mutation testing tool for C and C++.
- https://github.com/osandov/drgn : Facebook 写的侧重于编程的 debuggers
- https://clang.llvm.org/docs/AddressSanitizer.html

## 关键参考
- [100 个 gdb 技巧](https://github.com/hellogcc/100-gdb-tips)
- [gdb checksheet](https://darkdust.net/files/GDB%20Cheat%20Sheet.pdf)

## 调试 coroutine
原理参考：https://mp.weixin.qq.com/s/R0Ja-0HXdZyNSkpM2Y6Gsg

在 QEMU 中存在对应的脚本，在 gdb 的 cmdline 中使用 help qemu 可以有:
```txt
qemu bt -- Display backtrace including coroutine switches
qemu coroutine -- Display coroutine backtrace
qemu handlers -- Display aio handlers
qemu mtree -- Display the memory tree hierarchy
qemu tcg-lock-status -- Display TCG Execution Status
qemu timers -- Display the current QEMU timers
```

## TODO
- [ ] https://stackoverflow.com/questions/43701428/gdb-proc-info-mapping-vs-proc-pid-mapping
- [ ] 调查一下 https://github.com/smcdef/kprobe-template 难道 ebpf 不能实现这个功能 ?
- [ ] kgdb 到底是什么

[^1]: 如果 data type 是一个结构体，而且这个结构体很大，那怎么办 ?
