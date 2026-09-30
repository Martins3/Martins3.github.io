// SPDX-License-Identifier: GPL-2.0
/*
 * 直接读 userspace 可见的 [vvar] 页，观察 time namespace 下的页布局切换：
 *
 *   普通任务:  [vvar]+0      = 真正的 vvar(vdso_u_time_data)
 *              [vvar]+4K     = 未使用
 *
 *   timens 任务: [vvar]+0    = 该 time namespace 专属的 vvar，
 *                             clock_mode = VDSO_CLOCKMODE_TIMENS(=INT_MAX)
 *                             offset[] 里是该 namespace 的偏移
 *              [vvar]+4K     = 被挪过来的真正 vvar
 *
 * 结构体定义直接抄自 include/vdso/datapage.h，并且按本次实验内核的 .config
 * 特化：CONFIG_GENERIC_VDSO_OVERFLOW_PROTECT=y、无 ARCH_HAS_VDSO_TIME_DATA、
 * CONFIG_POSIX_AUX_CLOCKS=n。
 *
 *   gcc -O0 -o timens-vvar timens-vvar.c
 *   ./timens-vvar
 */
#define _GNU_SOURCE
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define VDSO_BASES	12
#define VDSO_CLOCKMODE_TIMENS	0x7fffffff

struct timens_offset {
	int64_t sec;
	uint64_t nsec;
};

struct vdso_clock {
	uint32_t seq;
	int32_t clock_mode;
	uint64_t cycle_last;
	uint64_t max_cycles;
	uint64_t mask;
	uint32_t mult;
	uint32_t shift;
	union {
		struct {
			uint64_t sec;
			uint64_t nsec;
		} basetime[VDSO_BASES];
		struct timens_offset offset[VDSO_BASES];
	};
};

struct vdso_time_data {
	struct vdso_clock clock_data[2];
	int32_t tz_minuteswest;
	int32_t tz_dsttime;
	uint32_t hrtimer_res;
	uint32_t __unused;
} __attribute__((aligned(64)));

static const char *clkname[] = {
	"REALTIME", "MONOTONIC", "PROCESS_CPUTIME", "THREAD_CPUTIME",
	"MONOTONIC_RAW", "REALTIME_COARSE", "MONOTONIC_COARSE",
	"BOOTTIME", "REALTIME_ALARM", "BOOTTIME_ALARM", "SGI_CYCLE", "TAI",
};

static sigjmp_buf jb;

static void sigbus_handler(int sig)
{
	(void)sig;
	siglongjmp(jb, 1);
}

static unsigned long find_vvar(void)
{
	char line[512];
	FILE *f = fopen("/proc/self/maps", "r");

	if (!f)
		return 0;
	while (fgets(line, sizeof(line), f)) {
		if (strstr(line, "[vvar]")) {
			unsigned long addr;

			if (sscanf(line, "%lx", &addr) == 1) {
				fclose(f);
				return addr;
			}
		}
	}
	fclose(f);
	return 0;
}

static void dump_page(const char *tag, unsigned long addr)
{
	const struct vdso_time_data *vd = (const void *)addr;
	int i;

	if (sigsetjmp(jb, 1)) {
		printf("  SIGBUS: 这个页没有映射\n");
		return;
	}

	printf("%s (addr=%#lx)\n", tag, addr);
	printf("  clock_data[CS_HRES_COARSE]: seq=%u clock_mode=%d%s\n",
	       vd->clock_data[0].seq, vd->clock_data[0].clock_mode,
	       vd->clock_data[0].clock_mode == VDSO_CLOCKMODE_TIMENS ?
	       "  <== VDSO_CLOCKMODE_TIMENS" : "");
	printf("  clock_data[CS_RAW]        : seq=%u clock_mode=%d%s\n",
	       vd->clock_data[1].seq, vd->clock_data[1].clock_mode,
	       vd->clock_data[1].clock_mode == VDSO_CLOCKMODE_TIMENS ?
	       "  <== VDSO_CLOCKMODE_TIMENS" : "");

	if (vd->clock_data[0].clock_mode != VDSO_CLOCKMODE_TIMENS) {
		printf("  (不是 timens 页，basetime 是真正的时间基)\n");
		return;
	}

	printf("  offset[]:\n");
	for (i = 0; i < VDSO_BASES; i++) {
		if (!vd->clock_data[0].offset[i].sec &&
		    !vd->clock_data[0].offset[i].nsec)
			continue;
		printf("    %-18s sec=%lld nsec=%llu\n", clkname[i],
		       (long long)vd->clock_data[0].offset[i].sec,
		       (unsigned long long)vd->clock_data[0].offset[i].nsec);
	}
}

int main(void)
{
	unsigned long vvar = find_vvar();
	struct sigaction sa = { .sa_handler = sigbus_handler };

	sigaction(SIGBUS, &sa, NULL);

	if (!vvar) {
		printf("[vvar] not found\n");
		return 1;
	}

	dump_page("vvar + 0  (VDSO_TIME_PAGE_OFFSET)", vvar);
	printf("\n");
	dump_page("vvar + 4K (VDSO_TIMENS_PAGE_OFFSET)",
		  vvar + 4096);

	return 0;
}
