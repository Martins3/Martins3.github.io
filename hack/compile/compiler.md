# Compiler for linux kernel

## latent_entropy
- [ ] although I know how it works, but I can't run a demo
   - https://lwn.net/Articles/688492/
   - https://github.com/ephox-gcc-plugins/latent_entropy

## build kernel with clang
- https://www.kernel.org/doc/html/latest/kbuild/llvm.html

- [ ] 但是不知道怎么在交叉编译 arm 

## `__attribute__((destructor))`

https://stackoverflow.com/questions/2053029/how-exactly-does-attribute-constructor-work

## `__ASSEMBLY__`

https://stackoverflow.com/questions/28924355/gcc-assembler-preprocessor-not-compatible-with-standard-headers

when reading `arch/x86/include/asm/idtentry.h`,
I can't find the caller of asm_common_interrupt.
There is only one, but ccls's highlight tell me that it will be compiled because of option `_ASSEMBLY__` not selected.
In fact, things doesn’t happened in that way.
ideentry will be include by normal C file and asm file,
when include by asm, `_ASSEMBLY__` will be selected.

## `__cacheline_aligned`

https://stackoverflow.com/questions/25947962/cacheline-aligned-in-smp-for-structure-in-the-linux-kernel

## asmlinkage
https://stackoverflow.com/questions/10459688/what-is-the-asmlinkage-modifier-meant-for


## volatile
https://gcc.gnu.org/onlinedocs/gcc/Volatiles.html#Volatiles

## [深入理解volatile关键字](https://blog.regehr.org/archives/28)
1. volative 到底实现了什么功能 ?
2. 使用 volative 的位置在什么地方 ? 
3. volative 需要硬件如何支持 ? 并不需要特殊指令，但是需要编译器的支持。

The way the volatile connects the abstract and real semantics is this:
> For every read from a volatile variable by the abstract machine, the actual machine must load from the memory address corresponding to that variable.  **Also, each read may return a different value.**  For every write to a volatile variable by the abstract machine, the actual machine must store to the corresponding address.  Otherwise, the address should not be accessed (with some exceptions) and also accesses to volatiles should not be reordered (with some exceptions).


Historically, the connection between the abstract and actual machines was established mainly through accident: **compilers weren’t good enough at optimizing to create an important semantic gap.**



最终结论，不要使用volatile 而要使用锁。

> 作者是犹他大学的教授，强的一匹。



## 如何正确的切换 gcc 版本
https://badsimplicity.com/2019/07/10/gcc-9-and-ubuntu-kernel-error/

```sh
sudo apt install gcc-8 # 安装
sudo update-alternatives --install /usr/bin/gcc gcc  /usr/bin/gcc-8 1 # 添加 alternatives
sudo update-alternatives --install /usr/bin/gcc gcc  /usr/bin/gcc-8 2 # 添加 alternatives
sudo update-alternatives --config gcc # 选择

sudo update-alternatives --config cc # 设置默认 clang
sudo update-alternatives --config c++
```
