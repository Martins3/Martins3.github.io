#include <stdio.h>
#include <stdlib.h>

void foo(void) { printf("%s\n", "bye"); }

int main() {
  atexit(foo);
  printf("%s\n", "End of main");
}
