#include<stdio.h>
#include<stdlib.h> // malloc 
#include<string.h> // strcmp ..
#include<stdbool.h> // bool false true
#include<stdlib.h> // sort
#include<limits.h> // INT_MAX
#include<math.h> // sqrt

int main(int argc, char *argv[], char * envp[]) {
  
  int i;
  for (i = 0; i < argc; ++i) {
    printf("%s\n", argv[i]);
  }

  while(envp[i] != NULL){
    printf("%s\n", argv[i]);
    i ++;
  }


  return 0;
}
