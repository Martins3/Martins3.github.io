## RTTI
RTTI（Run-Time Type Information，运行时类型信息）是 C++ 在程序运行期间识别对象真实类型的机制，主要用于多态继承场景。

例如：

```cpp
class Animal {
public:
    virtual ~Animal() = default;
};

class Dog : public Animal {
public:
    void bark() {}
};

Animal* animal = new Dog;
```

虽然变量类型是 `Animal*`，但它实际指向的是 `Dog` 对象。RTTI 可以在运行时识别这一点。

C++ 的 RTTI 主要提供两个工具。

**1. `dynamic_cast`：安全地向下转型**

```cpp
if (Dog* dog = dynamic_cast<Dog*>(animal)) {
    dog->bark();
}
```

如果 `animal` 实际指向 `Dog`，转换成功；否则返回 `nullptr`。

引用转换失败时不是返回空，而是抛出 `std::bad_cast`：

```cpp
try {
    Dog& dog = dynamic_cast<Dog&>(*animal);
} catch (const std::bad_cast&) {
    // 实际对象不是 Dog
}
```

常见转换方向：

```cpp
Dog* dog = new Dog;

// 向上转型：天然安全，不需要 RTTI
Animal* animal = dog;

// 向下转型：需要运行时检查
Dog* dog2 = dynamic_cast<Dog*>(animal);
```

要对类层次进行 `dynamic_cast`，基类通常必须是多态类型，也就是至少有一个虚函数。最常见的做法是定义虚析构函数：

```cpp
virtual ~Animal() = default;
```

**2. `typeid`：查询类型**

```cpp
#include <typeinfo>
#include <iostream>

std::cout << typeid(*animal).name() << '\n';
```

因为 `Animal` 是多态类型，`typeid(*animal)` 得到对象的实际类型 `Dog`。

区别要注意：

```cpp
typeid(animal)   // Animal*，表达式自身的静态类型
typeid(*animal)  // Dog，指向对象的动态类型
```

也可以直接比较：

```cpp
if (typeid(*animal) == typeid(Dog)) {
    // 对象恰好是 Dog
}
```

这里判断的是“恰好为 `Dog`”。如果对象属于 `SuperDog : public Dog`，结果也是 false。一般来说，判断“能否作为 `Dog` 使用”更适合 `dynamic_cast`。

**大致实现原理**

编译器通常会为多态类生成虚函数表 `vtable`。对象内部保存一个隐藏的虚表指针 `vptr`：

```text
对象
┌──────────────┐
│ vptr         │ ──> vtable
│ 成员变量     │       ├─ 虚函数地址
└──────────────┘       └─ 类型信息 RTTI
```

`dynamic_cast` 会利用虚表关联的类型信息，沿继承关系判断目标类型是否合法，并在多继承场景下计算正确的对象地址偏移。

**和其他 cast 的区别**

- `static_cast`：主要由编译器检查，不验证对象运行时的真实类型；错误向下转型可能产生未定义行为。
- `dynamic_cast`：运行时检查，安全，但有少量运行时成本。
- `reinterpret_cast`：主要用于底层位模式或指针解释，不负责类型安全。
- `const_cast`：增加或移除 `const`/`volatile` 限定。

实践上，RTTI 适合插件接口、异构对象集合、框架边界等确实需要识别动态类型的地方。但如果业务代码里到处都是 `dynamic_cast`，通常说明多态接口设计得不够完整，可以考虑把行为做成虚函数，让对象自己处理：

```cpp
class Animal {
public:
    virtual void speak() = 0;
    virtual ~Animal() = default;
};
```

一句话总结：**虚函数解决“调用实际对象的行为”，RTTI 解决“识别实际对象是什么类型”。**

## 参考
https://en.cppreference.com/w/cpp/types

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
