// SPDX-License-Identifier: BSD-3-Clause
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 10);
	__uint(map_flags, BPF_F_MMAPABLE);
	__type(key, int);
	__type(value, int);
} my_map SEC(".maps");

SEC("socket")
int my_socket_prog(struct __sk_buff *skb)
{
	int key = 0;
	int value = 572662306; // "22 22 22 22"
	int err = 0;

	// Write value into map
	bpf_map_update_elem(&my_map, &key, &value, BPF_ANY);
	// TODO 应该可以容易获取 export 到用户态吧

	bpf_printk("BPF triggered\n");
	return 0;
}
