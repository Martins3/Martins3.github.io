# 记录一些 Linux kernel 中好玩的东西
## 内核的娱乐性

1. 基本重复的问题
2. 相同的问题可以多层的思考，你以为你解决了，但是实际上你只是中了宇智波鼬的月读
3. 千变万化的问题
4. 内核中有很多时候的茅塞顿开的时候，忽然间，发现长期困扰的问题，是可以解决的

## 谈谈 kernel 的模板元编程
include/linux/interval_tree_generic.h

arch/x86/kvm/mmu/paging_tmpl.h

## 打开一个 shell 到底会调用多少次系统调用
```txt
+ sudo bpftrace -e 'kprobe:dput { @[curtask->comm] = count() }'
Attaching 1 probe...
^C

@[systemd-journal]: 1
@[irqbalance]: 4
@[ovsdb-server]: 4
@[rm]: 8
@[tty]: 12
@[dbus-daemon]: 12
@[uname]: 13
@[dirname]: 13
@[locale]: 14
@[clear]: 14
@[wc]: 15
@[date]: 15
@[uuidgen]: 15
@[getconf]: 16
@[in:imjournal]: 16
@[zoxide]: 19
@[users]: 20
@[tput]: 22
@[dircolors]: 22
@[diff]: 23
@[sort]: 24
@[bc]: 26
@[df]: 30
@[python]: 31
@[ls]: 33
@[mktemp]: 36
@[nvim]: 38
@[sed]: 38
@[ip]: 42
@[whoami]: 49
@[id]: 61
@[tmux: server]: 69
@[hostnamectl]: 81
@[ovs-vswitchd]: 87
@[mkdir]: 124
@[/nix/store/b1wv]: 130
@[sqlx-sqlite-wor]: 135
@[cat]: 142
@[grepconf.sh]: 156
@[atuin]: 225
@[grep]: 262
@[awk]: 352
@[direnv]: 402
@[bash]: 651
@[starship]: 682
@[nix]: 1052
@[zsh]: 11012
@[git]: 207446
```

```txt
Tracing syscalls, printing top 10... Ctrl+C to quit.
^C[17:33:32]
SYSCALL                   COUNT
rt_sigprocmask            36256 // 任何仓库下打开都有
newfstatat                19031 // 在 vn 这个仓库下打开就有
pread64                    8529
ppoll                      8296
sched_yield                7255
read                       5178
rt_sigaction               5124
close                      4537
openat                     4449
getdents64                 3589
```
只能说现在的机器是真的快啊


### 把 nvim 打开关闭之后
```txt
🧀   sudo bpftrace -e 'kprobe:do_pipe2 { @[curtask->comm] = count() }'
Attaching 1 probe...
^C

@[starship]: 2
@[marksman]: 8
@[.NET TP Worker]: 10
@[zsh]: 13
@[nvim]: 32
```
想不到系统中连 .NET 都有，实在是过分了


## 仅仅是打开一个新的 zsh (ctrl h + % )
调用 8万次 walk_component
```txt
+ sudo bpftrace -e 'kprobe:walk_component { @[curtask->comm] = count() } interval:s:1000 { exit(); }'
Attaching 2 probes...
^C

@[irqbalance]: 2
@[gmain]: 3
@[in:imjournal]: 4
@[dirname]: 8
@[tty]: 8
@[uname]: 8
@[locale]: 8
@[systemd-udevd]: 12
@[dircolors]: 12
@[date]: 14
@[dbus-daemon]: 21
@[rm]: 21
@[getconf]: 24
@[ls]: 26
@[clear]: 27
@[tput]: 27
@[sed]: 28
@[ip]: 36
@[diff]: 42
@[df]: 45
@[users]: 46
@[tmux: server]: 49
@[id]: 54
@[gix::status::in]: 55
@[gix_status::ind]: 63
@[zoxide]: 69
@[mkdir]: 69
@[whoami]: 71
@[(udev-worker)]: 72
@[cat]: 74
@[wc]: 74
@[hostnamectl]: 74
@[9]: 79
@[systemd-journal]: 115
@[bc]: 180
@[direnv]: 196
@[awk]: 270
@[sqlx-sqlite-wor]: 276
@[systemd]: 324
@[grepconf.sh]: 327
@[grep]: 460
@[systemd-hostnam]: 474
@[gix::status::tr]: 607
@[atuin]: 705
@[git]: 1578
@[starship]: 1731
@[gix_status::dir]: 2680
@[(ostnamed)]: 2859
@[gitoxide.in_par]: 26556
@[zsh]: 88158
```


