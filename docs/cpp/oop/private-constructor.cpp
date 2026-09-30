// # cpp private constructor
// <!-- 37455823-1008-49c0-a7e1-f0bc8f5221b6 -->
#include <cassert>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

// 场景一：创建前必须校验参数。外部只能通过 create() 创建 User。
class User {
    public:
	static std::unique_ptr<User> create(std::string name)
	{
		if (name.empty()) {
			throw std::invalid_argument("name must not be empty");
		}
		// new 写在类的成员函数中，可以访问私有构造函数。
		// std::make_unique<User>(...) 不行：实际调用构造函数的是库函数。
		return std::unique_ptr<User>(new User(std::move(name)));
	}

	const std::string &name() const
	{
		return name_;
	}

    private:
	explicit User(std::string name)
		: name_(std::move(name))
	{
	}

	std::string name_;
};

// 场景二：由类自己决定实例的创建时机和数量。
class Counter {
    public:
	static Counter &instance()
	{
		static Counter
			counter; // C++11 起局部静态对象的初始化是线程安全的。
		return counter;
	}

	void increment()
	{
		++value_;
	}
	int value() const
	{
		return value_;
	}

	Counter(const Counter &) = delete;
	Counter &operator=(const Counter &) = delete;

    private:
	Counter() = default;

	int value_ = 0;
};

// 如果目标是彻底禁止创建对象，直接 = delete 比 private 更明确。
class Utility {
    public:
	Utility() = delete;
	static int double_value(int value)
	{
		return value * 2;
	}
};

int main()
{
	auto user = User::create("Alice");
	assert(user->name() == "Alice");
	// User invalid("Alice"); // 错误：构造函数是 private。
	bool rejected = false;
	try {
		User::create("");
	} catch (const std::invalid_argument &) {
		rejected = true;
	}
	assert(rejected);

	Counter::instance().increment();
	assert(&Counter::instance() == &Counter::instance());
	assert(Counter::instance().value() == 1);
	// Counter another; // 错误：构造函数是 private。
	// Counter copy = Counter::instance(); // 错误：复制构造函数已删除。

	static_assert(!std::is_default_constructible<Utility>::value,
		      "Utility must not be instantiated");
	assert(Utility::double_value(3) == 6);
}
