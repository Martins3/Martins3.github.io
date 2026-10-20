先理解 non-local

这里的 non-local 指非局部变量，最典型的就是定义在函数外面的变量：

int a;         // 非局部变量
int b = 42;    // 非局部变量

void f()
{
    int c;         // 局部变量
    static int d;  // 也是局部变量，虽然具有静态存储期
}

a、b 即使没有写 static，也具有静态存储期，其存储贯穿整个程序运行。

所以标题中的 Static initialization，不是指“初始化写了 static 关键字的变量”。

全局变量的初始化，分成前后两个阶段

用这个例子理解：

#include <cstdio>

int read_config()
{
    std::puts("读取配置");
    return 7;
}

// 以下都是全局变量
int a;
int b = 42;
int c = read_config();

初始化过程是：

 变量    第一阶段：静态初始化    第二阶段：动态初始化
━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 a       零初始化，得到 0        不需要额外操作
──────  ──────────────────────  ────────────────────────────────
 b       常量初始化，得到 42     不需要
──────  ──────────────────────  ────────────────────────────────
 c       零初始化，暂时为 0      调用 read_config()，最终得到 7

所有静态初始化，都先于任何动态初始化。
因此动态初始化代码开始执行时，这些变量至少已经完成了第一阶段。标准草案：Static initialization
(https://timsong-cpp.github.io/cppwp/n4861/basic.start.static)

注意：b 不需要理解成“先清零，再写入 42”。它直接通过常量初始化得到 42。

zero 和 constant，就是第一阶段的两种处理方式

• Constant initialization，常量初始化：满足常量初始化的条件，可以预先确定初始状态。int b = 42; 就是最简单的例子。
• Zero initialization，零初始化：没有进行常量初始化时，先进行零初始化。对这里的 int 而言，就是得到 0。

“常量初始化”不要求变量本身是 const：

int b = 42;  // 常量初始化，但 b 以后仍然可以修改

可以把静态初始化想成“先把程序启动时的基础数据准备好”。实现上，常量数据通常可以放进程序文件，零值存储通常可
以由加载过程准备，但标准规定的是初始化效果和先后关系。cppreference：Static initialization
(https://en.cppreference.com/cpp/language/initialization#Static_initialization)

Dynamic initialization，就是还需要执行的初始化工作

例如上面的：

int c = read_config();

仅仅给 c 准备一个零值，还没有完成它要求的初始化。必须调用函数，拿到返回值。

这里的 dynamic 与堆内存、new 无关。它指的是这个初始化阶段。

全局变量的动态初始化通常发生在进入 main()
之前；标准也允许某些初始化推迟，并规定必须在什么使用之前完成，所以不能把“全部在 main()
之前”当成绝对规则。标准草案：Dynamic initialization
(https://timsong-cpp.github.io/cppwp/n4861/basic.start.dynamic)

ordered 和 unordered，进一步讨论这些初始化代码的执行顺序

假设有两个普通全局变量：

int load(const char* name)
{
    std::puts(name);
    return 7;
}

int first  = load("first");
int second = load("second");

在同一个翻译单元中——可以先近似理解为同一个 .cpp 连同它包含的头文件——这两个变量采用 ordered dynamic
initialization：

初始化 first
    ↓
初始化 second

所以这里先打印 first，再打印 second。

但如果分散到两个普通 .cpp：

// a.cpp
int first = load("first");

// b.cpp
int second = load("second");

就不能依赖这两个文件之间谁先初始化。它们仍属于 ordered 这一类别，只是这个类别并不保证任意两个翻译单元之间
的固定顺序。

而 unordered dynamic initialization
主要用于模板实例化产生的变量，例如变量模板实例、类模板的静态数据成员实例。不能套用“源代码写在前面就先初始
化”的规则。C++17 起还增加了 partially-ordered，主要涉及非实例化特化的 inline
变量。标准草案：动态初始化的分类和顺序 (https://timsong-cpp.github.io/cppwp/n4861/basic.start.dynamic)

 动态初始化类别       典型对象                    初步理解
━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Ordered              普通全局变量                同一翻译单元内按定义顺序
───────────────────  ──────────────────────────  ──────────────────────────────────────
 Partially-ordered    符合条件的 inline 变量      定义顺序满足规定条件时，建立先后关系
───────────────────  ──────────────────────────  ──────────────────────────────────────
 Unordered            模板实例化产生的某些变量    不能直接根据源码位置推断先后

它还能解释你刚才那个 B 的一个现象

struct B {
    int x;
    std::string name;
    B() {}
};

B global_b;

void f()
{
    B local_b;
}

结果是：

 对象        x           name
━━━━━━━━━━  ━━━━━━━━━━  ━━━━━━━━━━
 global_b    0           空字符串
──────────  ──────────  ──────────
 local_b     未初始化    空字符串

两个对象都调用 B()，而 B() 都没有初始化 x。区别是：

global_b：
    静态阶段先零初始化
        → 调用 B()
        → x 没有被改写，仍为 0；name 正常构造成空字符串

local_b：
    直接调用 B()
        → x 没有被初始化；name 正常构造成空字符串

因此，“默认初始化是否会给成员初始化数值”和“这个对象是否还经历了静态零初始化”，需要分别考虑。


## 这里的东西

https://cppreference.com/cpp/language/copy_initialization

Static non-local initialization
    zero – constant
Dynamic non-local initialization
    ordered – unordered

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
