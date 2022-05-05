# volatile

## [Nine ways to break your systems code using volatile](https://blog.regehr.org/archives/28) 读后感

简单来说，如果一个变量被 volatile 修饰，那么编译器不能优化对于其的读写操作，必须生成对应的指令。
- volatile 只能保证有指令生成，但是编译器可以调度这些指令
  - > Accesses to non-volatile objects are not ordered with respect to volatile accesses. [^1]
  - 但是 volatiles accesses 不会被 reordered [^2]
- 但是 CPU 未必真的会从内存中访问
- CPU 的执行对于指令的执行顺序也可能是乱序的

时钟计数器是一个经典的 const volatile 变量[^3]
```c
extern const volatile int real_time_clock;
```

勘误，下面的这个说法应该是错误了，从 stackoverflow 的这个回答[^4] 和 https://godbolt.org/ 的测试显示，asm volatile ("" : : : "memory") 只是阻止了指令调度，而没有进行将寄存器写回内存的操作。
> The effect is that the compiler dumps all registers to RAM before the barrier and reloads them afterwards.  Moreover, code motion is not permitted around the barrier in either direction.

## 在内核中的使用案例
> Indeed, this is why the variable is marked volatile in `<linux/jiffies.h>`.
> The volatile keyword instructs the compiler to reload the variable on each access from main memory and never alias the variable’s value in a register,
> guaranteeing that the previous loop completes as expected.
>
> Linux Kernel Development: Love, Robert


## memory order
stackoverflow : Working of asm volatile ("" : : : "memory") [^4] 介绍了 asm volatile ("" : : : "memory") 的基本原理，这里引出来
compiler barrier 和 CPU barrier 。

- [ ] 好吧，我不是非常理解，为什么 x86 会存在 mfence 和 lfence 的啊?
  - [ ] 不是说好的 x86 是 strong model, 所以 asm volatile("":::memory) 不用生成什么，那么为什么还存在 mfence
  - https://stackoverflow.com/questions/12183311/difference-in-mfence-and-asm-volatile-memory
  - https://stackoverflow.com/questions/27595595/when-are-x86-lfence-sfence-and-mfence-instructions-required
- [ ] arm 也是弱序，为什么 asm volatile 没有生成任何东西
- 反正 CPU 都可以进行调度，那么 asm volatile 在 x86 中什么都不生成，那么有个什么作用啊

[stackoverflow : What does—or did—"volatile void function( ... )" do?](https://stackoverflow.com/questions/14288603/what-does-or-did-volatile-void-function-do)
> volatile void as a function return value in C (but not in C++) is equivalent to __attribute__((noreturn)) on the function and tells the compiler that the function never returns.

[可以使用 volatile 或者 const 修饰 static 函数吗 ?](https://stackoverflow.com/questions/3078237/defining-volatile-class-object)

```c
bool static push(struct Data element) volatile;  // 这种形式，不可以
bool static volatile push(struct Data element); // 这种形式，可以
```

[^1]: https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html
[^2]: https://stackoverflow.com/questions/14785639/may-accesses-to-volatiles-be-reordered
[^3]: https://stackoverflow.com/questions/4592762/difference-between-const-const-volatile
[^4]: https://stackoverflow.com/questions/14950614/working-of-asm-volatile-memory
