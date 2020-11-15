## TODO
将本文件夹下的乱七八糟的东西整理干净

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

## question

- [ ] why we don't have 
