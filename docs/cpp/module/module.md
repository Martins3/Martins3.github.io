# C++20 Modules
<!-- df96f132-2310-40e0-8b5c-a70995a68e11 -->

本目录下的示例都可以用 `make` 一键编译运行：

```sh
make            # 编译并运行全部 6 个示例
make clean      # 删除 .out/.o/gcm.cache 等产物
```

编译器版本：GCC 15.2.0，需要 `-fmodules-ts` 打开模块支持（即使是 C++23 的 `import std`）。

## 为什么需要 module

`#include` 是文本包含（textual inclusion）：预处理器把被包含文件的内容原样拷贝进当前文件。这带来一系列老问题：

1. **宏污染**：头文件里定义的宏会泄漏到所有包含它的翻译单元。
2. **顺序敏感**：`#include` 的顺序可能改变编译结果（宏影响后面的头文件）。
3. **重复解析**：每个翻译单元都要重新解析一遍头文件，编译慢。
4. **ODR 违例风险**：同一个 inline 函数/模板在每个 TU 里都展开一份，定义不一致就是 UB。
5. **无接口边界**：头文件里写什么，外部全部看得到，`private` 成员改动也会触发大面积重编。

module 把「接口」和「实现」显式分离，模块只编译一次，产出 **BMI**（Binary Module Interface，也叫 CMI，Compiled Module Interface），导入者直接读这个二进制接口，不再重新解析源码。

| | `#include` | `import` |
|---|---|---|
| 模型 | 文本替换 | 编译产物（CMI） |
| 宏 | 会泄漏 | 不会泄漏（模块内部宏外部不可见） |
| 顺序 | 敏感 | 不敏感 |
| 编译速度 | 每个 TU 重解析 | 只编译一次 |
| 隔离性 | 无（头文件内容全可见） | 只有 `export` 的对外可见 |

## 基本语法

最小例子（[basic.cpp](basic.cpp) + [basic-main.cpp](basic-main.cpp)）：

```cpp
// basic.cpp —— module interface unit
export module math;                       // 声明这是一个模块接口

export int add(int a, int b) { return a + b; }   // export：对外可见
int sub(int a, int b) { return a - b; }          // 没有 export：模块内私有
```

```cpp
// basic-main.cpp —— 消费者
import math;
#include <iostream>

int main() {
    std::cout << add(3, 4) << '\n';
    // sub 没有被 export，这里用不了
}
```

要点：

- `export module math;` 声明模块名，**必须是文件的第一条语句**（除了 `module;` 见下文 GMF）。
- `import math;` 导入模块，只有被 `export` 的名字可用。
- 模块名习惯用点分形式（如 `std.io`、`company.lib`），但点只是名字的一部分，没有层级含义。

## module unit 的种类

一个模块可以由多个文件（translation unit）组成，每个文件属于下面四种之一：

| 种类 | 声明方式 | 说明 |
|---|---|---|
| 主接口单元 primary module interface unit | `export module name;` | 模块必须且只能有一个 |
| 分区接口单元 module partition interface unit | `export module name:part;` | 接口拆分，属于模块内部 |
| 实现单元 module implementation unit | `module name;` | 补实现，可有多个 |
| 分区实现单元 internal partition | `module name:part;` | 分区里的实现，外部不可 import |

### implementation unit

接口只放声明，实现放到独立文件（[impl.cpp](impl.cpp) + [impl-impl.cpp](impl-impl.cpp) + [impl-main.cpp](impl-main.cpp)）：

```cpp
// impl.cpp —— 只有声明
export module hello;
export int greet();
```

```cpp
// impl-impl.cpp —— 注意没有 export
module hello;
int greet() { return 42; }
```

好处：接口改动少、实现可以独立编译、实现细节（`private` 成员、内部函数）不会出现在 CMI 里。

### module partition

大模块可以拆成多个分区（[partition.cpp](partition.cpp) + [partition-circle.cpp](partition-circle.cpp) + [partition-square.cpp](partition-square.cpp)）：

