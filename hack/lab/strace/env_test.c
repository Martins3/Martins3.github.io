#include <stdio.h>
#include <stdlib.h>

int main(void) {
  char *p = NULL;
  if ((p = getenv("HG"))) {
    printf("HG enviromental variable value is %s\n", p);
  } else {
    printf("HG enviromental variable value is not set\n");
  }
  return 0;
}
