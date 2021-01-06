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
  
## 资源
godbolt 可以实现获取 clang AST 

## 操作
使用 llvm 对于 wren 进行重写
1. analysis pass : -dot-callgraph -dot-cfg -memdep
2. transform pass :  
3. utility pass : -view-dom -view-cfg
