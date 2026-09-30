// SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note
/*
 * vdso_clock_getres.c: Sample code to test clock_getres.
 * Copyright (c) 2019 Arm Ltd.
 *
 * Compile with:
 * gcc -std=gnu99 vdso_clock_getres.c
 *
 * Tested on ARM, ARM64, MIPS32, x86 (32-bit and 64-bit),
 * Power (32-bit and 64-bit), S390x (32-bit and 64-bit).
 * Might work on other architectures.
 *
 * 这里只走 system call，把每个 clockid 的 clock_getres 和 clock_gettime
 * 的输出都打印出来：
 *
 * clock_id: CLOCK_REALTIME          res=0.000000001s now=1789621661.861402986s [PASS]
 * clock_id: CLOCK_BOOTTIME          res=0.000000001s now=1479766.096430411s [PASS]
 * clock_id: CLOCK_TAI               res=0.000000001s now=1789621698.861416292s [PASS]
 * clock_id: CLOCK_REALTIME_COARSE   res=0.001000000s now=1789621661.860830936s [PASS]
 * clock_id: CLOCK_MONOTONIC         res=0.000000001s now=1479766.096422348s [PASS]
 * clock_id: CLOCK_MONOTONIC_RAW     res=0.000000001s now=1479761.293631315s [PASS]
 * clock_id: CLOCK_MONOTONIC_COARSE  res=0.001000000s now=1479766.095848772s [PASS]
 */

#define _GNU_SOURCE
#include <elf.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/auxv.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/syscall.h>

#define KSFT_FAIL -1
#define KSFT_PASS 1

static long syscall_clock_getres(clockid_t _clkid, struct timespec *_ts)
{
	long ret;

	ret = syscall(SYS_clock_getres, _clkid, _ts);

	return ret;
}

const char *vdso_clock_name[12] = {
	"CLOCK_REALTIME",	    "CLOCK_MONOTONIC",
	"CLOCK_PROCESS_CPUTIME_ID", "CLOCK_THREAD_CPUTIME_ID",
	"CLOCK_MONOTONIC_RAW",	    "CLOCK_REALTIME_COARSE",
	"CLOCK_MONOTONIC_COARSE",   "CLOCK_BOOTTIME",
	"CLOCK_REALTIME_ALARM",	    "CLOCK_BOOTTIME_ALARM",
	"CLOCK_SGI_CYCLE",	    "CLOCK_TAI",
};

/*
 * This function calls clock_getres by system call with different
 * values for clock_id, and prints the resolution as well as the
 * current time of that clock.
 *
 * Example of output:
 *
 * clock_id: CLOCK_REALTIME res=0.000000001s now=1789621661.861402986s [PASS]
 * clock_id: CLOCK_BOOTTIME res=0.000000001s now=1479766.096430411s [PASS]
 * clock_id: CLOCK_TAI res=0.000000001s now=1789621698.861416292s [PASS]
 * clock_id: CLOCK_REALTIME_COARSE res=0.001000000s now=1789621661.860830936s [PASS]
 * clock_id: CLOCK_MONOTONIC res=0.000000001s now=1479766.096422348s [PASS]
 * clock_id: CLOCK_MONOTONIC_RAW res=0.000000001s now=1479761.293631315s [PASS]
 * clock_id: CLOCK_MONOTONIC_COARSE res=0.001000000s now=1479766.095848772s [PASS]
 */
static inline int vdso_test_clock(unsigned int clock_id)
{
	struct timespec res, now;

	printf("clock_id: %-24s", vdso_clock_name[clock_id]);

	if (syscall_clock_getres(clock_id, &res) != 0) {
		printf(" clock_getres: %s\n", strerror(errno));
		return KSFT_FAIL;
	}

	printf(" res=%ld.%09lds", (long)res.tv_sec, res.tv_nsec);

	if (syscall(SYS_clock_gettime, clock_id, &now) != 0) {
		printf(" clock_gettime: %s\n", strerror(errno));
		return KSFT_FAIL;
	}

	printf(" now=%ld.%09lds [PASS]\n", (long)now.tv_sec, now.tv_nsec);

	return KSFT_PASS;
}

int main(int argc, char **argv)
{
	int ret = 0;

#if _POSIX_TIMERS > 0

#ifdef CLOCK_REALTIME
	ret += vdso_test_clock(CLOCK_REALTIME);
#endif

#ifdef CLOCK_BOOTTIME
	ret += vdso_test_clock(CLOCK_BOOTTIME);
#endif

#ifdef CLOCK_TAI
	ret += vdso_test_clock(CLOCK_TAI);
#endif

#ifdef CLOCK_REALTIME_COARSE
	ret += vdso_test_clock(CLOCK_REALTIME_COARSE);
#endif

#ifdef CLOCK_MONOTONIC
	ret += vdso_test_clock(CLOCK_MONOTONIC);
#endif

#ifdef CLOCK_MONOTONIC_RAW
	ret += vdso_test_clock(CLOCK_MONOTONIC_RAW);
#endif

#ifdef CLOCK_MONOTONIC_COARSE
	ret += vdso_test_clock(CLOCK_MONOTONIC_COARSE);
#endif

#endif
	if (ret > 0)
		return KSFT_FAIL;

	return KSFT_PASS;
}
