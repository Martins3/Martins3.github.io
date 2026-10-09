# C++ 资源

## 教程与书籍
- [快速浏览](https://github.com/changkun/modern-cpp-tutorial/blob/master/book/zh-cn/03-runtime.md) : 现代 C++ 中文教程
- [Cpp Templates : Complete Guide](https://www.amazon.com/C-Templates-Complete-Guide-2nd/dp/0321714121)
- https://github.com/parallel101/cppguidebook : 小彭老师领衔编写，现代 C++ 的中文百科全书
- https://learnmoderncpp.com/
- https://en.wikibooks.org/wiki/C%2B%2B_Programming
- https://hackingcpp.com/cpp/cheat_sheets.html : 通过图片做出来的 cheatsheets，主要是关于 std 使用之类的
- https://alandefreitas.github.io/moderncpp/ : 对于 moderncpp 提供一堆 snippet，没有维护，价值一般
- https://github.com/pezy/CppPrimer : Cpp Primer 习题答案
  - https://github.com/Mooophy/Cpp-Primer : 另一份 Cpp Primer 答案

## 写 C++ 的工具

- [用 Clang 和 GCC 观察 C++ 的隐式行为与实现](compiler-inspection.md)：AST、GIMPLE、对象布局、虚表、LLVM IR、优化报告和汇编。

静态检查:
- https://pvs-studio.com/en/blog/posts/0397/ : An Overview of Static Analyzers for C/C++ Code
- [ ] https://mull.readthedocs.io/en/latest/MutationTestingIntro.html : 变异测试工具
- https://news.ycombinator.com/item?id=43533516

构建与依赖:
- https://github.com/conan-io/conan : 包管理器
- https://github.com/TheLartians/ModernCppStarter : C++ 项目模板
  - https://github.com/cpp-best-practices/cmake_template : 类似的项目模板

交互与调试:
- https://github.com/jupyter-xeus/xeus-cling : Jupyter kernel for C++
- https://github.com/RuntimeCompiledCPlusPlus/RuntimeCompiledCPlusPlus : 运行时动态修改 C++ 代码（热更新）
- https://github.com/s9w/dt : Differential timer，性能测量的计时工具

## C++ 项目与源码
- https://github.com/sogou/workflow : 搜狗开发的，不到 2 万行，少年，你想做后端开发吗 ?
- https://github.com/nmwsharp/polyscope : 图形相关
- https://github.comseprite/aseprite : 动画编辑器
- https://github.com/fogleman/Craft : 只有 5000 行，但是实现了 minecraft
- https://github.com/k-vernooy/tetris : 俄罗斯方块
- https://github.com/ianlancetaylor/libbacktrace : backtrace 的实现，可读源码

## 算法与设计模式
- https://github.com/xtaci/algorithms : 各种算法的实现
- https://github.com/gzc/CLRS : 算法导论答案
- https://github.com/TheAlgorithms/C-Plus-Plus : C++ 的 algorithm 到底包括什么东西，已经不想花时间到细节上了
- https://github.com/Snaipe/libcsptr : 不是想要学习 smart pointer 的实现吗 ?
- https://github.com/orangeduck/Cello : C 语言的黑科技，写个 blog 介绍一下，很有意思
- https://github.com/me115/design_patterns : 图说设计模式
- https://github.com/JakubVojvoda/design-patterns-cpp : 设计模式，但是没有维护了
- https://github.com/AlfredTheBest/Design-Pattern : 虽然是 Java 写的，但是还是注意一下
- https://github.com/iluwatar/java-design-patterns : 设计模式到底存在多少种类，为什么各种设计模式总是 Java 喜欢用

## 文摘与文章
- [shafik](https://shafik.github.io/) 的 blog，谈论了很多 cpp 高级话题
- [ ] https://blog.visionappster.com/2020/08/06/overriding-virtual-functions-at-run-time/
- [ ] http://modernescpp.com/index.php/c-20-concept-syntactic-sugar : 这个作者写了一系列 cpp 文章，先把经典内容看完再看这个
- [ ] https://hackernoon.com/undefining-the-c-pre-processor-c4eeb3d06e1f : 替代 macro 的方法；不过感觉文章论证 macro is harmful 的部分不够有力
- [ ] https://www.gamasutra.com/view/news/169296/Indepth_Functional_programming_in_C.php : cpp 中的函数式编程
- [ ] https://www.fluentcpp.com/2021/12/13/the-evolutions-of-lambdas-in-c14-c17-and-c20/
- https://www.cppstories.com/2017/02/how-to-stay-sane-with-modern-c/ : how to stay sane with modern C++
- https://www.cppstories.com/2018/12/fromchars/ : 字符串转数字 from_chars
- https://blog.feabhas.com/2021/08/c20-modules-with-gcc11/ : GCC 11 的 C++20 Modules
- https://www.internalpointers.com/post/writing-custom-iterators-modern-cpp : Writing a custom iterator in modern C++
- https://www.cppstories.com/2019/10/cppecosystem/?m=1#vivim-emacs : C++ 工具链生态总结，总结的似乎都知道，应该付诸实现
- https://travisdowns.github.io/blog/2019/11/19/toupper.html : 性能分析（toupper）
- https://ppc.cs.aalto.fi/ch2/ : Aalto 的 Programming Parallel Computers 课程第 2 章，性能相关

问答与讨论:
- https://news.ycombinator.com/item?id=24901244 : Ask HN: Good C++ code bases to read?，介绍了一些好的 C++ 项目和做法
- https://news.ycombinator.com/item?id=42231489 : The two factions of C++
- https://news.ycombinator.com/item?id=42495135 : C++ is an absolute blast
- https://news.ycombinator.com/item?id=43468976 : Writing your own C++ standard library from scratch
- https://www.zhihu.com/question/451327108/answer/3299498791 : 知乎问答
- https://news.ycombinator.com/item?id=22998977 : Benchmarking C++ Allocators
    - https://github.com/google/tcmalloc
    - https://google.github.io/tcmalloc/tuning.html

## 资源合集与 FAQ
- https://github.com/rigtorp/awesome-modern-cpp : 首先将 interview 的打牢基础吧
- https://github.com/fffaraz/awesome-cpp : 难道其中包含的是不 modern 的部分 ?
- https://github.com/jobbole/awesome-cpp-cn
- https://github.com/MattPD/cpplinks : C++ 链接大合集
- https://github.com/CppCon/CppCon2017
- https://github.com/CppCon/CppCon2024
- https://www.stroustrup.com/ : Bjarne Stroustrup 主页，有很多资源
  - https://www.stroustrup.com/bs_faq.html
- https://isocpp.org/faq

## cpp lib
1. dbg.h : https://github.com/sharkdp/dbg-macro/blob/master/dbg.h : 1000 行的更好的printf debug工具, 正在被我的 leetcode 项目使用

- https://github.com/catchorg/Catch2
- https://github.com/microsoft/vcpkg
  - https://learn.microsoft.com/zh-cn/vcpkg/ : 我猜大家一般不会用吧
- https://github.com/gabime/spdlog : 日志库
- https://github.com/microsoft/STL : 微软的 STL 实现

- https://github.com/pybind/pybind11
- https://github.com/TNG/boost-python-examples : python 和 c++ 合并使用，其实感觉永远都是没有时间使用
- https://github.com/skypjack/entt#requirements : modern C++ 的项目
- https://github.com/google/googletest
  - https://github.com/onqtam/doctest : 和 googletest 有什么区别吗 ?
- [stb library](https://github.com/nothings/stb)

- https://gcc.gnu.org/onlinedocs/cpp/Predefined-Macros.html
- [modern cpp features](https://github.com/AnthonyCalandra/modern-cpp-features)

## stackoverflow cpp top
- https://github.com/ethsonliu/stackoverflow-top-cpp/blob/master/question/081%20-%20%E4%BB%80%E4%B9%88%E6%98%AF%E5%AF%B9%E8%B1%A1%E5%88%87%E5%89%B2.md
- https://github.com/EthsonLiu/stackoverflow-top-cpp

- https://stackoverflow.com/questions/4421706/what-are-the-basic-rules-and-idioms-for-operator-overloading

> - 运算符两边的操作数至少有一个是自定义的类型
> - 和其它函数一样，运算符重载既可作为成员函数，也可作为非成员函数。

- https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
