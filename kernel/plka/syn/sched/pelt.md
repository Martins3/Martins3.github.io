# pelt.c

## 首先，找到这些注释的证据是什么 ?
1. runnable_sum 是不是可以替换掉 runnable_load_sum 和 load_sum (根本没有 runnable_sum )
2. sched_avg: avg sum runnable 四个变量


```c
//
// sched_entity:
//  
//   task:
//     se_runnable() == se_weight() // 对于 task 而言，是该结果!
// 
//   group: [ see update_cfs_group() ]
//     se_weight()   = tg->weight * grq->load_avg / tg->load_avg // 将 tg->weight 切换为 shares，其他的两个近似也需要考察一下
//     se_runnable() = se_weight(se) * grq->runnable_load_avg / grq->load_avg  // 似乎是得到其 权重的 runnable 部分
// 
//   TODO 下面四个内容在何处？
//   对于 task group 成立 ?
//   load_sum := runnable_sum
//   load_avg = se_weight(se) * runnable_avg // TODO load_avg 的计算方法是这个 ?
// 
//   runnable_load_sum := runnable_sum
//   runnable_load_avg = se_runnable(se) * runnable_avg
// 
// XXX collapse load_sum and runnable_load_sum
// 
// 似乎下面才是两个函数计算的部分
// 更新 cfs_rq 中的 sched_avg 成员:
// avg 不用求和
// cfq_rq:
// 
//   load_sum = \Sum se_weight(se) * se->avg.load_sum
//   load_avg = \Sum se->avg.load_avg
// 
//   runnable_load_sum = \Sum se_runnable(se) * se->avg.runnable_load_sum
//   runnable_load_avg = \Sum se->avg.runable_load_avg

int __update_load_avg_blocked_se(u64 now, int cpu, struct sched_entity *se)
{
  // FIXME 似乎就是 runable weight 对于 task 而言，就是 load.weight
  // 但是对于 task group 而言不同
	if (entity_is_task(se))
		se->runnable_weight = se->load.weight;

	if (___update_load_sum(now, cpu, &se->avg, 0, 0, 0)) {
		___update_load_avg(&se->avg, se_weight(se), se_runnable(se));
		return 1;
	}

	return 0;
}

int __update_load_avg_se(u64 now, int cpu, struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	if (entity_is_task(se))
		se->runnable_weight = se->load.weight;

	if (___update_load_sum(now, cpu, &se->avg, !!se->on_rq, !!se->on_rq,
				cfs_rq->curr == se)) {

		___update_load_avg(&se->avg, se_weight(se), se_runnable(se));
		cfs_se_util_change(&se->avg);
		return 1;
	}

	return 0;
}
```

```c
static inline void
enqueue_runnable_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	cfs_rq->runnable_weight += se->runnable_weight;

	cfs_rq->avg.runnable_load_avg += se->avg.runnable_load_avg;
	cfs_rq->avg.runnable_load_sum += se_runnable(se) * se->avg.runnable_load_sum;
}

static inline void
dequeue_runnable_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	cfs_rq->runnable_weight -= se->runnable_weight;

	sub_positive(&cfs_rq->avg.runnable_load_avg, se->avg.runnable_load_avg);
	sub_positive(&cfs_rq->avg.runnable_load_sum,
		     se_runnable(se) * se->avg.runnable_load_sum);
}

static inline void
enqueue_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	cfs_rq->avg.load_avg += se->avg.load_avg;
	cfs_rq->avg.load_sum += se_weight(se) * se->avg.load_sum;
}

static inline void
dequeue_load_avg(struct cfs_rq *cfs_rq, struct sched_entity *se)
{
	sub_positive(&cfs_rq->avg.load_avg, se->avg.load_avg);
	sub_positive(&cfs_rq->avg.load_sum, se_weight(se) * se->avg.load_sum);
}
```

## load_avg 的作用是什么 ?
> 计算 tg 的 share 吗 ?



## load_sum 的作用是不是为了计算 load_avg 的 ?
是的，只有一个位置有点可疑.

```c
static inline void
update_tg_cfs_runnable(struct cfs_rq *cfs_rq, struct sched_entity *se, struct cfs_rq *gcfs_rq)
```

