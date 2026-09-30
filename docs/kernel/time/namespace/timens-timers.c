// SPDX-License-Identifier: GPL-2.0
/*
 * time namespace 下的定时器语义实验：
 *   - 相对时间 (relative) 和绝对时间 (absolute) 的区别
 *   - timerfd / clock_nanosleep / futex / POSIX timer 四条路径
 *
 * 所有 elapsed 都用 CLOCK_REALTIME 度量，因为 CLOCK_REALTIME 不受 timens 影响，
 * 可以当作"真实流逝时间"。
 *
 *   gcc -O0 -o timens-timers timens-timers.c
 *   ./timens-timers
 */
#define _GNU_SOURCE
#include <errno.h>
#include <linux/futex.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#define NS 1000000000LL

static double real_now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static struct timespec mono_now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts;
}

static struct timespec boot_now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_BOOTTIME, &ts);
	return ts;
}

static void report(const char *name, double t0, double t1)
{
	printf("  %-52s %6.3f s\n", name, t1 - t0);
}

static struct timespec add_ns(struct timespec ts, long long ns)
{
	ts.tv_sec += ns / NS;
	ts.tv_nsec += ns % NS;
	if (ts.tv_nsec >= NS) {
		ts.tv_nsec -= NS;
		ts.tv_sec++;
	}
	return ts;
}

/* ---------- timerfd ---------- */

static void tfd_run(const char *name, clockid_t clk, struct timespec val)
{
	struct itimerspec its = { .it_value = val };
	uint64_t expirations;
	double t0, t1;
	int fd;

	fd = timerfd_create(clk, 0);
	if (fd < 0) {
		printf("  %-52s timerfd_create: %s\n", name,
		       strerror(errno));
		return;
	}
	if (timerfd_settime(fd, TIMER_ABSTIME, &its, NULL) < 0) {
		printf("  %-52s settime: %s\n", name, strerror(errno));
		close(fd);
		return;
	}
	t0 = real_now();
	if (read(fd, &expirations, sizeof(expirations)) != sizeof(expirations)) {
		printf("  %-52s read: %s\n", name, strerror(errno));
		close(fd);
		return;
	}
	t1 = real_now();
	report(name, t0, t1);
	close(fd);
}

static void tfd_rel(const char *name, clockid_t clk, long long ns)
{
	struct itimerspec its = { .it_value = { ns / NS, ns % NS } };
	uint64_t expirations;
	double t0, t1;
	int fd = timerfd_create(clk, 0);

	if (timerfd_settime(fd, 0, &its, NULL) < 0) {
		printf("  %-52s settime: %s\n", name, strerror(errno));
		close(fd);
		return;
	}
	t0 = real_now();
	read(fd, &expirations, sizeof(expirations));
	t1 = real_now();
	report(name, t0, t1);
	close(fd);
}

/* ---------- clock_nanosleep ---------- */

static void sleep_abs(const char *name, clockid_t clk, struct timespec val)
{
	struct timespec rem = { 0, 0 };
	double t0, t1;
	int ret;

	t0 = real_now();
	ret = clock_nanosleep(clk, TIMER_ABSTIME, &val, &rem);
	t1 = real_now();
	if (ret)
		printf("  %-52s %6.3f s (ret=%d %s)\n", name, t1 - t0, ret,
		       strerror(ret));
	else
		report(name, t0, t1);
}

static void sleep_rel(const char *name, long long ns)
{
	struct timespec req = { ns / NS, ns % NS };
	struct timespec rem;
	double t0, t1;

	t0 = real_now();
	nanosleep(&req, &rem);
	t1 = real_now();
	report(name, t0, t1);
}

/* ---------- futex ---------- */

static int futex_wait_bitset(int *uaddr, struct timespec abs)
{
	return syscall(SYS_futex, uaddr, FUTEX_WAIT_BITSET | FUTEX_PRIVATE_FLAG,
		       0, &abs, NULL, FUTEX_BITSET_MATCH_ANY);
}

