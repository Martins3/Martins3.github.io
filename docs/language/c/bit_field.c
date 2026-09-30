#include <stdio.h>
#include <stdlib.h>  // malloc
#include <string.h>  // strcmp ..
#include <stdbool.h> // bool false true
#include <stdlib.h>  // sort
#include <limits.h>  // INT_MAX
#include <math.h>    // sqrt
#include <unistd.h>  // sleep
#include <assert.h>  // assert

struct {
  int  a : 1; // 1 bit  - bit 0
  int  b : 2; // 2 bits - bits 2 down to 1
  char c ;    // 8 bits - bits 15 down to 8
  char d;
  char e;
} reg1;

/* https://stackoverflow.com/questions/54054427/different-between-c-struct-bitfields-on-char-and-on-int */
int main(){
  printf("%lu", sizeof(reg1));
  return 0;
}
