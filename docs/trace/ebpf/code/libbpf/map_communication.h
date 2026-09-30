/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
#ifndef MAP_COMMUNICATION_H
#define MAP_COMMUNICATION_H

#include <linux/types.h>

struct map_command {
	__u64 addend;
};

struct mmap_state {
	__u64 userspace_value;
	__u64 bpf_value;
	__u64 runs;
};

struct map_event {
	__u32 pid;
	__u32 syscall_nr;
	__u64 hash_addend;
	__u64 array_input;
	__u64 array_output;
};

#endif
