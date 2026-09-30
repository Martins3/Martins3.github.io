// SPDX-License-Identifier: GPL-2.0-only
#define _GNU_SOURCE
#include <linux/bpf.h>
#include <signal.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
static volatile sig_atomic_t stop;
static void on_signal(int sig)
{
	stop = sig;
}
int main(void)
{
	union bpf_attr attr = {
		.map_type = BPF_MAP_TYPE_PERCPU_ARRAY,
		.key_size = sizeof(int),
		.value_size = 32 * 1024,
		.max_entries = 1,
	};
	int fds[4], i;
	signal(SIGINT, on_signal);
	signal(SIGTERM, on_signal);
	for (i = 0; i < 4; i++) {
		if ((fds[i] = syscall(__NR_bpf, BPF_MAP_CREATE, &attr,
				      sizeof(attr))) < 0) {
			perror("BPF_MAP_CREATE");
			break;
		}
	}
	while (i == 4 && !stop)
		pause();
	while (i > 0)
		close(fds[--i]);
	return stop ? 0 : 1;
}