```cpp
// partition.cpp —— 主接口，把分区聚合起来并 re-export
export module shapes;
export import :circle;
export import :square;
```

```cpp
// partition-circle.cpp —— 分区
export module shapes:circle;
export double circle_area(double r) { /* ... */ }
```

- 分区名是 `模块名:分区名`。
- 只有主接口单元（或者被主接口 `export import` 的分区）才能被外部 `import`。
- 分区之间可以互相 `import :other_partition;`。
- `export import :circle;` 里的 `export` 是「再导出」，让外部 `import shapes` 时也能看到 `circle_area`。

## export 的几种形式

```cpp
export module m;

export int f();                 // 1. 单个声明

export {                        // 2. 块
    int g();
    int h();
}

export namespace ns {           // 3. 整个命名空间
    int k();
}

export import other;            // 4. re-export 另一个模块
export using std::size_t;       // 5. using 声明
```

`export` 只能出现在模块接口单元的命名空间作用域里。

## 编译模型与 CMI

`export module` 的接口单元要先编译，生成 CMI；导入者编译时读取 CMI。**构建顺序很重要**：被 import 的模块必须先于导入者编译。

各编译器产物的名字不同：

| 编译器 | 接口产物 | 说明 |
|---|---|---|
| GCC | `gcm.cache/模块名.gcm` | 用 `-fmodules-ts` |
| Clang | `模块名.pcm` | 用 `--precompile` |
| MSVC | `模块名.ifc` | `/interface` |

接口单元同时还会产出一个普通 `.o`，链接时和普通目标文件一起链接。

## 可见性与可达性

两个概念要区分：

- **visible（可见）**：名字能被直接使用，只有 `export` 出来的名字对导入者可见。
- **reachable（可达）**：一个声明即使不可见，只要它在语义上被用到（比如作为某个导出函数的返回类型、参数类型），就能通过 CMI 间接获取。这是为了让 `export std::vector<int> f();` 这类声明正常工作，而不需要把 `std::vector` 再 export 一遍。

实际影响：导入者不能直接用没被 export 的名字，但编译器知道这些名字存在。

## global module fragment

接口单元里如果要用到「非模块」的声明（典型是标准库头文件），必须先把头文件 include 进 **global module fragment**（GMF）：

```cpp
// gmf.cpp —— module; 到 export module 之间的部分
module;
#include <string>
export module person;

export class Person {
    std::string name;          // 这里的 std::string 来自 GMF 里的 <string>
public:
    explicit Person(std::string n) : name(std::move(n)) {}
    std::string name_of() const { return name; }
};
```

规则：

- GMF 只能出现在主接口单元，写法是 `module;` 单独一行，然后 include，再 `export module name;`。
- GMF 里的声明属于模块，但**默认对外不可达**；只有被 export 声明引用到的部分才会「附着」到模块接口上。
- 普通 `export module` 文件里，`import` 只能出现在 `export module` 之后，所以想 include 头文件就只能用 GMF。

## private module fragment

如果想把整个模块写进一个文件（接口 + 实现），可以用 **private module fragment**：

```cpp
export module m;
export int f();
module :private;      // 从这里往后都是模块私有实现
int f() { return 0; }
```

它解决了「单文件模块」的问题。注意：**GCC 15 尚未实现**，会直接报 `sorry, unimplemented: private module fragment`。目前要么拆成 implementation unit，要么用支持它的编译器（MSVC 支持，Clang 也基本支持）。

## header unit

除了 `import 模块名`，还可以 `import` 头文件本身，把它当作一个模块单元（[header-unit.hpp](header-unit.hpp) + [header-unit-main.cpp](header-unit-main.cpp)）：

```cpp
import "header-unit.hpp";     // 自定义头文件
import <iostream>;            // 标准库头文件
```

和 `#include` 的区别：

- 头文件被预编译成 CMI，多个 TU 共享，不再文本展开。
- 宏不会泄漏（header unit 默认不导出宏）。
- 语法上是 `import`，语义上是「把整个头文件当作一个模块」。

GCC 用法：

