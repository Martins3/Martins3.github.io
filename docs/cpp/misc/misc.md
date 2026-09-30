# 杂记

## 如何调用 posix 中内容

cpp 并没有完整的封装，也就是 fstream 不能完全替代所有的内容:

可以参考:
https://android.googlesource.com/platform/system/libbase/+/refs/heads/main/file.cpp
```txt
  ReadFileToString(path, content)
    │
    ├─ open() 打开文件
    ├─ unique_fd 接管文件描述符
    └─ ReadFdToString()
         └─ 循环 read()
              ├─ 被信号中断：重试
              ├─ 读到数据：追加到 std::string
              ├─ 到达 EOF：返回 true
              └─ 读取失败：返回 false，保留 errno
```

## C++ 成员函数指针
<!-- 57d89c52-ec08-42d7-95d1-5f8a87fd95bc -->

### 什么时候使用

它适合需要保存一个对象方法、稍后再调用的场景。

#### 回调

```cpp
class Button {
public:
    void on_click(int x) {
        // 处理点击
    }
};

using Handler = void (Button::*)(int);

void call_handler(Button& button, Handler handler) {
    (button.*handler)(123);
}
```

#### 在几个成员函数之间选择一个

```cpp
class Calculator {
public:
    int add(int a, int b) { return a + b; }
    int sub(int a, int b) { return a - b; }
};

Calculator calculator;
int (Calculator::*operation)(int, int) = &Calculator::add;

int result = (calculator.*operation)(1, 2);
```

事件系统、GUI、定时器和一些旧式 C++ 框架也可能要求传入成员函数指针作为回调。

### 现代 C++ 中的替代方案

成员函数指针不是 C++ 日常代码中的必需品。很多情况下，lambda 更直观：

```cpp
Calculator calculator;

auto operation = [&calculator](int a, int b) {
    return calculator.add(a, b);
};

int result = operation(1, 2);
```

如果需要统一保存普通函数、成员函数、lambda 和函数对象，可以使用 `std::function`：

```cpp
#include <functional>

Calculator calculator;
std::function<int(int, int)> operation =
    [&calculator](int a, int b) {
        return calculator.add(a, b);
    };
```

可以按下面的原则选择：

- 需要运行时多态：通常使用虚函数。
- 只是传递一个操作：通常优先使用 lambda。
- 需要统一保存各种可调用对象：使用 `std::function`。
- 需要明确表达“某个类的成员函数”，或接口本身要求这种类型：使用成员函数指针。

因此，成员函数指针不是“C++ 为了炫技提供的功能”，而是处理“把某个类的方法保存下来并在指定对象上调用”这一需求的底层机制。现代代码中未必经常手写，但理解它有助于读懂回调接口、标准库工具和旧式框架代码。


## enum class
  - 新的enum的作用域可以不在是全局的 : 限定和非限定两种
  - 不能隐式转换成其他类型
  - 可以指定 enum 的类型

https://github.com/Neargye/magic_enum

需要找找 xiaopeng teacher 的课程看看

## union
- 联合（union）是一种节省空间的特殊的类，一个 union 可以有多个数据成员，但是在任意时刻只有一个数据成员可以有值。当某个成员被赋值后其他成员变为未定义状态。联合有如下特点：
  - 默认访问控制符为 public
  - 匿名 union 不能包含 protected 成员或 private 成员
  - 可以含有构造函数、析构函数
  - 不能含有引用类型的成员
  - 不能继承自其他类，不能作为基类
  - 不能含有虚函数
  - 匿名 union 在定义所在作用域可直接访问 union 成员
  - [ ] 全局匿名联合必须是静态（static）的
  - [ ] https://en.cppreference.com/w/cpp/language/union
  - [ ] 看看 primer 吧



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
