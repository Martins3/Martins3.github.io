// SPDX-License-Identifier: GPL-2.0 OR BSD-2-Clause

#include "trace.h"
#include <linux/types.h>
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

const volatile unsigned int target_pid = 0;

/* Keep the shared event layout in the generated Rust skeleton. */
struct event _event = {};

struct trace_event_raw_sys_enter {
    unsigned long long unused;
    long syscall_nr;
    unsigned long args[6];
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

SEC("tracepoint/syscalls/sys_enter_write")
int trace_write(struct trace_event_raw_sys_enter *ctx)
{
    unsigned int pid = bpf_get_current_pid_tgid() >> 32;
    int fd = (int)ctx->args[0];
    struct event *event;

    /* Avoid reporting this program's own terminal output. */
    if (fd <= 2 || (target_pid && target_pid != pid))
        return 0;

    event = bpf_ringbuf_reserve(&events, sizeof(*event), 0);
    if (!event)
        return 0;

    event->pid = pid;
    event->fd = fd;
    event->count = ctx->args[2];
    bpf_get_current_comm(event->comm, sizeof(event->comm));
    bpf_ringbuf_submit(event, 0);
    return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
