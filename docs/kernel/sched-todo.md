# sched 相关的问题
- io_schedule 和 schedule 有什么区别，或者说，当我已经知道可以开始因为 io schedule 的时候，需要什么额外操作吗？


## 分析下这里的输出

```txt
🧀  cat /proc/self/sched
cat (94551, #threads: 1)
-------------------------------------------------------------------
se.exec_start                                :       3025831.263037
se.vruntime                                  :       6246999.883286
se.sum_exec_runtime                          :             0.387002
se.nr_migrations                             :                    0
nr_switches                                  :                    0
nr_voluntary_switches                        :                    0
nr_involuntary_switches                      :                    0
se.load.weight                               :              1048576
se.avg.load_sum                              :                47211
se.avg.runnable_sum                          :             24365568
se.avg.util_sum                              :             24365568
se.avg.load_avg                              :                 1024
se.avg.runnable_avg                          :                  512
se.avg.util_avg                              :                  512
se.avg.last_update_time                      :        3025831179264
se.avg.util_est.ewma                         :                    0
se.avg.util_est.enqueued                     :                    0
policy                                       :                    0
prio                                         :                  120
clock-delta                                  :                   21
```

## 这里的内容也分析下

```txt
🧀  cat /proc/113281/schedstat
6292628058 35755133 1778
```

## 文摘
https://news.ycombinator.com/item?id=21919988

## htop 中的 CPU % 是怎么得到的

## 之前的几个问题
```c
struct sched_entity {
	/* For load-balancing: */
	struct load_weight		load;
	unsigned long			runnable_weight; // 难道 bandwidth 使用的 ?
	struct rb_node			run_node;
	struct list_head		group_node; // task group ?
	unsigned int			on_rq; // why not boolean ?

  // @todo how runtime works ?
	u64				exec_start;
	u64				sum_exec_runtime;
	u64				vruntime;
	u64				prev_sum_exec_runtime;

	u64				nr_migrations;
```
> 1. load 和 runnable_weight 之间的关系是什么 ?
