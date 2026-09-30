#include <stdint.h>
#include <stdio.h>

/* Same "calling convention" as do_div:
 * - divide (n << 32) by base
 * - put result in n
 * - return remainder
 */
#define do_shl32_div32(n, base)                              \
	({                                                   \
		uint32_t __quot, __rem;                      \
		asm("divl %2"                                \
		    : "=a"(__quot), "=d"(__rem)              \
		    : "rm"(base), "0"(0), "1"((uint32_t)n)); \
		n = __quot;                                  \
		__rem;                                       \
	})

static uint32_t div_frac(uint32_t dividend, uint32_t divisor)
{
	do_shl32_div32(dividend, divisor);
	return dividend;
}

static void kvm_get_time_scale(uint64_t scaled_hz, uint64_t base_hz,
			       int8_t *pshift, uint32_t *pmultiplier)
{
	uint64_t scaled64;
	int32_t shift = 0;
	uint64_t tps64;
	uint32_t tps32;

	tps64 = base_hz;
	scaled64 = scaled_hz;
	while (tps64 > scaled64 * 2 || tps64 & 0xffffffff00000000ULL) {
		tps64 >>= 1;
		shift--;
	}

	tps32 = (uint32_t)tps64;
	while (tps32 <= scaled64 || scaled64 & 0xffffffff00000000ULL) {
		if (scaled64 & 0xffffffff00000000ULL || tps32 & 0x80000000)
			scaled64 >>= 1;
		else
			tps32 <<= 1;
		shift++;
	}

	*pshift = shift;
	*pmultiplier = div_frac(scaled64, tps32);
}

/*
 * Scale a 64-bit delta by scaling and multiplying by a 32-bit fraction,
 * yielding a 64-bit result.
 */
static __always_inline uint64_t pvclock_scale_delta(uint64_t delta,
						    uint32_t mul_frac,
						    int shift)
{
	uint64_t product;
	unsigned long tmp;
	if (shift < 0)
		delta >>= -shift;
	else
		delta <<= shift;

	__asm__("mulq %[mul_frac] ; shrd $32, %[hi], %[lo]"
		: [lo] "=a"(product), [hi] "=d"(tmp)
		: "0"(delta), [mul_frac] "rm"((uint64_t)mul_frac));

	return product;
}

#define NSEC_PER_SEC 1000000000L
#define NSEC_PER_MSEC 1000000L

struct tk_read_base {
	uint32_t mult;
	uint32_t shift;
};

static inline uint64_t timekeeping_cycles_to_ns(const struct tk_read_base *tkr,
						uint64_t cycles)
{
	/* Calculate the delta since the last update_wall_time() */
	return ((cycles * tkr->mult)) >> tkr->shift;
}

#define do_div(n, base)                           \
	({                                        \
		uint32_t __base = (base);         \
		uint32_t __rem;                   \
		__rem = ((uint64_t)(n)) % __base; \
		(n) = ((uint64_t)(n)) / __base;   \
		__rem;                            \
	})

void clocks_calc_mult_shift(uint32_t *mult, uint32_t *shift, uint32_t from,
			    uint32_t to, uint32_t maxsec)
{
	uint64_t tmp;
	uint32_t sft, sftacc = 32;

	/*
   * Calculate the shift factor which is limiting the conversion
   * range:
   */
	tmp = ((uint64_t)maxsec * from) >> 32;
	while (tmp) {
		tmp >>= 1;
		sftacc--;
	}

	/*
   * Find the conversion shift/mult pair which has the best
   * accuracy and fits the maxsec conversion range:
   */
	for (sft = 32; sft > 0; sft--) {
		tmp = (uint64_t)to << sft;
		tmp += from / 2;
		do_div(tmp, from);
		if ((tmp >> sftacc) == 0)
			break;
	}
	*mult = tmp;
	*shift = sft;
}

struct pvclock_vcpu_time_info {
	uint64_t tsc_timestamp;
	uint64_t system_time;
	uint32_t tsc_to_system_mul;
	int8_t tsc_shift;
}; /* 32 bytes */

static __always_inline uint64_t
__pvclock_read_cycles(const struct pvclock_vcpu_time_info *src, uint64_t tsc)
{
	uint64_t delta = tsc - src->tsc_timestamp;
	uint64_t offset = pvclock_scale_delta(delta, src->tsc_to_system_mul,
					      src->tsc_shift);
	return src->system_time + offset;
}

