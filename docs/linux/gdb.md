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

### attach
gdb -p PID

### thread
- thread apply all bt
- info threads
- thread 2 : 切换的 threasd 2 中


### signal
- handle SIG127 nostop noprint pass : gdb 是否停止，是否打印信息，是否将信号传递给程序
- signal SIG127 : 给被调试的程序发送
- 可以通过 `$_siginfo` 变量读取一些额外的有关当前信号的信息

### frame

frame 4
info locals

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

### frame

分析 core 的时候，可以指定 frame 的位置，然后来分析:
```txt
(gdb) frame 4
(gdb) info local
```
参考：https://stackoverflow.com/questions/2770889/how-can-i-examine-the-stack-frame-with-gdb

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

#### 打印一个结构体
https://stackoverflow.com/questions/1768620/how-do-i-show-what-fields-a-struct-has-in-gdb

由于 gdb-dashboard ，直接使用这个就可以了:
```txt
# 如果就是函数的结构体
p *m->file->f_path.dentry

# 如果是地址
p *(struct dentry *)0xffff88800581d780
```

#### misc
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

- [qita](https://github.com/geohot/qira) : 大名鼎鼎的 George Hotz 写的基于 QEMU 的调试器
- [rr](https://github.com/rr-debugger/rr) : 真的可以用的，虽然存在一些限制
  - https://rr-project.org/
  - [相关论文](https://www.usenix.org/system/files/conference/atc17/atc17-o_callahan.pdf)

### 内存泄露调试技术
- valgrind
- [heaptrack](https://github.com/KDE/heaptrack) : a heap memory profiler for Linux
- bytehound

### 代码覆盖率工具
- [syzkaller](https://github.com/google/syzkaller)

### 静态检查工具

### misc
- [DynamoRIO](https://github.com/DynamoRIO/dynamorio) : 通过动态插桩来调试
- [mull](https://github.com/mull-project/mull) is a practical mutation testing tool for C and C++.
- https://clang.llvm.org/docs/AddressSanitizer.html

## 关键参考
- [100 个 gdb 技巧](https://github.com/hellogcc/100-gdb-tips)
- [gdb checksheet](https://darkdust.net/files/GDB%20Cheat%20Sheet.pdf)

## TODO
- [ ] https://stackoverflow.com/questions/43701428/gdb-proc-info-mapping-vs-proc-pid-mapping
- [ ] 调查一下 https://github.com/smcdef/kprobe-template 难道 ebpf 不能实现这个功能 ?
- [ ] https://www.cs.cmu.edu/~gilpin/tutorial/
- https://news.ycombinator.com/item?id=33166188
  - http://luajit.io/posts/gdb-black-magics/
- https://github.com/pwndbg/pwndbg

## [ ] 介绍下 coredump 的生成原理

corefile 在 /var/lib/systemd/coredump/ 中:

```txt
gdb ~/data/qemu/build/qemu-system-x86_64 core.qemu-system-x86.1000.7aa8ee09ca4945878399165f6bde1405.492790.1742904831000000
```

```txt
$ bt
#0  0x00007f5b41f55661 in ?? ()
#1  0x0000000000000000 in ?? ()
```
显然是有 debuginfo 的，但是都是问号，如何理解?

## [ ] 配合 coredump 使用
- https://linux-audit.com/understand-and-configure-core-dumps-work-on-linux/

如果 data type 是一个结构体，而且这个结构体很大，那怎么办 ?


## [ ] 看看这个
https://news.ycombinator.com/item?id=39170901

## Fedora 上调试自动下载 debuginfo 包，真的 nb
```txt
This GDB supports auto-downloading debuginfo from the following URLs:
  <https://debuginfod.fedoraproject.org/>
Enable debuginfod for this session? (y or [n]) y
Debuginfod has been enabled.
To make this setting permanent, add 'set debuginfod enabled on' to .gdbinit.
Downloading separate debug info for /lib64/libz.so.1
Downloading separate debug info for /lib64/libgbm.so.1
Downloading separate debug info for system-supplied DSO at 0xfffecc4bc000
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib64/libthread_db.so.1".
0x0000fffeca8c6604 in __GI_ppoll (fds=0xaaab6a61c720, nfds=<optimized out>, timeout=<optimized out>, timeout@entry=0xffffdff22960,
    sigmask=sigmask@entry=0x0) at ../sysdeps/unix/sysv/linux/ppoll.c:42
42        return SYSCALL_CANCEL (ppoll_time64, fds, nfds, timeout, sigmask,
─── Source ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
 37  #ifndef __NR_ppoll_time64
 38  # define __NR_ppoll_time64 __NR_ppoll
 39  #endif
 40
 41  #ifdef __ASSUME_TIME64_SYSCALLS
 42    return SYSCALL_CANCEL (ppoll_time64, fds, nfds, timeout, sigmask,
 43               __NSIG_BYTES);
 44  #else
 45    int ret;
 46    bool need_time64 = timeout != NULL && !in_int32_t_range (timeout->tv_sec);
──────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Missing separate debuginfos, use: dnf debuginfo-install mesa-libgbm-24.1.0~asahipre20240228-2.fc40.aarch64
```
但是问题是，他每次要下载

## [ ] 有用吗？
https://beej.us/guide/bggdb/

## [ ] 有趣的
https://poormansprofiler.org/

https://news.ycombinator.com/item?id=30512302

## gcore 的使用
<!-- e1971471-2d0a-4763-b955-a9a13fd0b294 -->

kvmclock_vm_state_change 中添加 abort

ls -la /var/lib/systemd/coredump

cp /var/lib/systemd/coredump/core.qemu-system-x86.1000.fa6ca1ad83a64170b4aa5ae69f86a4ed.35104.1744643421000000.zst /tmp

堆栈可以看的很清楚:
```txt
#0  0x00007fcf8fc0188c in __pthread_kill_implementation () from /nix/store/maxa3xhmxggrc5v2vc0c3pjb79hjlkp9-glibc-2.40-66/lib/libc.so.6
#1  0x00007fcf8fbaf576 in raise () from /nix/store/maxa3xhmxggrc5v2vc0c3pjb79hjlkp9-glibc-2.40-66/lib/libc.so.6
#2  0x00007fcf8fb97935 in abort () from /nix/store/maxa3xhmxggrc5v2vc0c3pjb79hjlkp9-glibc-2.40-66/lib/libc.so.6
#3  0x000055ba45f586da in kvmclock_vm_state_change (opaque=<optimized out>, running=<optimized out>, state=<optimized out>)
    at ../hw/i386/kvm/clock.c:171
#4  0x000055ba45e4db54 in vm_state_notify (running=running@entry=true, state=state@entry=RUN_STATE_RUNNING)
    at ../system/runstate.c:396
#5  0x000055ba45e44537 in vm_prepare_start (step_pending=step_pending@entry=false) at ../system/cpus.c:776
#6  0x000055ba45e4458b in vm_start () at ../system/cpus.c:783
#7  0x000055ba45e9f831 in qmp_cont (errp=0x0) at ../monitor/qmp-cmds.c:112
#8  0x000055ba45e55f35 in qemu_init (argc=<optimized out>, argv=<optimized out>) at ../system/vl.c:3843
#9  0x000055ba45bc2449 in main (argc=<optimized out>, argv=<optimized out>) at ../system/main.c:68
```

## 看看这个 help 工具

```txt
List of classes of commands:

aliases -- User-defined aliases of other commands.
breakpoints -- Making program stop at certain points.
data -- Examining data.
files -- Specifying and examining files.
internals -- Maintenance commands.
obscure -- Obscure features.
running -- Running the program.
stack -- Examining the stack.
status -- Status inquiries.
support -- Support facilities.
text-user-interface -- TUI is the GDB text based interface.
tracepoints -- Tracing of program execution without stopping the program.
user-defined -- User-defined commands.
```

## 看汇编的小技巧

layout asm

然后使用 si 逐条指令执行

## 当你的 vmcore 无法正常使用的时候在，也许，很多时候，是由于 debugsyml 无法自动找到导致的
手动执行这个东西:
symbol-file  /usr/lib/debug/usr/libexec/qemu-kvm-10.2.x86_64.debug

## 好东西
https://hugsy.github.io/gef/settings/print-format/

## 原来gdb 执行的命令可以直接放到文件中
gdb -x src_commands.txt --args

## 这个人写了很多 lldb 相关的 blog
https://www.moritz.systems/blog/full-multiprocess-support-in-lldb-server/


## coredump

- 存储在 /var/lib/systemd/coredump
- 解压方法: zstd -d core.qemu.zst
- 分析方法: gdb path/to/the/binary path/to/the/core/dump/file

nixos 的处理方式:

```txt
🧀  cat /proc/sys/kernel/core_pattern
|/nix/store/34am2kh69ll6q03731imxf21jdbizda2-systemd-251.15/lib/systemd/systemd-coredump %P %u %g %s %t %c %h
```

ubuntu 的处理方式:

```txt
var/lib/systemd/coredump$  cat /proc/sys/kernel/core_pattern
|/usr/share/apport/apport -p%p -s%s -c%c -d%d -P%P -u%u -g%g -- %E
```

通过检查 /var/log/apport.log 可以知道

```txt
ERROR: apport (pid 17768) Thu Apr 27 03:08:58 2023: called for pid 17767, signal 11, core limit 0, dump mode 1
ERROR: apport (pid 17768) Thu Apr 27 03:08:58 2023: executable: /a.out (command line "./a.out")
ERROR: apport (pid 17768) Thu Apr 27 03:08:58 2023: executable does not belong to a package, ignoring
```

所以需要调整一下:

```sh
ulimit -c unlimited
```

其路径也是在 /var/lib/apport/coredump 中。

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