```txt
     __GI___clone3                                                                                                                                ▒
     start_thread                                                                                                                                 ▒
   - 0x56020664a3f2                                                                                                                               ▒
      - 97.08% wget_http_get_response_cb                                                                                                          ▒
         - 92.42% wget_tcp_read                                                                                                                   ▒
            - 90.79% wget_ssl_read_timeout                                                                                                        ▒
               - 88.31% wget_ready_2_read                                                                                                         ▒
                  - 87.66% wget_ready_2_transfer                                                                                                  ▒
                     - 86.13% __poll                                                                                                              ▒
                        - 85.64% __syscall_cancel                                                                                                 ▒
                           - 82.45% __syscall_cancel_arch_end                                                                                     ▒
                              - 66.92% entry_SYSCALL_64_after_hwframe                                                                             ▒
                                 - do_syscall_64                                                                                                  ▒
                                    - 60.38% __x64_sys_poll                                                                                       ▒
                                       - 52.82% do_sys_poll                                                                                       ▒
                                          - 26.95% do_poll.constprop.0                                                                            ▒
                                             - 11.16% sock_poll                                                                                   ▒
                                                - tcp_poll                                                                                        ▒
                                                   - 4.70% add_wait_queue                                                                         ▒
                                                        3.48% __raw_spin_lock_irqsave                                                             ▒
                                                     2.53% __pollwait                                                                             ▒
                                                     0.66% _raw_spin_unlock_irqrestore                                                            ▒
                                             - 7.71% select_estimate_accuracy                                                                     ▒
                                                - 4.29% ktime_get_ts64                                                                            ▒
                                                     3.29% read_tsc                                                                               ▒
                                               3.03% fdget                                                                                        ▒
                                               2.29% fput                                                                                         ▒
                                          + 6.75% poll_freewait                                                                                   ▒
                                            4.78% _copy_from_user                                                                                 ▒
                                            0.88% __check_object_size.part.0                                                                      ▒
                                       + 5.11% ktime_get_ts64                                                                                     ▒
                                      2.83% arch_exit_to_user_mode_prepare.isra.0
                                9.56% entry_SYSCALL_64                                                                                            ▒
                                1.36% entry_SYSCALL_64_safe_stack                                                                                 ▒
                                0.67% syscall_return_via_sysret                                                                                   ▒
                             2.68% __internal_syscall_cancel                                                                                      ▒
           2.34% nghttp2_session_mem_recv2                                                                                                        ▒
        1.49% nghttp2_session_want_write
```

简单观察，发现
```txt
@[
        tcp_poll+5
        sock_poll+81
        do_poll.constprop.0+286
        do_sys_poll+482
        __x64_sys_poll+206
        do_syscall_64+132
        entry_SYSCALL_64_after_hwframe+118
]: 1770722
```

```txt
0 S martins3 2914848       1 99  80   0 - 27328 futex_ Oct24 ?        21:26:27 wget --header User-Agent: mason.nvim v2.1.0 (+https://github.com/mason-org/mason.nvim) -o /dev/null -O /home/martins3/.local/share/nvim/mason/registries/github/mason-org/mason-registry/registry.json.zip -T 30 https://github.com/mason-org/mason-registry/releases/download/2025-10-24-lazy-chrome/registry.json.zip
0 S martins3 2916855       1 99  80   0 - 27330 futex_ Oct24 ?        21:20:32 wget --header User-Agent: mason.nvim v2.1.0 (+https://github.com/mason-org/mason.nvim) -o /dev/null -O /home/martins3/.local/share/nvim/mason/registries/github/mason-org/mason-registry/registry.json.zip -T 30 https://github.com/mason-org/mason-registry/releases/download/2025-10-24-lazy-chrome/registry.json.zip
```

pkill wget ，所有的问题都解决了。

## 2025 总结
<img width="180" height="180" alt="Image" src="https://github.com/user-attachments/assets/25f8c413-7f75-41b2-8b22-46d2c62d9512" />


## 有意思的东西
https://scottjg.com/posts/2026-05-05-egpu-mac-gaming/

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
