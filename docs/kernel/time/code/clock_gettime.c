#define _XOPEN_SOURCE 600
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define SECS_IN_DAY (24 * 60 * 60)

static void displayClock(clockid_t clock, const char *name, bool showRes)
{
	struct timespec ts;

	if (clock_gettime(clock, &ts) == -1) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}

	printf("%-15s: %10jd.%03ld (", name, (intmax_t)ts.tv_sec,
	       ts.tv_nsec / 1000000);

	long days = ts.tv_sec / SECS_IN_DAY;
	if (days > 0)
		printf("%ld days + ", days);

	printf("%2dh %2dm %2ds", (int)(ts.tv_sec % SECS_IN_DAY) / 3600,
	       (int)(ts.tv_sec % 3600) / 60, (int)ts.tv_sec % 60);
	printf(")\n");

	if (clock_getres(clock, &ts) == -1) {
		perror("clock_getres");
		exit(EXIT_FAILURE);
	}

	if (showRes)
		printf("     resolution: %10jd.%09ld\n", (intmax_t)ts.tv_sec,
		       ts.tv_nsec);
}

static void test_clock_gettime(int argc)
{
	bool showRes = argc > 1;

	displayClock(CLOCK_REALTIME, "CLOCK_REALTIME", showRes);
#ifdef CLOCK_TAI
	displayClock(CLOCK_TAI, "CLOCK_TAI", showRes);
#endif
	displayClock(CLOCK_MONOTONIC, "CLOCK_MONOTONIC", showRes);
#ifdef CLOCK_BOOTTIME
	displayClock(CLOCK_BOOTTIME, "CLOCK_BOOTTIME", showRes);
#endif
	displayClock(CLOCK_MONOTONIC_RAW, "CLOCK_MONOTONIC_RAW", showRes);
}

static int test_time()
{
	// Declare a variable to store the time in seconds
	time_t current_time;

	// Get the current time using time(2)
	// Pass NULL to time() to get the current time
	current_time = time(NULL);

	// Check if time() was successful
	if (current_time == (time_t)-1) {
		perror("time");
		return 1;
	}

	// Print the raw time in seconds since the Epoch
	printf("Seconds since the Epoch: %ld\n", (long)current_time);

	// Convert the time to a human-readable string using ctime()
	printf("Human-readable time: %s", ctime(&current_time));

	return 0;
}

static int test_gettimeofday()
{
	// Declare a timeval structure to store the time
	struct timeval tv;

	// Get the current time using gettimeofday
	if (gettimeofday(&tv, NULL) == 0) {
		// Print the raw time in seconds and microseconds
		printf("Seconds since the Epoch: %ld\n", (long)tv.tv_sec);
		printf("Microseconds: %ld\n", (long)tv.tv_usec);

		// Convert the time to a human-readable string using ctime()
		printf("Human-readable time: %s", ctime(&tv.tv_sec));
	} else {
		perror("gettimeofday");
		return 1;
	}

	return 0;
}

int main(int argc, char *argv[])
{
	int test = 0;
	if (argv[1] != NULL)
		test = atoi(argv[1]);

	switch (test) {
	case 0:
		printf("=== test clock_gettime(2) ===\n");
		test_clock_gettime(true);
		break;
	case 1:
		printf("=== test time(2) ===\n");
		test_time();
		break;
	case 2:
		printf("=== test gettimeofday(2) ===\n");
		test_gettimeofday();
		break;
	}
	return 0;
}
