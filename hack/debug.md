# debug 内核

1. kprobes
2. debugfs
3. tracefs

wowotech 

linux kernle labs 的问题:
1. qemu 配合 gdb 调试的原理
2. gdb
```c
    1. disassemble do_dentry_open  
    2. list *(do_dentry_open +0x340)
```

3. addr2line 和 objdump 都是 gdb 功能的子集 ?


- [ ] read this : https://www.kernel.org/doc/html/latest/dev-tools/gdb-kernel-debugging.html