```sh
g++ -std=c++20 -fmodules-ts -fmodule-header=user -c header-unit.hpp
g++ -std=c++20 -fmodules-ts header-unit-main.cpp -o header-unit-main.out
```

注意：header unit 和 `#include` 同一个头文件不要混用，否则声明会重复（见下方「常见坑」）。

## import std（C++23）

C++23 提供 `import std;`，一次性导入整个标准库，替代一长串 `#include`（[import-std.cpp](import-std.cpp)）：

```cpp
import std;
int main() {
    std::vector<int> v{1, 2, 3};
    std::cout << v.size() << '\n';
}
```

GCC 上需要先编译 libstdc++ 自带的 `bits/std.cc`（位于 libstdc++ 的 `include/c++/<版本>/bits/std.cc`，本目录 Makefile 通过 `g++ -E -x c++ -v` 的输出自动定位并编译它）：

```sh
g++ -std=c++23 -fmodules-ts -c /path/to/include/c++/15.2.0/bits/std.cc -o std.o
g++ -std=c++23 -fmodules-ts std.o import-std.cpp -o import-std.out
```

> 注：GCC 15 里 `import std` 仍需要 `-fmodules-ts`，且同一个程序里所有模块的 C++ 语言版本（`-std`）必须一致，否则报 `language dialect differs`。

## 编译命令速查

GCC（本目录示例用的就是这套）：

```sh
# 编译接口单元（自动生成 gcm.cache/math.gcm 和 math.o）
g++ -std=c++20 -fmodules-ts -c math.cpp -o math.o
# 编译消费者（会读 gcm.cache/math.gcm）
g++ -std=c++20 -fmodules-ts math.o main.cpp -o main.out
```

Clang（概念相同，产物是 `.pcm`）：

```sh
clang++ -std=c++20 -x c++-module math.cpp --precompile -o math.pcm
clang++ -std=c++20 math.pcm main.cpp -o main.out
```

MSVC（产物是 `.ifc`）：

```bat
cl /std:c++20 /interface /TP math.cpp /ifcOutput math.ifc
cl /std:c++20 /TP main.cpp math.obj /reference math=math.ifc
```

## 常见坑

1. **构建顺序**：先编译被 import 的接口单元，再编译导入者。CMI 不存在时 GCC 报 `failed to read compiled module: No such file or directory`。

2. **`-fmodules-ts` 不能省**：GCC 15 里 `export`/`import` 关键字默认不启用，不加会报 `keyword 'export' is enabled with '-fmodules'`。

3. **implementation unit 里 `#include <iostream>` 会挂**：GCC 报 `conflicting declaration of 'class std::type_info' in global module`。实现单元里要输出建议用 `import <iostream>;`（header unit）或 `import std;`。GMF 里 `#include <iostream>` 反而没问题。

4. **不要混用 `#include` 和 `import` 同一个头文件**：如果模块 GMF 里 include 了 `<string>`，消费者又 `#include <iostream>`（它传递包含 `<string>`），会得到一长串 `redefinition of '...'`。要么统一 `import`，要么统一 `#include`。

5. **语言版本要一致**：模块接口和导入者必须用相同的 `-std`（比如都是 `-std=c++23`），否则报 `language dialect differs`。

6. **模块名冲突**：两个不同的模块不能同名；同一个程序里 `import` 的模块名要全局唯一。

7. **private module fragment 未实现**：GCC 15 报 `sorry, unimplemented`，用 implementation unit 代替。

## 参考资料

- [Modules (C++) - cppreference](https://en.cppreference.com/w/cpp/language/modules)
- [C++20 Modules: Getting Started - GCC Wiki](https://gcc.gnu.org/wiki/cxx-modules)
- [Standard C++ Modules - Clang documentation](https://clang.llvm.org/docs/StandardCPlusPlusModules.html)
- [C++ modules with GCC 11 - Feabhas](https://blog.feabhas.com/2021/08/c20-modules-with-gcc11/)
- [Overview of modules in C++ - Microsoft Learn](https://learn.microsoft.com/en-us/cpp/cpp/modules-cpp)

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
