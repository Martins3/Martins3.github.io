# version
核心参考: https://www.zhihu.com/question/644556732/answer/3399033383

## C++11

引入的移动语义和 RAII 库的标准化完胜 C++98/03。自 C++11
开始，这门语言就彻底就放飞自我，敢于创新。C++11/14 输的原因：C++11
的模板元编程过于反人类，学习路线陡峭，虽然 14
有改进，但是仍然不尽人意，当时的模板元编程表达能力略差。但是 14
引入了一些库解决了 11 某些难以表达的功能，如编译期展开 tuple，C++14 引入的
std::integer_sequence 解决了这个问题。

## C++17

可以说是从诞生以来，第一个真正算得上完善语言的一个版本，它完善了 constexpr
函数编译期求值，提出了 if-constexpr
替代了一部分繁琐的标签分发，引入了结构化绑定，折叠表达式，模板形参推导，
允许了大多数可用 auto 简化的场景。而且它完善了 C++11 引入的带有缺陷的模板，例如 17
重写了可调用对象相关的 type traits，解决了 C++11 引入的坑。
更重要的是，它完善了内存管理里边的细节，如复制消除，临时量实质化等


### init-statement
 具体来说是 init-statement（初始化语句），即 if (init; condition) 和 switch (init; condition) 语法：

 ```cpp
   // C++17 之前不能这样写，现在可以
   if (auto it = map.find(key); it != map.end()) {
       // 使用 it
   }

   switch (auto ch = getchar(); ch) {
       case 'a': ...
   }
 ```

 需要区分的是：

 - if (int x = ...)（在条件里直接声明变量，作为条件判断）——这是 C++98 就有的老特性。
 - if (init; condition)（用分号分隔，初始化语句 + 条件）——这才是 C++17 新增的"变量声明强化"。



## C++20
提出新特性替代了可读性差到极致的将
SFINAE 用于类型约束的技巧，为了更方便地进行类型约束，引入了 concept 和 requires
语句，又引入了协程，格式化，模块和范围库和 view 库（理解成既包含 Java8 的 stream
又包含类似 python 的切片类型） 等特性，真正地变成了一个现代化的语言

C++23：C++
发展至今，已经全面加快新时代现代化的步伐。显式对象形参语法吸收了 Rust
精确控制接收者值类别和限定的精髓！std::expected 吸收了 Rust 的 Result<T, U>
提供了新的错误处理模式。新增 <stacktrace>
头文件使运行时观察调用栈踪迹成为了可能。线性代数库<linalg>
为工业控制/科学计算等领域赋能。

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
