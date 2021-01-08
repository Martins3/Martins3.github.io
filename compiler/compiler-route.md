# 收集的资源
https://github.com/vtil-project/VTIL-Core : 又一个二进制翻译器

https://github.com/aalhour/awesome-compilers : **key**
可以学习 llvm 之类的

http://craftinginterpreters.com/contents.html : 清晰的教程
https://arzg.github.io/lang/ : 基于 rust 的教程

https://tenthousandmeters.com/blog/python-behind-the-scenes-4-how-python-bytecode-is-executed/ : python 的字节码如何执行的 ？

https://github.com/vnmakarov/mir : redhat 做的，带有中间表示的编译器 ？
## 自己收集的
https://github.com/asmjit/asmjit : Machine code generation for C++
https://github.com/netcan : maybe read his blogs

https://github.com/airbus-seclab/bincat

https://github.com/instagram/MonkeyType : 将 Python 动态运行信息插入类型

https://andrewkelley.me/post/zig-cc-powerful-drop-in-replacement-gcc-clang.html 号称替代 gcc clang 的东西 ?

https://github.com/chapel-lang/chapel : 并发的编程语言。

https://github.com/rxi/fe/blob/master/doc/impl.md : 其中可以参考Garbage Collection，Error Handling 的设计。

https://github.com/Feral-Lang/Feral : 也许用于学习一下 C++

https://blog.josephmorag.com/posts/mcc3/ : 利用llvm 实现 C 语言。

https://github.com/TrustInSoft/tis-interpreter : 对于 C 进行检查错误

https://github.com/tboox/vm86 : 别的到时没有什么印象，但是说 ida 98% 反汇编可以直接运行，还是非常震惊的。

https://github.com/Microsoft/checkedc/wiki : 并不是很清楚是如何做的，这种东西不可以向 llvm 中间插入一个 pass 吗 ?

https://github.com/lifting-bits/remill : 人家写好的工具，将各种二进制转化为llvm bytecode

https://github.com/banach-space/llvm-tutor : 按照这个项目，改写之前的 llvm 作业。

https://oneraynyday.github.io/dev/2020/05/03/Analyzing-The-Simplest-C++-Program/#table-of-contents : blog 复写一下链接器

https://nim-lang.org/ : 不知道这个语言老是被人提起，还有Julia .... 也许可以整理一下。

https://news.ycombinator.com/item?id=23333891 : 糟糕的面向对象

https://cjting.me/2020/06/07/chip8-emulator/ : chip-8 虚拟机

https://github.com/rethab/awk-jvm : 利用 awk 实现 jvm，非常有意思，同时可以学习两个

https://github.com/jamiebuilds/the-super-tiny-compiler : 包含大量的注释的情况下大约1000行，js 书写的，可以推荐给别人作为学习资源

https://rekcarc-tsc-uht.readthedocs.io/en/latest/%E5%A4%A7%E4%B8%89%E4%B8%8A.html#id6 : 清华的教程，基于 java 的 decaf

https://github.com/rswier/c4 : 大名鼎鼎的 c4 如果可以的，理解他，写一个 blog

https://github.com/lunatic-lang/lunatic

https://github.com/google/cel-spec : The Common Expression Language (CEL) implements common semantics for expression evaluation, enabling different applications to more easily interoperate.

## blog
https://blog.llvm.org/posts/2020-12-21-interactive-cpp-for-data-science/ : cpp in juperter notebook

## 想法
1. 各种内存分析工具都是如何实现的: 靠编译器静态分析，或者内核的动态分析。
2. 现在，感觉 taichi 才是最佳的娱乐项目 : 既有编译，又有图形学相关的东西


## TODO
1. https://github.com/green7ea/cpp-compilation
2. live register 的分析 ?
    1. 是不是说，当进入一个新的函数之后，所有的 register 都被保存了


## when boring
https://juejin.im/post/5d50d05ee51d4561da620117 为什么不把做成二进制翻译的工作转化为 JIT，其中容易运行直接执行，不容易的使用动态翻译的方法。

https://github.com/vasyop/miniC-hosting  : 可以借此了解一下虚拟机实现

https://news.ycombinator.com/item?id=23376357 : C 语言和汇编的联系

https://qntm.org/perl_en : learn perl in 150 minutes

https://news.ycombinator.com/item?id=25663403 : port v8 to riscv

## 链接
https://tinylab.gitbooks.io/cbook/zh/chapters/02-chapter4.html : 名字是 C 语言课程，但是实际上是动态链接的之类的东西


## 二进制翻译
https://github.com/lifting-bits/mcsema : Framework for lifting x86, amd64, and aarch64 program binaries to LLVM bitcode 
    1. 问一下，它们是怎么搞二进制到 binary 的 ?


## backend
目前看来，其实编译的各种优化，短期是难以有时间的，其实很多时候，
不一定非要准备齐全，才可以开始工作的，即便不知道，也是需要保持自信的。

https://asmjit.com/ 

## vm
v8 虚拟机让我开始好奇, v8 自带几个小文件例子.

https://github.com/wren-lang/wren : 只有 6000 行，似乎是非常简单的东西，作者还写过 v8 虚拟机的教程，不过的很老
https://github.com/marcobambini/gravity : 16000 ，也是使用了 vm, 文档非常的详细
https://github.com/KCreate/charly-vm : 80个star, vm 很大(4000)，其余都是很模块性质的几百行

https://github.com/rubinius/rubinius : **这似乎就是想象的内容**, 可以仔细观察一下
https://github.com/jakogut/tinyvm : 非常短小，可以仔细阅读一下

https://github.com/janet-lang/janet : vm 和 

https://news.ycombinator.com/item?id=24170201 : 感觉完成度还可以的 浏览器实现，所以，好的，可以去做前端了

## jvm



## 问题
1. runtime 和 vm 的关系是什么 ?
    1. JIT 的他们的关系是什么 ?
