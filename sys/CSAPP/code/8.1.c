#include<stdio.h>
#include<stdlib.h> // malloc 
#include<string.h> // strcmp ..
#include<stdbool.h> // bool false true
#include<stdlib.h> // sort
#include<limits.h> // INT_MAX
#include<math.h> // sqrt

int main(int argc, char *argv[]) {
  
  int i;
  for (i = 0; i < argc; ++i) {
    printf("%s\n", argv[i]);
  }

  char * * envp = &(argv[argc + 1]);

  i = 0;
  while(envp[i] != 0 && envp[i][0] != '\0'){
    printf("%s\n", envp[i]);
    i ++;
  }
  return 0;
}
