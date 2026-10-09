#include <iostream>

#include <source_location>

static inline void
log(std::source_location loc = std::source_location::current())
{
	std::cout << loc.function_name() << " ";
}

// 普通的 reference
class Dog {
    public:
	int val;

	Dog(int a)
	{
		val = a;
	}

	Dog(const Dog &d)
	{
		std::cout << "constructor" << std::endl;
	}

	Dog(Dog &&d)
		: val(d.val)
	{
		std::cout << "move constructor" << std::endl;
	}

	Dog &operator=(const Dog &d)
	{
		std::cout << "operator =" << std::endl;
		return *this;
	}
};

// template <typename T> void logged_process(T s) 不可以继续定义了，和 T s 和 T && s 都无法区分
// 重载规则无法选出唯一更优的函数，因此报错。编译器不会因为引用版本避免了拷贝，就优先选择引用版本。
// 不过 T & s 和 T && s 是可以同时定义的，因为 T && s 是 forwarding reference ，
// 而 T & 是非常精确的结果:

// 相当于这个产生了两个了
// void logged_process<Dog>(Dog && s)
// void logged_process<Dog &>(Dog & s)
template <typename T> void logged_process(T &&s)
{
	log();
	// 但是这个显然导致了一个问题，如果我想要清晰的表达，我就是需要一个 xvalue
	// 在书写这个函数的时候，实际上 无论是 T = Dog & 还是 T = Dog
	//
	// 也就是参数是 lvalue 还是 xvalue ，都是可以继续 std::move 的
	// 这会调用
	//
	// TODO 真的可以构建出来，让 lvalue 还是 xvalue 不同的情况吗?
	auto m = std::move(s);
	std::cout << "T &&" << std::endl;
}

// 如果不去定义这个，那么就可以自动的产生出来
template <typename T> void logged_process(T &s)
{
	log();
	std::cout << "T &" << std::endl;
}


int main(int argc, char *argv[])
{
	Dog d(123);
	const Dog c(123);

	// 这两个的输出结果是非常典型的:
	// void logged_process(T&&) [with T = Dog&] T &&
	// void logged_process(T&&) [with T = Dog] T &&
	//
	// 当 lvalue 的时候，就是传递引用
	// 而当是 prvalue 的时候，就是直接传递到 Dog
	logged_process(d);
	logged_process(std::move(d));

	// const 也是可以的，对于规则没有影响
	logged_process(c);
	logged_process(std::move(c));
}
