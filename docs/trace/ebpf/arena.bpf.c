#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "bpf_arena_common.h"

struct {
	__uint(type, BPF_MAP_TYPE_ARENA);
	__uint(map_flags, BPF_F_MMAPABLE);
	__uint(max_entries, 10);
	__ulong(map_extra, 0x1ull << 44); /* start of mmap() region */
} arena SEC(".maps");

__u64 __arena_global add64_value = 1;
__u64 __arena_global add64_result = 0;

SEC("raw_tp/sys_enter")
int add(const void *ctx)
{
	add64_result = __sync_fetch_and_add(&add64_value, 2);
	return 0;
}

char _license[] SEC("license") = "GPL";
