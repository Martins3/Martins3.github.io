## [CppTemplateTutorial](https://github.com/wuye9036/CppTemplateTutorial)' Notes

由于C++11正式废弃“模板导出”这一特性，因此在模板类的变量在调用成员函数的时候，需要看到完整的成员函数定义。**因此现在的模板类中的成员函数，通常都是以内联的方式实现**。 例如：

```cpp
template <typename T>
class vector
{
public:
    void clear()
    {
        // Function body
    }
	
private:
    T* elements;
};
```

整型也可是Template参数:
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

template <float a> class E {}; // ERROR: 别闹！早说过只能是整数类型的啦！
```


当然和“设计模式”一样，模板在实际应用中，也会有一些固定的需求和解决方案。比较常见的场景包括：
1. 泛型（最基本的用法
2. 通过类型获得相应的信息（型别萃取）
3. 编译期间的计算
4. 类型间的推导和变换（从一个类型变换成另外一个类型，比如boost::function）

但是模板和宏也有很大的不同，否则此文也就不能成立了。模板最大的不同在于它是“可以运算”的。

尽管在《Modern C++ Design》中（别问我为什么老举这本书，因为《C++ Templates》和《Generic Programming》我只是囫囵吞枣读过，基本不记得了)大量运用模板来简化工厂方法；同时C++11/14中的一些机制如Variadic Template更是让这一问题的解决更加彻底。但无论如何，直到C++11/14，光靠模板你就是写不出依靠类名或者ID变量产生类型实例的代码。

```c
using namespace std;

template <typename T> class TypeToID {
public:
  static int const ID = -1;
};

template <> class TypeToID<uint8_t> {
public:
  static int const ID = 0;
  static int fuck() { cout << "fuck me" << endl;  return 1;}
};

int main(int argc, char *argv[]) {
  cout << "ID of uint8_t: " << TypeToID<uint8_t>::ID << endl;
  cout << "ID of uint8_t: " << TypeToID<uint8_t>::fuck() << endl;
  return 0;
}
```
不过这里有一个问题要理清一下。和继承不同，类模板的“原型”和它的特化类在实现上是没有关系的，并不是在类模板中写了 ID 这个Member，那所有的特化就必须要加入 ID 这个Member，或者特化就自动有了这个成员。完全没这回事。我们把类模板改成以下形式，或许能看的更清楚一点：

> - [ ] https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords

为了缓解这些问题，在C++11中，引入了变参模板（Variadic Template）。我们来看看支持了变参模板的C++11是如何实现tuple的：
```cpp
template <typename... Ts> class tuple;
```


C语言的不定长参数一样，它通常只能放在参数列表的最后。看下面的例子：
```cpp
template <typename... Ts, typename U> class X {};              // (1) error!
template <typename... Ts>             class Y {};              // (2)
template <typename... Ts, typename U> class Y<U, Ts...> {};    // (3)
template <typename... Ts, typename U> class Y<Ts..., U> {};    // (4) error!
```
答案在这一节的早些时候。(3)和(1)不同，它并不是模板的原型，它只是Y的一个偏特化。回顾我们在之前所提到的，偏特化时，模板参数列表并不代表匹配顺序，它们只是为偏特化的模式提供的声明，也就是说，它们的匹配顺序，只是按照`<U, Ts...>`来，而之前的参数只是告诉你Ts是一个类型列表，而U是一个类型，排名不分先后。

在上一节中，我们介绍了模板对默认实参的支持。当时我们的例子很简单，默认模板实参是一个确定的类型void或者自定义的null_type：

```cpp
template <
    typename T0, typename T1 = void, typename T2 = void
> class Tuple;
```
