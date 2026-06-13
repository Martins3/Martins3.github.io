观察代码:

https://github.com/iovisor/bcc
- introspection : 提供 bps 这个工具
- docs : 一共五个文档
- [ ] examples/networking/tcp_mon_block
- [ ] src/cc 似乎看不懂了

|----------------------------------|----------------------------------------------------------------------------------|
| kernel-versions.md               | 列出了 bpf 的各种特性 kernel 的支持版本， 可以看到 5.10 的时候，大部分功能都支持 |
| kernel_config.md                 |
| reference_guide.md               | 分为 c 和 python 的 api                                                          |
| special_filtering.md             |
| tutorial.md                      | bcc 工具的使用                                                                   |
| tutorial_bcc_python_developer.md | python 的写法，这里引用了 examples 下很多例子，从这里入手吧 ⭐                                                   |


## bcc 的打包
可以参考这个: https://gitee.com/src-openeuler/bcc/blob/master/bcc.spec ，主要提供

- python3-bcc-0.27.0-4.fc39.noarch python3-bpfcc-0.15.0-2.oe1.noarch
- bcc-tools-0.27.0-4.fc39.x86_64
- bcc-0.27.0-4.fc39.x86_64 : 提供

```txt
/usr/lib64/libbcc.so.0
/usr/lib64/libbcc_bpf.so.0
```

## libbpf-tools 可以平替吗 bcc-tools 吗?

## 工具总结
|------------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 特殊       | argdist ksnoop funccount funcinterval funclatency funcslower profile trace virtiostat syscount                                                                                      |
| 用户态     | bashreadline dbslower mysqld_qslower dbstat sslsniff
| 用户态特殊 | ucalls uflow ugc uobjnew ustat uthreads
| snoop      | bindsnoop biosnoop compactsnoop dcsnoop opensnoop shmsnoop sofdsnoop syncsnoop statsnoop spfdsnoop ttysnoop threadsnoop mountsnoop drsnoop execsnoop exitsnoop killsnoop stackcount |
| 存储       | biolatency biolatpcts biopattern biotop bitesize cachestat cachetop dcstat dirtop readahead mdflush swapin
| 辅助工具   | bpflist bps tplist reset-trace
| 模块相关   | btrfsdist btrfsslower xfsdist xfsslower zfsdist zfsslower ext4dist ext4slower f2fsslower nfsdist nfsslower
| 调度器     | runqlat runqlen runqslower cpudist cpuunclaimed offcputime offwaketime wakeuptime
| tcp        | tcpaccept tcpcong tcpconnect tcpconnlat tcpdrop tcplife tcpretrans tcprtt tcpstates tcpsubnet tcpsynbl tcptop tcptracer
| fs         | vfscount vfsstat filegone filelife fileslower filetop
| 一般网络   | solisten sofdsnoop gethostlatency
| lock       | deadlock klockstat
| workqueue  | wqlat
| memory     | memleak oomkill slabratetop
| irq        | hardirqs softirqs
| kvm        | kvmexit ppchcalls
| 其他       | llcstat pidpersec rdmaucma capable

### 有待分析的
```txt
🧀  sudo criticalstat -p

ERROR: required tracing events are not available
Make sure the kernel is built with CONFIG_DEBUG_PREEMPT CONFIG_PREEMPT_TRACER and CONFIG_PREEMPTIRQ_EVENTS (CONFIG_PREEMPTIRQ_TRACEPOINTS in kernel 4.19 and later) enabled. Also please disable CONFIG_PROVE_LOCKING and CONFIG_LOCKDEP on older kernels.
```


inject : 真的可以吗?

# 使用 bcc 来学习内核

https://github.com/iovisor/bcc/tree/master/tools

工具使用分析。

## 具体工具
### cachestat
- [ ] page cache 和 buffer 的命中率分析，但是感觉这个工具有 bug

### [ ] runqlat

### biolatency
认为比 blktrace 好用

测试的是 disk 的 latency ，得到的结果是

通过 -Q 可以获取到系统的 latency 的:
-Q option can be used to include time queued in the kernel.

