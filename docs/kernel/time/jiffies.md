## 更新 jiffies_64 的路径

```txt
@[
    tick_do_update_jiffies64+1
    tick_irq_enter+93
    sysvec_apic_timer_interrupt+103
    asm_sysvec_apic_timer_interrupt+26
    cpuidle_enter_state+205
    cpuidle_enter+45
    do_idle+474
    cpu_startup_entry+42
    start_secondary+286
    secondary_startup_64_no_verify+388
]: 469
@[
    tick_do_update_jiffies64+1
    tick_nohz_restart_sched_tick+27
    tick_nohz_idle_exit+137
    do_idle+344
    cpu_startup_entry+42
    start_secondary+286
    secondary_startup_64_no_verify+388
]: 1453
```

## jiffies 只要一个 CPU 更新就可以了吧 ?

基本测试下，就是如果没有负载，那么有中断就更新，所以会超过 1000 ，如果有负载，那么会切换的模式，
不会出现互相覆盖的情况。

```sh
sudo funccount tick_do_update_jiffies64 -i 1
```

如果 idle ，那么就会使用，否则就是
```txt
FUNC                                    COUNT
tick_do_update_jiffies64                 2531

FUNC                                    COUNT
tick_do_update_jiffies64                 2754

FUNC                                    COUNT
tick_do_update_jiffies64                 2404
```

但是给系统之后，
```txt
FUNC                                    COUNT
tick_do_update_jiffies64                 1000

FUNC                                    COUNT
tick_do_update_jiffies64                 1001
```

```txt
sudo bpftrace -e 'kprobe:tick_do_update_jiffies64 { @[cpu] = count();} interval:s:1 { exit(); }'
Attaching 2 probes...


@[9]: 2
@[13]: 2
@[1]: 2
@[7]: 2
@[25]: 4
@[27]: 4
@[15]: 4
@[17]: 4
@[18]: 4
@[19]: 5
@[26]: 5
@[24]: 5
@[20]: 5
@[22]: 6
@[23]: 6
@[3]: 6
@[21]: 6
@[28]: 7
@[31]: 7
@[16]: 9
@[5]: 22
@[0]: 32
@[30]: 35
@[4]: 43
@[2]: 62
@[29]: 65
@[8]: 77
@[12]: 83
@[10]: 128
@[6]: 160
@[11]: 234
@[14]: 964
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
