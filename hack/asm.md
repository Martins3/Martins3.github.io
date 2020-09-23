# Tutorial about assembly

- [ ] this is enough https://mentorembedded.github.io/advancedlinuxprogramming/alp-folder/alp-ch09-inline-asm.pdf

## inline assembly

#### eg 1
`__asm__ __volatile__ ("" ::: "memory")`
https://stackoverflow.com/questions/14950614/working-of-asm-volatile-memory

- [ ] why three `:` ?
- [ ] `=t` `=r`

#### eg2
```c
#include<stdio.h>

int main(){
  int answer;
  int operand = 0x100;
  asm ("shrl $8, %0" : "=r" (answer) : "r" (operand) : "cc");
  /* asm ("shrl $8, %0" : "=m" (answer) : "m" (operand) : "cc"); */
  printf("%x", answer);
  return 0;
}
```
why change to `=m` `m` failed ?
- [ ] check the asm
