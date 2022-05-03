# Why GDB

## 常用命令
1. backtrace
2. next line
3. step in
4. info 功能
  - proc

## 高级
- 条件 break
- continue 多次

## gdb 脚本

## 文档
[Gdb qs](http://web.eecs.umich.edu/~sugih/pointers/gdbQS.html)
[Dealing C gdb](https://ccrma.stanford.edu/~jos/stkintro/Dealing_C_gdb.html)
[100 个 gdb 技巧](https://github.com/hellogcc/100-gdb-tips)
[gdb function tables](https://objectkuan.gitbooks.io/ucore-docs/lab0/lab0_2_3_3_gdb.html)
[gdb check list](https://lldb.llvm.org/use/map.html)

## StackOverflow 回答
https://stackoverflow.com/questions/12938067/how-clear-gdb-command-screen

## coredump
1. https://stackoverflow.com/questions/2065912/core-dumped-but-core-file-is-not-in-the-current-directory


## TODO
- https://www.bilibili.com/video/BV1kP4y1K7Eo

调试内核模块
```sh
cat /proc/modules
objdump -dS --adjust-vma=0xffffffff85037434 vmlinux
```

## how to use kgdb
