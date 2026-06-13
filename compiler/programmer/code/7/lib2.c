#include <stdio.h>
#include "lib2.h"
int num = 0;
void foobar(int * i) { printf("Printing from lib2.so %d \n", *i); }
