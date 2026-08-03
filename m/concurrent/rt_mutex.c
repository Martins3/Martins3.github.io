#include "internal.h"

/*
 *
 * Real-time mutexes (RT-mutexes) are another form of mutex supported by the kernel.

In contrast
to regular mutexes, they implement *priority inheritance*, which, in turn, allows for solving (or, at least,
attenuating) the effects of *priority inversion*. Both are well-known effects, respectively, methods and are
discussed in most operating systems textbooks

 * */

int test_rt_mutex(long action)
{
	switch (action) {
	case 0:
		break;
	}
	return 0;
}
