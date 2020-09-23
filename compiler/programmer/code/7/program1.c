#include "lib.h"
#include "lib2.h"


#include <stdio.h>

extern int global_var;
void fuck(){
  printf("%s\n", "fuck");
}

int main(int argc, char *argv[]) {
  int a = 111;
  foobar(&a);
  num++;
  printf("%d\n", num);
  printf("%d\n", global_var);
  return 0;
}