### mdflush

居然是专门来监控 raid 的，对应的函数是 : `md_flush_request`

### klockstat

### netqtop

跟踪 net_dev_start_xmit 和 netif_receive_skb 获取其中的 skb->len

## 通用工具

### funccount

### stackcount
P101 中展示了如何使用 stackcount 也是可以制作火焰图的，展示其调用来源。

```txt
wget https://raw.githubusercontent.com/brendangregg/FlameGraph/master/flamegraph.pl -O ${flamegraph_pl}
stackcount -f -P -D 3 do_sys_open > ${stackcount_data}
perl ${flamegraph_pl} --title "martins3" --hash --bgcolors=grey < ${stackcount_data} > ${img}
```

### trace

```txt

  echo "跟踪函数和其返回值"
  # TMP_TODO 我们发现在 return 位置上增加一个 arg1 的时候
  # trace-bpfcc 'r:./program.out:add "%d %d", retval'
  trace-bpfcc 'r:./program.out:add "%d %d %d", arg1, arg2, retval'
  ;;
  # TMP_TODO BFP performance Tool P12.2.6 最后的举例 argdist, stackcount 和 profile 没看懂
```

### offcputime
<!-- be5a9a14-28df-4fcd-b908-18c0e29b0055 -->

问题背景，现在我不断的通过拷贝来 clone 虚拟机，他们的容量大致为
```txt
├──────────────────────────────────────────────┼───────┼────┼─────┼──────────┼──────┤
│ yyds-fs                                      │ 67750 │ 70 │     │          │ 64G  │
│ yyds                                         │ 67109 │ 29 │     │          │ 64G  │
│ wt-eevdf                                     │ 64658 │ 72 │     │          │ 64G  │
│ wt-block-scheduler                           │ 62352 │ 71 │     │          │ 64G  │
│ yyds-kata                                    │ 17121 │ 67 │     │          │ 74G  │
│ yyds-cuda                                    │       │ 68 │     │          │ 64G  │
│ yyds-nanobot                                 │       │ 69 │     │          │ 65G  │
```
然后，可以发现 qemu-system-x86 利用率很高，而且都是用于 wa ，这个时候，想知道当时到底在干什么
就可以:
```txt
top - 19:21:13 up  9:00,  5 users,  load average: 2.66, 3.18, 3.15
Tasks: 590 total,   2 running, 588 sleeping,   0 stopped,   0 zombie
%Cpu(s):  6.1 us,  7.1 sy,  0.0 ni, 64.4 id, 21.8 wa,  0.4 hi,  0.2 si,  0.0 st
MiB Mem : 128022.6 total,  80470.8 free,  30139.6 used,  40147.9 buff/cache
MiB Swap:   8192.0 total,   8192.0 free,      0.0 used.  97882.9 avail Mem

    PID USER      PR  NI    VIRT    RES    SHR S  %CPU  %MEM     TIME+ COMMAND
  67109 martins3  20   0   14.4g   4.6g   4.5g S  56.0   3.7 135:43.92 qemu-system-x86
  64658 martins3  20   0   12.1g   4.4g   4.3g S  52.0   3.5 146:37.45 qemu-system-x86
  67750 martins3  20   0   12.9g   3.5g   3.4g S  52.0   2.8 144:54.35 qemu-system-x86
  62352 martins3  20   0   11.8g   4.4g   4.2g S  48.0   3.5 146:48.95 qemu-system-x86
  17121 martins3  20   0   13.6g   4.8g   4.7g S  12.0   3.8 110:01.63 qemu-system-x86
  89765 martins3  20   0  767288 217780  27716 S   8.0   0.2   0:51.57 nvim
```

