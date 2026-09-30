/*
 * vdso-tsc.c - 对比 vDSO 取时间与直接读 TSC 的开销，x86-64 Linux。
 *
 * 1. vDSO：直接调用 libc 的 clock_gettime / gettimeofday / time。glibc 默认
 *          就走 vDSO，全程不进入内核。
 * 2. TSC ：裸 RDTSC 读时间戳计数器，或用校准出的 mult/shift 把它换算成 ns，
 *          这正是 vDSO 内部做的一部分工作，只是不经过 vvar 的 seqlock、
 *          也不做 clockid 分发和 CLOCK_COARSE 判断。
 *
 * vDSO 里同一套 rdtsc 逻辑会分发给不同的 clockid，语义与精度都不同：
 *   CLOCK_MONOTONIC / CLOCK_REALTIME   走 hres 路径，rdtsc + vvar 换算
 *   CLOCK_MONOTONIC_COARSE             直接读 vvar 里每 tick 刷新的粗值，不读 tsc
 *   gettimeofday / time                精度分别为 us / s
 * 想测真正的系统调用得用 syscall(2) 强制陷入内核，但那开销远高于前两者，
 * 本程序不涉及。
 *
 * 编译： gcc -O2 -Wall -Wextra -o vdso-tsc.out vdso-tsc.c
 * 运行： ./vdso-tsc.out
 * 输出是未扣开销的 TSC tick 与换算出的 ns，对比 baseline 行即可得到净开销。
 * 依赖：constant_tsc，以及 LFENCE 具备执行排序语义的 Intel x86-64 CPU。
 */
#define _GNU_SOURCE
#include <inttypes.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#define WARMUP 2000
#define SAMPLES 20000

struct out {
	struct timespec ts;
	struct timeval tv;
	time_t t;
	long ret;
};

/* ------------------------------------------------------------------ */
/* 计时与 TSC 校准                                                     */
/* ------------------------------------------------------------------ */

static uint64_t g_tsc_hz; /* 每秒 TSC tick 数 */
static uint64_t g_base_tsc;
static uint64_t g_base_ns; /* 校准基点的 CLOCK_MONOTONIC 纳秒 */
static __uint128_t g_mult; /* (1e9 << 32) / g_tsc_hz，Q32 定点 */

static inline uint64_t rdtsc_fenced(void)
{
	unsigned lo, hi;

	__asm__ volatile("lfence; rdtsc" : "=a"(lo), "=d"(hi) : : "memory");
	return ((uint64_t)hi << 32) | lo;
}

