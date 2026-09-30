#include <iostream>
#include <stdio.h>
using namespace std;

typedef void (*Fun)();

class Base {
    public:
	Base(){};
	virtual void fun1()
	{
		cout << "Base::fun1()" << endl;
	}
	virtual void fun2()
	{
		cout << "Base::fun2()" << endl;
	}
	virtual void fun3()
	{
	}
	~Base(){};
};

/**
 * @brief 派生类
 */
class Derived : public Base {
    public:
	Derived(){};
	void fun1()
	{
		cout << "Derived::fun1()" << endl;
	}
	void fun2()
	{
		cout << "DerivedClass::fun2()" << endl;
	}
	~Derived(){};
};

/**
 * 通过这个演示，简单清晰。
 * 虚表的地址存在 object 的首地址上，虚表中存有函数的地址。
 */
Fun getAddr(void *obj, unsigned int offset)
{
	cout << "=======================" << endl;
	void *vptr_addr = (void *)*(
		unsigned long *)obj; // 64位操作系统，占8字节，通过*(unsigned
				     //
	printf("vptr_addr:%p\n", vptr_addr);

	void *func_addr = (void *)*((unsigned long *)vptr_addr + offset);
	printf("func_addr:%p\n", func_addr);
	return (Fun)func_addr;
}
int main(void)
{
	Base *pt = new Derived(); // 基类指针指向派生类实例

	// 手动查找 vptr 和 vtable
	Fun f1 = getAddr(pt, 0);
	(*f1)();
	Fun f2 = getAddr(pt, 1);
	(*f2)();

	delete pt;
	return 0;
}