为了找到进入到 wa 的原因，可以使用 (--state 2) 是关键:
```txt
 sudo offcputime -p 67750 --state 2
Tracing off-CPU time (us) of PIDs 67750 by user + kernel stack... Hit Ctrl-C to end.
^C
    finish_task_switch.isra.0
    __schedule
    schedule
    io_schedule
    __iomap_dio_rw
    iomap_dio_rw
    xfs_file_dio_write_aligned
    xfs_file_write_iter
    vfs_write
    __x64_sys_pwrite64
    do_syscall_64
    entry_SYSCALL_64_after_hwframe
    pwrite
    handle_aiocb_rw_linear
    handle_aiocb_rw
    worker_thread
    qemu_thread_start
    start_thread
    __clone3
    -                worker (106537)
        43
```

如果分析用户态程序，可以参考:
sudo syscount -p 2114125 -m
```txt
Tracing syscalls, printing top 10... Ctrl+C to quit.
^C[17:08:23]
SYSCALL                   COUNT        TIME (us)
ppoll                        17      1381429.564
ioctl                        14      1247892.259
write                         7           28.264
io_uring_enter                8           20.720
poll                          2            9.510
read                          8            8.266
```


### argdist
获取到参数的分布，可以对于参数进行过滤

## deadlock 这个组件如何来分析内核的，估计不是什么高级技能了


## https://github.com/iovisor/bcc/tree/master/libbpf-tools

nix-shell '<nixpkgs>' -A bcc --command " make -j"

应该还是可以解决的
```txt
biostacks.bpf.c:83:14: error: A call to built-in function '__stack_chk_fail' is not supported.
int BPF_PROG(blk_account_io_done, struct request *rq)
```

# 洞悉 Linux 系统和应用性能

nixos 下是 execsnoop 而 debian execsnoop-bpfcc

## 随书代码
- https://github.com/brendangregg/bpf-perf-tools-book/blob/master/originals/Ch14_Kernel/kmem.bt

## 什么，还可以使用这个生成火焰图
profile -p `pidof fio` -f

https://github.com/iovisor/bcc/blob/master/docs/reference_guide.md


## 常用

观察 syscount : 观察一个 process 的所有系统调用，看看这个程序在搞什么
https://github.com/iovisor/bcc/blob/master/tools/syscount_example.txt


## 观测 kvm_exit 的原因

kvm_stat 似乎不能观测 exit 的原因

使用 tracepoint kvm:kvm_exit 或者 bcc 中的 kvmexit 来处理

## 让 qemu -kernel 启动的 guest 中观测内核

```txt
[root@localhost tools]# ./trace nvme_complete_rq
modprobe: FATAL: Module kheaders not found in directory /lib/modules/6.8.0-rc4-00180-g4f5e5092fdbf
Unable to find kernel headers. Try rebuilding kernel with CONFIG_IKHEADERS=m (module)
or installing the kernel development package for your running kernel version.
chdir(/lib/modules/6.8.0-rc4-00180-g4f5e5092fdbf/build): No such file or directory
Failed to compile BPF module <text>
```
/usr/share/bcc/tools/trace 还是一个 python 代码。

```txt
CONFIG_IKCONFIG=y
CONFIG_IKCONFIG_PROC=y
CONFIG_IKHEADERS=y
```

到此，ftrace , bpftrace , perf 都可以在 guest 中使用

但是 https://github.com/mechpen/sockdump 用不起来:
```txt

```txt
include/linux/bpf.h:334:10: error: invalid application of 'sizeof' to an incomplete type 'struct bpf_rb_root'
                return sizeof(struct bpf_rb_root);
                       ^     ~~~~~~~~~~~~~~~~~~~~
include/linux/bpf.h:334:24: note: forward declaration of 'struct bpf_rb_root'
                return sizeof(struct bpf_rb_root);
                                     ^
include/linux/bpf.h:336:10: error: invalid application of 'sizeof' to an incomplete type 'struct bpf_rb_node'
                return sizeof(struct bpf_rb_node);
                       ^     ~~~~~~~~~~~~~~~~~~~~
include/linux/bpf.h:336:24: note: forward declaration of 'struct bpf_rb_node'
                return sizeof(struct bpf_rb_node);
                                     ^
include/linux/bpf.h:338:10: error: invalid application of 'sizeof' to an incomplete type 'struct bpf_refcount'
                return sizeof(struct bpf_refcount);
                       ^     ~~~~~~~~~~~~~~~~~~~~~
