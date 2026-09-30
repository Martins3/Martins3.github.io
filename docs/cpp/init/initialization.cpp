#include <array>
#include <iostream>
#include <utility>
#include <vector>

struct Point {
	int x;
	int y = 7;
};

class Trace {
    public:
	explicit Trace(int value) : value_(value)
	{
		std::cout << "constructor: " << value_ << '\n';
	}

	Trace(const Trace &other) : value_(other.value_)
	{
		std::cout << "copy constructor: " << value_ << '\n';
	}

	Trace(Trace &&other) noexcept : value_(other.value_)
	{
		other.value_ = -1;
		std::cout << "move constructor: " << value_ << '\n';
	}

	Trace &operator=(const Trace &other)
	{
		value_ = other.value_;
		std::cout << "copy assignment: " << value_ << '\n';
		return *this;
	}

	Trace &operator=(Trace &&other) noexcept
	{
		value_ = other.value_;
		other.value_ = -1;
		std::cout << "move assignment: " << value_ << '\n';
		return *this;
	}

	int value() const
	{
		return value_;
	}

    private:
	int value_;
};

void title(const char *text)
{
	std::cout << "\n=== " << text << " ===\n";
}

void scalar_and_aggregate_demo()
{
	title("scalar and aggregate initialization");

	int zero{};
	int direct(8);
	int list{ 9 };
	std::cout << "scalars: " << zero << ' ' << direct << ' ' << list << '\n';

	Point empty{};
	Point partial{ 1 };
	Point list_point{ 2, 3 };
	Point designated{ .x = 4, .y = 5 };
	Point parenthesized(6, 7); // C++20 聚合体圆括号初始化

	std::cout << "points: (" << empty.x << ", " << empty.y << ") ("
		  << partial.x << ", " << partial.y << ") (" << list_point.x
		  << ", " << list_point.y << ") (" << designated.x << ", "
		  << designated.y << ") (" << parenthesized.x << ", "
		  << parenthesized.y << ")\n";
}

void copy_and_move_demo()
{
	title("construction versus assignment");

	Trace source{ 10 };
	Trace direct(source); // 直接初始化：拷贝构造
	Trace copy = source; // 拷贝初始化：仍是拷贝构造

	Trace assigned{ 0 };
	assigned = source; // 对象已存在：拷贝赋值

	Trace moved(std::move(source)); // 初始化新对象：移动构造
	assigned = Trace{ 20 }; // 修改已有对象：移动赋值

	std::cout << "values: source=" << source.value() << ", direct="
		  << direct.value() << ", copy=" << copy.value() << ", assigned="
		  << assigned.value() << ", moved=" << moved.value() << '\n';
}

void array_and_container_demo()
{
	title("arrays and containers");

	int raw[5]{ 1, 2 };
	std::cout << "raw array:";
	for (int value : raw)
		std::cout << ' ' << value;
	std::cout << '\n';

	std::array<int, 3> source{ 3, 4, 5 };
	std::array<int, 3> copy = source;
	std::cout << "std::array copy:";
	for (int value : copy)
		std::cout << ' ' << value;
	std::cout << '\n';

	std::vector<int> count_and_value(3, 9);
	std::vector<int> elements{ 3, 9 };
	std::cout << "vector(3, 9): size=" << count_and_value.size() << '\n';
	std::cout << "vector{3, 9}: size=" << elements.size() << '\n';
}

int main()
{
	scalar_and_aggregate_demo();
	copy_and_move_demo();
	array_and_container_demo();
}
