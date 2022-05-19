#include <assert.h>  // assert
#include <fcntl.h>   // open
#include <limits.h>  // INT_MAX
#include <math.h>    // sqrt
#include <stdbool.h> // bool false true
#include <stdio.h>
#include <stdlib.h> // malloc sort
#include <string.h> // strcmp ..
#include <sys/sdt.h>
#include <unistd.h> // sleep

void hello() { printf("[huxueshi:%s:%d] \n", __FUNCTION__, __LINE__); }
void hello_two() { printf("[huxueshi:%s:%d] \n", __FUNCTION__, __LINE__); }

int add(int a, int b) {
  a = 1111;
  b = 1111;
  return a + b;
}

int main(int argc, char *argv[]) {
  printf("[huxueshi:%s:%d] \n", __FUNCTION__, __LINE__);
  DTRACE_PROBE("hello-usdt", probe - main);
  for (int i = 0; i < 10; ++i) {
    hello();
    hello_two();
    hello_two();
  }
  add(1, 2);
  return 0;
}
