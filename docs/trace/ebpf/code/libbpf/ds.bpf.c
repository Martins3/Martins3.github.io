#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

int my_pid = 0;

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 10);
	__type(key, int);
	__type(value, int);
} my_map SEC(".maps");


SEC("tp/syscalls/sys_enter_write")
int handle_tp(void *ctx)
{
	// bpf_map_update_elem(&my_map, &key, &value, BPF_ANY);

	int pid = bpf_get_current_pid_tgid() >> 32;
	bpf_printk("BPF triggered from PID %d.\n", pid);

	int *value = bpf_map_lookup_elem(&my_map, &pid);
	if (value)
		__sync_fetch_and_add(value, *value + 1);

	return 0;
}
