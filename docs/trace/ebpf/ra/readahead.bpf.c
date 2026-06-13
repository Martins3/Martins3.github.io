// readahead.bpf.c
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "GPL";

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 2048);  // 足够存所有可能值
    __type(key, __u64);         // nr_to_read 值
    __type(value, __u64);       // 计数
} nr_to_read_count SEC(".maps");

SEC("kprobe/page_cache_ra_unbounded")
int BPF_KPROBE(kprobe_page_cache_ra_unbounded,
               struct readahead_control *ractl,
               unsigned long nr_to_read,
               unsigned long lookahead_count)
{
    if (nr_to_read == 0)
        return 0;

    __u64 key = nr_to_read;
    __u64 zero = 0, *count;

    count = bpf_map_lookup_elem(&nr_to_read_count, &key);
    if (!count) {
        bpf_map_update_elem(&nr_to_read_count, &key, &zero, BPF_ANY);
        count = bpf_map_lookup_elem(&nr_to_read_count, &key);
        if (!count)
            return 0;
    }
    __sync_fetch_and_add(count, 1);

    return 0;
}
