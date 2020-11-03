# Tutorial about assembly

- [ ] this is enough https://mentorembedded.github.io/advancedlinuxprogramming/alp-folder/alp-ch09-inline-asm.pdf

- [ ] https://www.kernel.org/doc/html/latest/asm-annotations.html : kernel doc for 

- [ ] really worth reading : https://www.ibiblio.org/gferg/ldp/GCC-Inline-Assembly-HOWTO.html

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
- [ ] check the assembly line by line, please !


- [ ] understand it ?
```c
	asm volatile ("call preempt_schedule_thunk" : ASM_CALL_CONSTRAINT)
```

## TODO
https://stackoverflow.com/questions/27594297/how-to-print-a-string-to-the-terminal-in-x86-64-assembly-nasm-without-syscall


## examples
https://stackoverflow.com/questions/10461798/asm-code-containing-0-what-does-that-mean
```c
int a=10, b;
asm ("movl %1, %%eax; 
      movl %%eax, %0;"
     :"=r"(b)        /* output */
     :"r"(a)         /* input */
     :"%eax"         /* clobbered register */
     );
```

## clobbered register


## Official Documentation
https://gcc.gnu.org/onlinedocs/gcc/Using-Assembly-Language-with-C.html#Using-Assembly-Language-with-C

**This is main framework, merge anything here!**

### Basic asm
[Basic-Asm](https://gcc.gnu.org/onlinedocs/gcc/Basic-Asm.html#Basic-Asm)

too basic ?

### extended asm
[Extended-Asm](https://gcc.gnu.org/onlinedocs/gcc/Extended-Asm.html#Extended-Asm)

#### 6.47.2.5 Input Operands
- When not using an asmSymbolicName, use the (zero-based) position of the operand in the list of operands in the assembler template. 


### constraints 
[](https://stackoverflow.com/questions/3323445/what-is-the-difference-between-asm-asm-and-asm)

## Awesome
- [ ] https://www.agner.org/optimize/