static uint64_t mono_ns(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static void calibrate(void)
{
	uint64_t n0, c0, n1, c1;

	/* 先跑热，避免第一轮受频率爬升/缺页影响。 */
	(void)mono_ns();
	n0 = mono_ns();
	c0 = rdtsc_fenced();
	do {
		n1 = mono_ns();
		c1 = rdtsc_fenced();
	} while (n1 - n0 < 200000000ull); /* 约 200 ms 的忙等 */

	g_tsc_hz =
		(uint64_t)((__uint128_t)(c1 - c0) * 1000000000ull / (n1 - n0));
	g_base_tsc = c0;
	g_base_ns = n0;
	g_mult = ((__uint128_t)1000000000ull << 32) / g_tsc_hz;
}

/* ------------------------------------------------------------------ */
/* 被测路径                                                            */
/* ------------------------------------------------------------------ */

typedef uint64_t (*work_fn)(struct out *);

static uint64_t work_baseline(struct out *o)
{
	(void)o;
	return 0;
}

static uint64_t work_rdtsc(struct out *o)
{
	(void)o;
	return rdtsc_fenced();
}

static uint64_t work_tsc_ns(struct out *o)
{
	uint64_t c;

	(void)o;
	c = rdtsc_fenced();
	return g_base_ns +
	       (uint64_t)(((__uint128_t)(c - g_base_tsc) * g_mult) >> 32);
}

static uint64_t work_cg_mono(struct out *o)
{
	o->ret = clock_gettime(CLOCK_MONOTONIC, &o->ts);
	return (uint64_t)o->ts.tv_nsec;
}

static uint64_t work_cg_coarse(struct out *o)
{
	o->ret = clock_gettime(CLOCK_MONOTONIC_COARSE, &o->ts);
	return (uint64_t)o->ts.tv_nsec;
}

static uint64_t work_cg_real(struct out *o)
{
	o->ret = clock_gettime(CLOCK_REALTIME, &o->ts);
	return (uint64_t)o->ts.tv_nsec;
}

static uint64_t work_gtod(struct out *o)
{
	o->ret = gettimeofday(&o->tv, NULL);
	return (uint64_t)o->tv.tv_usec;
}

static uint64_t work_time(struct out *o)
{
	o->t = time(&o->t);
	return (uint64_t)o->t;
}

static struct {
	const char *name;
	work_fn fn;
} cases[] = {
	// 即便是调用 c 语言的函数，对于时间的影响几乎没有
	{ "rdtsc raw", work_rdtsc },
	{ "rdtsc -> ns", work_tsc_ns },
	{ "clock_gettime MONOTONIC", work_cg_mono },
	{ "clock_gettime REALTIME", work_cg_real },
	{ "gettimeofday", work_gtod },

	// 这三个 rdtsc 不会去执行，所以明显要更快
	{ "clock_gettime MONOTONIC_COARSE", work_cg_coarse },
	{ "baseline (empty call)", work_baseline },
	{ "time", work_time },
};

#define NCASES ((int)(sizeof(cases) / sizeof(cases[0])))

static volatile uint64_t g_sink;

static void bench(work_fn fn, struct out *o, uint64_t *samples, int n)
{
	for (int i = 0; i < WARMUP; i++)
		g_sink = fn(o);
	for (int i = 0; i < n; i++) {
		uint64_t start, end;
		uint64_t r;

		start = rdtsc_fenced();
		r = fn(o);
		end = rdtsc_fenced();
		g_sink = r;
		samples[i] = end - start;
	}
}

static int cmp_u64(const void *a, const void *b)
{
	uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;

	return (x > y) - (x < y);
}

static uint64_t samples_buf[SAMPLES];

static void pin_cpu(void)
{
	cpu_set_t allowed, chosen;
	int cpu;

	if (sched_getaffinity(0, sizeof(allowed), &allowed)) {
		perror("sched_getaffinity");
		exit(EXIT_FAILURE);
	}
	for (cpu = 0; cpu < CPU_SETSIZE; cpu++)
		if (CPU_ISSET(cpu, &allowed))
			break;
	if (cpu == CPU_SETSIZE) {
		fprintf(stderr, "No allowed CPU\n");
		exit(EXIT_FAILURE);
	}
	CPU_ZERO(&chosen);
	CPU_SET(cpu, &chosen);
	if (sched_setaffinity(0, sizeof(chosen), &chosen)) {
		perror("sched_setaffinity");
		exit(EXIT_FAILURE);
	}
}

/* 展示几条路径的取整粒度差异 */
static void print_values(void)
{
	struct timespec ts;
	struct timeval tv;
	time_t t;

	clock_gettime(CLOCK_MONOTONIC, &ts);
	printf("CLOCK_MONOTONIC        %" PRId64 ".%09ld s (ns)\n",
	       (int64_t)ts.tv_sec, ts.tv_nsec);
	clock_gettime(CLOCK_MONOTONIC_COARSE, &ts);
	printf("CLOCK_MONOTONIC_COARSE %" PRId64 ".%09ld s (ns)\n",
	       (int64_t)ts.tv_sec, ts.tv_nsec);
	clock_gettime(CLOCK_REALTIME, &ts);
	printf("CLOCK_REALTIME         %" PRId64 ".%09ld s (ns)\n",
	       (int64_t)ts.tv_sec, ts.tv_nsec);
	gettimeofday(&tv, NULL);
	printf("gettimeofday           %" PRId64 ".%06ld s (us)\n",
	       (int64_t)tv.tv_sec, (long)tv.tv_usec);
	t = time(NULL);
	printf("time                   %" PRId64 " s\n", (int64_t)t);
}

int main(void)
{
	struct out o;

	pin_cpu();
	calibrate();
	printf("%d samples/case, TSC %.3f GHz\n\n", SAMPLES,
	       (double)g_tsc_hz / 1e9);

	printf("%-38s %8s %9s %9s\n", "path", "min/tick", "min/ns",
	       "median/ns");
	for (int c = 0; c < NCASES; c++) {
		uint64_t min, med;

		bench(cases[c].fn, &o, samples_buf, SAMPLES);
		qsort(samples_buf, SAMPLES, sizeof(samples_buf[0]), cmp_u64);
		min = samples_buf[0];
		med = samples_buf[SAMPLES / 2];
		printf("%-38s %8" PRIu64 " %9" PRIu64 " %9" PRIu64 "\n",
		       cases[c].name, min,
		       (uint64_t)(min * 1000000000ull / g_tsc_hz),
		       (uint64_t)(med * 1000000000ull / g_tsc_hz));
	}

	printf("\n");
	print_values();
	return 0;
}
