// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause)
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include "map_communication.h"
#include "map_communication.skel.h"

struct event_context {
	struct map_event event;
	int received;
};

static int handle_event(void *ctx, void *data, size_t data_sz)
{
	struct event_context *event_ctx = ctx;

	if (data_sz != sizeof(event_ctx->event)) {
		fprintf(stderr, "unexpected ring buffer record size: %zu\n", data_sz);
		return -EINVAL;
	}

	event_ctx->event = *(const struct map_event *)data;
	event_ctx->received = 1;
	return 0;
}

static size_t page_align(size_t value, size_t page_size)
{
	return (value + page_size - 1) & ~(page_size - 1);
}

int main(void)
{
	struct event_context event_ctx = {};
	struct map_communication_bpf *skel = NULL;
	struct mmap_state *shared = MAP_FAILED;
	void *hash_mapping;
	struct ring_buffer *ring = NULL;
	struct map_command command = { .addend = 2 };
	long page_size;
	size_t map_size;
	__u32 pid;
	int err = 0;
	int i;

	page_size = sysconf(_SC_PAGESIZE);
	if (page_size <= 0) {
		perror("sysconf(_SC_PAGESIZE)");
		return 1;
	}

	skel = map_communication_bpf__open();
	if (!skel) {
		fprintf(stderr, "failed to open BPF skeleton\n");
		return 1;
	}

	pid = getpid();
	skel->bss->target_pid = pid;
	if (map_communication_bpf__load(skel)) {
		fprintf(stderr, "failed to load BPF skeleton\n");
		err = 1;
		goto cleanup;
	}

	if (map_communication_bpf__attach(skel)) {
		fprintf(stderr, "failed to attach raw tracepoint\n");
		err = 1;
		goto cleanup;
	}

	if (bpf_map_update_elem(bpf_map__fd(skel->maps.commands), &pid,
				&command, BPF_ANY)) {
		perror("bpf_map_update_elem(commands)");
		err = 1;
		goto cleanup;
	}
	printf("hash map: user -> BPF through one bpf(BPF_MAP_UPDATE_ELEM) call\n");

	errno = 0;
	hash_mapping = mmap(NULL, page_size, PROT_READ, MAP_SHARED,
			    bpf_map__fd(skel->maps.commands), 0);
	if (hash_mapping != MAP_FAILED) {
		fprintf(stderr, "unexpectedly mmaped a hash map\n");
		munmap(hash_mapping, page_size);
		err = 1;
		goto cleanup;
	}
	printf("hash map: mmap rejected as expected: errno=%d "
	       "(this map type has no mmap operation)\n", errno);

	map_size = page_align(sizeof(*shared), page_size);
	shared = mmap(NULL, map_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		      bpf_map__fd(skel->maps.shared_array), 0);
	if (shared == MAP_FAILED) {
		perror("mmap(shared_array)");
		err = 1;
		goto cleanup;
	}

	ring = ring_buffer__new(bpf_map__fd(skel->maps.events), handle_event,
				&event_ctx, NULL);
	if (!ring) {
		fprintf(stderr, "failed to create ring buffer: %s\n", strerror(errno));
		err = 1;
		goto cleanup;
	}

	shared->userspace_value = 40;
	skel->bss->requested_run = 1;
	if (syscall(SYS_gettid) < 0) {
		perror("gettid");
		err = 1;
		goto cleanup;
	}

	for (i = 0; i < 10 && !event_ctx.received; i++) {
		int poll_result = ring_buffer__poll(ring, 100);

		if (poll_result < 0 && poll_result != -EINTR) {
			fprintf(stderr, "ring buffer poll failed: %d\n", poll_result);
			err = 1;
			goto cleanup;
		}
	}

	if (!event_ctx.received) {
		fprintf(stderr, "timed out waiting for the ring buffer event\n");
		err = 1;
		goto cleanup;
	}

	printf("array mmap: user wrote %llu, BPF wrote %llu, runs=%llu\n",
	       (unsigned long long)shared->userspace_value,
	       (unsigned long long)shared->bpf_value,
	       (unsigned long long)shared->runs);
	printf("ring buffer: pid=%u syscall=%u hash_addend=%llu result=%llu\n",
	       event_ctx.event.pid, event_ctx.event.syscall_nr,
	       (unsigned long long)event_ctx.event.hash_addend,
	       (unsigned long long)event_ctx.event.array_output);

	if (shared->bpf_value != 42 || shared->runs != 1 ||
	    event_ctx.event.array_input != 40 ||
	    event_ctx.event.array_output != 42) {
		fprintf(stderr, "shared data verification failed\n");
		err = 1;
	}

cleanup:
	if (shared != MAP_FAILED)
		munmap(shared, map_size);
	ring_buffer__free(ring);
	map_communication_bpf__destroy(skel);
	return err;
}
