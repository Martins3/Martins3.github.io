//go:build ignore

#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__type(key, __u32);
	__type(value, __u64);
	__uint(max_entries, 1);
} pkt_count SEC(".maps");

// count_packets atomically increases a packet counter on every invocation.
SEC("xdp")
int count_packets()
{
	__u32 key = 0;
	__u64 *count = bpf_map_lookup_elem(&pkt_count, &key);
	if (count) {
		__sync_fetch_and_add(count, 1);
	}

	return XDP_PASS;
}

char __license[] SEC("license") = "Dual MIT/GPL";
