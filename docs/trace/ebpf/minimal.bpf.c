#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "net_shared.h"
#include <linux/in.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

int my_pid = 0;
#define TASK_COMM_LEN 16

SEC("tp/syscalls/sys_enter_write")
int handle_tp(void *ctx)
{
	int pid = bpf_get_current_pid_tgid() >> 32;

	if (pid != my_pid)
		return 0;

	bpf_printk("BPF triggered from PID %d.", pid);

	return 0;
}

// 的确，非常容易触发，通过这个方法可以无感的修改一个 socket 的属性为 mptcp
SEC("fmod_ret/update_socket_protocol")
int mptcpify(int family, int type, int protocol)
{
	char comm[TASK_COMM_LEN];
	bpf_get_current_comm(&comm, sizeof(comm));

	bpf_printk("fmod_ret/update_socket_protocol by %s.", comm);
	return IPPROTO_MPTCP;
}
