## TODO
- [ ] 成员函数指针
```cpp
template <int i> class A 
{
public:
    void foo(int)
    {
    }
};
template <uint8_t a, typename b, void* c> class B {};
template <bool, void (*a)()> class C {};
template <void (A<3>::*a)(int)> class D {};

template <int i> int Add(int a)	// 当然也能用于函数模板
{
    return a + i;
}

void foo()
{
    A<5> a;
    B<7, A<5>, nullptr>	b; // 模板参数可以是一个无符号八位整数，可以是模板生成的类；可以是一个指针。
    C<false, &foo> c;      // 模板参数可以是一个bool类型的常量，甚至可以是一个函数指针。
    D<&A<3>::foo> d;       // 丧心病狂啊！它还能是一个成员函数指针！
    int x = Add<3>(5);     // x == 8。因为整型模板参数无法从函数参数获得，所以只能是手工指定啦。
}
```
- [ ] RTTI


## When boring
https://docs.google.com/document/d/e/2PACX-1vTJmRADDPyybMjBxQ5r-PHEdHQWoOW-Wk87IVoT_EvFv9B5Ks3Mjuk8IXIDYPKFvWW6ezsl9PSZ1JbF/pub
    https://github.com/google/tcmalloc

C++为什么要弄出虚表这个东西？ - 果冻虾仁的回答 - 知乎
https://www.zhihu.com/question/389546003/answer/1194780618

# 拷贝控制
1. 拷贝构造函数 和 拷贝赋值运算符 的区别
```
class A{
public:
  A(const A&) = default;  //Out-of-line defaulted 拷贝构造函数
  A& operator = (const A&) = default; //Out-of-line defaulted 
};
```
默认拷贝构造函数和默认拷贝赋值函数 简单将对象中间的空间赋值， 包括指针， 这就是问题的根源。

# 友元

# 异常

# 面向对象
1. 析构函数定义为虚函数
下面的代码将会造成X的部分被释放， 但是Y的独有的部分无法被释放
```c
class X { 
private: 
 int x; 
}; 
 
class Y: public X { 
private: 
 int y; 
}; 
 
int main(){ 
  X* x = new Y; 
  delete x; 
}
```
2. access control

# MISC
1. typedef

2. Lambda
  1. 捕获方式
```c
int main() {
  int a = 123;
  int m = -123;
  auto value_lambda = [a, m]{
    return a + m;
  };

  auto ref_lambda = [&a, m]{
    return a + m;
  };

  auto automatic_value = [=]{
    return a + 10;
  };

  auto automatic_ref = [&]{
    return a + 10;
  };
}
```

# static

# namespace

# Overload Operator


# MISC
## tuple
1. Widely used for return more than one value

## bitset

## regex
1. syntax of regex is analyze at the runtime

# Question
1. `lvalue` `rvalue` `lvalue reference` `rvalue reference` what is the fucking difference ?
2. difference with `new` and `malloc`
3. how to implement `share_ptr`
4. why new array use a specific syntax `delete []`
5. what the job of `allocator`
6. static 修饰的成员变量为什么需要放到类的外面申明 ? 为什么还有添加类型说明 ? 为什么修饰为常量之后就又可以定义在内部了 ?
7. 在类声明中定义的函数，除了虚函数的其他函数都会自动隐式地当成内联函数。为什么虚函数这么特殊 ?
8. 位域

## Basic
> 模板是用于产生类型

1. `typename` 和 `class` 两个关键字, 可以交换使用，推荐前者

2. 非类型模板参数 **it seems that this relate to auto, but what's that**
```
template<typename T, typename U>
auto add3(T x, U y){
    return x + y;
}
```
3. 和类的实例化不同， 模板要求清楚函数模板的定义。**???????**


5. 模板中间使用`using`实现typedef的功能

6. 变长参数
```cpp
template<typename T0>
void print(T0 value) {
    std::cout << value << std::endl;
}
template<typename T, typename... Args>
void print(T value, Args... args) {
    std::cout << value << std::endl;
    print(args...);
}

// 编译这个代码需要开启 -std=c++14
template<typename T, typename... Args>
auto print2(T value, Args... args) {
    std::cout << value << std::endl;
    return std::initializer_list<T>{([&] { std::cout << args << std::endl; }(), value)...};
}
```

