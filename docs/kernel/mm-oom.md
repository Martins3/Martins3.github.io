# oom

核心结构体 `oom_control`，主要记录事发现场。

主要的入口为 `out_of_memory`，调用着有三个
- `mmecontrol:mem_cgroup_out_of_memory` : 用户程序分配内存的时候，经过 cgroup 的检查 `mem_cgroup_charge` 没有通过。
- `page_alloc.c:__alloc_pages_may_oom` : 在此处失败，是因为不受 cgroup 管理的用户进程分配内存失败。
- `sysrq:moom_callback` : 通过  sudo echo f > /proc/sysrq-trigger 手动触发

1. 为什么 oom 会因为 cpuset ？
```c
struct oom_control {
	/* Used to determine cpuset */
	struct zonelist *zonelist;
```
因为该进程运行执行的 node 上没有内存了。

2. reaper 是做啥的?

在 `oom_kill_process` 中，将那些**已经被杀死**进程持有的内存直接释放掉[^1]。

<!-- ## TODO
- Taming the OOM killer : https://lwn.net/Articles/317814/
- https://superuser.com/questions/1150215/disabling-oom-killer-on-ubuntu-14-04
- 为什么 OOM 的调用时间那么长
-->

## 一次 oom 可以告诉我们什么
5.10.0-60.18.0.50.oe2203.x86_64
```txt
[  112.939900] Out of memory: Killed process 5515 (a.out) total-vm:22022692kB, anon-rss:10875852kB, file-rss:1336kB, shmem-rss:0kB, UID:0 pgtables:41296kB oom_score_adj:0
[  113.314803] oom_reaper: reaped process 5515 (a.out), now anon-rss:0kB, file-rss:0kB, shmem-rss:0kB
[  130.772276] a.out invoked oom-killer: gfp_mask=0x100cca(GFP_HIGHUSER_MOVABLE), order=0, oom_score_adj=0
[  130.773033] CPU: 17 PID: 5532 Comm: a.out Kdump: loaded Not tainted 5.10.0-60.18.0.50.oe2203.x86_64 #1
[  130.773692] Hardware name: Martins3 Inc Hacking Alpine, BIOS 12 2022-2-2
[  130.774184] Call Trace:
[  130.774403]  dump_stack+0x57/0x6a
[  130.774676]  dump_header+0x4a/0x1f0
[  130.774954]  oom_kill_process.cold+0xb/0x10
[  130.775276]  out_of_memory+0x100/0x310
[  130.775569]  __alloc_pages+0xe78/0xf50
[  130.775866]  pagecache_get_page+0x1cc/0x380
[  130.776188]  filemap_fault+0x2f1/0x510
[  130.776494]  ext4_filemap_fault+0x2d/0x40 [ext4]
[  130.776850]  __do_fault+0x38/0x110
[  130.777122]  do_read_fault+0x31/0xc0
[  130.777405]  do_fault+0x71/0x150
[  130.777667]  __handle_mm_fault+0x3dd/0x6d0
[  130.777983]  ? _copy_from_user+0x3c/0x80
[  130.778287]  handle_mm_fault+0xbe/0x290
[  130.778586]  exc_page_fault+0x273/0x550
[  130.778887]  ? asm_exc_page_fault+0x8/0x30
[  130.779546]  asm_exc_page_fault+0x1e/0x30
[  130.779872] RIP: 0033:0x4011cb
[  130.780126] Code: Unable to access opcode bytes at RIP 0x4011a1.
[  130.780557] RSP: 002b:00007ffdb61e5780 EFLAGS: 00010206
[  130.780944] RAX: 00007f48aad81000 RBX: 0000000000000000 RCX: 00007f4b41007887
[  130.781455] RDX: 00007f4880f08000 RSI: 0000000000000001 RDI: 00007f4b410fd570
[  130.781969] RBP: 00007ffdb61e57a0 R08: 00007f4b410fd570 R09: 000000000000000b
[  130.782482] R10: 0000000000000000 R11: 0000000000000246 R12: 00007ffdb61e58b8
[  130.782997] R13: 0000000000401142 R14: 0000000000403e08 R15: 00007f4b41150000
[  130.783519] Mem-Info:
[  130.783726] active_anon:285 inactive_anon:2820935 isolated_anon:0
                active_file:28 inactive_file:59 isolated_file:0
                unevictable:2873 dirty:29 writeback:0
                slab_reclaimable:8117 slab_unreclaimable:18581
                mapped:2936 shmem:2213 pagetables:6126 bounce:0
                free:40346 free_pcp:62 free_cma:0
[  130.786048] Node 0 active_anon:1140kB inactive_anon:11283740kB active_file:112kB inactive_file:236kB unevictable:11492kB isolated(anon):0kB isolated(file):0kB mapped:11744kB dirty:116kB writeback:0kB shmem:8852kB shmem_thp: 0kB shmem_pmdmapped: 0kB anon_thp: 10727424kB writeback_tmp:0kB kernel_stack:6896kB all_unreclaimable? yes
[  130.787958] Node 0 DMA free:13296kB min:144kB low:180kB high:216kB reserved_highatomic:0KB active_anon:0kB inactive_anon:0kB active_file:0kB inactive_file:0kB unevictable:0kB writepending:0kB present:15992kB managed:15360kB mlocked:0kB pagetables:0kB bounce:0kB free_pcp:0kB local_pcp:0kB free_cma:0kB
[  130.789699] lowmem_reserve[]: 0 2412 11403 11403 11403
[  130.790083] Node 0 DMA32 free:59900kB min:24044kB low:30052kB high:36060kB reserved_highatomic:0KB active_anon:0kB inactive_anon:2439692kB active_file:172kB inactive_file:84kB unevictable:0kB writepending:0kB present:3129204kB managed:2503388kB mlocked:0kB pagetables:80kB bounce:0kB free_pcp:248kB local_pcp:248kB free_cma:0kB
[  130.791976] lowmem_reserve[]: 0 0 8991 8991 8991
[  130.792325] Node 0 Normal free:88188kB min:88444kB low:110552kB high:132660kB reserved_highatomic:0KB active_anon:1140kB inactive_anon:8843776kB active_file:268kB inactive_file:68kB unevictable:11492kB writepending:116kB present:9437184kB managed:9207308kB mlocked:11492kB pagetables:24424kB bounce:0kB free_pcp:0kB local_pcp:0kB free_cma:0kB
[  130.794295] lowmem_reserve[]: 0 0 0 0 0
[  130.794597] Node 0 DMA: 0*4kB 0*8kB 1*16kB (U) 1*32kB (U) 1*64kB (U) 1*128kB (U) 1*256kB (U) 1*512kB (U) 0*1024kB 2*2048kB (UM) 2*4096kB (M) = 13296kB
[  130.795862] Node 0 DMA32: 14*4kB (UM) 46*8kB (U) 50*16kB (UME) 40*32kB (UE) 31*64kB (UE) 25*128kB (UME) 17*256kB (UME) 17*512kB (UM) 39*1024kB (UME) 0*2048kB 0*4096kB = 60680kB
[  130.796931] Node 0 Normal: 1371*4kB (UME) 343*8kB (UE) 600*16kB (UE) 394*32kB (UME) 222*64kB (UME) 153*128kB (UME) 60*256kB (UE) 19*512kB (UME) 3*1024kB (U) 0*2048kB 0*4096kB = 92388kB
[  130.798040] Node 0 hugepages_total=0 hugepages_free=0 hugepages_surp=0 hugepages_size=1048576kB
[  130.798654] Node 0 hugepages_total=0 hugepages_free=0 hugepages_surp=0 hugepages_size=2048kB
[  130.799250] 4778 total pagecache pages
[  130.799543] 0 pages in swap cache
[  130.799816] Swap cache stats: add 2567968, delete 2567967, find 5630/8293
[  130.800313] Free swap  = 0kB
[  130.800553] Total swap = 0kB
[  130.800796] 3145595 pages RAM
[  130.801042] 0 pages HighMem/MovableOnly
[  130.801341] 214081 pages reserved
[  130.801608] 0 pages hwpoisoned
[  130.801864] Tasks state (memory values in pages):
[  130.802219] [  pid  ]   uid  tgid total_vm      rss pgtables_bytes swapents oom_score_adj name
[  130.802859] [    784]     0   784     6662     1221    77824        0          -250 systemd-journal
[  130.803502] [    818]     0   818     7430     1714    77824        0         -1000 systemd-udevd
[  130.804128] [    964]    32   964     2722     1139    61440        0             0 rpcbind
[  130.804724] [   1007]     0  1007      812       27    45056        0             0 mdadm
[  130.805303] [   1022]     0  1022     4442      290    49152        0         -1000 auditd
[  130.805892] [   1128]    81  1128     2187      770    53248        0          -900 dbus-daemon
[  130.806504] [   1133]   997  1133      678      450    45056        0             0 lsmd
[  130.807084] [   1137]     0  1137    19916       87    57344        0          -500 irqbalance
[  130.807694] [   1141]   987  1141    58815      991    90112        0             0 polkitd
[  130.808292] [   1149]     0  1149    77789     1292   110592        0             0 rngd
[  130.808873] [   1154]   984  1154    19541      410    61440        0             0 chronyd
[  130.809464] [   1157]     0  1157     4193     1177    69632        0             0 systemd-logind
[  130.810097] [   1161]     0  1161     3573      739    65536        0             0 systemd-machine
[  130.810735] [   1163]     0  1163     2786      939    65536        0             0 restorecond
[  130.811621] [   1204]     0  1204    36925     7350   176128        0             0 firewalld
[  130.812293] [   1209]     0  1209    86700     1758   151552        0             0 NetworkManager
[  130.812927] [   1228]     0  1228     3476     1486    69632        0         -1000 sshd
[  130.813502] [   1232]     0  1232    52252     5367   151552        0             0 targetclid
[  130.814114] [   1234]     0  1234    69191     3357   143360        0             0 tuned
[  130.814696] [   1235]     0  1235    17609      578    90112        0             0 gssproxy
[  130.815292] [   1513]     0  1513     2256      922    53248        0             0 dhclient
[  130.815890] [   1531]     0  1531     2890     2819    61440        0           -17 iscsid
[  130.816475] [   1534]     0  1534    57333     1142    90112       34             0 rsyslogd
[  130.817073] [   1574]     0  1574      947      526    45056        0             0 atd
[  130.817641] [   1576]     0  1576     5845      686    69632        0             0 crond
[  130.818219] [   1582]     0  1582     5435      416    53248        0             0 agetty
[  130.818803] [   1584]     0  1584     5341      472    57344        0             0 agetty
[  130.819387] [   2033]   992  2033     2188      275    57344        0             0 dnsmasq
[  130.819979] [   2034]     0  2034     2181       90    57344        0             0 dnsmasq
[  130.820565] [   5259]     0  5259     3970     1603    73728        0             0 sshd
[  130.821139] [   5264]     0  5264     4637     1624    77824        0             0 systemd
[  130.821729] [   5267]     0  5267     6025     1512    90112        0             0 (sd-pam)
[  130.822323] [   5273]     0  5273    21817      597    73728        0             0 gcr-ssh-agent
[  130.822948] [   5274]     0  5274     3934      891    73728        0             0 sshd
[  130.823521] [   5276]     0  5276     6879     1214    69632        0             0 zsh
[  130.824089] [   5406]     0  5406     3970     1597    69632        0             0 sshd
[  130.824663] [   5409]     0  5409     3934      908    69632       30             0 sshd
[  130.825237] [   5410]     0  5410     6915     1253    69632        0             0 zsh
[  130.825810] [   5532]     0  5532  2884233  2793370 22437888        0             0 a.out
[  130.826389] oom-kill:constraint=CONSTRAINT_NONE,nodemask=(null),cpuset=/,mems_allowed=0,global_oom,task_memcg=/user.slice/user-0.slice/session-3.scope,task=a.out,pid=5532,uid=0
[  130.827731] Out of memory: Killed process 5532 (a.out) total-vm:11536932kB, anon-rss:11172224kB, file-rss:1256kB, shmem-rss:0kB, UID:0 pgtables:21912kB oom_score_adj:0
[  130.843472] oom_reaper: reaped process 5532 (a.out), now anon-rss:0kB, file-rss:0kB, shmem-rss:0kB
```

