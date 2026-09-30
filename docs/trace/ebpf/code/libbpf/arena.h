/* SPDX-License-Identifier: (GPL-2.0-only OR BSD-2-Clause) */
#ifndef ARENA_DEMO_H
#define ARENA_DEMO_H

#include <linux/types.h>

struct arena_page {
	__u64 userspace_value;
	__u64 bpf_value;
};

#endif