include/linux/bpf.h:338:24: note: forward declaration of 'struct bpf_refcount'
                return sizeof(struct bpf_refcount);
                                     ^
include/linux/bpf.h:361:10: error: invalid application of '__alignof' to an incomplete type 'struct bpf_rb_root'
                return __alignof__(struct bpf_rb_root);
                       ^          ~~~~~~~~~~~~~~~~~~~~
include/linux/bpf.h:361:29: note: forward declaration of 'struct bpf_rb_root'
                return __alignof__(struct bpf_rb_root);
                                          ^
include/linux/bpf.h:363:10: error: invalid application of '__alignof' to an incomplete type 'struct bpf_rb_node'
                return __alignof__(struct bpf_rb_node);
                       ^          ~~~~~~~~~~~~~~~~~~~~
include/linux/bpf.h:363:29: note: forward declaration of 'struct bpf_rb_node'
                return __alignof__(struct bpf_rb_node);
                                          ^
include/linux/bpf.h:365:10: error: invalid application of '__alignof' to an incomplete type 'struct bpf_refcount'
                return __alignof__(struct bpf_refcount);
                       ^          ~~~~~~~~~~~~~~~~~~~~~
include/linux/bpf.h:365:29: note: forward declaration of 'struct bpf_refcount'
                return __alignof__(struct bpf_refcount);
                                          ^
2 warnings and 6 errors generated.
Traceback (most recent call last):
  File "/root/sockdump/./sockdump.py", line 431, in <module>
    main(args)
  File "/root/sockdump/./sockdump.py", line 363, in main
    b = BPF(text=text)
  File "/usr/lib/python3.10/site-packages/bpfcc/__init__.py", line 479, in __init__
    raise Exception("Failed to compile BPF module %s" % (src_file or "<text>"))
Exception: Failed to compile BPF module <text>
```

解压之后:
```txt
tar -xvf /sys/kernel/kheaders.tar.xz .
```
依赖的在 include/uapi/linux/bpf.h 中可以找到:

也许是 bcc 太老吗? 用 openEuler 2404 可以解决吗?
```txt
➜  include cat /etc/os-release
NAME="openEuler"
VERSION="23.03"
ID="openEuler"
VERSION_ID="23.03"
PRETTY_NAME="openEuler 23.03"
ANSI_COLOR="0;31"
```

## filetop 是观察什么的？

## snoop 的重新整理
- biosnoop 有用
- compactsnoop
- dcsnoop # 有趣的，跟踪 d_lookup
- sofdsnoop # 似乎 socket fd 相关的 https://github.com/iovisor/bcc/blob/master/tools/sofdsnoop.py
- statsnoop 跟踪什么的
- spfdsnoop ?
- ttysnoop threadsnoop
- drsnoop # direct reclaim
- stackcount ? 不会用

## bcc 可以获取结构体中的参数
通过 trace -h 已经可以很清楚了
```txt
trace -I 'kernel/sched/sched.h' \
      'p::__account_cfs_rq_runtime(struct cfs_rq *cfs_rq) "%d", cfs_rq->runtime_remaining'
        Trace the cfs scheduling runqueue remaining runtime. The struct cfs_rq is defined
        in kernel/sched/sched.h which is in kernel source tree and not in kernel-devel
        package.  So this command needs to run at the kernel source tree root directory
        so that the added header file can be found by the compiler.
trace -I 'net/sock.h' \
      'udpv6_sendmsg(struct sock *sk) (sk->sk_dport == 13568)'
        Trace udpv6 sendmsg calls only if socket's destination port is equal
        to 53 (DNS; 13568 in big endian order)
trace -I 'linux/fs_struct.h' 'mntns_install "users = %d", $task->fs->users'
        Trace the number of users accessing the file system of the current task
