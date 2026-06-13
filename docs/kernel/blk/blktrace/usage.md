# blktrace 基本使用

## 如何理解 raid1_log

```c
#define raid1_log(md, fmt, args...)				\
	do { if ((md)->queue) blk_add_trace_msg((md)->queue, "raid1 " fmt, ##args); } while (0)
```

最后调用到 blktrace.c:`__blk_trace_note_message` 中

当执行:

```txt
 mdadm --grow --force --raid-devices=1000 /dev/md10 || true
```

可以得到:

```txt
➜  ~ blktrace -d /dev/md10 -o - | blkparse -i -
  9,10  10        0     0.000000000  1614 1,0  m   N md md_update_sb
  9,10  10        0     0.003608231  1614 1,0  m   N raid1 wait freeze
  9,10  10        0     0.003627965  1614 1,0  m   N md md_update_sb
  9,10  28        0     0.009725873  1546 1,0  m   N md md_update_sb
```

啥?

```txt
[53210.169322] blktrace: Concurrent blktraces are not allowed on dummy
```

配合 virtio-dummy 测试

echo 1 > /dev/dummy 得到的结果:

```txt
             zsh-3158    [015] d..1.  5758.124989: 249,1    Q   R 0 + 8 [zsh]
             zsh-3158    [015] d..1.  5758.125002: 249,1    G   R 0 + 8 [zsh]
             zsh-3158    [015] d..1.  5758.125007: 249,1    P   N [zsh]
             zsh-3158    [015] d..1.  5758.125007: 249,1    U   N [zsh] 1
             zsh-3158    [015] d..2.  5758.125007: 249,1    I   R 0 + 8 [zsh]
             zsh-3158    [015] d..1.  5758.125013: 249,1    D   R 0 + 8 [zsh]
          <idle>-0       [021] d.s1.  5758.125079: 249,1   1,0  m   N --> swapper/21 # dummy_complete_rq 中调用 blk_add_trace_msg 的结果
          <idle>-0       [021] d.s2.  5758.125080: 249,1    C   R 0 + 8 [0]
```
(也就是，我的 dummy 设备也是有 scheduler 的吗?)

好吧，这些 trace 点都在那里

原来是在:

