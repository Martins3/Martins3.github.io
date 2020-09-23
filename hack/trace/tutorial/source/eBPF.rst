====
eBPF
====

原理
----
eBPF是extended Berkely Package Filter，其意义已经远远不再是作为网络包过滤，而是可以将内核变为一个沙盒(sandbox)。

使用eBPF的理由有很多，但是:
3. 为了向内核中间添加功能，如果修改kernel source code，需要等到用户更新内核。如果使用kernel module，每次内核升级，都需要发布对应的kernel module.
1. eBPF 是 100% modular and composable 的
2. eBPF 可以实现 hotpatching 

1. 确定数据来源
2. 从内核中间导出数据
3. 对于数据进行过滤
4. 利用 kprobe 和 uporbe 可以可以动态进行修改。

https://css.csail.mit.edu/jitk/ : BPF 的文章

bcc
---

bpftrace
--------

.. todo::
  1.  bpftrace -e 'BEGIN { printf("Hello, World!\n"); }' BEGIN 是什么意思，是否存在类似的工具
  2.  bpftrace -e 'tracepoint:syscalls:sys_enter_nanosleep { printf("%s is sleeping.\n", comm); }'
        1. 参数 comm 是什么指定的 ?
        2. 能不能直接 sys_enter_nanosleep 不要前面的前缀
        3. sudo bpftrace -e 'tracepoint:syscalls:sys_enter_nanosleep { printf("%s is sleeping ==> %d.\n", comm, __syscall_nr); }' 居然不知道参数 __syscall_nr，但是
