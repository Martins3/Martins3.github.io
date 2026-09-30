#include <stdio.h>
#include <stdint.h>

#define NSEC_PER_SEC 1000000000L
#define NSEC_PER_MSEC 1000000L

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

struct tk_read_base {
	uint32_t mult;
	uint32_t shift;
};

uint64_t frequency_khz = 2995200l;
struct tk_read_base tk;

void init()
{
	/*
	 * 忽然意识到，这个问题是有范围的。
	 *
	 * 计算的一模一样 :
	 * [    0.689508] clocksource: [martins3:clocks_calc_mult_shift:63] 2995200 1000000 600000
	 * [    0.689845] clocksource: [martins3:clocks_calc_mult_shift:88] 5601368 24
	 */
	clocks_calc_mult_shift(&tk.mult, &tk.shift, frequency_khz,
			       NSEC_PER_MSEC, 600 * 1000);
	printf("tk.shift=%d tk.mult=%u\n", tk.shift, tk.mult);
}

static inline uint64_t timekeeping_cycles_to_ns(const struct tk_read_base *tkr,
						uint64_t cycles)
{
	/* Calculate the delta since the last update_wall_time() */
	return ((cycles * tkr->mult)) >> tkr->shift;
}

void test3()
{
	// 获取两次调用的 kernel_ns 和 tsc_timestamp
	/* [198700.791773] kvm: [martins3:kvm_get_time_and_clockread:2941] 198700628220634 595222500209800 */
	/* [200243.950432] kvm: [martins3:kvm_get_time_and_clockread:2941] 200243787106228 599844569308979 */

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
}

int main(int argc, char *argv[])
{
	init();
	test3();
	return 0;
}