在 TRACE_EVENT(block_bio_complete 中定义了一个类似的，但是并不是。

实际是，输出的位置在 blk_log_action 中。

## 为什么需要 blktrace 这个 tracer 来处理

## 从源码中分析

https://git.kernel.org/pub/scm/linux/kernel/git/axboe/blktrace.git/about/

一个源码中包含了超级多东西。

## 基本使用

1. blktrace : cgexec --sticky -g cpu,memory,cpuset:/ blktrace -d /dev/sdc
2. blkparse : blkparse -i sdc -d sdc.blktrace.bin
3. btt : btt -i sdc.blktrace.bin

4. btrace : blktrace 的简单封装，其实就是 `blktrace -d /dev/md10 -o - | blkparse -i -`
5. iowatcher : 生成 svg 和 mp4

### 实时观察
sudo cgexec --sticky -g cpu,memory,cpuset:/ \
blktrace -d /dev/sdj -o - | blkparse -i -

观察一个卡住的磁盘:
```txt
  8,144  5        1     0.000000000 56207  Q   R 0 + 8 [a.out]
  8,144  5        2     0.000004697 56207  G   R 0 + 8 [a.out]
  8,144  5        3     0.000005692 56207  P   N [a.out]
  8,144  5        4     0.000006012 56207  U   N [a.out] 1
  8,144  5        5     0.000013544 56207  I   R 0 + 8 [a.out]
```
然后就结束了，没有 D 也没有 C

### 通过 ./raw.sh 观察 io 流程

目前看 :
1. remap 就是给 device mapper 用的
2. trace_block_rq_insert 主要给 scheduler 使用的


和 code/src/c/fs/aio-minimal.c 配合测试，这就是 blktrace 中的结果了:

如果是 scsi_debug 盘:
```txt
           a.out-2182    [001] d..1.   183.260509:   8,0    Q   R 0 + 8 [a.out]
           a.out-2182    [001] d..1.   183.260516:   8,0    G   R 0 + 8 [a.out]
           a.out-2182    [001] d..1.   183.260517:   8,0    P   N [a.out]
           a.out-2182    [001] d..1.   183.260518:   8,0    U   N [a.out] 1
           a.out-2182    [001] d..2.   183.260519:   8,0    I   R 0 + 8 [a.out]
           a.out-2182    [001] d..1.   183.260523:   8,0    D   R 0 + 8 [a.out]
          <idle>-0       [001] d.s2.   183.261539:   8,0    C   R 0 + 8 [0]
```

切换 scheduler 为 none
```txt
           <...>-7358    [000] d.... 48662.429131:   8,0   1,0  m   N elv switch: none
```

当 scheduler 为 none 的时候，就没有 insert 了:
```txt
           a.out-7860    [001] d..1. 48727.830123:   8,0    Q   R 0 + 8 [a.out]
           a.out-7860    [001] d..1. 48727.830133:   8,0    G   R 0 + 8 [a.out]
           a.out-7860    [001] d..1. 48727.830134:   8,0    P   N [a.out]
           a.out-7860    [001] d..1. 48727.830135:   8,0    U   N [a.out] 1
           a.out-7860    [001] d..1. 48727.830138:   8,0    D   R 0 + 8 [a.out]
          <idle>-0       [001] d.s2. 48727.831105:   8,0    C   R 0 + 8 [0]
```

如果是 device mapper ，让 aio-minimal.c 写 /home/martins3/data ，然后
使用 raw.sh 来观察其底层的盘为 /dev/sdc ，结果为:
```txt
           a.out-6318    [001] d..1. 47995.013437:   8,32   A   R 2048 + 8 <- (253,1) 0
           a.out-6318    [001] d..1. 47995.013442:   8,32   Q   R 2048 + 8 [a.out]
           a.out-6318    [001] d..1. 47995.013447:   8,32   G   R 2048 + 8 [a.out]
           a.out-6318    [001] d..1. 47995.013448:   8,32   P   N [a.out]
           a.out-6318    [001] d..1. 47995.013449:   8,32   U   N [a.out] 1
           a.out-6318    [001] d..1. 47995.013454:   8,32   D   R 2048 + 8 [a.out]
          <idle>-0       [001] d.h3. 47995.013516:   8,32   C   R 2048 + 8 [0]
```
添加了一个 'A' (做了 remap )，没有 'I' (insert) 。其实有一个小疑惑，其实有点奇怪哦。


- do_syscall_64
  - do_syscall_x64
    - ksys_read
      - vfs_read
        - tracing_read_pipe
          - blk_trace_event_print

cat /sys/kernel/tracing/trace_pipe

- entry_SYSCALL_64
  - do_syscall_64
    - do_syscall_x64
      - ksys_read
        - vfs_read
          - seq_read
            - seq_read_iter
              - s_show
                - blk_trace_event_print

## 综合文摘看看

https://access.redhat.com/documentation/en-us/red_hat_enterprise_linux/6/html/performance_tuning_guide/ch06s03

## 读读文档

- [blkparse](https://linux.die.net/man/1/blkparse)
- [btt](https://man7.org/linux/man-pages/man1/btt.1.html)

- [btrecord](https://linux.die.net/man/8/btrecord) && [btreplay](https://linux.die.net/man/8/btreplay)
- [iowatcher](https://man7.org/linux/man-pages/man1/iowatcher.1.html)
- [bno_plot](https://man7.org/linux/man-pages/man1/bno_plot.1.html)

## blktrace -h

- https://developer.aliyun.com/article/698568

- call_bio_endio 中最后会调用到 `bio_end_io_acct`，是给 blktrace 来处理的吗?

https://man7.org/linux/man-pages/man8/blktrace.8.html ，似乎主要是这个:
```txt
       The following masks may be passed with the -a command line
       option, multiple filters may be combined via multiple -a command
       line options.

              barrier: barrier attribute
              complete: completed by driver
              discard: discard / trim traces
              fs: requests
              issue: issued to driver
              pc: packet command events
              queue: queue operations
              read: read traces
              requeue: requeue operations
              sync: synchronous attribute
              write: write traces
              notify: trace messages
              drv_data: additional driver specific trace
```

## D2C 的延迟如何看待
blk_mq_start_request 到 blk_mq_start_issue 直接的延迟了

似乎 iowait 也可以看 ?

## blktrace 如何实现过滤的
1. 为什么 blktrace 可以仅仅分析一个盘，如何过滤其他的盘的
2. 过滤操作
3. 仅仅关注一个操作

## 注意: 当使用 iosnoop 这种纯粹的 ftrace 接口的时候，并不会有 blk_add_trace_rq_issue

## podman 中为什么无法运行 blktrace -d /dev/sda

全部都是这个报错:

```txt
setup_mmap: mmap: Invalid argument
Thread 14 failed read of /sys/kernel/debug/block/sda/trace14: 14/Bad address
Thread 14 failed read of /sys/kernel/debug/block/sda/trace14: 14/Bad address
Thread 11 failed read of /sys/kernel/debug/block/sda/trace11: 14/Bad address
Thread 11 failed read of /sys/kernel/debug/block/sda/trace11: 14/Bad address
setup_mmap: mmap: Invalid argument
Thread 3 failed read of /sys/kernel/debug/block/sda/trace3: 14/Bad address
Thread 3 failed read of /sys/kernel/debug/block/sda/trace3: 14/Bad address
setup_mmap: mmap: Invalid argument
setup_mmap: mmap: Invalid argument
```

## btrace 有时候会有这个问题

```txt
🦭 🤒  btrace /dev/dummy
BLKTRACESETUP(2) /dev/dummy failed: 16/Device or resource busy
Trace started at Thu Jan  1 00:00:00 1970
```

## blktrace 如今的更新已经很少了，那么 aboxe 在用什么排查问题

## fio 的延迟和 Q2C 的延迟分析下

fio io depth = 1
```txt
            ALL           MIN           AVG           MAX           N
--------------- ------------- ------------- ------------- -----------
D2C               0.000354400   0.002734293   0.029403617        3609
Q2C               0.000356501   0.002738688   0.029404622        3609
```

fio io depth = 64
```txt
D2C               0.003275484   0.062115296   1.954556241        5189
Q2C               0.006752063   0.122568471   1.966793279        5189
```

D: blk_mq_start_request : issue 的位置

从 fio io depth = 64 中 D2C 和 Q2C ，看来 schduler 的确是花费了时间的。

sdd
```txt
==================== All Devices ====================

            ALL           MIN           AVG           MAX           N
--------------- ------------- ------------- ------------- -----------

Q2Q               0.000003283   0.006834719   0.254648379        8627
Q2G               0.000000573   0.000003154   0.000031573        8471
G2I               0.000000317   0.000004266   0.000289556        8401
Q2M               0.000000893   0.000001979   0.000012464         158
I2D               0.000002373   0.001047240   0.296104725        8398
M2D               0.000010396   0.057153165   0.294460526         158
D2C               0.000177908   0.119485464   6.029711636        8618
Q2C               0.000191171   0.121573269   6.029721581        8620
```
sdc
```txt
--------------- ------------- ------------- ------------- -----------

Q2Q               0.000001972   0.005028068   0.255607201       21370
Q2G               0.000000409   0.000003065   0.001450006       21341
G2I               0.000000283   0.000004790   0.000277756       21025
Q2M               0.000000372   0.000001020   0.000001981          33
I2D               0.000002539   0.000007002   0.001234222       20995
M2D               0.000004945   0.000019187   0.000132128          31
D2C               0.000080726   0.014170400   0.625845945       21344
Q2C               0.000085519   0.014202668   0.625852192       21370
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