```
1. p:: 是什么意思
2. 为什么 mntns_install 不需要参数
  - 应该是没有去用
3. 注意，其实是 $task 和参数的区别是什么?

./trace 'default_send_IPI_single_phys(int cpu, int vector) (cpu == 46) "%x", vector'
./trace '__default_send_IPI_dest_field(unsigned int mask, int vector, unsigned int dest) (vector == 49)'

一个自己使用的案例:
```sh
./trace -I 'linux/device.h' 'mdev_set_iommu_device(struct device *dev, struct device *iommu_device) "%x %x", dev->devt, iommu_device->devt'
```

通过这个我才意识到，原来 bcc 是需要 kernel devel 的，但是 bpftrace 是不需要的，bpftrace 用的 btf 。

当然也可以，也是可以处理 tracepoint
1. 对于参数过滤 : ./trace 't:kmem:kmalloc (args->bytes_alloc == 512)'
2. 打印参数 :
./trace 't:kmem:kmalloc (args->bytes_alloc == 512) "%lx", args->call_site'
3. 获取到内核的 backtrace : ./trace 't:kmem:kmalloc (args->bytes_alloc == 512)' -K
 效果不行，没有完整的

类似的例子:
```txt
./trace 't:kvm:kvm_userspace_exit (args->reason == 9) "%lx", args->errno'
```

## trace 一个 kmalloc 的使用

(这里写的很混乱，倒时候修改下)

也是有问题
居然找到了一个 : https://github.com/iovisor/bcc/pull/3109/files

可以 refernece 多个文件，但是这个方法也有问题:

```sh
./trace -I 'linux/slab.h' -I 'linux/slub_def.h' 'kmem_cache_alloc(struct kmem_cache *s, gfp_t gfpflags) (STRCMP("kmalloc-512", s->name))'
```

```sh
./trace -I 'linux/slab.h' -I 'linux/slub_def.h' \
'kmem_cache_alloc(struct kmem_cache *s, gfp_t gfpflags) (STRCMP("task_delay_info", s->name))'
```

这里有两个注意:
1. strcmp 需要用 macro ，也就是大写的
2. 不可以给 STRCMP 再添加等于

然后可以继续添加 -K 来记录 backtrace 的

参考 https://github.com/iovisor/bcc/blob/master/tools/trace_example.txt

### upstream
```sh
sudo trace -I 'mm/slab.h' 'kmem_cache_alloc(struct kmem_cache *s, gfp_t gfpflags) (s->name == "kmalloc-512")'
```
mm/slab.h 不是一个文件，可以补充吗?



### 指针被 hash 过了
sudo trace 'do_filp_open "%x", arg2'
```txt
946     946     systemd-udevd   do_filp_open     c546f000
1671    1671    ovs-vswitchd    do_filp_open     f086f000
1671    1671    ovs-vswitchd    do_filp_open     f086f000
```
类似的:
sudo bpftrace -e 'kfunc:vmlinux:do_filp_open {printf("%x\n", args->pathname);}'
```txt
c9855000
c9855000
c9855000
```

但是:
```txt
🧀  sudo bpftrace -e 'kfunc:vmlinux:do_filp_open {printf("%px\n", args->pathname);}'
Attaching 1 probe...
0xffff9e6cc95d8000x
0xffff9e6cc9855000x
```

暂时不知道如何实现 trace 打印真实地址，不过用到了再说。

## bcc 可以获取函数的返回值
./trace 'r::vfio_mdev_attach_domain "%llx", retval'


## tracepoint 的 stackcount bcc 无法获取到?
只能用这种方法:
sudo perf record -e kvm:kvm_update_master_clock  -g -a

并不是，stackcount 可以这么使用的:

./stackcount 't:kmem:kmalloc'


## 通过 trace 可以额外的配合
/usr/share/bcc/tools/trace -K 't:kmem:kmem_cache_alloc' > trace 2> /dev/null & trace_pid="$!" && sleep 5 && kill -sigkill "$trace_pid"

## bcc 如何在 nix 虚拟机中运行
<!-- 29841411-2eba-439e-8cba-b433d5415c38 -->

安装 kernel-devel 或者 ikheader 模块就可以了:

然后直接 sudo execsnoop

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
