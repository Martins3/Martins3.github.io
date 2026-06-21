#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

int main(int argc, char *argv[])
{
	register uint64_t x0 __asm__("x0");
	__asm__("mrs x0, CurrentEL;" : : : "%x0");
	printf("EL = %" PRIu64 "\n", x0 >> 2);

	return 0;
}
