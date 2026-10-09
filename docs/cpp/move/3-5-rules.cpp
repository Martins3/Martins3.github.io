#include <iostream>
#include <cassert>
#include <utility>

// 经典例子：类独占一块动态内存，拷贝时深拷贝，移动时转移所有权。
// 三法则：析构函数、拷贝构造函数、拷贝赋值运算符。
// 五法则：再加上移动构造函数、移动赋值运算符（C++11）。
class A {
	int *data;

    public:
	explicit A(int value = 0)
		: data(new int(value))
	{
	}

	// 1. 析构函数：释放自己拥有的资源。
	~A()
	{
		delete data;
	}

	// 2. 拷贝构造函数：分配独立的内存，避免两个对象重复释放同一指针。
	A(const A &other)
		: data(other.data ? new int(*other.data) : nullptr)
	{
	}

	// 3. 拷贝赋值：先分配，再释放，分配失败时保留原来的值。
	virtual A &operator=(const A &other)
	{
		if (this != &other) {
			int *new_data = other.data ? new int(*other.data) : nullptr;
			delete data;
			data = new_data;
		}
		return *this;
	}

	// 4. 移动构造：接管指针，让源对象不再拥有资源。
	A(A &&other) noexcept
		: data(other.data)
	{
		other.data = nullptr;
	}

	// 5. 移动赋值：释放旧资源，再接管源对象的资源。
	A &operator=(A &&other) noexcept
	{
		if (this != &other) {
			delete data;
			data = other.data;
			other.data = nullptr;
		}
		return *this;
	}

	const int *get() const { return data; }
};

int main()
{
	A a(42);
	A b = a; // 拷贝构造
	assert(a.get() != b.get() && *b.get() == 42);

	A c(7);
	c = a; // 拷贝赋值
	assert(c.get() != a.get() && *c.get() == 42);
	c = c; // 自拷贝赋值
	assert(*c.get() == 42);

	const int *original = b.get();
	A d = std::move(b); // 移动构造，std::move 本身只是类型转换
	assert(b.get() == nullptr && d.get() == original);

	A e(9);
	e = std::move(d); // 移动赋值
	assert(d.get() == nullptr && e.get() == original);

	b = a; // 移动后的对象仍可重新赋值
	assert(b.get() != a.get() && *b.get() == 42);
	A empty(d); // 也允许拷贝处于空状态的对象
	assert(empty.get() == nullptr);
	std::cout << "Rule of Five: all checks passed" << std::endl;
	return 0;
}
