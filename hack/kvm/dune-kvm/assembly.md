## links
- [`.no at`](https://stackoverflow.com/questions/7911964/is-set-noat-unsupported-for-mips-assembly)
- [mips doesn't allow access pc directly](https://stackoverflow.com/questions/54830168/mips-assembler-can-we-initialize-the-program-counter-on-our-own)
    - https://stackoverflow.com/questions/15331033/how-to-get-current-pc-register-value-on-mips-arch 
- https://stackoverflow.com/questions/744055/gcc-inline-assembly-jump-to-label-outside-block
```c
#include <stdio.h>

int
main(void)
{
  asm("jmp label");
  puts("You should not see this.");
  asm("label:");

  return 0;
}
```
- [what's hi and lo in MIPS](https://stackoverflow.com/questions/2320196/in-mips-what-are-hi-and-lo)

- :star: [A short reference for MIPS](http://www.sci.tamucc.edu/~sking/Courses/Compilers/Assignments/MIPS.html)
- [read register to variable with inline assembly code](https://stackoverflow.com/questions/7944071/read-mips-cpu-register-using-asm-instruction)
```c
 unsigned int x;
 asm volatile ("move %0, $ra" : "=r" (x));
 asm volatile ("sw $ra, %0" : "=m" (x));
```
## [MIPS Assembly Language Programmer’s Guide](http://vbrunell.github.io/docs/MIPS%20Programming%20Guide.pdf)
