#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <bpf/libbpf.h>
#include "arena.h"
#include "arena.skel.h"

static int run_once(struct arena_bpf *skel, __u64 run)
{
	skel->bss->requested_run = run;
	if (syscall(SYS_gettid) < 0) {
		perror("gettid");
		return -1;
	}
	if (skel->arena->completed_run != run) {
		fprintf(stderr, "arena BPF program did not complete run %llu\n",
			(unsigned long long)run);
		return -1;
	}
	return 0;
}

int main(void)
{
	struct arena_bpf *skel = NULL;
	struct arena_page *page;
	int err = 1;

	skel = arena_bpf__open();
	if (!skel) {
		fprintf(stderr, "failed to open BPF skeleton\n");
		return 1;
	}
	skel->bss->target_pid = getpid();

	if (arena_bpf__load(skel)) {
		fprintf(stderr, "failed to load arena BPF program; Linux 6.9+ "
				"and a recent Clang/libbpf are required\n");
		goto cleanup;
	}
	if (arena_bpf__attach(skel)) {
		fprintf(stderr, "failed to attach raw tracepoint\n");
		goto cleanup;
	}

	if (run_once(skel, 1))
		goto cleanup;
	if (skel->bss->allocation_failed || !skel->bss->dynamic_page) {
		fprintf(stderr, "BPF failed to allocate an arena page\n");
		goto cleanup;
	}

	page = skel->bss->dynamic_page;
	printf("arena globals: user wrote %llu, BPF wrote %llu\n",
	       (unsigned long long)skel->arena->userspace_value,
	       (unsigned long long)skel->arena->bpf_value);
	printf("BPF-allocated page: user sees input=%llu output=%llu\n",
	       (unsigned long long)page->userspace_value,
	       (unsigned long long)page->bpf_value);

	skel->arena->userspace_value = 100;
	page->userspace_value = 200;
	if (run_once(skel, 2))
		goto cleanup;
	printf("second run without map syscalls: globals=%llu page=%llu\n",
	       (unsigned long long)skel->arena->bpf_value,
	       (unsigned long long)page->bpf_value);

	if (skel->arena->bpf_value != 102 || page->bpf_value != 201) {
		fprintf(stderr, "arena shared data verification failed\n");
		goto cleanup;
	}
	err = 0;

cleanup:
	arena_bpf__destroy(skel);
	return err;
}
