/* Read-only HPET/TSC interval comparison, x86-64 Linux.
 * Usage: sudo ./hpet-tsc-compare.out [samples (default 10)]
 * Fixed one-second busy wait; best of 32 reads at each endpoint.
 * Never enables HPET or writes hardware registers. CPUID 0x15 must give Hz.
 *
 * `/dev/hpet` 节点存在，但本内核未启用 `CONFIG_HPET_MMAP`。 实验从 ACPI
 * 表取得地址，通过 `/dev/mem` 的 `O_RDONLY`、`mmap(PROT_READ)` 映射 HPET 页；
 * 当前内核允许该只读映射。程序不写任何硬件寄存器，不加载模块，不重启机器。
 */
#define _GNU_SOURCE
#include <cpuid.h>
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static uint64_t stamp(void)
{
    unsigned lo, hi;
    __asm__ volatile("lfence; rdtsc; lfence"
                     : "=a"(lo), "=d"(hi) : : "memory");
    return ((uint64_t)hi << 32) | lo;
}

struct sample {
    uint64_t before, after;
    uint32_t hpet;
};

static struct sample sample(volatile const uint32_t *counter)
{
    struct sample s;
    s.before = stamp();
    s.hpet = *counter;
    s.after = stamp();
    return s;
}

static struct sample best_sample(volatile const uint32_t *counter)
{
    struct sample best = sample(counter);
    for (int i = 1; i < 32; i++) {
        struct sample s = sample(counter);
        if (s.after - s.before < best.after - best.before)
            best = s;
    }
    return best;
}

static long parse(const char *s, long max)
{
    char *end;
    errno = 0;
    long n = strtol(s, &end, 10);
    if (errno || end == s || *end || n < 1 || n > max) {
        fprintf(stderr, "Invalid numeric argument: %s\n", s);
        exit(1);
    }
    return n;
}

int main(int argc, char **argv)
{
    if (argc > 2) {
        fprintf(stderr, "Usage: %s [samples 1..120]\n", argv[0]);
        return 1;
    }
    long samples = argc > 1 ? parse(argv[1], 120) : 10;
    setvbuf(stdout, NULL, _IOLBF, 0);
    cpu_set_t allowed, chosen;
    if (sched_getaffinity(0, sizeof(allowed), &allowed)) {
        perror("sched_getaffinity");
        return 1;
    }
    int cpu;
    for (cpu = 0; cpu < CPU_SETSIZE && !CPU_ISSET(cpu, &allowed); cpu++)
        ;
    if (cpu == CPU_SETSIZE)
        return 1;
    CPU_ZERO(&chosen);
    CPU_SET(cpu, &chosen);
    if (sched_setaffinity(0, sizeof(chosen), &chosen)) {
        perror("sched_setaffinity");
        return 1;
    }
    unsigned den, num, crystal, unused;
    if (!__get_cpuid_count(0x15, 0, &den, &num, &crystal, &unused) ||
        !den || !num || !crystal) {
        fprintf(stderr, "No complete CPUID 0x15 frequency; do not guess.\n");
        return 1;
    }
    double tsc_hz = (double)crystal * num / den;

    /* ACPI HPET: GAS at byte 40, address at byte 44, little endian x86. */
    unsigned char table[56];
    int fd = open("/sys/firmware/acpi/tables/HPET", O_RDONLY);
    if (fd < 0) {
        perror("open ACPI HPET");
        return 1;
    }
    ssize_t n = read(fd, table, sizeof(table));
    close(fd);
    if (n != sizeof(table) || memcmp(table, "HPET", 4) || table[40] != 0) {
        fprintf(stderr, "Unsupported ACPI HPET table/address space\n");
        return 1;
    }
    uint64_t addr;
    memcpy(&addr, table + 44, sizeof(addr));
    long page = sysconf(_SC_PAGESIZE);
    if (page < 256 || addr % (uint64_t)page) {
        fprintf(stderr, "Unsupported HPET mapping alignment\n");
        return 1;
    }
    fd = open("/dev/mem", O_RDONLY | O_SYNC);
    if (fd < 0) {
        perror("open /dev/mem");
        return 1;
    }
    void *map = mmap(NULL, (size_t)page, PROT_READ, MAP_SHARED, fd, (off_t)addr);
    close(fd);
    if (map == MAP_FAILED) {
        perror("read-only mmap HPET (kernel policy may prohibit it)");
        return 1;
    }
    volatile const uint32_t *regs = map;
    uint32_t period_fs = regs[1], cfg = regs[4];
    if (!(cfg & 1) || !period_fs || period_fs > 100000000) {
        fprintf(stderr, "HPET is disabled or has an invalid period\n");
        munmap(map, (size_t)page);
        return 2;
    }
    puts("sample,tsc_s,hpet_s,hpet_minus_tsc_ns");
    for (long i = 0; i < samples; i++) {
        struct sample start = best_sample(regs + 0xf0 / 4);
        uint64_t begin = stamp();
        while (stamp() - begin < (uint64_t)tsc_hz)
            __asm__ volatile("pause");
        struct sample stop = best_sample(regs + 0xf0 / 4);
        /* Measure the actual interval, including endpoint sampling time.
         * A 32-bit counter is valid only if the interval is below one wrap.
         */
        double cycles = (double)(stop.before - start.before) +
            ((double)(stop.after - stop.before) -
             (double)(start.after - start.before)) / 2;
        double ts = cycles / tsc_hz;
        if (stop.hpet == start.hpet || ts >= 4294967296.0 * period_fs / 1e15) {
            fprintf(stderr, "HPET stopped or sample exceeded counter wrap period\n");
            munmap(map, (size_t)page);
            return 2;
        }
        double hs = (uint32_t)(stop.hpet - start.hpet) * (double)period_fs / 1e15;
        printf("%ld,%.12f,%.12f,%+.3f\n", i, ts, hs, (hs - ts) * 1e9);
    }
    munmap(map, (size_t)page);
    return 0;
}
