# Notes
In this case, we choose to add four optimization passes. The passes we choose here are a pretty standard set of “cleanup” optimizations that are useful for a wide variety of code. I won’t delve into what they do but, believe me, they are a good starting place :).

Its API is very simple: addModule adds an LLVM IR module to the JIT, making its functions available for execution; removeModule removes a module, freeing any memory associated with the code in that module; and findSymbol allows us to look up pointers to the compiled code.
> addModule 给JIT 提交数据，removeModule 结束Module
> 但是Module 中间的内容再被不停的刷新啊 !

# 当前编译cmake的方法
```c
cmake ../llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_BUILD_EXAMPLES=on -DCMAKE_BUILD_TYPE=Release
```
