# C++ 类型转换总结
<!-- a0cf0ce9-cd97-43ec-84c9-98da07ddf2ed -->

C++ 有 4 个命名转换运算符，外加 2 个旧式转换，下面按用途总结。

## 1. static_cast —— 编译期可检查的“合理”转换

用途最广，转换在编译期确定，行为良好定义。

```cpp
// 数值类型转换（含窄化，会截断/舍入）
double d = 3.14;
int i = static_cast<int>(d);          // 3

// void* -> T*（恢复类型，标准做法）
void* p = &i;
int* ip = static_cast<int*>(p);

// 类层次：基类 <-> 派生类（不检查运行时类型，由你保证正确）
Base* b = static_cast<Base*>(derived_ptr);
Derived* d = static_cast<Derived*>(b);   // 危险：不做运行时检查

// 隐式转换的显式化
char c = static_cast<char>(i);

// 调用转换构造函数 / 转换运算符
std::string s = static_cast<std::string>("hi");
```

特点：不做运行时类型检查，出错责任在程序员。**不能**去掉 const、不能做指针与整数互转。

## 2. dynamic_cast —— 运行时安全检查的类层次转换

专用于**多态类型**（有虚函数）的向下转型，失败时给出明确结果。

```cpp
// 指针版本：失败返回 nullptr
Derived* d = dynamic_cast<Derived*>(base_ptr);
if (d) { /* 安全使用 */ }

// 引用版本：失败抛 std::bad_cast
try {
    Derived& d = dynamic_cast<Derived&>(base_ref);
} catch (const std::bad_cast&) {}

// 交叉转型：同一继承树中基类之间
```

要求：源和目标都是多态类型（有虚函数）。有运行时开销（RTTI）。

## 3. const_cast —— 唯一能增删 const/volatile 的转换

```cpp
const int* cp = &i;
int* p = const_cast<int*>(cp);     // 去掉 const

// 典型场景：调用“逻辑上不改”但签名没加 const 的旧 API
void legacy_api(char* s);
legacy_api(const_cast<char*>(str.c_str()));
```

注意：对**真正 const 的对象**（如 `const int x = 5;`）去掉 const 后写入是未定义行为，只能用于“对象本身不是 const，只是被 const 指针指着”的情形。

## 4. reinterpret_cast —— 底层位模式重解释，最危险

不做任何检查，直接把内存位模式当另一种类型看。

```cpp
// 指针 <-> 整数
auto addr = reinterpret_cast<std::uintptr_t>(p);
void* p2 = reinterpret_cast<void*>(addr);

// 无关类型指针互转
long* lp = reinterpret_cast<long*>(int_ptr);
```

特点：平台相关、不可移植、极易 UB。看到它就该警觉。唯一常见正当用途：指针与 `uintptr_t` 互转（哈希、日志、序列化地址）。

## 5. C 风格转换 `(type)expr` —— 不推荐

```cpp
int* p = (int*)some_ptr;   // 旧式
```

它会依次尝试：const_cast → static_cast → (static_cast + const_cast) → reinterpret_cast → (reinterpret_cast + const_cast)。即“什么都干”，语义模糊，出错时编译器帮不了你，还容易被模板、宏掩盖。C++ 里应一律避免。

## 6. 函数式转换 `type(expr)` —— 仅限单 token 类型

```cpp
int(3.14);            // 等价 static_cast
unsigned(42);
```

只对简单类型（单 token 名字）有效，本质等价于 C 风格转换，现代代码少用。

---

## 选择速查表

| 需求 | 用什么 |
|---|---|
| 数值类型转换、void* 到 T*、基类与派生类互转（自信正确） | `static_cast` |
| 多态向下转型且要安全检查 | `dynamic_cast` |
| 去掉 const/volatile | `const_cast` |
| 指针与整数互转、无关类型重解释 | `reinterpret_cast` |
| 旧代码里的 `(T)x` | 重构为上面四个 |

## 优先级原则

1. 优先依赖**隐式转换**和语言本身（`std::move`、`std::forward` 里的转换也别自己瞎写）
2. 需要显式时按上表选最**具体**的那个
3. 少用 `reinterpret_cast`，**禁用** C 风格转换

一个常见误区：很多人以为 `static_cast` 万能，其实它被严格限制在“类型系统可推理”的范围内；越界的操作（指针与整数互转、去 const、无关类型互转）标准刻意要求用更“显眼”的运算符，让危险在代码里可见。

---

# 为什么 static_cast 不可以将 void* 转换为 long

这是 C++ 标准里刻意区分的两类转换语义问题。

## 核心规则

- `static_cast` 允许 `void*` → `T*`（T 是对象类型），这是合法的：

  ```cpp
  void* p = ...;
  int* ip = static_cast<int*>(p);   // OK，从 void* 恢复类型
  ```

- `static_cast` **不允许** 指针 → 整数，无论有没有经过 `void*`：

  ```cpp
  long n = static_cast<long>(p);    // 编译错误
  ```

  想这么干必须用 `reinterpret_cast`：

  ```cpp
  long n = reinterpret_cast<long>(p);                // 可以编译，但非首选
  auto n = reinterpret_cast<std::uintptr_t>(p);      // 推荐
  ```

## 为什么这样设计

`static_cast` 定位是“编译期可检查、行为良好定义的转换”：

1. 数值类型之间的转换（int、long、double 等互转）
2. 类层次中的上/下转型（配合运行时检查不跨越 virtual 时是静态的）
3. 隐式转换的显式化
4. `void*` → `T*`

而“指针 → 整数”这个操作：

- **平台相关**：指针是 64 位，`long` 在 Windows 上是 32 位，会截断丢信息。`static_cast` 不愿意承担这种“可能无声无息截断”的语义。
- **实现定义**：指针到底按位映射成什么整数，标准不规定，这属于“低层、危险的位级操作”，恰好是 `reinterpret_cast` 的职责。

所以标准把这类转换交给 `reinterpret_cast`，让代码里出现它时读者能一眼看出“这里在做底层、不可移植的事”。

## 正确写法

不要用 `long`，用专门为“装得下指针”而存在的类型：

```cpp
#include <cstdint>

void* p = ...;
std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(p);
```

`uintptr_t`/`intptr_t`（若平台提供）保证能无损容纳一个指针值，再用 `static_cast` 在整数类型间转换就安全了。

一句话总结：`static_cast` 只做“类型系统内可证明合理”的转换，指针与整数互转属于实现定义的底层操作，标准要求用 `reinterpret_cast` 显式表达这一意图。

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
