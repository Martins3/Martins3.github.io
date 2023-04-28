# 到底存在那些线程

## 分析 `nr_threads`
kernel/fork.c
```c
int nr_threads;			/* The idle threads do not count.. */
```

- copy_process 的增加

* wait_task_zombie
* find_child_reaper
* exit_notify
* de_thread
- release_task
  - `__exit_signal`
    - `__unhash_process` --

man proc 中可以看到:

> The value after the slash is the number of kernel scheduling entities that currently exist on the system.

## [ ] /proc/$pid1 是怎么创建的？
`proc_root_init` 来创建所有的根目录。

使用 for_each_process_thread 统计了一波，确实没毛病啊

proc_pid_lookup

```txt
#0  proc_pid_lookup (dentry=dentry@entry=0xffff88810577f680, flags=flags@entry=17) at fs/proc/base.c:3436
#1  0xffffffff814c44b1 in proc_root_lookup (dir=0xffff88810caa0048, dentry=0xffff88810577f680, flags=17) at fs/proc/root.c:324
#2  0xffffffff81439e81 in __lookup_slow (name=name@entry=0xffffc90001df3dd0, dir=dir@entry=0xffff88810c9315c0, flags=flags@entry=17) at fs/namei.c:1686
#3  0xffffffff8143e165 in lookup_slow (flags=17, dir=0xffff88810c9315c0, name=0xffffc90001df3dd0) at fs/namei.c:1703
#4  walk_component (nd=0xffffc90001df3dc0, flags=0) at fs/namei.c:1994
#5  0xffffffff8143e3da in link_path_walk (name=0xffff88810b8d002b "oom_score_adj", name@entry=0xffff88810b8d0020 "/proc/self/oom_score_adj", nd=nd@entry=0xffffc90001df3dc0) at
 fs/namei.c:2318
#6  0xffffffff8143ec13 in link_path_walk (nd=0xffffc90001df3dc0, name=0xffff88810b8d0020 "/proc/self/oom_score_adj") at ./include/linux/err.h:36
#7  path_openat (nd=nd@entry=0xffffc90001df3dc0, op=op@entry=0xffffc90001df3edc, flags=flags@entry=65) at fs/namei.c:3711
#8  0xffffffff81440d06 in do_filp_open (dfd=dfd@entry=-100, pathname=pathname@entry=0xffff88810b8d0000, op=op@entry=0xffffc90001df3edc) at fs/namei.c:3742
#9  0xffffffff81427aea in do_sys_openat2 (dfd=-100, filename=<optimized out>, how=how@entry=0xffffc90001df3f18) at fs/open.c:1356
#10 0xffffffff81427fe7 in do_sys_open (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1372
#11 __do_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1388
#12 __se_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1383
#13 __x64_sys_openat (regs=<optimized out>) at fs/open.c:1383
#14 0xffffffff822a0f0c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90001df3f58) at arch/x86/entry/common.c:50
#15 do_syscall_64 (regs=0xffffc90001df3f58, nr=<optimized out>) at arch/x86/entry/common.c:80
#16 0xffffffff824000ae in entry_SYSCALL_64 () at arch/x86/entry/entry_64.S:120
#17 0x0000000000000000 in ?? ()
```


`proc_pid_make_inode` 居然是查询的时候动态创建的，

```txt
#0  proc_pid_make_inode (sb=0xffff888103c28000, task=task@entry=0xffff8881109b8000, mode=33188) at fs/proc/base.c:1890
#1  0xffffffff814c8f15 in proc_pident_instantiate (dentry=dentry@entry=0xffff88811da25680, task=task@entry=0xffff8881109b8000, ptr=ptr@entry=0xffffffff8264c0e8 <tgid_base_stuff+520>) at fs/proc/base.c:2643
#2  0xffffffff814c908e in proc_pident_lookup (end=0xffffffff8264c6b0, p=0xffffffff8264c0e8 <tgid_base_stuff+520>, dentry=0xffff88811da25680, dir=<optimized out>) at fs/proc/base.c:2679
#3  proc_tgid_base_lookup (dir=<optimized out>, dentry=0xffff88811da25680, flags=<optimized out>) at fs/proc/base.c:3378
#4  0xffffffff8143f15a in lookup_open (op=0xffffc90000017edc, op=0xffffc90000017edc, got_write=false, file=0xffff888106eceb00, nd=0xffffc90000017dc0) at fs/namei.c:3394
#5  open_last_lookups (op=0xffffc90000017edc, file=0xffff888106eceb00, nd=0xffffc90000017dc0) at fs/namei.c:3484
#6  path_openat (nd=nd@entry=0xffffc90000017dc0, op=op@entry=0xffffc90000017edc, flags=flags@entry=65) at fs/namei.c:3712
#7  0xffffffff81440d06 in do_filp_open (dfd=dfd@entry=-100, pathname=pathname@entry=0xffff888110a72000, op=op@entry=0xffffc90000017edc) at fs/namei.c:3742
#8  0xffffffff81427aea in do_sys_openat2 (dfd=-100, filename=<optimized out>, how=how@entry=0xffffc90000017f18) at fs/open.c:1356
#9  0xffffffff81427fe7 in do_sys_open (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1372
#10 __do_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1388
#11 __se_sys_openat (mode=<optimized out>, flags=<optimized out>, filename=<optimized out>, dfd=<optimized out>) at fs/open.c:1383
#12 __x64_sys_openat (regs=<optimized out>) at fs/open.c:1383
#13 0xffffffff822a0f0c in do_syscall_x64 (nr=<optimized out>, regs=0xffffc90000017f58) at arch/x86/entry/common.c:50
#14 do_syscall_64 (regs=0xffffc90000017f58, nr=<optimized out>) at arch/x86/entry/common.c:80
```

才意识到，ls /proc 下只是显示 process 的，但是你可以 cd /proc/$thread 中

看来半天，还是无法理解，无所谓了吧
