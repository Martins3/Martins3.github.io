# ssh 的操作会过 tty 机制吗?
```txt
+ sudo bpftrace -e 'kprobe:tty_port_default_receive_buf { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
Attaching 2 probes...
^C

@[
    tty_port_default_receive_buf+5
    flush_to_ldisc+153
    process_one_work+325
    worker_thread+715
    kthread+207
    ret_from_fork+49
    ret_from_fork_asm+26
]: 494

+ sudo bpftrace -e 'kprobe:tty_write { @[kstack(bpftrace)] = count(); } interval:s:1000 { exit(); }'
Attaching 2 probes...
@[
    tty_write+5
    vfs_write+665
    ksys_write+110
    do_syscall_64+95
    entry_SYSCALL_64_after_hwframe+118
]: 1317
```

```txt
🧀  sudo bpftrace -e 'kprobe:tty_write { @[curtask->comm] = count() } interval:s:1000 { exit(); }'
Attaching 2 probes...
^C

@[sudo]: 1
@[a.out]: 3
@[sshd]: 12
@[zsh]: 59
```

tty_write
tty_port_default_receive_buf

sudo bpftrace -e 'kfunc:vmlinux:tty_write { printf("%s\n", args->iocb->ki_filp->f_path.dentry->d_iname); }'

## 通过 ssh 的时候
```txt
🧀  sudo bpftrace -e 'kfunc:vmlinux:tty_write { @[args->iocb->ki_filp->f_path.dentry->d_iname]=count() }'
Attaching 1 probe...
^C

@[tty]: 1
@[ptmx]: 12
@[0]: 605
```

```txt
🧀  ls -la /proc/self/fd
lrwx------ - martins3 19 Mar 12:41  0 -> /dev/pts/0
lrwx------ - martins3 19 Mar 12:41  1 -> /dev/pts/0
lrwx------ - martins3 19 Mar 12:41  2 -> /dev/pts/0
lr-x------ - martins3 19 Mar 12:41  3 -> /proc/3088/fd
```

## 通过 kitty

```txt
🧀  ls -la /proc/self/fd
lrwx------ - martins3 19 Mar 12:47 0 -> /dev/pts/8
lrwx------ - martins3 19 Mar 12:47 1 -> /dev/pts/8
lrwx------ - martins3 19 Mar 12:47 2 -> /dev/pts/8
lr-x------ - martins3 19 Mar 12:47 3 -> /proc/1939399/fd
```

## 在 vnc 界面
```txt
🧀  sudo bpftrace -e 'kfunc:vmlinux:tty_write { @[args->iocb->ki_filp->f_path.dentry->d_iname]=count() }'
Attaching 1 probe...
^C

@[tty]: 1
@[ptmx]: 2
@[tty1]: 61
```


## 在 console=ttyS0 中
```txt
🧀  sudo bpftrace -e 'kfunc:vmlinux:tty_write { @[args->iocb->ki_filp->f_path.dentry->d_iname]=count() }'
Attaching 1 probe...
^C

@[tty]: 1
@[ptmx]: 2
@[ttyS0]: 1190
```

## console=hvc0 中各种输出
```txt
🧀  sudo bpftrace -e 'kfunc:vmlinux:tty_write { @[args->iocb->ki_filp->f_path.dentry->d_iname]=count() }'
Attaching 1 probe...
^C

@[tty]: 1
@[ptmx]: 2
@[hvc0]: 639
```

```txt
🤒  ls -la /proc/self/fd
lrwx------ - martins3 19 Mar 12:43  0 -> /dev/hvc0
lrwx------ - martins3 19 Mar 12:43  1 -> /dev/hvc0
lrwx------ - martins3 19 Mar 12:43  2 -> /dev/hvc0
lr-x------ - martins3 19 Mar 12:43  3 -> /proc/2802/fd
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
