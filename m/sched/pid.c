// 配套文档 :
// docs/kernel/sched/sched-pid-basic.md
// docs/kernel/sched/sched-pid.md
//
// 源码 include/linux/pid.h 开头的日志也是不错的
#include "internal.h"
#include <linux/pid.h>
#include <linux/sched/task.h>
#include <linux/sched/signal.h>

// 各种转换
//
// get_pid_task
// get_task_pid
// find_get_pid : 获取的 pid 结构体
// 获取到 task_active_pid_ns
//
// enum pid_type {
// 	PIDTYPE_PID,
// 	PIDTYPE_TGID,
// 	PIDTYPE_PGID,
// 	PIDTYPE_SID,
// 	PIDTYPE_MAX,
// };

// TODO 测试一下 namespace 下各个 pid

static void get_name(struct task_struct *task)
{
	struct pid *pid;
	for (enum pid_type type = 0; type < PIDTYPE_MAX; type++) {
		pid = get_task_pid(task, type);
		task = get_pid_task(pid, PIDTYPE_PID);
		pr_info("%d : %s\n", type, task->comm);
	}
}
static void test_pid_ns(void)
{
	pr_info("pid=%d\n", current->pid);
	pr_info("tgid=%d\n", current->tgid);

	// TODO 我以为 session 是 tmux 之类的，
	// process group 也是非预期的
	//
	// [ 2081.120352] 0 : child
	// [ 2081.120410] 1 : pid.out
	// [ 2081.120517] 2 : pid.out
	// [ 2081.120568] 3 : sudo
	get_name(current);
}

// TODO 似乎遍历所有的 thread 的接口已经消失了
static void iter_all_thread(void)
{
}

int test_pid(long action)
{
	switch (action) {
	case 0:
		test_pid_ns();
		break;
	case 1:
		iter_all_thread();
		break;
	}
	return 0;
}
