/*
 * man clock_getres(2) / man timer_create(2)
 *
 * CLOCK_REALTIME_ALARM 和 CLOCK_BOOTTIME_ALARM 的用法
 *
 * 1. 取时间和对应的普通 clock 完全一样，它们只是给 timer_create / timerfd_create 用的
 * 2. 不可设置，clock_settime 返回 EINVAL
 * 3. 建定时器需要 CAP_WAKE_ALARM，否则 EPERM
 * 4. 只有它对应的定时器可以在 suspend 期间唤醒系统(alarmtimer_suspend 把最近的
 *    一个 alarm 写进 RTC)
 *
 * 用法:
 *   ./alarm-clock.out               # 基本用法
 *   sudo ./alarm-clock.out suspend 10   # 睡 10 秒，由 alarm timer 唤醒
 */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

#define NSEC_PER_SEC 1000000000L
#define RT_SIG (SIGRTMIN + 1)

static long long ts_to_ns(const struct timespec *ts)
{
	return (long long)ts->tv_sec * NSEC_PER_SEC + ts->tv_nsec;
}

static long long now_ns(clockid_t id)
{
	struct timespec ts;

	clock_gettime(id, &ts);
	return ts_to_ns(&ts);
}

/* ALARM 系列的取值和 REALTIME / BOOTTIME 完全一样 */
static void show_values(void)
{
	struct timespec ts;
	struct {
		clockid_t id;
		const char *name;
	} clocks[] = {
		{ CLOCK_REALTIME, "CLOCK_REALTIME" },
		{ CLOCK_REALTIME_ALARM, "CLOCK_REALTIME_ALARM" },
		{ CLOCK_BOOTTIME, "CLOCK_BOOTTIME" },
		{ CLOCK_BOOTTIME_ALARM, "CLOCK_BOOTTIME_ALARM" },
	};

	printf("=== 1. clock_gettime ===\n");
	for (size_t i = 0; i < sizeof(clocks) / sizeof(clocks[0]); i++) {
		if (clock_gettime(clocks[i].id, &ts) < 0) {
			printf("  %-22s failed: %s\n", clocks[i].name,
			       strerror(errno));
			continue;
		}
		printf("  %-22s %lld.%09ld\n", clocks[i].name,
		       (long long)ts.tv_sec, ts.tv_nsec);
	}

	printf("  REALTIME_ALARM - REALTIME = %lld ns\n",
	       now_ns(CLOCK_REALTIME_ALARM) - now_ns(CLOCK_REALTIME));
	printf("  BOOTTIME_ALARM - BOOTTIME = %lld ns\n",
	       now_ns(CLOCK_BOOTTIME_ALARM) - now_ns(CLOCK_BOOTTIME));
	printf("  BOOTTIME - MONOTONIC      = %lld ns\n",
	       now_ns(CLOCK_BOOTTIME) - now_ns(CLOCK_MONOTONIC));
	printf("    (两者的差就是累计的 suspend 时间，本机没 suspend 过所以只有几十 ns 噪声)\n\n");
}

static void test_settime(void)
{
	struct timespec ts;
	clockid_t clocks[] = { CLOCK_REALTIME_ALARM, CLOCK_BOOTTIME_ALARM };

	printf("=== 2. clock_settime ===\n");
	for (size_t i = 0; i < sizeof(clocks) / sizeof(clocks[0]); i++) {
		if (clock_gettime(clocks[i], &ts) < 0)
			continue;
		/* 设置成当前值，不会真的把系统时间改掉 */
		if (clock_settime(clocks[i], &ts) < 0)
			printf("  clock_settime(%d) failed: %s\n", clocks[i],
			       strerror(errno));
		else
			printf("  clock_settime(%d) ok\n", clocks[i]);
	}
	printf("  (CLOCK_REALTIME 可以设置，只是需要特权)\n\n");
}

static timer_t arm_timer(clockid_t id, int sec)
{
	struct sigevent sev = { 0 };
	struct itimerspec its = { 0 };
	timer_t tid;

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = RT_SIG;
	if (timer_create(id, &sev, &tid) < 0)
		return (timer_t)-1;

	its.it_value.tv_sec = sec;
	if (timer_settime(tid, 0, &its, NULL) < 0) {
		timer_delete(tid);
		return (timer_t)-1;
	}
	return tid;
}

