#include <linux/wait_bit.h>
#include "internal.h"
/**
 * wait_on_bit 的功能: 等待 bit 被清理掉，如果本来就没设置 bit ，那么就会直接通过
 *
 * 通过 bit_waitqueue 可以看到，使用 hash ，让不同的 bit 共享一个 wait queue 。
 * 可以看看 hash 到一个 bucket 中的东西如何做到互相不影响的。
 */

struct phone {
	unsigned long flags;
};

static struct phone my_phone;

enum PHONE_TYPE { PHONE_TYPE_XIAOMI, PHONE_TYPE_IPHONE };
int test_waitbit(long action)
{
	switch (action) {
	case 0:
		my_phone.flags = 1 << PHONE_TYPE_XIAOMI;
		wait_on_bit(&(my_phone.flags), PHONE_TYPE_XIAOMI,
			    TASK_UNINTERRUPTIBLE);
		break;
	case 1:
		wake_up_bit(&(my_phone.flags), PHONE_TYPE_XIAOMI);
		break;
	case 2:
		my_phone.flags &= ~(1 << PHONE_TYPE_XIAOMI);
		break;
	}
	return 0;
}
