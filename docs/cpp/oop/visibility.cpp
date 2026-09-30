// 顺便这里展示了如何调用 parent 的 virtual 函数:
// https://stackoverflow.com/questions/8824587/what-is-the-purpose-of-the-final-keyword-in-c11-for-functions
//
// ┌───────────┬─────────────┬────────┬──────────┐
// │ 说明符    │ 类自身/友元 │ 派生类 │ 外部代码 │
// ├───────────┼─────────────┼────────┼──────────┤
// │ public    │ 可以        │ 可以   │ 可以     │
// ├───────────┼─────────────┼────────┼──────────┤
// │ protected │ 可以        │ 可以   │ 不可以   │
// ├───────────┼─────────────┼────────┼──────────┤
// │ private   │ 可以        │ 不可以 │ 不可以   │
// └───────────┴─────────────┴────────┴──────────┘

#include <bits/stdc++.h>

using namespace std;

class Base {
    private:
	virtual void gg()
	{
		cout << "Base" << endl;
	}
};

class Derive : public Base {
    public:
	void gg()
	{
		cout << "Derive" << endl;
	}
};

// 权限只能静态的分析的结果，指针是什么类型就是什么类型
void test1()
{
	Base b;
	// b.gg(); 错误
	Base *d = new Derive;
	// d->gg(); // 错误

	Derive *c = new Derive;
	c->gg();
	// c->Base::gg(); // 错误
}

class A {
    public:
	virtual void gg()
	{
		cout << "A" << endl;
	}
};

class B : public A {
    private:
	void gg()
	{
		cout << "B" << endl;
	}
};

void bb(A *a)
{
	a->gg();
}

void test2()
{
	B *b = new B;
	// b->gg(); // 错误

	b->A::gg(); // 可以访问 parent 的
	bb(b); // 可以通过动态绑定访问
}

int main(int argc, char *argv[])
{
	test1();
	test2();
	return 0;
}
