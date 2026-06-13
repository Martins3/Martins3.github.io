#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static void usage(const char *prog)
{
	fprintf(stderr, "usage: %s --seconds N [--cpu CPU]\n", prog);
}

static double monotonic_seconds(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}

	return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

static long parse_long_arg(const char *value, const char *name)
{
	char *end = NULL;
	long ret;

	errno = 0;
	ret = strtol(value, &end, 10);
	if (errno != 0 || end == value || *end != '\0') {
		fprintf(stderr, "invalid %s: %s\n", name, value);
		exit(EXIT_FAILURE);
	}

	return ret;
}

static double parse_double_arg(const char *value, const char *name)
{
	char *end = NULL;
	double ret;

	errno = 0;
	ret = strtod(value, &end);
	if (errno != 0 || end == value || *end != '\0' || ret <= 0.0) {
		fprintf(stderr, "invalid %s: %s\n", name, value);
		exit(EXIT_FAILURE);
	}

	return ret;
}

static void pin_to_cpu(int cpu)
{
	cpu_set_t set;

	CPU_ZERO(&set);
	CPU_SET(cpu, &set);

	if (sched_setaffinity(0, sizeof(set), &set) != 0) {
		perror("sched_setaffinity");
		exit(EXIT_FAILURE);
	}
}

int main(int argc, char **argv)
{
	double seconds = 3.0;
	int cpu = -1;
	double start;
	double deadline;
	volatile uint64_t sink = 0;
	uint64_t iterations = 0;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--seconds") == 0) {
			if (++i >= argc) {
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			seconds = parse_double_arg(argv[i], "seconds");
		} else if (strcmp(argv[i], "--cpu") == 0) {
			if (++i >= argc) {
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			cpu = (int)parse_long_arg(argv[i], "cpu");
			if (cpu < 0) {
				fprintf(stderr, "cpu must be >= 0\n");
				return EXIT_FAILURE;
			}
		} else if (strcmp(argv[i], "--help") == 0) {
			usage(argv[0]);
			return EXIT_SUCCESS;
		} else {
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (cpu >= 0)
		pin_to_cpu(cpu);

	start = monotonic_seconds();
	deadline = start + seconds;

	while (monotonic_seconds() < deadline) {
		for (int i = 0; i < 4096; i++) {
			sink = sink * 2862933555777941757ULL + 3037000493ULL;
			iterations++;
		}
	}

	printf("elapsed=%.3f iterations=%" PRIu64 " sink=%" PRIu64 "\n",
	       monotonic_seconds() - start, iterations, sink);

	return EXIT_SUCCESS;
}
