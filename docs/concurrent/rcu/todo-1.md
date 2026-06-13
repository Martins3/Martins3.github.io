## 找到为什么需要使用 rcu_barrier 的地方

## 为什么 softirq 中执行的 RCU callback 是什么?

```txt
🧀  cat /proc/softirqs
                    CPU0       CPU1       CPU2       CPU3
          HI:          0          0          0          0
       TIMER:     165934     214905     234256     140886
      NET_TX:        102         41         40        123
      NET_RX:     108032    5830758    5819179      84390
       BLOCK:          0          0          0          0
    IRQ_POLL:          0          0          0          0
     TASKLET:        337        857         45        833
       SCHED:     802183     448203     529477     289775
     HRTIMER:          0          0          0          0
         RCU:     136717     122756     127614     120001
```

## 为什么叫做 irq_exit_rcu ，中断和 rcu 什么关系

```c
/**
 * irq_exit_rcu() - Exit an interrupt context without updating RCU
 *
 * Also processes softirqs if needed and possible.
 */
void irq_exit_rcu(void)
{
	__irq_exit_rcu();
	 /* must be last! */
	lockdep_hardirq_exit();
}
```
和其相对应的函数叫做 irq_exit

似乎没有人调用 irq_exit

他们的区别在于 : ct_irq_exit ，这个就是之前一直没有看懂的

kernel/context_tracking.c


## 观察一个事情，有时候看到 rcu_read_lock 就很容易知道
但是有时候并不是如此，例如

1. mm/rmap.c 中的 anon_vma_cachep 使用
SLAB_TYPESAFE_BY_RCU

## 看看这个
https://mp.weixin.qq.com/s/is1XID2rSWy3vnd0rmhEJA

## 这个 kthread 是做什么的
```txt
[root@localhost 15:05:16 ~]$ ps -elf | grep kworker | grep rcu
1 I root           4       2  0  60 -20 -     0 rescue 14:37 ?        00:00:00 [kworker/R-rcu_g]
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