uint64_t frequency_khz = 2995200l;

#define SECOND_ONE_YEAR (60l * 60 * 24 * 365)
int test_kvmclock()
{
	uint32_t tsc_to_system_mul;
	int8_t tsc_shift;

	printf("=== kvm-clock 一年的误差 === \n");
	// 参考 kvm_guest_time_update
	kvm_get_time_scale(NSEC_PER_SEC, frequency_khz * 1000, &tsc_shift,
			   &tsc_to_system_mul);
	printf("shift=%d tsc_to_system_mul=%u\n", tsc_shift, tsc_to_system_mul);

	uint64_t host_s = SECOND_ONE_YEAR;

	uint64_t host_cycles = host_s * frequency_khz * 1000;
	uint64_t host_ns = host_s * NSEC_PER_SEC;

	printf("host_cycles : %ld\n", host_cycles);

	/* 参考 __pvclock_read_cycles */
	uint64_t guest_ns =
		pvclock_scale_delta(host_cycles, tsc_to_system_mul, tsc_shift);
	printf("diff between host and guest is : %ldns\n",
	       (host_ns - guest_ns));

	return 0;
}

struct data {
	uint64_t tsc[2];
	uint64_t time[2];
};

struct tk_read_base tk;
void init_tk()
{
	clocks_calc_mult_shift(&tk.mult, &tk.shift, frequency_khz,
			       NSEC_PER_MSEC, 600 * 1000);
	printf("tk.shift=%d tk.mult=%u\n", tk.shift, tk.mult);
}

void test_timekeeping()
{
	// 在函数 kvm_get_time_and_clockread 获取两次调用的 kernel_ns 和 tsc_timestamp
	/* [198700.791773] kvm: [martins3:kvm_get_time_and_clockread:2941] 198700628220634 595222500209800 */
	/* [200243.950432] kvm: [martins3:kvm_get_time_and_clockread:2941] 200243787106228 599844569308979 */

	printf("=== timekeeping 的误差 == \n");
	init_tk();
	uint64_t tsc[2] = { 626416616692626, 636700424618294 };
	uint64_t time[2] = { 209115331461933, 212548761217680 };

	uint64_t cycles_diff = tsc[1] - tsc[0];
	uint64_t s = cycles_diff / (frequency_khz * 1000);
	uint64_t cycles = cycles_diff - s * (frequency_khz * 1000);
	uint64_t tk_ns = timekeeping_cycles_to_ns(&tk, cycles);
	uint64_t host_diff_ns = tk_ns + s * 1000000000;

	printf("物理机的时间变化 : %lds\n", (time[1] - time[0]) / 1000000000);
	printf("物理机的时间变化和物理机算法的理论 : %ldns\n",
	       host_diff_ns - (time[1] - time[0]));

	/* 物理机的时间变化 : 245 */
	/* 物理机的时间变化和物理机算法的理论 : -20935ns */

	/* 物理机的时间变化 : 387s */
	/* 物理机的时间变化和物理机算法的理论 : -33070ns */

	/* 物理机的时间变化 : 3433s */
	/* 物理机的时间变化和物理机算法的理论 : -293348ns */

	/* 也就是 timekeeping 计算从 cycle 到 ns 的转换，有 85ns 的误差*/
	printf("timekeeping 一年的误差为 : %ldns ，也就是 %ldms\n",
	       85l * SECOND_ONE_YEAR, (85l * SECOND_ONE_YEAR) / NSEC_PER_MSEC);
}

void test_timekeeping_theory()
{
	// 分析 logarithmic_accumulation 中累积 raw time 时候的误差
	init_tk();
	uint64_t cycles = frequency_khz * 1000; // 假如经过的时间为 1s
						//
	uint64_t snsec_per_sec = NSEC_PER_SEC << tk.shift;
	uint64_t xtime_nsec = cycles * tk.mult;
	printf("snsec_per_sec=%ld xtime_nsec=%ld diff=%ld\n", snsec_per_sec,
	       xtime_nsec, (xtime_nsec -snsec_per_sec) >> tk.shift);
}

int main(int argc, char *argv[])
{
	test_kvmclock();
	test_timekeeping();
	test_timekeeping_theory();
	return 0;
}
