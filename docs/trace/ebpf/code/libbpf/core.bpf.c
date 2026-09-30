// SPDX-License-Identifier: GPL-2.0 OR BSD-3-Clause
#include <vmlinux.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>

struct core_result {
	__u32 tgid_exists;
	__u32 tgid_offset;
};

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__type(key, __u32);
	__type(value, struct core_result);
} results SEC(".maps");

SEC("cgroup_skb/egress")
int record_target_layout(struct __sk_buff *skb)
{
	struct core_result result = {};
	__u32 key = 0;

	result.tgid_exists = bpf_core_field_exists(struct task_struct, tgid);
	result.tgid_offset = bpf_core_field_offset(struct task_struct, tgid);
	bpf_map_update_elem(&results, &key, &result, BPF_ANY);
	return 1;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
