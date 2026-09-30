#define _GNU_SOURCE
#include <sched.h> // For CPU affinity
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h> // For usleep

// 获取当前时间的纳秒值
uint64_t get_monotonic_coarse_time_ns() {
  struct timespec ts;
  if (clock_gettime(CLOCK_MONOTONIC_COARSE, &ts) != 0) {
    perror("clock_gettime");
    exit(EXIT_FAILURE);
  }
  return (uint64_t)ts.tv_sec * 1000000000 + ts.tv_nsec;
}read_tsc

// 绑定到指定 CPU core
void bind_to_core(int core_id) {
  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(core_id, &cpuset);

  if (sched_setaffinity(0, sizeof(cpu_set_t), &cpuset) == -1) {
    perror("sched_setaffinity");
    exit(EXIT_FAILURE);
  }
}

int main() {
  uint64_t previous_time = get_monotonic_coarse_time_ns();
  int current_core = 0; // Start with core 0

  // Initial binding to core 0
  bind_to_core(0);

  while (1) {
    uint64_t current_time = get_monotonic_coarse_time_ns();

    // 检查是否发生了时间戳回退
    if (current_time < previous_time) {
      uint64_t rollback_amount = previous_time - current_time;
      fprintf(stderr, "Timestamp rollback detected!\n");
      fprintf(stderr, "Previous time: %lu ns\n", previous_time);
      fprintf(stderr, "Current time: %lu ns\n", current_time);
      fprintf(stderr, "Rollback amount: %lu ns\n", rollback_amount);
    }

    previous_time = current_time;

    // 休眠 1 秒 (1000000 微秒)
    usleep(10);

    // 切换到另一个 core
    current_core = (current_core == 0) ? 22 : 0;
    /* bind_to_core(current_core); */
    /* printf("Switched to core %d\n", current_core); */
  }

  return 0;
}
