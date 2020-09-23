> 更新到 tutorial 上去

# Notes
In this case, we choose to add four optimization passes. The passes we choose here are a pretty standard set of “cleanup” optimizations that are useful for a wide variety of code. I won’t delve into what they do but, believe me, they are a good starting place :).

Its API is very simple: addModule adds an LLVM IR module to the JIT, making its functions available for execution; removeModule removes a module, freeing any memory associated with the code in that module; and findSymbol allows us to look up pointers to the compiled code.
> addModule 给JIT 提交数据，removeModule 结束Module
> 但是Module 中间的内容再被不停的刷新啊 !

# 启动
1. 使用教程的最佳方法是直接clone源代码:
```
git clone https://mirrors.tuna.tsinghua.edu.cn/git/llvm/llvm.git
```

2. 根据文档编译 GettingStarted.rst 
```
mkdir build && cd build
cmake -DLLVM_BUILD_EXAMPLES=on ../llvm
make -j8
```

3. 注意事项, 编译的时候使用全部线程，最后链接会由于内存不足而失败，解决方法是:
添加更大的swap 或者 减少并发度

4. cmake 创建的compile_commands.json 并不能使用，还是需要使用 `bear make`

# lab1 的其实并不用从内核中间编译
```Makefile
FLAGS=-lclangBasic -lclangAST -lclangBasic -lclangFrontend -lclangTooling -lLLVM

all:ASTInterpreter.cpp Environment.h
	clang++ ASTInterpreter.cpp $(FLAGS) -o ast 
```
> 参考其中CMakelist中间内容即可!

# 当前编译cmake的方法
```c
cmake ../llvm -DLLVM_ENABLE_PROJECTS="clang;clang-tools-extra" -DLLVM_BUILD_EXAMPLES=on -DCMAKE_BUILD_TYPE=Release
```