static void test_posix_timer(clockid_t id, const char *name, int sec)
{
	sigset_t mask;
	timer_t tid;
	long long start;
	int err;

	printf("=== 3. timer_create(%s) + 相对 %d 秒定时器 ===\n", name, sec);

	/* 先把信号屏掉，否则定时器到点时默认动作会把进程干掉 */
	sigemptyset(&mask);
	sigaddset(&mask, RT_SIG);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	tid = arm_timer(id, sec);
	if (tid == (timer_t)-1) {
		err = errno;
		printf("  失败: %s\n", strerror(err));
		if (err == EPERM)
			printf("  需要 CAP_WAKE_ALARM，请用 sudo 再跑一次\n");
		if (err == ENOTSUP)
			printf("  内核没找到支持 wakeup 的 RTC，"
			       "见 alarmtimer_get_rtcdev()\n");
		printf("\n");
		return;
	}

	start = now_ns(CLOCK_MONOTONIC);
	sigwaitinfo(&mask, NULL);
	printf("  信号到达，实际等了 %.3f 秒\n\n",
	       (double)(now_ns(CLOCK_MONOTONIC) - start) / NSEC_PER_SEC);
	timer_delete(tid);
}

/* timerfd 也支持 alarm clock，而且支持 poll/epoll */
static void test_timerfd(clockid_t id, const char *name, int sec)
{
	struct itimerspec its = { 0 };
	unsigned long long expirations;
	long long start;
	int fd;

	printf("=== 4. timerfd_create(%s) + %d 秒定时器 ===\n", name, sec);

	fd = timerfd_create(id, 0);
	if (fd < 0) {
		printf("  失败: %s\n\n", strerror(errno));
		return;
	}

	its.it_value.tv_sec = sec;
	start = now_ns(CLOCK_MONOTONIC);
	if (timerfd_settime(fd, 0, &its, NULL) < 0) {
		printf("  失败: %s\n\n", strerror(errno));
		close(fd);
		return;
	}

	if (read(fd, &expirations, sizeof(expirations)) !=
	    (ssize_t)sizeof(expirations)) {
		printf("  read 失败: %s\n\n", strerror(errno));
		close(fd);
		return;
	}
	printf("  可读，expirations = %llu，实际等了 %.3f 秒\n\n", expirations,
	       (double)(now_ns(CLOCK_MONOTONIC) - start) / NSEC_PER_SEC);
	close(fd);
}

static void show_rtc(void)
{
	char buf[64] = { 0 };
	int fd;

	fd = open("/sys/class/rtc/rtc0/name", O_RDONLY);
	if (fd < 0) {
		printf("  没有 /sys/class/rtc/rtc0，alarm clock 会返回 ENOTSUP\n");
		return;
	}
	if (read(fd, buf, sizeof(buf) - 1) > 0) {
		buf[strcspn(buf, "\n")] = 0;
		printf("  RTC: %s", buf);
	}
	close(fd);

	fd = open("/sys/class/rtc/rtc0/device/power/wakeup", O_RDONLY);
	if (fd >= 0) {
		memset(buf, 0, sizeof(buf));
		if (read(fd, buf, sizeof(buf) - 1) > 0) {
			buf[strcspn(buf, "\n")] = 0;
			printf(", wakeup =  %s", buf);
		}
		close(fd);
	}
	printf("\n");
}

/*
 * 真正的用法: 睡下去，等 alarm timer 到点把系统唤醒
 *
 * 同时起两个定时器做对照:
 *   CLOCK_MONOTONIC      : 普通 hrtimer，suspend 期间硬件 timer 停了，它在 resume
 *                          之后还要再等 sec 秒(awake 时间)才到点
 *   CLOCK_*_ALARM        : 进 suspend 的时候 alarmtimer_suspend() 把它的到期时间
 *                          写进 RTC，系统被 RTC 唤醒
 */
