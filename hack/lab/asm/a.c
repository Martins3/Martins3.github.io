#include<stdio.h>

int main(){
  int answer;
  int operand = 0x100;
  asm ("shrl $8, %0" : "=r" (answer) : "m" (operand) : "cc");
  printf("%x", answer);
  return 0;
}
