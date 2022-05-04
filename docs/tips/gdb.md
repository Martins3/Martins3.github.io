## gdb 的原理和使用

## 常用命令
1. backtrace
2. next line
3. step in
4. info 功能
  - proc

## 高级
- 条件 break
- continue 多次


给定一个虚拟地址，获取其所在源代码地址，gdb 比 addr2
1. add2line

```plain
➜  test git:(fix-kern) ✗ addr2line -e posted_ipi 0x406aec
/home/maritns3/core/dune/libdune/apic.c:53
```
2. gdb 中使用 disassemble 0x401234 来查看(似乎前提是静态链接)
   - 直接反汇编一个函数 : disassemble `do_dentry_open`
   - disassemble /m 0x401234 可以显示源码和汇编的代码

## gdb 脚本

## 文档
[Gdb qs](http://web.eecs.umich.edu/~sugih/pointers/gdbQS.html)
[Dealing C gdb](https://ccrma.stanford.edu/~jos/stkintro/Dealing_C_gdb.html)
[100 个 gdb 技巧](https://github.com/hellogcc/100-gdb-tips)
[gdb function tables](https://objectkuan.gitbooks.io/ucore-docs/lab0/lab0_2_3_3_gdb.html)
[gdb check list](https://lldb.llvm.org/use/map.html)

## 小技巧
- [gdb 中如何清屏](https://stackoverflow.com/questions/12938067/how-clear-gdb-command-screen) : Ctrl-l

## coredump
1. https://stackoverflow.com/questions/2065912/core-dumped-but-core-file-is-not-in-the-current-directory

## TODO
- https://www.bilibili.com/video/BV1kP4y1K7Eo
- https://felix-knorr.net/posts/2022-02-27-direct-gdb.html
- https://news.ycombinator.com/item?id=31102872 : 更加快的 gdb 启动 ???

- https://github.com/rohanrhu/gdb-frontend : 以前收集过的吗 ?
  - [ ] 将 vim 中的内容整合到这里

## how to use kgdb

## vim 中调试技术顺便学习一下

## [ ] 调查一下 https://github.com/smcdef/kprobe-template

难道 ebpf 不能实现这个功能 ?

## debug
- https://github.com/rr-debugger/rr : rr is a lightweight tool for recording, replaying and debugging execution of applications (trees of processes and threads).
    - record 调试技术
- [DynamoRIO](https://github.com/DynamoRIO/dynamorio) : runtime code manipulation system  : 这不就是 dune 需要的内容吗?
- [heaptrack](https://github.com/KDE/heaptrack) : a heap memory profiler for Linux
- [mull](https://github.com/mull-project/mull) is a practical mutation testing tool for C and C++.

## 内存泄露调试技术
- valgrind

## 代码覆盖率工具

## 静态检查工具
