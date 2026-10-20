# 各种定义的嵌套关系


可以。C++ 允许在函数内部定义 class，也允许在 class 内部定义另一个 class。

实际上，C++ 对各种定义的嵌套关系有一套规则，而且并不完全对称。例如：

- 函数里面可以定义 class，但不能定义普通函数。
- class 里面既可以定义 class，也可以定义成员函数。
- namespace 里面可以定义 namespace，但函数和 class 里面不能定义 namespace。
- template 可以定义在 class 里面，却不能直接定义在函数里面。

下面系统整理一下。

## 1. C++ 各种定义的嵌套关系总表

✅ 允许　❌ 不允许　⚠️ 有特殊限制

| 定义的内容               | namespace 内 | class 内 | 函数内 |
| ------------------------ | ------------ | -------- | ------ |
| `namespace`              | ✅           | ❌       | ❌     |
| 普通函数定义             | ✅           | ⚠️       | ❌     |
| 函数模板                 | ✅           | ✅       | ❌     |
| 类模板                   | ✅           | ✅       | ❌     |
| `concept`（C++20）       | ✅           | ❌       | ❌     |
| 变量                     | ✅           | ✅       | ✅     |
| `using` 类型别名         | ✅           | ✅       | ✅     |
| Lambda 表达式            | ✅           | ✅       | ✅     |
| `class / struct / union` | ✅           | ✅       | ✅     |
| `enum`                   | ✅           | ✅       | ✅     |

注意表中的三个细节：

1. class 内定义的函数通常是成员函数，不是普通的自由函数。另有 `friend`
   函数的特殊情况。
2. class 内可以定义成员函数模板、成员类模板。
3. Lambda 严格来说是表达式，不是普通函数定义；namespace 或 class
   作用域可以通过变量初始化等方式使用 Lambda。

## 2. 函数内部定义 class

这种 class 称为 local class（局部类）。

```
void foo()
{
    class A {
    public:
        int x;

        void hello()
        {
            // 成员函数定义
        }
    };

    A a;
    a.x = 10;
}
```

完全合法。

局部类的主要特点：

- 类型名称 `A` 的作用域局限于对应的块。
- 可以有构造函数、成员函数、嵌套类等。
- 不能有静态数据成员。
- 不能在函数内部直接定义类模板。
- 不能像 Lambda 那样自动捕获外围函数的局部变量。

例如：

```
void foo()
{
    int x = 10;

    class A {
    public:
        void f()
        {
            // std::cout << x;  // 错误
        }
    };
}
```

局部类并不拥有外围函数的局部变量环境。

但 Lambda 不一样：

```
void foo()
{
    int x = 10;

    auto f = [x]() {
        return x;
    };
}
```

这也是为什么 Lambda 的 capture 机制有独立的设计意义。

## 3. class 内部定义 class

这叫 nested class（嵌套类）。

```
class Outer {
public:
    class Inner {
    public:
        void foo() {}
    };
};

int main()
{
    Outer::Inner obj;
    obj.foo();
}
```

这种嵌套可以继续下去：

```
class A {
    class B {
        class C {
            class D {};
        };
    };
};
```

需要注意，嵌套类并不会自动获得外围类对象的 `this` 指针。

```
class Outer {
    int x = 10;

public:
    class Inner {
    public:
        void foo(Outer& obj)
        {
            obj.x = 20;  // 合法
            // x = 20;   // 错误：没有 Outer 对象
        }
    };
};
```

嵌套类可以访问外围类的 private 成员，但访问非静态数据成员依然需要对应的对象。

## 4. 函数里面为什么不能定义普通函数？

下面是不合法的：

```
void foo()
{
    void bar()
    {
    }
}
```

C++ 不允许这种普通的嵌套函数定义。

但可以使用 Lambda：

```
void foo()
{
    auto bar = []() {
        return 42;
    };

    bar();
}
```

也可以通过局部类实现：

```
void foo()
{
    struct Helper {
        static int bar()
        {
            return 42;
        }
    };

    Helper::bar();
}
```

还有一个容易忽略的区别：函数内部可以声明普通函数，只是不能在那里定义它。

```
void foo()
{
    void bar();  // 合法，函数声明
    bar();
}

void bar()       // 在 namespace 作用域定义
{
}
```

## 5. class 与函数的更复杂嵌套

下面这段代码在 C++ 中是合法的：

```
class A {
public:
    class B {
    public:
        void f()
        {
            class C {
            public:
                void g()
                {
                    class D {};
                    D d;
                }
            };

            C c;
            c.g();
        }
    };
};
```

其嵌套关系是：

class A

class B

void f()

局部 class C

void g()

局部 class D

也就是说，可以通过 class → function → class → function 的方式不断嵌套。

## 6. template 的限制比较特殊

namespace 内定义模板：

```
template<typename T>
class A {};

template<typename T>
void foo(T x) {}
```

class 内定义成员模板：

```
class A {
public:
    template<typename T>
    class B {};

    template<typename T>
    void foo(T x) {}
};
```

这些都合法。

但是，不能在函数的块作用域直接声明模板：

```
void foo()
{
    template<typename T>
    class A {};           // 错误

    template<typename T>
    void bar(T x) {}      // 错误
}
```

不过 C++14 开始支持泛型 Lambda：

```
void foo()
{
    auto bar = [](auto x) {
        return x + x;
    };

    bar(10);
    bar(3.14);
}
```

C++20 甚至支持显式模板参数列表的 Lambda：

```
void foo()
{
    auto bar = []<typename T>(T x) {
        return x + x;
    };
}
```

因此，禁止在函数块作用域直接声明模板，并不意味着函数内部无法使用泛型编程。

## 7. 从 C++ 语言设计角度理解

可以把 C++ 这套规则理解为三种不同的作用域承担不同职责。

| 作用域          | 主要设计用途                              |
| --------------- | ----------------------------------------- |
| Namespace scope | 组织程序级名称、类型、自由函数与模板      |
| Class scope     | 定义类型的组成、成员、接口与嵌套类型      |
| Block scope     | 实现局部计算、局部变量、局部类型与 Lambda |

还有一点值得注意：

class 的定义位置和 class 的对象生命周期是两回事。

例如：

```
void foo()
{
    class A {
        int x;
    };

    A a;
}
```

这里 `A` 是一个局部类型，但并不意味着 `A` 这个类型在运行时被创建，也不意味着所有
`A` 对象都必须在栈上。类型仍然是编译期概念。

总结起来，记住四条最核心的规则就够了：

1. namespace 只能定义在 namespace 作用域。
2. class 可以定义在 namespace、class、函数块作用域。
3. 普通函数只能定义在 namespace 作用域或作为类成员在 class 内定义（另外存在
   friend 函数特例）。
4. 模板声明不能直接出现在函数块作用域，但 class 内允许成员模板。

其中最有意思的是：C++ 允许 `function → class → function`，却不允许直接
`function → function`。这也反映出 C++
把普通函数、局部类型和闭包分别当作不同的语言机制来设计。

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