## 在 guest 中 gdb 调试，结果导致 double fault
```txt
#0  native_halt () at ./arch/x86/include/asm/irqflags.h:67
#1  0xffffffff8105ce45 in halt () at ./arch/x86/include/asm/paravirt.h:99
#2  kvm_wait (ptr=0xffffffff82ab060c <logbuf_lock> "\003", val=3 '\003') at arch/x86/kernel/kvm.c:798
#3  0xffffffff811083f1 in pv_wait (val=<optimized out>, ptr=<optimized out>) at ./arch/x86/include/asm/paravirt.h:689
#4  pv_wait_head_or_lock (node=<optimized out>, lock=<optimized out>) at kernel/locking/qspinlock_paravirt.h:469
#5  __pv_queued_spin_lock_slowpath (lock=0xffffffff82ab060c <logbuf_lock>, val=<optimized out>) at kernel/locking/qspinlock.c:541
#6  0xffffffff818c9a7d in pv_queued_spin_lock_slowpath (val=<optimized out>, lock=<optimized out>) at ./arch/x86/include/asm/paravirt.h:679
#7  queued_spin_lock_slowpath (val=<optimized out>, lock=<optimized out>) at ./arch/x86/include/asm/qspinlock.h:58
#8  queued_spin_lock (lock=<optimized out>) at ./include/asm-generic/qspinlock.h:88
#9  do_raw_spin_lock (lock=<optimized out>) at ./include/linux/spinlock.h:180
#10 __raw_spin_lock (lock=<optimized out>) at ./include/linux/spinlock_api_smp.h:143
#11 _raw_spin_lock (lock=0xffffffff82ab060c <logbuf_lock>) at kernel/locking/spinlock.c:144
#12 0xffffffff81116fcb in vprintk_emit (facility=<optimized out>, level=3, dict=0xffff88807ffac300 "\f\006\253\202\377\377\377\377\200\065r}\200\210\3
77\377", dictlen=8, fmt=0x8 <irq_stack_union+8> <error: Cannot access memory at address 0x8>, args=0x44 <irq_stack_union+68>) at kernel/printk/printk.
c:1968
#13 0xffffffff8111761c in vprintk_deferred (fmt=<optimized out>, args=<optimized out>) at kernel/printk/printk.c:2997
#14 0xffffffff811179ce in printk_deferred (fmt=<optimized out>) at kernel/printk/printk.c:3009
#15 0xffffffff81024057 in get_stack_info (stack=0xfffffe0000217000, task=0xffff88800b978000, info=0xfffffe0000217e70, visit_mask=0xfffffe0000217e68) a
t arch/x86/kernel/dumpstack_64.c:140
#16 0xffffffff81024929 in show_trace_log_lvl (task=0xffff88800b978000, regs=0x0 <irq_stack_union>, stack=0xfffffe0000217000, log_lvl=<optimized out>)
at arch/x86/kernel/dumpstack.c:196
#17 0xffffffff81024ce3 in show_regs (regs=<optimized out>) at arch/x86/kernel/dumpstack.c:418
#18 0xffffffff810595ed in df_debug (regs=0xfffffe0000217f58, error_code=<optimized out>) at arch/x86/kernel/doublefault.c:80
#19 0xffffffff810218ef in do_double_fault (regs=0xffffffff82ab060c <logbuf_lock>, error_code=0) at arch/x86/kernel/traps.c:444
#20 0xffffffff81a00d2e in double_fault () at arch/x86/entry/entry_64.S:1039
#21 0x0000000000000000 in ?? ()
```

## oomd 源码分析
- https://github.com/facebookincubator/oomd : 了解这个工具的原理

## 其他参考
- http://linux.laoqinren.net/linux/out-of-memory/

[^1]: https://lwn.net/Articles/666024/
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
