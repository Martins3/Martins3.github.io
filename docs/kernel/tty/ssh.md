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

## sshd 冒号后面的东西是什么?

```txt
 ps -elf | grep sshd
4 S root      671620       1  0  80   0 -  2277 -      Aug31 ?        00:00:00 sshd: /usr/sbin/sshd -D [listener] 0 of 10-100 startups
4 S root     1329075  671620  0  80   0 -  4489 -      10:35 ?        00:00:00 sshd-session: martins3 [priv]
5 S martins3 1329087 1329075  0  80   0 -  4800 -      10:35 ?        00:00:12 sshd-session: martins3@pts/121
4 S root     1797791  671620  0  80   0 -  4489 -      15:11 ?        00:00:00 sshd-session: martins3 [priv]
5 S martins3 1797801 1797791  0  80   0 -  4545 -      15:11 ?        00:00:00 sshd-session: martins3@pts/22
```

当前机器 localhost.localdomain 上，实际有 两条 SSH 连接，都有伪终端，没有 notty。进程关系如下：

671620 root  sshd: /usr/sbin/sshd -D [listener] 0 of 10-100 startups
  |
  +-- 1329075 root      sshd-session: martins3 [priv]
  |     +-- 1329087 martins3  sshd-session: martins3@pts/121
  |           +-- 1329095 zsh
  |                 +-- 1329422 tmux attach -d
  |
  +-- 1797791 root      sshd-session: martins3 [priv]
        +-- 1797801 martins3  sshd-session: martins3@pts/22
              +-- 1797808 zsh
                    +-- 1797980 collei-action.py -a ssh -s
                          +-- 1798031 ssh -p 51404 martins3@localhost

结合 TCP socket，可以准确对应：

 会话                 当前运行的程序
━━━━━━━━━━━━━━━━━━   ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 martins3@pts/121     tmux attach -d
──────────────────   ─────────────────────────────────────
 martins3@pts/22      通过 collei 脚本再发起一条 SSH 连接

这里有几个值得对应理解的细节：

- [priv]：两个进程都属于 root，各自负责一条连接的特权操作；其子进程已经降权为 martins3。
- @pts/121、@pts/22：表示连接分配的伪终端。实际使用这些终端的是下方的 zsh 等进程。
- ps 的 TTY 列仍是 ?：这是 sshd 会话进程自身没有控制终端；标题中的 pts/... 描述它管理的会话终端，两者不矛盾。
- 0 of 10-100 startups：当前没有等待认证的连接。现有两条连接都已认证，因此这里仍为 0。本机 sshd -T 输出确实是 maxstartups 10:30:100。
- sshd-session:：当前运行的会话进程使用这个名字，和你原始示例中的 sshd: 承担相同的会话角色。本机 /usr/sbin/sshd -V 显示版本为 OpenSSH_10.2p1。

这对应 两条 SSH 连接，每条各有一个 root 特权进程和一个用户权限进程，是 OpenSSH 的权限分离机制。相关实现见 sshd.c
(https://github.com/openssh/openssh-portable/blob/V_9_6_P1/sshd.c) 中的 privsep_postauth()。

另外，@ 后面这里是终端名称，不是客户端地址；notty 也不代表连接空闲或异常。标题由 session.c
(https://github.com/openssh/openssh-portable/blob/V_9_6_P1/session.c) 中的 session_proctitle() 和 session_tty_list() 生成。

类似:
```txt
5 S martins3   17639   17627 42  80   0 -  5572 -      15:13 ?        00:01:11 sshd: martins3@notty
```
martins3@notty 表示 以 martins3 身份处理连接，没有分配伪终端。常见于远程执行命令、SFTP、SCP 等



## ssh -tt 的作用
你平时用 ssh 主机 登录，确实通常会自动分配 PTY。但 SSH 也经常用来执行一条远程命令、传输数据，这些情况默认不分配 PTY。

主要区别是：

 用法                                          默认分配远端 PTY
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━
 在本地终端执行 ssh 主机，进入远程 shell       是
────────────────────────────────────────────  ──────────────────
 ssh 主机 '某条命令'                           否
────────────────────────────────────────────  ──────────────────
 本地标准输入不是终端，比如管道或自动化脚本    通常否

例如：

ssh 主机 'hostname'

只需要把主机名输出回来，不需要终端。但如果远程命令是交互程序：

ssh -t 主机 'tmux attach'

就要用 -t 告诉 SSH：“虽然我指定了命令，但这个命令需要终端。”

为什么不干脆总是分配？ 因为 PTY 会引入终端行为，例如回显输入、转换换行符，以及让程序输出颜色或交互提示。执行脚本或传输二进制数据时，这
些行为反而会干扰结果。例如：

ssh 主机 'cat /tmp/archive.tar' > archive.tar

这里需要原样传输字节，不应该经过 PTY。

-tt 则更进一步：本地标准输入即使不是终端，也强制分配远端 PTY，常用于自动化环境中运行需要终端的程序。

所以你之前那条 ssh -tt -p 51404 martins3@localhost，如果是在普通终端里手动执行，通常不加 -tt 也一样能获得 PTY。

## https://github.com/trzsz/trzsz-ssh 中提供了一个 option

-t -T RequestTTY

ssh -tt 表示强制给远端 SSH 会话分配一个伪终端（TTY），让远端程序可以像在普通终端里一样接收键盘输入、显示交互界面。

  • -t：请求分配终端。
  • -tt：即使 SSH 本地没有终端，也强制给远端分配终端。

  tmux 的 attach 需要终端，所以命令里用了 -tt。如果你直接在普通终端中执行 SSH 登录，通常会自动分配终端，不加也可以。

  它不能解决 tmux 版本不一致；之前能进入旧会话，关键是使用了 3.2 版的 tmux 客户端。

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
