## 用户态符号基础

两个差不多
```txt
objdump -t /home/martins3/data/qemu/build/qemu-system-x86_64 | grep kvm
nm -a /home/martins3/data/qemu/build/qemu-system-x86_64 | grep kvm
```

```txt
-t
--syms
Print the symbol table entries of the file. This is similar to the information provided by the nm program, although the display format is different. The format of the output depends upon the format of the file being dumped, but there are two main types. One looks like this:
[  4](sec  3)(fl 0x00)(ty   0)(scl   3) (nx 1) 0x00000000 .bss
[  6](sec  1)(fl 0x00)(ty   0)(scl   2) (nx 0) 0x00000000 fred
where the number inside the square brackets is the number of the entry in the symbol table, the sec number is the section number, the fl value are the symbol's flag bits, the ty number is the symbol's type, the scl number is the symbol's storage class and the nx value is the number of auxilary entries associated with the symbol. The last two fields are the symbol's value and its name.
The other common output format, usually seen with ELF based files, looks like this:

00000000 l    d  .bss   00000000 .bss
00000000 g       .text  00000000 fred
Here the first number is the symbol's value (sometimes refered to as its address). The next field is actually a set of characters and spaces indicating the flag bits that are set on the symbol. These characters are described below. Next is the section with which the symbol is associated or *ABS* if the section is absolute (ie not connected with any section), or *UND* if the section is referenced in the file being dumped, but not defined there.
After the section name comes another field, a number, which for common symbols is the alignment and for other symbol is the size. Finally the symbol's name is displayed.

The flag characters are divided into 7 groups as follows:

"l"
"g"

"u"

"!"

The symbol is a local (l), global (g), unique global (u), neither global nor local (a space) or both global and local (!). A symbol can be neither local or global for a variety of reasons, e.g., because it is used for debugging, but it is probably an indication of a bug if it is ever both local and global. Unique global symbols are a GNU extension to the standard set of ELF symbol bindings. For such a symbol the dynamic linker will make sure that in the entire process there is just one symbol with this name and type in use.

"w"

The symbol is weak (w) or strong (a space).

"C"

The symbol denotes a constructor (C) or an ordinary symbol (a space).

"W"

The symbol is a warning (W) or a normal symbol (a space). A warning symbol's name is a message to be displayed if the symbol following the warning symbol is ever referenced.

"I"

"i"

The symbol is an indirect reference to another symbol (I), a function to be evaluated during reloc processing (i) or a normal symbol (a space).

"d"

"D"

The symbol is a debugging symbol (d) or a dynamic symbol (D) or a normal symbol (a space).

"F"

"f"

"O"

The symbol is the name of a function (F) or a file (f) or an object (O) or just a normal symbol (a space).
```

居然没有 stack ，太惨了:
```txt
sudo bpftrace -e 'uprobe:/home/martins3/data/qemu/build/qemu-system-x86_64:kvm_slot_get_dirty_log.isra.0 { @[ustack] = count(); }'
Attaching 1 probe...
^C

@[
    0x556567265000
]: 36
```


## 使用 perf 来跟踪程序


## 从这个 ticket 来看，必须支持 framepointer 才可以发
- https://github.com/bpftrace/bpftrace/issues/1744
  - https://dxuuu.xyz/stack-symbolize.html : 不容易啊

## ftrace user
[Documentation/trace/user_events.rst](https://www.kernel.org/doc/html/latest/trace/user_events.html)


/sys/kernel/debug/tracing/user_events_data
/sys/kernel/debug/tracing/user_events_status


这个目录中:
kernel/trace/trace_events_user.c

看看这个 sample 可以用否

## [Using user-space tracepoints with BPF](https://lwn.net/Articles/753601/)


## 有趣的
https://www.usenix.org/system/files/srecon23apac_slides-liang_zhichuan.pdf

## qemu 自己的 trace 机制整理
主要是其中的 dtrace 了

## 用 perf 来 trace 用户态程序看这里
/home/martins3/data/vn/code/src/c/elf/README.md

可以自动聚合 backtrace ，而 perf 不可以 ，如果有工具可以同时解决这两个问题就好了。

## USDT systemtab

## lttng

[lttng](https://lttng.org/docs/)

需要使用这个来分析 : https://babeltrace.org/

lttng-ust/doc/examples/demo 这个就是最好的

操作完成的之后，执行这个:
```sh
babeltrace2 ~/lttng-traces/auto-20250604-165445
```

通过反汇编发现，lttng 没有办法实现内核中的完全无开销的
还是我的理解不够?

## 即便是添加了 -fno-omit-frame-pointer

```txt
--extra-cflags=\"-fno-omit-frame-pointer\"
```
bakctrace 还是在使用函数指针的时候有问题，例如
```txt
@[
    qdev_get_parent_bus+0
    memory_region_write_accessor+130
    access_with_adjusted_size+336
    memory_region_dispatch_write+273
    flatview_write_continue_step.isra.0+370
    flatview_write+191
    address_space_rw+281
    kvm_cpu_exec+1172
    kvm_vcpu_thread_fn+157
    qemu_thread_start+161
    start_thread+915
]: 5442
```

memory_region_write_accessor+130 对应的地方为:

```c
    mr->ops->write(mr->opaque, addr, tmp, size);
```

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
