#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../user/sysfs.h"

static void dump_current_sched(const char *tag)
{
	char path[PATH_MAX];
	FILE *file;
	char buf[4096];
	pid_t pid = getpid();

	snprintf(path, sizeof(path), "/proc/%d/sched", pid);
	file = fopen(path, "r");
	if (!file) {
		fprintf(stderr, "failed to open %s: %s\n", path,
			strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("===== %s: %s =====\n", tag, path);
	while (fgets(buf, sizeof(buf), file) != NULL)
		fputs(buf, stdout);
	fflush(stdout);

	fclose(file);
}

static void usage(const char *prog)
{
	fprintf(stderr, "Usage: %s <action>\n", prog);
	fprintf(stderr, "  action 8: repeated schedule()\n");
	fprintf(stderr, "  action 9: repeated cond_resched()\n");
}

int main(int argc, char **argv)
{
	long action;
	char *end;

	if (argc != 2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	errno = 0;
	action = strtol(argv[1], &end, 10);
	if (errno != 0 || *end != '\0') {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	printf("pid=%d action=%ld\n", getpid(), action);
	if (action == 8 || action == 9)
		dump_current_sched("before");
	fflush(stdout);

	sysfs_write(action);

	if (action == 8 || action == 9)
		dump_current_sched("after");

	return 0;
}
