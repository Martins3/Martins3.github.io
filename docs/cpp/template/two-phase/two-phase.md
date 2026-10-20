# Two-phase name lookup

C++ Template 并不是等到 Template Instantiation（模板实例化）时才进行所有的名称查找。编译器在 Template Definition（模板定义）时，就已经完成了一部分 Name Lookup（名称查找）。

两个阶段很容易理解，就是有的 name 依赖 template 的参数，有的不依赖，依赖的需要等 template Instantiation 的时候才知道
template 具体用的参数，如果不依赖，仅仅看 template 定义就可以知道了。


## 为什么 C++ 要设计 Two-phase Name Lookup？

我认为从编译器和语言设计角度看，主要有两个原因。

第一，保证 Template Definition 的语义稳定。

假如所有名称都推迟到 Instantiation 才查找，那么用户在不同 Translation Units 中 Include 不同的 Header，就可能让同一个 Function Template 获得不同的 Overload Candidates，从而产生不同的行为，甚至造成难以诊断的 One Definition Rule（ODR）问题。

C++ 使用 Definition Context 固定普通查找的结果，同时允许 ADL 为用户定义类型提供扩展机制。

第二，支持 Template 的早期检查。


## TODO
它解释了为什么某些函数明明已经定义，Template 却找不到；为什么 `this->foo()` 和 `foo()` 在 Template 中可能表现不同；

## 1. 两个阶段到底是什么？

Phase 1 : Template Definition 检查模板定义，查找 Non-dependent Names
Phase 2 : Template Instantiation : 模板参数确定，处理 Dependent Names

注意：这是 Name Lookup 的两阶段，而不是说整个 Template 的所有语义分析都被严格分成两个互不重叠的步骤。

两阶段的核心区分是：

- Non-dependent Name：名称的含义不依赖 Template Parameters。通常在 Template Definition 时完成查找。
- Dependent Name：名称的含义依赖 Template Parameters。相关查找或最终解析可能需要等到 Template Instantiation；但并不是所有候选都到第二阶段才收集。

## 5. 一个更常见的坑：Dependent Base Class

这是使用 Class Template 时特别容易遇到的问题。

```cpp
template<typename T>
struct Base {
    void hello() {}
};

template<typename T>
struct Derived : Base<T> {
    void test() {
        hello();  // Error!
    }
};
```

为什么不行？明明 `Base<T>` 就有 `hello()`。

原因是：

```
Base<T>
```

属于 Dependent Base Class。

在 Template Definition 时，编译器不知道 `Base<T>` 最终会是哪一个 Specialization。

例如：

```
template<>
struct Base<int> {
    void goodbye() {}
};
```

`Base<int>` 可以完全不同于 Primary Template。

因此，当编译器遇到：

```
hello();
```

Unqualified Name Lookup 不会自动搜索 Dependent Base Class。

正确写法是：

```
template<typename T>
struct Derived : Base<T> {
    void test() {
        this->hello();
    }
};
```

或者：

```
Base<T>::hello();
```

前者通过显式 Member Access，让查找可以考虑 Dependent Base Class；
后者通过 Dependent Qualified Name 指定查找路径。二者对于 Virtual Function Dispatch 等情况可能有差异。

这里还有个容易忽视的情况：

```cpp
void hello();  // Global function

template<typename T>
struct Base {
    void hello();
};

template<typename T>
struct Derived : Base<T> {
    void test() {
        hello();        // Calls ::hello()
        this->hello();  // Calls Base<T>::hello()
    }
};
```

第一个调用不会因为实例化后发现了 Base Member 而改变 Binding。

所以在 Template Inheritance 中，`this->` 不仅是 Style 问题，而是会影响 Name Lookup 的语义。


## 8. 最后把规则整理成一张表

| 情形                                        | Name Lookup 发生在哪里                      |
| ----------------------------------------- | -------------------------------------- |
| Non-dependent Name                        | Template Definition                    |
| Dependent Qualified Name，例如 `T::foo`      | 通常推迟到 Instantiation                    |
| Dependent Function Call 的 Ordinary Lookup | 候选来自 Definition Context                |
| Dependent Function Call 的 ADL             | 可考虑 Definition 和 Instantiation Context |
| Dependent Base Member                     | 通常需要显式 `this->` 或 Qualified Name 才能查找  |
| `typename T::type`                        | 用 `typename` 消除 Type Parsing 歧义        |
| `obj.template f<int>()`                   | 用 `template` 消除 Template-id Parsing 歧义 |

## 关于 dependent name 的说明
注意，
泛型，只有类型，没有函数，什么的

```cpp
void func<T>(T a){
// 所以，depent 只有两种可能
 // 1. T.x 对于 type 的使用
 // 2. func(a) 继续使用 type
}
```

对于 dependent name ，显然不存在，但是对于 dependent function call ，
也就是考虑函数重载而已了。

所以，这里考虑的函数仅仅考虑 tempalate 定义的时候能够
看到的函数

adl 是特殊情况。

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
