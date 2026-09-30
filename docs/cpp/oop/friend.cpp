// # cpp friend
// <!-- fb3a24b6-9c23-4f02-828b-59830fd5a33a -->
#include <cassert>
#include <iostream>
#include <sstream>

// 前向声明：Account 此时还不完整，只能先声明 TaxOffice::collect。
class Account;

class TaxOffice {
    public:
	int collect(const Account &a);
};

// 场景一：三种 friend 授权方式
class Account {
    public:
	explicit Account(int balance)
		: balance_(balance)
	{
	}

    private:
	int balance_;

	// 1) 友元函数：非成员、无 this，但被授权访问 private 成员。
	friend int balance_of(const Account &a)
	{
		return a.balance_;
	}

	// 2) 友元类：整个 Auditor 类都能访问 Account 的私有成员。
	friend class Auditor;

	// 3) 友元成员函数：只授权 TaxOffice::collect 这一个成员函数。
	friend int TaxOffice::collect(const Account &a);

	friend int out_of(const Account &a);
};

// 额外说明一个细节，注意，friend 只能使用 class 内部使用，
int out_of(const Account &a)
{
	return a.balance_;
}

// collect 的定义放在 Account 完整之后，此时才能访问 a.balance_。
int TaxOffice::collect(const Account &a)
{
	return a.balance_;
}

class Auditor {
    public:
	static int balance(const Account &a)
	{
		return a.balance_;
	}
};

// 场景二：friend + operator —— 对称二元运算与流运算符
class Vec2 {
    public:
	Vec2(int x, int y)
		: x_(x)
		, y_(y)
	{
	}

	// hidden friend：非成员、无 this；普通名字查找找不到它，只能通过
	// ADL（实参依赖查找）在 a + b 中定位到它。
	friend Vec2 operator+(Vec2 lhs, const Vec2 &rhs)
	{
		lhs.x_ += rhs.x_;
		lhs.y_ += rhs.y_;
		return lhs;
	}

	friend bool operator==(const Vec2 &a, const Vec2 &b)
	{
		return a.x_ == b.x_ && a.y_ == b.y_;
	}

	// 流运算符：左操作数是 std::ostream，不可能写成成员函数，
	// 只能是非成员；要访问私有成员就必须声明为 friend。
	friend std::ostream &operator<<(std::ostream &os, const Vec2 &v)
	{
		return os << "(" << v.x_ << ", " << v.y_ << ")";
	}

    private:
	int x_;
	int y_;
};

int main()
{
	Account a{ 100 };
	assert(balance_of(a) == 100);
	assert(out_of(a) == 100);
	assert(Auditor::balance(a) == 100);
	assert(TaxOffice{}.collect(a) == 100);

	const Vec2 p{ 1, 2 };
	const Vec2 q{ 3, 4 };
	const Vec2 r = p + q;
	assert(r == Vec2(4, 6));
	assert(p + q == Vec2(4, 6));

	std::ostringstream os;
	os << p;
	assert(os.str() == "(1, 2)");
}
