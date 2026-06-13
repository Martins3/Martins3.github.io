#include <stdio.h>

int x = 15213;
int y = 15212;

void f(void);

int main() {

  f();
  printf("x = 0x%x y = 0x%x \n", x, y);
  return 0;
}
