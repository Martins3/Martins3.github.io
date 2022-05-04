#### (ioremap) volatile 和 memory IO 的

1. http://www.informit.com/articles/article.aspx?p=1832575&seqNum=3

对于
```c
a = 1
b = 2
a = 3
```
由于编译器的分析，那么第一条指令会被取消掉，但是这对于该位置是被 IO 映射的，那么该操作是不可接受的。

https://blog.regehr.org/archives/28

## volatile
https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html#Volatiles

## [深入理解 volatile 关键字](https://blog.regehr.org/archives/28)
1. volative 到底实现了什么功能 ?
2. 使用 volative 的位置在什么地方 ?
3. volative 需要硬件如何支持 ? 并不需要特殊指令，但是需要编译器的支持。

The way the volatile connects the abstract and real semantics is this:
> For every read from a volatile variable by the abstract machine, the actual machine must load from the memory address corresponding to that variable.  **Also, each read may return a different value.**  For every write to a volatile variable by the abstract machine, the actual machine must store to the corresponding address.  Otherwise, the address should not be accessed (with some exceptions) and also accesses to volatiles should not be reordered (with some exceptions).

Historically, the connection between the abstract and actual machines was established mainly through accident: **compilers weren’t good enough at optimizing to create an important semantic gap.**


最终结论，不要使用 volatile 而要使用锁。

> 作者是犹他大学的教授，强的一匹。

## lkd 中的描述
Indeed, this is why the variable is marked volatile in `<linux/jiffies.h>`.
The volatile keyword instructs the compiler to reload the variable on each access from main memory and never alias the variable’s value in a register, guaranteeing that the previous loop completes as expected.


## 在 cpp 中间
 static method 不可以被 const 和 [volatile](https://stackoverflow.com/questions/3078237/defining-volatile-class-object) 修饰


#### (mem) asm 和 memory barrier
https://stackoverflow.com/questions/14950614/working-of-asm-volatile-memory
