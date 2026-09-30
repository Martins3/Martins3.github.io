#include <bits/stdc++.h>
using namespace std;

//  为什么 cast 成为 private 的 base 是不可以的
// - https://stackoverflow.com/questions/3674876/why-would-the-conversion-between-derived-to-base-fails-with-private-inheritanc
//
// 首先来回忆下，为什么 private 修饰的变量和继承意味着:
// 很多 cpp 的故事都是如此，相似相关，但是有点不同。
//
// 继承的两个关键字非常容易理解:
//
// 1. 对于外部访问，Base 成员上的修饰和继承是与关系
// 2. 对于 Derived 访问 Base ，只看 Base 成员的修饰
//
//
// 如果你说的是 C++ 继承，选择 public、protected、private，主要看“子类与基类的关系要不要暴露给外部”：
//
//   写法         什么时候用                                          外部能否把子类当基类用
//  ━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━
//   public       子类是一种基类，例如 Dog 是 Animal                  能
//  ───────────  ──────────────────────────────────────────────────  ────────────────────────
//   protected    只希望子类及其后代保留这种基类关系，不向外部开放    不能
//  ───────────  ──────────────────────────────────────────────────  ────────────────────────
//   private      只是借用基类实现细节，不希望后代继续获得基类接口    不能
//
//  class Dog : public Animal {};        // 最常见：公开的“是一种”关系
//  class Derived : protected Base {};   // 关系只对派生类体系可见
//  class Worker : private Helper {};     // 把 Base 当实现手段
//
//  访问权限也会随继承方式变化：基类的 public/protected 成员在 public 继承下保持原样，在 protected 继承下都变成 protected，
//  在 private 继承下都变成 private。基类的 private 成员无论哪种方式，子类都不能直接访问。
//
//  实践中优先考虑 public 继承；如果只是想复用实现，通常优先用组合（把对象作为成员），而不是 private 继承。protected 继承相
//  对少见。另外，class Derived : Base 默认是 private 继承，struct Derived : Base 默认是 public 继承，建议都显式写出来。

class A {
	int a;

    public:
	int b;

    protected:
	int c;
};

class B : private A {
    public:
	B()
	{
		// a = 12;
		b = 12;
	}
};

class C : public A {
    public:
	C()
	{
		c = 12;
	}
};
int main(int argc, char *argv[])
{
	// 这是错误的，private 继承的含义是当前
	// A *b = new B();

	B b;
	// b.b;

	C c;
	c.b;

	return 0;
}
