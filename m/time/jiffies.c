#include "internal.h"

static void compare(void)
{
	unsigned long jiffies_at_begin = jiffies;
	while (time_after(jiffies_at_begin + HZ * 3, jiffies)) {
		cpu_relax();
	}
}
int test_jiffies(long action)
{
	switch (action) {
	case 1:
		compare();
		break;
	}
	return 0;
}
