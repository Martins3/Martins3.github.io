# signal 的简单测试
<!-- b13f5aff-9641-4fbe-9195-e0d43e0d6240 -->

## unix signal 的编号和数字直接的关系
<!-- ea038703-6802-4ce4-9810-49b25f60081c -->

kill -L

不过，注意 zsh 中 kill 是默认命令，而且其 kill 不支持 kill -L

```txt
# 直接执行
🧀  kill -L
kill: unknown signal: SIGL
kill: type kill -l for a list of signals

martins3@localhost:/usr/share/bcc/tools$ kill -L
 1) SIGHUP       2) SIGINT       3) SIGQUIT      4) SIGILL     5) SIGTRAP
 6) SIGABRT      7) SIGBUS       8) SIGFPE       9) SIGKILL   10) SIGUSR1
11) SIGSEGV     12) SIGUSR2     13) SIGPIPE     14) SIGALRM   15) SIGTERM
16) SIGSTKFLT   17) SIGCHLD     18) SIGCONT     19) SIGSTOP   20) SIGTSTP
21) SIGTTIN     22) SIGTTOU     23) SIGURG      24) SIGXCPU   25) SIGXFSZ
26) SIGVTALRM   27) SIGPROF     28) SIGWINCH    29) SIGIO     30) SIGPWR
31) SIGSYS      34) SIGRTMIN    35) SIGRTMIN+1  36) SIGRTMIN+237) SIGRTMIN+3
38) SIGRTMIN+4  39) SIGRTMIN+5  40) SIGRTMIN+6  41) SIGRTMIN+742) SIGRTMIN+8
43) SIGRTMIN+9  44) SIGRTMIN+10 45) SIGRTMIN+11 46) SIGRTMIN+1247) SIGRTMIN+13
48) SIGRTMIN+14 49) SIGRTMIN+15 50) SIGRTMAX-14 51) SIGRTMAX-1352) SIGRTMAX-12
53) SIGRTMAX-11 54) SIGRTMAX-10 55) SIGRTMAX-9  56) SIGRTMAX-857) SIGRTMAX-7
58) SIGRTMAX-6  59) SIGRTMAX-5  60) SIGRTMAX-4  61) SIGRTMAX-362) SIGRTMAX-2
63) SIGRTMAX-1  64) SIGRTMAX
martins3@localhost:/usr/share/bcc/tools$ which kill
~/.nix-profile/bin/kill
martins3@localhost:/usr/share/bcc/tools$ ~/.nix-profile/bin/kill -L
 1 HUP      2 INT      3 QUIT     4 ILL      5 TRAP     6 ABRT     7 BUS
 8 FPE      9 KILL    10 USR1    11 SEGV    12 USR2    13 PIPE    14 ALRM
15 TERM    16 STKFLT  17 CHLD    18 CONT    19 STOP    20 TSTP    21 TTIN
22 TTOU    23 URG     24 XCPU    25 XFSZ    26 VTALRM  27 PROF    28 WINCH
29 POLL    30 PWR     31 SYS
```
也就是 bash

## bcc

1. killsnoop 用来监听 kill 系统调用而已

2. 可以在 send_signal_locked 的这个地方打点，从而知道一个进程是如何被杀掉的。

sudo trace '__send_signal_locked (arg1 == 6) "%d", arg1'

采用 fault injection 的时候，不应该是还没有到达 wbt 吗?


## ctrl z 的实现原理

如果执行 sleep 1000 ，ctrl-z 不是通过发送信号，
但是是 continue 是发送信号实现的。

使用 bcc signal 可以容易看到:

```txt
[sudo] password for martins3:
TIME      PID      COMM             SIG  TPID     RESULT
08:10:12  16733    zsh              0    83820    -3
08:10:12  16733    zsh              0    83826    0
08:10:12  16733    zsh              0    83826    -3
08:10:12  16733    zsh              0    -83827   0
08:10:12  16733    zsh              0    83829    -3
08:10:12  16733    zsh              0    83845    -3
08:10:13  83854    zsh              0    83855    -3
08:10:14  16733    zsh              0    83858    -3
08:10:14  16733    zsh              18   -82675   0
```

接受过程:
```txt
[root@Node96 10:00:26 701531]$ cat stack
[<0>] do_signal_stop+0xff/0x200
[<0>] get_signal+0x2ae/0x7b0
[<0>] do_signal+0x36/0x610
[<0>] exit_to_usermode_loop+0x71/0xe0
[<0>] prepare_exit_to_usermode+0x90/0xc0
[<0>] retint_user+0x8/0x8
[<0>] 0xffffffffffffffff
[root@Node96 10:00:28 701531]$ timed out waiting for input: auto-logout
```

发送过程:

```txt
@[
        tty_insert_flip_string_and_push_buffer+5
        pty_write+46
        n_tty_write+879
        file_tty_write.isra.0+379
        vfs_write+611
        ksys_write+113
        do_syscall_64+116
        entry_SYSCALL_64_after_hwframe+118
]: 4
```

```txt
@[
        send_signal_locked+5
        group_send_sig_info+241
        __kill_pgrp_info+73
        kill_pgrp+52
        isig+142
        n_tty_receive_signal_char+22
        n_tty_receive_buf_standard+1067
        n_tty_receive_buf_common+263
        tty_port_default_receive_buf+66
        flush_to_ldisc+152
        process_one_work+501
        worker_thread+462
        kthread+268
        ret_from_fork+479
        ret_from_fork_asm+26
]: 1
```

## cat /proc/self/status
```txt
SigQ:   0/513462
SigPnd: 0000000000000000
SigBlk: 0000000000000000
SigIgn: 0000000000000000
SigCgt: 0000000000000000
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