static int do_suspend(int sec, clockid_t id, const char *name)
{
	struct itimerspec remain;
	sigset_t mask;
	timer_t alarm_tid, mono_tid;
	long long mono_before, boot_before, mono_after, boot_after;
	int fd;

	printf("=== suspend %d 秒，由 %s 唤醒 ===\n", sec, name);
	if (sec < 2)
		printf("  (注意 alarmtimer_suspend() 有个 2 秒的限制，"
		       "小于 2 秒会拒绝 suspend)\n");
	show_rtc();

	sigemptyset(&mask);
	sigaddset(&mask, RT_SIG);
	sigprocmask(SIG_BLOCK, &mask, NULL);

	mono_tid = arm_timer(CLOCK_MONOTONIC, sec);
	if (mono_tid == (timer_t)-1) {
		printf("  CLOCK_MONOTONIC 定时器失败: %s\n", strerror(errno));
		return 1;
	}
	alarm_tid = arm_timer(id, sec);
	if (alarm_tid == (timer_t)-1) {
		printf("  %s 定时器失败: %s\n", name, strerror(errno));
		if (errno == EPERM)
			printf("  需要 CAP_WAKE_ALARM，用 sudo 跑\n");
		timer_delete(mono_tid);
		return 1;
	}
	errno = 0;
	printf("  两个 %d 秒的定时器都建好了\n", sec);

	fd = open("/sys/power/state", O_WRONLY);
	if (fd < 0) {
		printf("  打开 /sys/power/state 失败: %s\n", strerror(errno));
		return 1;
	}
	mono_before = now_ns(CLOCK_MONOTONIC);
	boot_before = now_ns(CLOCK_BOOTTIME);
	printf("  写 mem 到 /sys/power/state ...\n");
	fflush(stdout);
	if (write(fd, "mem", 3) < 0) {
		printf("  suspend 失败: %s\n", strerror(errno));
		close(fd);
		return 1;
	}
	close(fd);
	/* 走到这里说明已经 resume 了 */
	mono_after = now_ns(CLOCK_MONOTONIC);
	boot_after = now_ns(CLOCK_BOOTTIME);
	printf("  已经 resume\n");
	printf("    MONOTONIC 增加了 %.3f 秒(不含 suspend 时间)\n",
	       (double)(mono_after - mono_before) / NSEC_PER_SEC);
	printf("    BOOTTIME  增加了 %.3f 秒(包含 suspend 时间)\n",
	       (double)(boot_after - boot_before) / NSEC_PER_SEC);

	/* alarm timer 应该马上到点，因为它的到期时间是按 BOOTTIME/REALTIME 算的 */
	sigwaitinfo(&mask, NULL);
	printf("  收到 alarm timer 的信号，BOOTTIME 才过了 %.3f 秒\n",
	       (double)(now_ns(CLOCK_BOOTTIME) - boot_before) / NSEC_PER_SEC);

	/* 对照: 普通 MONOTONIC 定时器还剩 sec 秒 */
	timer_gettime(mono_tid, &remain);
	printf("  而 CLOCK_MONOTONIC 定时器还剩 %ld.%09ld 秒\n\n",
	       (long)remain.it_value.tv_sec, remain.it_value.tv_nsec);

	timer_delete(alarm_tid);
	timer_delete(mono_tid);
	return 0;
}

int main(int argc, char **argv)
{
	clockid_t alarm_id = CLOCK_BOOTTIME_ALARM;
	const char *alarm_name = "CLOCK_BOOTTIME_ALARM";

	printf("uid = %d\n\n", getuid());

	if (argc >= 3 && !strcmp(argv[1], "suspend")) {
		if (argc >= 4 && !strcmp(argv[3], "realtime")) {
			alarm_id = CLOCK_REALTIME_ALARM;
			alarm_name = "CLOCK_REALTIME_ALARM";
		}
		return do_suspend(atoi(argv[2]), alarm_id, alarm_name);
	}

	show_values();
	test_settime();

	/* 系统不 suspend 的时候，它就是一个普通的 POSIX timer */
	test_posix_timer(CLOCK_REALTIME_ALARM, "CLOCK_REALTIME_ALARM", 1);
	test_posix_timer(CLOCK_BOOTTIME_ALARM, "CLOCK_BOOTTIME_ALARM", 1);
	test_timerfd(CLOCK_BOOTTIME_ALARM, "CLOCK_BOOTTIME_ALARM", 1);

	printf("=== 5. 验证它真的能唤醒系统 ===\n");
	printf("  sudo %s suspend 10              # 用 CLOCK_BOOTTIME_ALARM\n",
	       argv[0]);
	printf("  sudo %s suspend 10 realtime     # 用 CLOCK_REALTIME_ALARM\n",
	       argv[0]);
	printf("  它自己会写 mem 到 /sys/power/state，等 RTC 把它唤醒\n");
	return 0;
}
