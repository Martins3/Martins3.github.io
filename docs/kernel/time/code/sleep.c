// 1.
// https://stackoverflow.com/questions/7794955/why-is-clock-nanosleep-preferred-over-nanosleep-to-create-sleep-times-in-c
// 2. man clock_nanosleep(2)
//
// clock_nanosleep vs nanosleep 的区别在于前者可以选择到底使用那个 clock 的

#include <errno.h>
#include <stdio.h>
#include <time.h>

int main() {
  struct timespec req, rem;

  req.tv_sec = 2;
  req.tv_nsec = 500000000;

  printf("Sleeping for 2 seconds and 500 milliseconds...\n");

  int ret = clock_nanosleep(CLOCK_MONOTONIC, 0, &req, &rem);
  // TODO 这个 return 数值的含义可以 check 一下
  // TODO 当然，对比一下 CLOCK_REALTIME 的区别
  // clock_nanosleep(CLOCK_REALTIME, 0, {tv_sec=1, tv_nsec=0}, 0xfffff77bda18) =
  // 0
  //
  // TODO 可以思考一下，既然 clock_nanosleep 可以是 CLOCK_REALTIME ，
  // 如果睡眠的时候，
  //
  // 还是用这个来计算时间的 delta 的，然后就不管了
  return 0;
}
