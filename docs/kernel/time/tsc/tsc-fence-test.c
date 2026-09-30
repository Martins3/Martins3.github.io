#define _GNU_SOURCE
#include <inttypes.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Intel x86-64: demonstrate RDTSC overtaking an unfinished load.
 * Build: gcc -O2 -Wall -Wextra -o tsc-fence-test.out tsc-fence-test.c
 * Run: ./tsc-fence-test.out
 * Results are raw TSC ticks, not core clock cycles; no overhead subtraction.
 */
#define SAMPLES 20000
#define CASES 10

static uint64_t cache_line[8] __attribute__((aligned(64))) = { 42 };
static uint64_t samples[CASES][SAMPLES];

/* Keep the load and both timestamps in one asm block so the compiler cannot
 * move/delete the load. The start boundary is identical in every variant.
 * The final LFENCE waits for outstanding work AFTER the end timestamp, so
 * unfinished loads cannot carry into the next sample. It cannot fix an end
 * timestamp that was already read too early.
 */
#define DEFINE_MEASURE(name, work, end_fence, end_read)                        \
	static __attribute__((noinline)) uint64_t name(const uint64_t *ptr)    \
	{                                                                      \
		unsigned start_low, start_high, end_low, end_high;             \
		asm volatile("lfence\n\t"                                      \
			     "rdtsc\n\t"                                       \
			     "mov %%eax, %[start_low]\n\t"                     \
			     "mov %%edx, %[start_high]\n\t"                    \
			     "lfence\n\t" work end_fence end_read "lfence\n\t" \
			     : [start_low] "=&r"(start_low),                   \
			       [start_high] "=&r"(start_high), "=&a"(end_low), \
			       "=&d"(end_high)                                 \
			     : [ptr] "r"(ptr)                                  \
			     : "rbx", "rcx", "cc", "memory");                  \
		return (((uint64_t)end_high << 32) | end_low) -                \
		       (((uint64_t)start_high << 32) | start_low);             \
	}

DEFINE_MEASURE(empty_unfenced, "", "", "rdtsc\n\t")
DEFINE_MEASURE(empty_fenced, "", "lfence\n\t", "rdtsc\n\t")
DEFINE_MEASURE(load_unfenced, "mov (%[ptr]), %%rcx\n\t", "", "rdtsc\n\t")
DEFINE_MEASURE(load_fenced, "mov (%[ptr]), %%rcx\n\t", "lfence\n\t",
	       "rdtsc\n\t")
DEFINE_MEASURE(load_rdtscp, "mov (%[ptr]), %%rcx\n\t", "", "rdtscp\n\t")

/* Fix the CPUID leaf/subleaf so its cost does not depend on register residue.
 * CPUID and its input setup are inside the measured interval. All variants
 * share the same clobbers, keeping register allocation comparable.
 */
#define CPUID_FENCE "xor %%eax, %%eax\n\txor %%ecx, %%ecx\n\tcpuid\n\t"
DEFINE_MEASURE(empty_cpuid, "", CPUID_FENCE, "rdtsc\n\t")
DEFINE_MEASURE(load_cpuid, "mov (%[ptr]), %%rcx\n\t", CPUID_FENCE, "rdtsc\n\t")

static int pin_cpu(void)
{
	cpu_set_t allowed, selected;
	int cpu;

	if (sched_getaffinity(0, sizeof(allowed), &allowed)) {
		perror("sched_getaffinity");
		exit(EXIT_FAILURE);
	}
	for (cpu = 0; cpu < CPU_SETSIZE; cpu++) {
		if (CPU_ISSET(cpu, &allowed))
			break;
	}
	if (cpu == CPU_SETSIZE) {
		fprintf(stderr, "No allowed CPU\n");
		exit(EXIT_FAILURE);
	}
	CPU_ZERO(&selected);
	CPU_SET(cpu, &selected);
	if (sched_setaffinity(0, sizeof(selected), &selected)) {
		perror("sched_setaffinity");
		exit(EXIT_FAILURE);
	}
	return cpu;
}

static int compare(const void *a, const void *b)
{
	uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;

	return (x > y) - (x < y);
}

int main(void)
{
	enum cache_state { EMPTY, WARM, COLD };
	const struct {
		const char *name;
		uint64_t (*measure)(const uint64_t *);
		enum cache_state cache;
	} cases[CASES] = {
		{ "empty / no end fence", empty_unfenced, EMPTY },
		{ "empty / end fence", empty_fenced, EMPTY },
		{ "warm load / no end fence", load_unfenced, WARM },
		{ "warm load / end fence", load_fenced, WARM },
		{ "cold load / no end fence", load_unfenced, COLD },
		{ "cold load / end fence", load_fenced, COLD },
		{ "cold load / RDTSCP", load_rdtscp, COLD },
		{ "empty / CPUID", empty_cpuid, EMPTY },
		{ "warm load / CPUID", load_cpuid, WARM },
		{ "cold load / CPUID", load_cpuid, COLD },
	};

	printf("CPU %d, %d samples/case, raw TSC ticks\n", pin_cpu(), SAMPLES);
	/* Rotate case order to reduce bias from drift; discard warmup samples. */
	for (int i = -1000; i < SAMPLES; i++) {
		for (int j = 0; j < CASES; j++) {
			int c = (i + 1002 + j) % CASES;
			uint64_t elapsed;

			if (cases[c].cache == COLD) {
				asm volatile("clflush (%0)\n\tmfence"
					     :
					     : "r"(cache_line)
					     : "memory");
			} else if (cases[c].cache == WARM) {
				asm volatile("mov (%0), %%rcx\n\tlfence"
					     :
					     : "r"(cache_line)
					     : "rcx", "memory");
			}
			elapsed = cases[c].measure(cache_line);
			if (i >= 0)
				samples[c][i] = elapsed;
		}
	}
	printf("%-26s %8s %8s %8s %8s\n", "case", "min", "median", "p90",
	       "p99");
	for (int c = 0; c < CASES; c++) {
		qsort(samples[c], SAMPLES, sizeof(samples[c][0]), compare);
		printf("%-26s %8" PRIu64 " %8" PRIu64 " %8" PRIu64 " %8" PRIu64
		       "\n",
		       cases[c].name, samples[c][0], samples[c][SAMPLES / 2],
		       samples[c][SAMPLES * 90 / 100],
		       samples[c][SAMPLES * 99 / 100]);
	}
	return 0;
}
