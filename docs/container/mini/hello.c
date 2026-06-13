#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  for (;;) {
    sleep(1);
    printf("good\n");
  }
  return 0;
}
