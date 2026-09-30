#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <libaio.h>
#include <errno.h>
#include "../lib.h"

#define FILE_NAME "/tmp/test_file.txt"
#define NUM_IO 10
#define PAGE_SIZE 4096

int main()
{
	io_context_t ctx;
	struct iocb cb[NUM_IO];
	struct iocb *cbs[NUM_IO];
	struct io_event events[NUM_IO];
	int fd, ret, i;

	memset(&ctx, 0, sizeof(ctx));
	if (io_setup(NUM_IO, &ctx) < 0)
		error("io_setup\n");

	void *addr = mmap_region(NUM_IO * PAGE_SIZE, -1, false);
	fd = get_file("/home/martins3/a.dump", PAGE_SIZE * NUM_IO);
	for (i = 0; i < NUM_IO; i++) {
		io_prep_pread(&cb[i], fd, addr + i * PAGE_SIZE, PAGE_SIZE,
			       i * PAGE_SIZE);
		cbs[i] = &cb[i];
	}

	ret = io_submit(ctx, NUM_IO, cbs);
	if (ret != NUM_IO)
		error("io_submit");

	ret = io_getevents(ctx, NUM_IO, NUM_IO, events, NULL);
	if (ret != NUM_IO)
		error("io_getevents");

	for (i = 0; i < ret; i++) {
		struct io_event *ev = &events[i];
		if (ev->res != PAGE_SIZE)
			error("io event");
	}
	sleep(1000);
	return 0;
}