static void futex_abs(const char *name, struct timespec val)
{
	static int word;
	double t0, t1;
	int ret;

	t0 = real_now();
	ret = futex_wait_bitset(&word, val);
	t1 = real_now();
	if (ret < 0 && errno != ETIMEDOUT)
		printf("  %-52s %6.3f s (errno=%s)\n", name, t1 - t0,
		       strerror(errno));
	else
		report(name, t0, t1);
}

/* ---------- POSIX timer ---------- */

static void posix_timer_abs(const char *name, struct timespec val)
{
	struct sigevent sev = { .sigev_notify = SIGEV_SIGNAL,
				.sigev_signo = SIGUSR1 };
	struct itimerspec its = { .it_value = val };
	sigset_t set;
	timer_t tid;
	double t0, t1;

	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	sigprocmask(SIG_BLOCK, &set, NULL);

	if (timer_create(CLOCK_MONOTONIC, &sev, &tid) < 0) {
		printf("  %-52s timer_create: %s\n", name, strerror(errno));
		return;
	}
	if (timer_settime(tid, TIMER_ABSTIME, &its, NULL) < 0) {
		printf("  %-52s timer_settime: %s\n", name, strerror(errno));
		timer_delete(tid);
		return;
	}
	t0 = real_now();
	sigwaitinfo(&set, NULL);
	t1 = real_now();
	report(name, t0, t1);
	timer_delete(tid);
}

int main(void)
{
	struct timespec now = mono_now();

	/* 防止某个绝对定时器真的等到 1 小时之后 */
	alarm(20);

	printf("CLOCK_MONOTONIC now = %lld.%09ld\n", (long long)now.tv_sec,
	       now.tv_nsec);
	printf("CLOCK_REALTIME  now = %.3f\n\n", real_now());

	printf("相对时间 (relative):\n");
	tfd_rel("timerfd  MONOTONIC  relative 200 ms", CLOCK_MONOTONIC, 200000000LL);
	tfd_rel("timerfd  BOOTTIME   relative 200 ms", CLOCK_BOOTTIME, 200000000LL);
	tfd_rel("timerfd  REALTIME   relative 200 ms", CLOCK_REALTIME, 200000000LL);
	sleep_rel("nanosleep              relative 200 ms", 200000000LL);

	printf("\n绝对时间 (absolute) = timens 视角的 now + 200ms:\n");
	tfd_run("timerfd  MONOTONIC  absolute now+200ms", CLOCK_MONOTONIC,
		add_ns(mono_now(), 200000000LL));
	tfd_run("timerfd  BOOTTIME   absolute now+200ms", CLOCK_BOOTTIME,
		add_ns(boot_now(), 200000000LL));
	sleep_abs("clock_nanosleep MONOTONIC ABSTIME now+200ms", CLOCK_MONOTONIC,
		  add_ns(mono_now(), 200000000LL));
	futex_abs("futex FUTEX_WAIT_BITSET  absolute now+200ms",
		  add_ns(mono_now(), 200000000LL));
	posix_timer_abs("POSIX timer MONOTONIC absolute now+200ms",
			add_ns(mono_now(), 200000000LL));

	printf("\n绝对时间 (absolute) = timens 视角的启动后 1s (远小于 offset):\n");
	tfd_run("timerfd  MONOTONIC  absolute boot+1s", CLOCK_MONOTONIC,
		(struct timespec){ 1, 0 });
	sleep_abs("clock_nanosleep MONOTONIC ABSTIME boot+1s", CLOCK_MONOTONIC,
		  (struct timespec){ 1, 0 });
	futex_abs("futex FUTEX_WAIT_BITSET  absolute boot+1s",
		  (struct timespec){ 1, 0 });

	printf("\n绝对时间 (absolute) = timens 视角的 now - 1s (已经过去):\n");
	tfd_run("timerfd  MONOTONIC  absolute now-1s", CLOCK_MONOTONIC,
		add_ns(mono_now(), -1000000000LL));

	return 0;
}
