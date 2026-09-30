#include <limits.h>  // INT_MAX
#include <math.h>    // sqrt
#include <stdbool.h> // bool false true
#include <stdio.h>
#include <stdlib.h> // malloc
#include <stdlib.h> // sort
#include <string.h> // strcmp ..

int a[100][10];
int b[10];

void func(int a[10][10]) {}

void func2(int (*a)[10]) {}

void func3(int *a) {}

int main() {
  int(*ptr)[10];
  ptr = a;

  func(a);
  func(ptr);

  func2(a);
  func2(ptr);
  return 0;
}
