/*
 * rt_timer.c: Real-time timer latency test.
 *
 * Based on perfbook Chapter 14, Section 14.4.2.4 "Event-Driven Applications".
 *
 * Demonstrates the correct way to measure timed waits for real-time use:
 * 1. Use CLOCK_MONOTONIC (not CLOCK_REALTIME).
 * 2. Raise thread priority with sched_setscheduler().
 * 3. Pin memory with mlockall().
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sched.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#define NR_ITERATIONS 1000
#define SLEEP_NS 1000000  /* 1 millisecond */

static inline uint64_t ts_to_ns(struct timespec *ts)
{
	return (uint64_t)ts->tv_sec * 1000000000ULL + ts->tv_nsec;
}

int main(void)
{
	struct timespec timestart, timeend, timewait;
	uint64_t min_latency = UINT64_MAX;
	uint64_t max_latency = 0;
	uint64_t total_latency = 0;
	int i;
	int ret;

	/* Step 1: Pin memory to avoid page faults */
	ret = mlockall(MCL_CURRENT | MCL_FUTURE);
	if (ret != 0) {
		perror("mlockall");
		printf("Warning: mlockall failed, page faults may cause jitter\n");
	} else {
		printf("mlockall: OK\n");
	}

	/* Step 2: Raise to real-time priority */
	struct sched_param param;
	param.sched_priority = sched_get_priority_max(SCHED_FIFO);
	if (param.sched_priority < 0) {
		perror("sched_get_priority_max");
		return 1;
	}
	ret = sched_setscheduler(0, SCHED_FIFO, &param);
	if (ret != 0) {
		perror("sched_setscheduler");
		printf("Warning: cannot set real-time priority (need root?)\n");
	} else {
		printf("sched_setscheduler(SCHED_FIFO, prio=%d): OK\n",
		       param.sched_priority);
	}

	/* Step 3: Use CLOCK_MONOTONIC */
	timewait.tv_sec = 0;
	timewait.tv_nsec = SLEEP_NS;

	printf("Running %d iterations of %d ns sleep...\n",
	       NR_ITERATIONS, SLEEP_NS);

	for (i = 0; i < NR_ITERATIONS; i++) {
		ret = clock_gettime(CLOCK_MONOTONIC, &timestart);
		if (ret != 0) {
			perror("clock_gettime start");
			return 1;
		}

		ret = nanosleep(&timewait, NULL);
		if (ret != 0) {
			perror("nanosleep");
			return 1;
		}

		ret = clock_gettime(CLOCK_MONOTONIC, &timeend);
		if (ret != 0) {
			perror("clock_gettime end");
			return 1;
		}

		uint64_t actual = ts_to_ns(&timeend) - ts_to_ns(&timestart);
		uint64_t latency = actual > SLEEP_NS ? actual - SLEEP_NS : 0;

		if (latency < min_latency)
			min_latency = latency;
		if (latency > max_latency)
			max_latency = latency;
		total_latency += latency;
	}

	printf("\nResults (%d iterations, requested %d ns):\n",
	       NR_ITERATIONS, SLEEP_NS);
	printf("  Min extra latency: %llu ns\n", (unsigned long long)min_latency);
	printf("  Max extra latency: %llu ns\n", (unsigned long long)max_latency);
	printf("  Avg extra latency: %llu ns\n",
	       (unsigned long long)(total_latency / NR_ITERATIONS));

	return 0;
}
