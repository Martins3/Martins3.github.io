#include "lib.h"
#include <stdio.h>
#define LEARN_PIC

#ifdef LEARN_PIC
static int a;
extern int b;
extern void ext();

void bar() {
  a = 1;
  b = 2;
}

void foo() {
  bar();
  ext();
}
#endif

int num = 0;
void foobar(int *i) {
#ifdef LEARN_PIC
  foo();
#endif
  printf("Printing from lib.so %d \n", *i);
}
