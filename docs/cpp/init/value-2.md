# value initialization 和 default initialization

**default initialization 通常由“没有写初始化器”触发；
value initialization 则由 `T()` 等形式触发，它在某些情况下会先把对象清零。**

## 基本类型的区别

严格来说，`int b = int();`
中，`int()` 执行值初始化，而变量 `b`
使用拷贝初始化语法，最终得到 `0`。

## 类的值初始化不保证成员都是零

对于类类型，default initialization 会选择并调用默认构造函数。

Value initialization 也会选择默认构造函数，但有一个关键规则：

> 如果选中的默认构造函数不是 **user-provided（用户提供的）**，先执行
> zero initialization，再执行 default initialization。

看这个例子：

```cpp
struct A {
    int x;
    A() = default;
};

struct B {
    int x;
    B() {}             // 用户提供的默认构造函数
};

void demo() {
    A a;               // default initialization：a.x 没有初始化值
    A* pa = new A();    // value initialization：pa->x == 0

    B b;               // default initialization：b.x 没有初始化值
    B* pb = new B();    // value initialization：pb->x 仍没有初始化值

    delete pa;
    delete pb;
}
```

原因是：

- `A() = default` 写在首次声明处，**不算 user-provided**。`new A()`
  会先清零，所以 `x` 为 `0`。
- `B() {}` **算 user-provided**。`new B()` 不会预先清零，而构造函数又没有初始化 `x`。

因此，**`= default` 和空函数体 `{}` 在这里并不等价。**

还有一个容易忽略的细节：

```cpp
struct C {
    int x;
    C();
};

C::C() = default;       // 首次声明之后才 = default，算 user-provided
```

`new C()` 也不会因为 value initialization 而先把 `x` 清零。

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
