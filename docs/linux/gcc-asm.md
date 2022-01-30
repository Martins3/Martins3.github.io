我感觉可以按照 makefile 的教程然后重写一下这个东西

https://gcc.gnu.org/onlinedocs/gcc/Using-Assembly-Language-with-C.html

## 一些有趣的东西

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