1. 
```
  const char * a = "fuck you";
  // char * b = "are you ok"; // 错误的写法
```

2. nullptr
  1. C++11 引入了 nullptr 关键字，专门用来区分空指针、0。而 nullptr 的类型为 nullptr_t，能够隐式的转换为任何指针或成员指针的类型，也能和他们进行相等或者不等的比较。
  2. `#include<cstddef>`
  3. C++ 不允许直接将 void * 隐式转换到其他类型

3. constexpr

4. if/switch 变量声明强化
  1. 可以在if()中间声明局部变量

5. 初始化列表
  1. 
```
#include <initializer_list>
```
  2. 构造函数和普通函数都可以使用

6. 类型推倒
  1. auto常用写法
```
auto i = 5;              // i 被推导为 int
auto arr = new auto(10); // arr 被推导为 int *
```
  2. auto 不能用于函数传参
  3. auto 不可以用于数组类型的推倒
  4. decltype 关键字是为了解决 auto 关键字只能对变量进行类型推导的缺陷而出现
  5. 尾返回类型推导
```
template<typename T, typename U>
auto add3(T x, U y){
    return x + y;
}
```
  6. decltype(auto) **看不懂**

7. 尖括号 ">"
```
vector<vector<int>> a; // 合法
```

8. 面向对象
  1. 委托构造
  2. 继承构造  
  参看[知乎](https://zhuanlan.zhihu.com/p/26916022)
  ```
class Base {
public:
    int value1;
    int value2;
    Base() {
        value1 = 1;
    }
    Base(int value) : Base() { // 委托 Base() 构造函数
        value2 = 2;
    }
};
class Subclass : public Base {
public:
    using Base::Base; // 继承构造
};
int main() {
    Subclass s(3);
    std::cout << s.value1 << std::endl;
    std::cout << s.value2 << std::endl;
}
  ```
  3. 显式虚函数重载  
  用于C++ base中间的函数定义为virtual之后，subClass的同名函数自动为重载， `override`用于显示的告知此函数为重载函数
  final可以用于函数和类上面

  4. 显式禁用默认函数
  为了能够让程序员显式的禁用某个函数，C++11 标准引入了一个新特性：deleted 函数。程序员只需在函数声明后加上“=delete;”，就可将该函数禁用。例如，我们可以将类 X 的拷贝构造函数以及拷贝赋值操作符声明为 deleted 函数，就可以禁止类 X 对象之间的拷贝和赋值

```
class X{            
     public: 
       X(); 
       X(const X&) = delete;  // 声明拷贝构造函数为 deleted 函数
       X& operator = (const X &) = delete; // 声明拷贝赋值操作符为 deleted 函数
     }; 
 
 int main(){ 
  X x1; 
  X x2=x1;   // 错误，拷贝构造函数被禁用
  X x3; 
  x3=x1;     // 错误，拷贝赋值操作符被禁用
  } 
}
```
Deleted 函数特性还可用于禁用类的某些转换构造函数，从而避免不期望的类型转换
```c
class X{ 
public: 
 X(double);              
 X(int) = delete;     
}; 
 
int main(){ 
 X x1(1.2);        
 X x2(2); // 错误，参数为整数 int 类型的转换构造函数被禁用          
}
```

Deleted 函数特性还可以用来禁用某些用户自定义的类的 new 操作符，从而避免在自由存储区创建类的对象
```c
#include <cstddef> 
using namespace std; 
 
class X{ 
public: 
 void *operator new(size_t) = delete; 
 void *operator new[](size_t) = delete; 
}; 
 
int main(){ 
 X *pa = new X;  // 错误，new 操作符被禁用
 X *pb = new X[10];  // 错误，new[] 操作符被禁用
}
```
9. 枚举类
```
enum class movie_type{
  action,
  thrill
};
```
