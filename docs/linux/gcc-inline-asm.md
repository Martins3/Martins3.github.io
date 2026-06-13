我感觉可以按照 makefile 的教程然后重写一下这个东西

https://gcc.gnu.org/onlinedocs/gcc/Using-Assembly-Language-with-C.html

## 分析一下
[loongson-community/musl](https://github.com/loongson-community/musl/blob/b8191f5561f351da90e120cab4a739872808b8de/arch/loongarch64/syscall_arch.h#L17)

```c
static inline long __syscall0(long n)
{
  register long a0 __asm__("$a0");
  register long a7 __asm__("$a7") = n;

  __asm__ __volatile__ (
    "syscall 0"
    : "+&r"(a0)
    : "r"(a7)
    : SYSCALL_CLOBBERLIST);
  return a0;
}
```

glibc:
```c
internal_syscall0(number, err, dummy...){
  long int _sys_result;
  {
  register long int __a7 asm ("$a7") = number;
  register long int __a0 asm ("$a0");
  __asm__ volatile (
  "syscall  0\n\t"
  : "=r" (__a0)
  : "r" (__a7)
  : __SYSCALL_CLOBBERS);
  _sys_result = __a0;
  }
  _sys_result;
}
```



## asm clobber
```c
#include <assert.h>  // assert
#include <fcntl.h>   // open
#include <limits.h>  // INT_MAX
#include <math.h>    // sqrt
#include <stdbool.h> // bool false true
#include <stdio.h>
#include <stdlib.h> // malloc sort
#include <string.h> // strcmp ..
#include <unistd.h> // sleep

#define __SYSCALL_CLOBBERS                                                     \
  "$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7", "$t8", "memory"

long hello(long arg0, long arg1, long arg2, long arg3,
                                  long arg4, long arg5, long arg6,
                                  long number) {
  return 77;
}

long __syscall0(long number) {
  long int _sys_result;

  {
    register long int __a7 asm("$a7") = number;
    register long int __a0 asm("$a0");
    __asm__ volatile("bl hello" "\n\t"
                     : "=r"(__a0)
                     : "r"(__a7)
                     : __SYSCALL_CLOBBERS);
    _sys_result = __a0;
  }
  asm(
    "move $ra, $zero \n\t"
  );
  return _sys_result;
}

int main(int argc, char *argv[]) {
  printf("%ld\n", __syscall0(12));
  return 0;
}
```

## 找个经典例子
musl/arch/x86_64/atomic_arch.h
```c
static inline int a_cas(volatile int *p, int t, int s)
{
  __asm__ __volatile__ (
    "lock ; cmpxchg %3, %1"
    : "=a"(t), "=m"(*p) : "a"(t), "r"(s) : "memory" );
  return t;
}
```
- [ ] lock ?
- [ ] =a
- [ ] "a"
- [ ] "r"
- [ ] "memory"
- [ ] 为什么要强调参数是 volatile 的

### Asm label
```c
/* Declare sqrt for use within GLIBC.  Compilers typically inline sqrt as a
   single instruction.  Use an asm to avoid use of PLTs if it doesn't.  */
float (sqrtf) (float) asm ("__ieee754_sqrtf");
double (sqrt) (double) asm ("__ieee754_sqrt");
```


# Tutorial about assembly

- [ ] analyze this file : /home/maritns3/core/musl/arch/mips64/syscall_arch.h

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


## example

来源: https://zhuanlan.zhihu.com/p/41872203
```c
inline void xchg(volatile int *x, volatile int *y) {
  asm volatile("xchgl %0, %1"
               : "+r"(x), "+m"(y)::"memory", "cc"); // swap(x, tmp)
}
```
