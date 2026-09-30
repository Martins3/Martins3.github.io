#include <stdint.h>

/* Volatile keeps the example's actual guest-memory accesses visible. */
static volatile uint64_t seed = 7;
static volatile uint64_t results[4];

static void guest_putc(char value)
{
	/* The VMM handles this port as a debug console, not as a UART. */
	__asm__ volatile("outb %0, %1" : : "a"((uint8_t)value), "Nd"((uint16_t)0xe9));
}

static void guest_puts(const char *text)
{
	while (*text)
		guest_putc(*text++);
}

static void guest_put_u64(uint64_t value)
{
	char digits[20];
	unsigned int count = 0;

	do {
		digits[count++] = '0' + value % 10;
		value /= 10;
	} while (value);
	while (count)
		guest_putc(digits[--count]);
}

/* Leave a real C function call in the demo even with optimization enabled. */
__attribute__((noinline)) static uint64_t calculate(uint64_t value)
{
	return value * 6;
}

int guest_main(void)
{
	guest_puts("Hello from C guest!\n");
	for (unsigned int i = 0; i < 4; i++) {
		if (results[i] != 0) {
			guest_puts("BSS check failed\n");
			return 1;
		}
	}
	results[0] = calculate(seed);
	guest_puts("BSS zeroed; result = ");
	guest_put_u64(results[0]);
	guest_putc('\n');
	return results[0] == 42 ? 0 : 2;
}
