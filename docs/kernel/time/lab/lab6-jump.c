#define _GNU_SOURCE
#include <sched.h> // For CPU affinity
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void bind_to_core(int core_id) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);

  if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == -1) {
    perror("sched_setaffinity");
    exit(EXIT_FAILURE);
  }
  printf("bind to core %d\n", core_id);
}

static inline uint64_t read_tsc(void) {
  unsigned cycles_low, cycles_high;
  asm volatile("CPUID\n\t" // serialize
               "RDTSC\n\t" // read clock
               "MOV %%edx, %0\n\t"
               "MOV %%eax, %1\n\t"
               : "=r"(cycles_high), "=r"(cycles_low)::"%rax", "%rbx", "%rcx",
                 "%rdx");
  return ((uint64_t)cycles_high << 32) | cycles_low;
}

int main(int argc, char *argv[]) {
  struct timespec ts;
  // 0.1 second = 100,000,000 nanoseconds
  struct timespec delay = {1, 1000000L};
  struct timespec last_ts = {0, 0};
  clockid_t t = CLOCK_MONOTONIC;
  t = CLOCK_MONOTONIC_RAW;

  bind_to_core(0);
  printf("clock id : %d\n", t);

  uint64_t tsc;
  uint64_t last_tsc = 0;

  int found_bug = 0;
  int counter = 0;
  while (1) {
    if (argc >= 2) {
      clock_gettime(t, &ts);
      if (last_ts.tv_sec > ts.tv_sec ||
          (last_ts.tv_sec == ts.tv_sec && last_ts.tv_nsec > ts.tv_nsec)) {
        printf("found bug\n");
        printf("Last Seconds: %ld, Nanoseconds: %ld\n", last_ts.tv_sec,
               last_ts.tv_nsec);
        printf("Now : Seconds: %ld, Nanoseconds: %ld\n", ts.tv_sec, ts.tv_nsec);
        found_bug = 1;
      }
      if (found_bug) {
        counter++;
        if (counter > 1000)
          exit(1);
        printf("->Seconds: %ld, Nanoseconds: %ld\n", ts.tv_sec, ts.tv_nsec);
      } else
        printf("Seconds: %ld, Nanoseconds: %ld\n", ts.tv_sec, ts.tv_nsec);

      last_ts.tv_sec = ts.tv_sec;
      last_ts.tv_nsec = ts.tv_nsec;
    } else {
      tsc = read_tsc();
      if (last_tsc > tsc) {
        printf("Last tsc: %ld, tsc: %ld\n", last_tsc, tsc);
        exit(1);
      }
      printf("%ld\n", tsc);
      last_tsc = tsc;
    }

    // 去掉这个，复现概率大大增加
    /* nanosleep(&delay, NULL); */
  }

  return 0;
}
