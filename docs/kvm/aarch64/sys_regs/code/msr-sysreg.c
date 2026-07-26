#include <stdio.h>
#include <stdint.h>

static inline uint64_t read_id_aa64dfr0_el1(void)
{
    uint64_t val;
    __asm__ __volatile__("mrs %0, id_aa64dfr0_el1\n" : "=r" (val));
    return val;
}

int main(void)
{
    uint64_t dfr0 = read_id_aa64dfr0_el1();

    printf("ID_AA64DFR0_EL1 = 0x%016llx\n\n", (unsigned long long)dfr0);

    return 0;
}
