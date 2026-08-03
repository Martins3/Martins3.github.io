#include <linux/sched/clock.h>
#include <linux/sched/cputime.h>
#include <linux/sched/debug.h>
#include <linux/sched/isolation.h>
#include <linux/sched/loadavg.h>
#include <linux/sched/nohz.h>
#include <linux/sched/mm.h>
#include <linux/sched/rseq_api.h>
#include <linux/sched/task_stack.h>

#include <linux/cpufreq.h>
#include <linux/cpumask_api.h>
#include <linux/cpuset.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/energy_model.h>
#include <linux/hashtable_api.h>
#include <linux/irq.h>
#include <linux/kobject_api.h>
#include <linux/membarrier.h>
#include <linux/mempolicy.h>
#include <linux/nmi.h>
#include <linux/nospec.h>
#include <linux/proc_fs.h>
#include <linux/psi.h>
#include <linux/ptrace_api.h>
#include <linux/sched_clock.h>
#include <linux/security.h>
#include <linux/spinlock_api.h>
#include <linux/swait_api.h>
#include <linux/timex.h>
#include <linux/utsname.h>
#include <linux/wait_api.h>
#include <linux/workqueue_api.h>

#include <uapi/linux/prctl.h>
#include <uapi/linux/sched/types.h>

#include <asm/switch_to.h>

#include "sched.h"
#include "sched-pelt.h"
#include "stats.h"
#include "autogroup.h"
// SPDX-License-Identifier: GPL-2.0
/*
 * martins3-task scheduling class.
 *
 * The martins3 task is the highest priority task in the system, it preempts
 * everything and will be preempted by nothing.
 *
 * See kernel/martins3_machine.c
 */

#ifdef CONFIG_SMP
static int select_task_rq_martins3(struct task_struct *p, int cpu, int flags)
{
	return task_cpu(p); /* martins3 tasks as never migrate */
}

static int balance_martins3(struct rq *rq, struct task_struct *prev,
			    struct rq_flags *rf)
{
	return 0;
}
#endif /* CONFIG_SMP */

static void wakeup_preempt_martins3(struct rq *rq, struct task_struct *p,
				    int flags)
{
	/* we're never preempted */
}

static void set_next_task_martins3(struct rq *rq, struct task_struct *martins3,
				   bool first)
{
	martins3->se.exec_start = rq_clock_task(rq);
}

static struct task_struct *pick_task_martins3(struct rq *rq)
{
	return NULL;
}

static struct task_struct *pick_next_task_martins3(struct rq *rq,
						   struct task_struct *prev)
{
	struct task_struct *p = pick_task_martins3(rq);

	if (p)
		set_next_task_martins3(rq, p, true);

	return p;
}

static void enqueue_task_martins3(struct rq *rq, struct task_struct *p,
				  int flags)
{
	add_nr_running(rq, 1);
}

static bool dequeue_task_martins3(struct rq *rq, struct task_struct *p,
				  int flags)
{
	sub_nr_running(rq, 1);
	return true;
}

static void yield_task_martins3(struct rq *rq)
{
	BUG(); /* the martins3 task should never yield, its pointless. */
}

static void put_prev_task_martins3(struct rq *rq, struct task_struct *prev,
				   struct task_struct *next)
{
	update_curr_common(rq);
}

/*
 * scheduler tick hitting a task of our scheduling class.
 *
 * NOTE: This function can be called remotely by the tick offload that
 * goes along full dynticks. Therefore no local assumption can be made
 * and everything must be accessed through the @rq and @curr passed in
 * parameters.
 */
static void task_tick_martins3(struct rq *rq, struct task_struct *curr,
			       int queued)
{
}

static void switched_to_martins3(struct rq *rq, struct task_struct *p)
{
	BUG(); /* its impossible to change to this class */
}

static void prio_changed_martins3(struct rq *rq, struct task_struct *p,
				  int oldprio)
{
	BUG(); /* how!?, what priority? */
}

static void update_curr_martins3(struct rq *rq)
{
}

/*
 * Simple, special scheduling class for the per-CPU martins3 tasks:
 */
DEFINE_SCHED_CLASS(martins3) = {

	.enqueue_task = enqueue_task_martins3,
	.dequeue_task = dequeue_task_martins3,
	.yield_task = yield_task_martins3,

	.wakeup_preempt = wakeup_preempt_martins3,

	.pick_next_task = pick_next_task_martins3,
	.put_prev_task = put_prev_task_martins3,
	.set_next_task = set_next_task_martins3,

#ifdef CONFIG_SMP
	.balance = balance_martins3,
	.pick_task = pick_task_martins3,
	.select_task_rq = select_task_rq_martins3,
	.set_cpus_allowed = set_cpus_allowed_common,
#endif

	.task_tick = task_tick_martins3,

	.prio_changed = prio_changed_martins3,
	.switched_to = switched_to_martins3,
	.update_curr = update_curr_martins3,
};
