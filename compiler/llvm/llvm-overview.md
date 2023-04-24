[Chris](https://en.wikipedia.org/wiki/Chris_Lattner)

项目构成:
1. http://clang.llvm.org/extra
2. [C++ Insights - See your source code with the eyes of a compiler](https://github.com/andreasfertig/cppinsights)
2. compiler-rt : https://compiler-rt.llvm.org/
3. libc
    - [Rich Felker of musl libc comments on Google's LLVM libc proposal](https://news.ycombinator.com/item?id=20280487)
4. libc++
5. [lld](https://lld.llvm.org/index.html)
6. lldb : llvm debugger
7. https://mlir.llvm.org/ : middle level for accelerator programming language
7. https://polly.llvm.org/


llvm IR 三种形式 : human readble , bitcode, memory

llc(llvm static compiler):
1. instruction selection
2. register allocation
3. instruction scheduling
4. Machine IR
5. Machine Optimization
6. Machine Specific Features (stack protection, etc)
6. Exception Handling
6. "Assembly Printing"
6. Target Dependency Breaking
6. Calls heavily into Target code


lexer-parser:
==> ast, sema, cfg, codegen, libtooling(https://clang.llvm.org/docs/LibTooling.html)
==> llvm ir, opt, llc, llvm adt

还有一些看不懂的东西:
- [ThinLTO](https://clang.llvm.org/docs/ThinLTO.html)
- PGO(profile-guidede Optimization)

## 教程
- https://llvm.org/devmtg/2019-10/slides/ClangTutorial-Stulova-vanHaastregt.pdf
- http://llvm.org/devmtg/2019-04/slides/Tutorial-Bridgers-LLVM_IR_tutorial.pdf
- https://youtu.be/5kkMpJpIGYU
- https://youtu.be/m8G_S5LwlTo
- https://youtu.be/objxlZg01D0

- https://hujun1413.github.io/2017/01/15/LLVM/3.LLVM%E7%9A%84%E7%BC%96%E7%A8%8B%E6%8A%80%E5%B7%A7/ : LLVM 编程技巧

## 资源
godbolt 可以实现获取 clang AST

## 操作
使用 llvm 对于 wren 进行重写
1. analysis pass : -dot-callgraph -dot-cfg -memdep
2. transform pass :
3. utility pass : -view-dom -view-cfg

## 文章合集
- https://github.com/banach-space/llvm-tutor : 按照这个项目，改写之前的 llvm 作业。
- https://www.zhihu.com/people/frankwang-55/posts?page=2
- https://blog.josephmorag.com/posts/mcc3/ : 利用llvm 实现 C 语言，其实可以阅读一下，虽然作者之后就不更新了，而且是使用 Haskell 写的

## 资源
- https://github.com/Microsoft/checkedc/wiki : 并不是很清楚是如何做的，这种东西不可以向 llvm 中间插入一个 pass 吗 ?
- https://github.com/lifting-bits/remill : 人家写好的工具，将各种二进制转化为llvm bytecode

## llvm
- https://secret.club/2021/04/09/std-clamp.html
- https://www.intel.com/content/www/us/en/developer/articles/technical/adoption-of-llvm-complete-icx.html
  - intel 也是采用 llvm 了，没太看懂
- https://github.com/lijiansong : 当时的指针分析
- https://www.npopov.com/2021/06/02/Design-issues-in-LLVM-IR.html
  - 分析了一下 LLVM 中存在的问题

## 官方文档
### https://llvm.org/docs/LangRef.html
### https://llvm.org/docs/ProgrammersManual.html

## 代码分析笔记: https://github.com/csstormq/csstormq.github.io
