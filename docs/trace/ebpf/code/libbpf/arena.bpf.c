#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include "bpf_arena_common.h"
#include "arena.h"

struct {
	__uint(type, BPF_MAP_TYPE_ARENA);
	__uint(map_flags, BPF_F_MMAPABLE);
	__uint(max_entries, 10);
	__ulong(map_extra, 0x1ull << 44); /* start of mmap() region */
} arena SEC(".maps");

__u64 __arena_global userspace_value = 40;
__u64 __arena_global bpf_value;
__u64 __arena_global completed_run;

__u32 target_pid;
__u64 requested_run;
struct arena_page __arena *dynamic_page;
int allocation_failed;

SEC("raw_tp/sys_enter")
int share_arena(const void *ctx)
{
	__u32 pid = bpf_get_current_pid_tgid() >> 32;

	if (pid != target_pid || completed_run >= requested_run)
		return 0;

	if (!dynamic_page) {
		dynamic_page = bpf_arena_alloc_pages(&arena, NULL, 1,
					     NUMA_NO_NODE, 0);
		if (!dynamic_page) {
			allocation_failed = 1;
			completed_run = requested_run;
			return 0;
		}
		dynamic_page->userspace_value = 10;
	}

	bpf_value = userspace_value + 2;
	dynamic_page->bpf_value = dynamic_page->userspace_value + 1;
	completed_run = requested_run;
	return 0;
}

char _license[] SEC("license") = "GPL";
