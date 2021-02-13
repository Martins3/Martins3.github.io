# https://github.com/Light-City/CPlusPlusThings

1. `CPlusPlusThings/basic_content/vptr_vtable/vptr1.cpp` 对于 vtable 分析:
  - object 开头存在一个 vptr 指向 vtable

2. **默认参数是静态绑定的，虚函数是动态绑定的。 默认参数的使用需要看指针或者引用本身的类型，而不是对象的类型**。
  - 由于 vtable 的实现，就算是调用了函数，其实也不知道调用的是哪一个函数，所以只能使用静态信息来确定 default parameter
  - [所以最好不要在 virtual function 中间使用 default parameter](https://stackoverflow.com/questions/3533589/can-virtual-functions-have-default-parameters)

3. static method 不可以被 const 和 [volatile](https://stackoverflow.com/questions/3078237/defining-volatile-class-object) 修饰

4. 为什么构造函数不可以为虚函数？
- 尽管虚函数表vtable是在编译阶段就已经建立的，但指向虚函数表的指针vptr是在运行阶段实例化对象时才产生的。 如果类含有虚函数，编译器会在构造函数中添加代码来创建vptr。 问题来了，如果构造函数是虚的，那么它需要vptr来访问vtable，可这个时候vptr还没产生。 因此，构造函数不可以为虚函数。
- 我们之所以使用虚函数，是因为需要在信息不全的情况下进行多态运行。而构造函数是用来初始化实例的，实例的类型必须是明确的。 因此，构造函数没有必要被声明为虚函数。

5. **析构函数可以声明为虚函数。如果我们需要删除一个指向派生类的基类指针时，应该把析构函数声明为虚函数。 事实上，只要一个类有可能会被其它类所继承， 就应该声明虚析构函数(哪怕该析构函数不执行任何操作)。**
  - https://stackoverflow.com/questions/461203/when-to-use-virtual-destructors

6. **虚函数可以为私有函数吗？**
  - [最好是 private 的](https://stackoverflow.com/questions/2170688/private-virtual-method-in-c)
  - 问题是，多态这个时候不好用了, 在 `CPlusPlusThings/basic_content/virtual/set3/virtual_function.cpp` 中，需要向 base class 中间添加 firend int main();

7. **虚函数可以被内联吗？**
  - **通常类成员函数都会被编译器考虑是否进行内联。 但通过基类指针或者引用调用的虚函数必定不能被内联。 当然，实体对象调用虚函数或者静态调用时可以被内联，虚析构函数的静态调用也一定会被内联展开。**

8. 为什么 cast 成为 private 的 base 是不可以的
  - Since `Base` is not an accessible class of `Derived` when accessed in `main`, the Standard conversion from `Derived` class to `Base` class is ill-formed. Hence the error.
  - https://stackoverflow.com/questions/3674876/why-would-the-conversion-between-derived-to-base-fails-with-private-inheritanc

9. 抽象类中：在成员函数内可以调用纯虚函数，在构造函数/析构函数内部不能使用纯虚函数。
  - 其他函数为什么可以使用 纯虚函数, 因为这个 纯虚函数 在子类中间被定义了
  - 而 constructor 和 deconstructor 当在 抽象函数 调用的时候, 已经失去了 derived class 的环境了
  - https://stackoverflow.com/questions/8630160/call-to-pure-virtual-function-from-base-class-constructor


**WHEN COMMING BACK**
被阅读过的内容会从 core 目录下仓库中间删除，基础知识还是很有意思的

# TODO
/home/maritns3/core/CPlusPlusThings/basic_content/virtual/set4/warn_rtti.cpp

