# 中断分析的工具

## 触发的 hi 和 tasklet 的
```sh
sudo bpftrace -e "kprobe:__tasklet_hi_schedule { @[kstack] = count(); }"
sudo bpftrace -e "kprobe:__tasklet_schedule { @[kstack] = count(); }"
```

## sudo perf top -e irq:softirq_entry

然后使用 iperf 测试:
```txt
Samples: 4M of event 'irq:softirq_entry', 1 Hz, Event count (approx.): 968755 lost: 0/0 drop: 0/0
Overhead  Trace output
  48.91%  vec=3 [action=NET_RX]
  46.43%  vec=6 [action=TASKLET]
   3.16%  vec=2 [action=NET_TX]
   0.67%  vec=7 [action=SCHED]
   0.40%  vec=0 [action=HI]
   0.22%  vec=9 [action=RCU]
   0.20%  vec=1 [action=TIMER]
```

tasklet_hi_action 是另外的东西，action=HI ，但是都是谁在执行?

## bcc softirqs

在 mac 中测试的，没有负载的时候:

sudo ./softirqs 1 10
```txt
SOFTIRQ          TOTAL_usecs
net_tx                    20
block                    485
tasklet                  892
timer                    996
rcu                     1227
net_rx                  1395
sched                   2772
```

使用 funccount 测试，发现 `__tasklet_schedule` 完全有调用，
而 `__tasklet_hi_schedule` 则是完全没有
```txt
FUNC                                    COUNT
__tasklet_schedule                         10
```

- [ ] 不知道这个 bcc 的 softirq 是如何统计的，和实际触发的差别很大。

## 获取到底执行了那些 callback

使用  sudo perf top -e irq:tasklet_entry + cat /proc/kallsyms 可以知道是那两个函数:
tasklet_entry
```txt
  51.20%  tasklet=0xffff5c94c3e10c08 function=0xffffa9b644b0fe78
  48.80%  tasklet=0xffff5c94c313f060 function=0xffffa9b644b35a50
```
```txt
ffffa9b644b0fe78 t usbnet_bh_tasklet	[usbnet]
ffffa9b644b35a50 t cdc_ncm_txpath_bh	[cdc_ncm]
```

## docs/kernel/sysfs-irq.md

/proc/softirq 和 /proc/stat 就是了

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
