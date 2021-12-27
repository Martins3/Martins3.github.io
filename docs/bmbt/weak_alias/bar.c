#include <stdio.h>

extern int x;
void bar() { printf("%s %d \n", __FUNCTION__, x); }
