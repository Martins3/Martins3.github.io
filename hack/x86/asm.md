# ASM
记录如何书写 .S 文件，记录在 entry.S 的遇到的任何问题

- [ ] [这是绝对的核心](https://cs.lmu.edu/~ray/notes/nasmtutorial/)
- [ ] [可以作为实战](x86-bare-metal-examples)


- 可以用来学习一下各种 SIMD 指令的写法吧 : https://github.com/pigirons/cpufp


## as(GNU Assembler)
https://en.wikipedia.org/wiki/X86_assembly_language
https://en.wikibooks.org/wiki/X86_Assembly/GAS_Syntax#Additional_GAS_reading 入门教程
https://sourceware.org/binutils/docs/as/ 完整文档
https://cs.lmu.edu/~ray/notes/gasexamples/ 最佳入门内容

## gas 关键语法

### code16
1. .code32 && .code16

https://stackoverflow.com/questions/32395542/objdump-of-code16-and-code32-x86-assembly
https://stackoverflow.com/questions/26539603/why-bootloaders-for-x86-use-16bit-code-first/31528128

`.code` 16 tells the assembler to assume the code will be run in 16bit mode

### macro


### global
- [ ] 应该是让一个 label 或者变量是全局的吧!



## 问题
#### include 
1. include 源文件

https://stackoverflow.com/questions/39457263/include-assembly-file-in-another-assembly-file
```
# include "test.S"
```

2. include C header

https://stackoverflow.com/questions/4928238/include-header-with-c-declarations-in-an-assembly-file-without-errors


## TODO
2. 可以参考的资源:

* [The NASM Manual](https://www.nasm.us/doc/)
* [Compiler Explorer](https://godbolt.org/)
* [System V Application Binary Interface](https://www.uclibc.org/docs/psABI-x86_64.pdf)
* [The Intel Processor Manuals](https://software.intel.com/en-us/articles/intel-sdm)
* [The AMD Processor Manuals](https://developer.amd.com/resources/developer-guides-manuals/)
* [Agner Fog's optimization guides](https://www.agner.org/optimize/)
* [x86 and amd64 instruction reference](https://www.felixcloutier.com/x86/)

3. https://asmjit.com/ : 神奇的内联汇编

4. exit.c:do_exit 的 set_fs

5. check code in uaccess.c
  - for example, understand `get_fs`

- [ ] https://www.kernel.org/doc/html/latest/x86/index.html 
- [ ] http://infophysics.net/att0.pdf
- [ ] https://docs.oracle.com/cd/E19253-01/817-5477/817-5477.pdf


## syntax
1. ~/Core/x86-64-assembly 似乎非常不错:

配合答案使用:
https://github.com/0xAX/asm
也许可以写一个简化版的，感觉应该非常不错吧!


需要回答的问题:
1. global 是否存在对称的关键字
2. 定义 static 变量的方法

3. 宏
4. inlcude
5. section 的定义

## TODO
https://github.com/nemasu/asmttpd : 终极实战项目

## 资源

1. [缓冲区溢出试验](https://nagarrosecurity.com/blog/interactive-buffer-overflow-exploitation)
    2. https://stackoverflow.com/questions/1345670/stack-smashing-detected 有一个缓冲区溢出的试验
 [main maybe is not a function](https://jroweboy.github.io/c/asm/2015/01/26/when-is-main-not-a-function.html)

https://www.nasm.us/doc/nasmdoc0.html : nasm 的教程，nasm 和 gcc 的关系是什么 ? gcc 会调用 nasm 吗 ?
，似乎 linux 使用的是as 作为汇编器，从 Makefile 中间可以找到证据吗 ?

2. 项目 : 找到那个 assembly 的 http 作为项目练手。

3. 内联汇编彻底搞懂，出一个 examble based 的教程。

https://github.com/cirosantilli/x86-assembly-cheat : 看来如何编译这些代码都是不简单的

https://github.com/cirosantilli/x86-bare-metal-examples
  - https://stackoverflow.com/questions/22054578/how-to-run-a-program-without-an-operating-system/32483545#32483545

https://news.ycombinator.com/item?id=22279051 : 学习amd64的最佳教程

https://blog.stephenmarz.com/2020/05/20/assemblys-perspective : 汇编blog

https://blog.yossarian.net/2020/06/13/How-x86_64-addresses-memory : 终于看到总结 x86 寻址的总结了

https://news.ycombinator.com/item?id=24195627 : win 下的汇编

## 目标
1. gdb disassembly 中间的汇编代码和源代码之间可以看的懂
2. context switch 的代码可以阅读
3. 各种内联汇编可以看懂

## 利用 https://godbolt.org/ 实现 routing 的查看
有没有 vim 插件啊 ?

看指令手册的长度:
https://sandpile.org/
