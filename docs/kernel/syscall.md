# syscall

- [ ] https://unix.stackexchange.com/questions/612506/is-it-safe-to-restart-system-calls

- [ ] https://www.linuxjournal.com/content/creating-vdso-colonels-other-chicken : 甚至需要自己创建出来一个 vdso

## TODO
- [ ] 把 x86 dune 整理一下吧，移除没有用的 patch, 复现一下内容


## MIPS 的 vdso 的实现
- [x] `vdso_data.pcpu_data[cpu_id].pid = next->tgid;`，可以采用 fallback 机制的
- [ ] 如果只要发生了上下文切换，然后就刷新 vdso_data, 那么 pid 就是永远都是正确的
  - 除非是没有正确的刷新 timerid 数值

1. 当从 CPU A 切换到 CPU B 中，还是进行了 context switch

- [x] 为什么读出数值为 0 的情况啊 ? 可能是 idle 进程的代码吧!

```c
static __always_inline pid_t __do_getpid(const union loongarch_vdso_data *data)
{
	u32 start_seq;
	u32 cpu_id;
	pid_t pid;

	do {
		cpu_id = read_cpu_id();
		start_seq = vdso_pcpu_data_read_begin(&data->pcpu_data[cpu_id]);
		pid = data->pcpu_data[cpu_id].pid;

	} while (cpu_id != read_cpu_id() ||
		vdso_pcpu_data_read_retry(&data->pcpu_data[cpu_id], start_seq));

    return pid;
}
```

```c
void vdso_per_cpu_switch_thread(struct task_struct *prev,
	struct task_struct *next)
{
	unsigned int cpu_id;

	cpu_id = (current_thread_info())->cpu;

	if (next->cred->user_ns != &init_user_ns) {
		vdso_data.pcpu_data[cpu_id].uid = VDSO_INVALID_UID;
	} else {
		vdso_data.pcpu_data[cpu_id].uid = task_uid(next).val;
	}

	if (!pid_alive(next) ||
		next->thread_pid->numbers[next->thread_pid->level].ns !=
			&init_pid_ns) {
		vdso_data.pcpu_data[cpu_id].pid = VDSO_INVALID_PID;
	} else {
		vdso_data.pcpu_data[cpu_id].pid = next->tgid;
	}

	vdso_pcpu_data_update_seq(&vdso_data.pcpu_data[cpu_id]);
}
```

## inside
/home/maritns3/core/vn/kernel/insides/syscall.md 整理一下

## 整理这个 issue
https://github.com/Martins3/Martins3.github.io/issues/8
