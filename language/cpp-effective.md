# Effective C++ : 55 specific ways to improve your programs and Design, 3rd Edition

1. template P16
2. inline P16
4. static 在 C 语言的中间的作用 和 C++ 中间的作用的如何统一的 ？

5. 为了的免除 "跨编译单元之初始化的的次序问题" 使用local 对象代替的 non-local 对象



# 其他的一些笔记

<!-- vim-markdown-toc GitLab -->

* [为什么 copy move assignment 这些拷贝控制函数不用是 virtual](#为什么-copy-move-assignment-这些拷贝控制函数不用是-virtual)
* [如何调用 parent 的 virtual function 即便是被 override](#如何调用-parent-的-virtual-function-即便是被-override)
* [final 关键字的作用](#final-关键字的作用)

<!-- vim-markdown-toc -->
## 为什么 copy move assignment 这些拷贝控制函数不用是 virtual
1. 首先，constructor 是不能成为 virtual 的
2. copy assignment / move assignment 想要 override，那么在 derived class 中间的定义就会变得相当的诡异，效果如下
  1. 更加重要的是，这个代码的含义很奇怪 : `a = b` 如果 b 是 base 类型那么就只是拷贝 base 部分，如果是 derived 就全部拷贝，那么当只是拷贝 base 部分的时候，derived 的部分咋办 ?

可以进一步参考 : https://stackoverflow.com/questions/669818/virtual-assignment-operator-c

[代码](https://stackoverflow.com/questions/31423632/virtual-assignment-operator)
```cpp
using namespace std;

#include <iostream>

class A {
public:
  virtual A &operator=(const A &a_) {
    std::cout << "Calling A" << std::endl;
    return *this;
  }
};

class B : public A {
public:
  virtual B &operator=(const B &b_) {
    std::cout << "Calling B" << std::endl;
    return *this;
  }

  virtual B &operator=(const A &b_) override {
    std::cout << "Calling B" << std::endl;
    return *this;
  }
};

int main() {
  B b1;
  B b2;
  A &a = b1;
  a = b2; // Prints "Calling A", should be "Calling B"?

  return 0;
}
```

## 如何调用 parent 的 virtual function 即便是被 override
[代码](https://stackoverflow.com/questions/8824587/what-is-the-purpose-of-the-final-keyword-in-c11-for-functions)
```c
class Foo {
public:
  int x = 12;
  virtual void printStuff() { std::cout << x << std::endl; }
};

class Bar : public Foo {
public:
  int y = 22;

  void printStuff() override final {
    Foo::printStuff();
    std::cout << y << std::endl;
  }
};

int main(int argc, char *argv[]) {
  Bar b;
  b.Foo::printStuff();
  b.printStuff();
  return 0;
}
```

## final 关键字的作用
[修饰 virtual function 或者 class](https://en.cppreference.com/w/cpp/language/final)
