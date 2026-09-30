// https://en.cppreference.com/w/cpp/language/copy_assignment
// 其中一共列举了如下的内容：
// - Eligible copy assignment operator
// - Trivial copy assignment operator
// - Deleted copy assignment operator
// - Implicitly-defined copy assignment operator
// - Implicitly-declared copy assignment operator
//
// 显然，这里的定义是没有看懂的

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>

struct A {
	int n;
	std::string s1;

	A() = default;
	A(A const &) = default;

	// user-defined copy assignment (copy-and-swap idiom)
	//
	// 这里是 pass by value 的
	// https://stackoverflow.com/questions/61574514/c-assignment-operator-pass-by-value-copy-and-swap-vs-pass-by-reference
	A &operator=(A other)
	{
		std::cout << "copy assignment of A\n";
		std::swap(n, other.n);
		std::swap(s1, other.s1);
		return *this;
	}

	/**
	 * 如果取消这个注释，A a1 = a2; 会无法区分到底是谁
	 */

	/*
	A &operator=(A &other)
	{
		std::cout << "what's this ?\n";
		std::swap(n, other.n);
		std::swap(s1, other.s1);
		return *this;
	}
	*/
};

struct B : A {
	std::string s2;
	// implicitly-defined copy assignment
};

struct C {
	std::unique_ptr<int[]> data;
	std::size_t size;

	// user-defined copy assignment (non copy-and-swap idiom)
	// note: copy-and-swap would always reallocate resources
	C &operator=(const C &other)
	{
		if (this != &other) // not a self-assignment
		{
			if (size != other.size) // resource cannot be reused
			{
				data.reset(new int[other.size]);
				size = other.size;
			}
			std::copy(&other.data[0], &other.data[0] + size,
				  &data[0]);
		}
		return *this;
	}
};

int main()
{
	A a1, a2;
	std::cout << "a1 = a2 calls ";
	a1 = a2; // user-defined copy assignment

	B b1, b2;
	b2.s1 = "foo";
	b2.s2 = "bar";
	std::cout << "b1 = b2 calls ";
	b1 = b2; // implicitly-defined copy assignment

	std::cout << "b1.s1 = " << b1.s1 << "; b1.s2 = " << b1.s2 << '\n';
}
