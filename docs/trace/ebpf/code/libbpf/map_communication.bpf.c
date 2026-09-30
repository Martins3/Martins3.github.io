// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>
#include "map_communication.h"

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 16);
	__type(key, __u32);
	__type(value, struct map_command);
} commands SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, 1);
	__uint(map_flags, BPF_F_MMAPABLE);
	__type(key, __u32);
	__type(value, struct mmap_state);
} shared_array SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 4096);
} events SEC(".maps");

__u32 target_pid;
__u64 requested_run;

SEC("raw_tp/sys_enter")
int communicate(struct bpf_raw_tracepoint_args *ctx)
{
	struct map_command *command;
	struct mmap_state *state;
	struct map_event *event;
	__u32 key = 0;
	__u32 pid;

	pid = bpf_get_current_pid_tgid() >> 32;
	if (pid != target_pid)
		return 0;

	state = bpf_map_lookup_elem(&shared_array, &key);
	if (!state || state->runs >= requested_run)
		return 0;

	command = bpf_map_lookup_elem(&commands, &pid);
	if (!command)
		return 0;

	state->bpf_value = state->userspace_value + command->addend;
	state->runs++;

	event = bpf_ringbuf_reserve(&events, sizeof(*event), 0);
	if (!event)
		return 0;

	event->pid = pid;
	event->syscall_nr = ctx->args[1];
	event->hash_addend = command->addend;
	event->array_input = state->userspace_value;
	event->array_output = state->bpf_value;
	bpf_ringbuf_submit(event, 0);
	return 0;
}

char LICENSE[] SEC("license") = "Dual BSD/GPL";
