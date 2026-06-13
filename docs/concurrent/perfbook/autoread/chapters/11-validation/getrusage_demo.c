/*
 * getrusage_demo.c: Using getrusage() to detect context switches.
 *
 * Based on Listing 11.1 from perfbook Chapter 11 (Validation).
 * If voluntary or involuntary context switches occurred during the test,
 * the results might be corrupted by interference from other processes.
 */

#include <sys/time.h>
#include <sys/resource.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

/* A simple CPU-bound workload */
static uint64_t do_work(int iterations)
{
	volatile uint64_t sum = 0;
	int i;

	for (i = 0; i < iterations; i++)
		sum += i * i;
	return sum;
}

/*
 * Run test and return 0 if results should be rejected due to context switches.
 */
static int runtest(int iterations)
{
	struct rusage ru1;
	struct rusage ru2;
	uint64_t result;

	if (getrusage(RUSAGE_SELF, &ru1) != 0) {
		perror("getrusage");
		abort();
	}

	result = do_work(iterations);
	(void)result;

	if (getrusage(RUSAGE_SELF, &ru2) != 0) {
		perror("getrusage");
		abort();
	}

	printf("voluntary ctx switches: %ld -> %ld\n",
	       (long)ru1.ru_nvcsw, (long)ru2.ru_nvcsw);
	printf("involuntary ctx switches: %ld -> %ld\n",
	       (long)ru1.ru_nivcsw, (long)ru2.ru_nivcsw);

	/* Reject if any context switch occurred */
	if (ru1.ru_nvcsw != ru2.ru_nvcsw || ru1.ru_nivcsw != ru2.ru_nivcsw) {
		printf("REJECT: context switches detected during test\n");
		return 0;
	}

	printf("ACCEPT: no context switches detected\n");
	return 1;
}

int main(int argc, char *argv[])
{
	int iterations = 100000000;

	if (argc > 1)
		iterations = atoi(argv[1]);

	printf("Running CPU-bound test with %d iterations...\n", iterations);
	runtest(iterations);

	return 0;
}
