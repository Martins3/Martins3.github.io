// https://github.com/ethsonliu/stackoverflow-top-cpp/blob/master/question/015%20-%20%E8%BF%90%E7%AE%97%E7%AC%A6%E9%87%8D%E8%BD%BD%E7%9A%84%E5%9F%BA%E6%9C%AC%E8%A7%84%E5%88%99%E5%92%8C%E4%B9%A0%E6%83%AF%E7%94%A8%E6%B3%95%E6%98%AF%E4%BB%80%E4%B9%88.md
//
// 赋值运算符 =、数组下标运算符 []、成员访问符 -> 和 函数调用运算符 ()，只能作为成员函数，因为 C++ 语法就是这么要求的。
//
// If a binary operator treats both operands equally (it leaves them unchanged), implement this operator as a non-member function.
// 所以: Compound assignment	+=, |=, *=, ... 应该是 member
// +, ==, <=>, /  倾向于 non-member 的
//
// https://stackoverflow.com/questions/9351166/does-overloading-operator-works-inside-the-class
// 	为什么 operator << 可以定义成员函数，但是这样 obj 就是第一个参数了
//
// https://stackoverflow.com/questions/57927030/is-there-any-reason-to-not-overload-operator-as-member-when-only-comparing-to
#include <iostream>
#include <string>
class Kitty {
    public:
	std::string name;
	friend std::ostream &operator<<(std::ostream &os, const Kitty &obj)
	{
		os << obj.name << std::endl;
		return os;
	}

	std::ostream &operator<<(std::ostream &os)
	{
		os << name << std::endl;
		return os;
	}

	friend std::istream &operator>>(std::istream &is, Kitty &obj)
	{
		// TODO 找一个 example 出来吧
		is.setstate(std::ios::failbit);

		return is;
	}

	int operator()(const std::string &y)
	{
		std::cout << "operator(const string)" << std::endl;
		return 1;
	}
	int operator()(int i)
	{
		std::cout << "operator(const i)" << std::endl;
		return 2;
	}
	friend inline bool operator==(const Kitty &lhs, const Kitty &rhs)
	{
		std::cout << "function ==" << std::endl;
		return lhs.name == rhs.name;
	}

	// TODO 但是定义为成员函数似乎也没什么问题
	// 两个不可以同时存在，会警告的
	// inline bool operator==(const Kitty &rhs) const
	// {
	// 	std::cout << "method ==" << std::endl;
	// 	return this->name == rhs.name;
	// }

	auto &operator++()
	{
		std::cout << "operator ++" << std::endl;
		return *this;
	}

	auto operator++(int)
	{
		Kitty tmp(*this);
		std::cout << "operator ++(int)" << std::endl;
		operator++();
		return tmp;
	}
};

int main(int argc, char *argv[])
{
	Kitty k;
	k.name = "hi";
	std::cout << k;

	k << std::cout;

	// 函数调用运算符，有点逆天
	std::cout << k("string") << std::endl;
	std::cout << k(2) << std::endl;

	Kitty t;
	std::cout << (k == t) << std::endl;

	t++;
	++t;

	return 0;
}
