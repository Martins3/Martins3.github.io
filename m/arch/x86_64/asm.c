#include "internal.h"

long more_simple_add(long, long);

int test_asm_init(void)
{
	return 0;
}

int test_asm_exit(void)
{
	return 0;
}

int test_asm(long action)
{
	switch (action) {
	case 1:
		pr_info("[martins3:%s:%d] %ld\n", __FUNCTION__, __LINE__,
			more_simple_add(1, 2));
		break;
	}
	return 0;
}
