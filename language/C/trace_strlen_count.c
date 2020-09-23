#include <assert.h>  // assert
#include <limits.h>  // INT_MAX
#include <math.h>    // sqrt
#include <stdbool.h> // bool false true
#include <stdio.h>
#include <stdlib.h> // malloc
#include <stdlib.h> // sort
#include <string.h> // strcmp ..
#include <string.h>
#include <unistd.h> // sleep

void fuckoff(char *wowo) {
  int x = 12;
  if (wowo[0] == 'f') {
    x = 12;
  } else {
    x = 13;
  }

  if (x == 123) {
    printf("fasdfa");
  }
  printf("%s", wowo);
}

int main() {
  int x = 12;
  for (int i = 0; i < 100; ++i) {
    fuckoff("sssss");
  }
  printf("%d", x);
  return 0;
}
