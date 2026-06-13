#include <assert.h>  // assert
#include <errno.h>   // strerror
#include <fcntl.h>   // open
#include <limits.h>  // INT_MAX
#include <math.h>    // sqrt
#include <stdbool.h> // bool false true
#include <stdio.h>
#include <stdlib.h> // malloc sort
#include <string.h> // strcmp
#include <unistd.h> // sleep

int main(int argc, char *argv[]) {
  const char *path = "/sys/block/sda/device/timeout";
  sleep(3);
  FILE *f = fopen(path, "r");
  if (f == NULL) {
    exit(1);
  } else {
    char a[100];
    fread(a, 1, 100, f);
    printf("[huxueshi:%s:%d] %s\n", __FUNCTION__, __LINE__, a);
    exit(0);
  }

  while (1) {
    printf("[huxueshi:%s:%d] \n", __FUNCTION__, __LINE__);
    sleep(1);
  }
  return 0;
}
