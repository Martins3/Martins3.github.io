#include <list>
#include <vector>
#include <mutex>
#include <queue>

struct Counter {
	std::mutex mtx; // std::mutex 不可拷贝、不可移动
	int value = 0;

	Counter() = default;
	Counter(const Counter &) = delete;
	Counter(Counter &&) = delete;
	Counter(const Counter &&) = delete;

	int a() = delete;
	// 这显然不行，但是不是只有构造函数才可以有 default 的
	// int b() = default;
};

struct Empty {};

// TODO 这里的问题可以继续的，很有意思
// 1. queue 还有其他的内容 ?
//    2. <stack>
// 2. 移动 / 拷贝?
// 4. 所有的构造函数的定义都是完全相同的吗?  Counter(const Counter &&)
// 5. default 就只有一个吗？
// 6. 还有多少个这种当默认给定义，当我定义了这个，就不会给默认定义的场景?
int main(int argc, char *argv[])
{
	Counter a;
	Counter b{};
	// 这个声明方法没有错误，但是会引入警告，因为这个和函数声明很类似
	// Counter c();

	// 先说明一个普通的例子
	std::vector<Empty> ev;
	ev.emplace_back();
	ev.push_back(Empty{});
	ev.push_back(Empty()); // c++ 的逆天的地方，这样定义又是没问题的

	std::list<Counter> counters;
	counters.emplace_back(); // 可以：原地构造
	// counters.push_back(Counter()); // 错误：Counter 不可移动

	std::vector<Counter> mm;
	// mm.emplace_back(); // 错误:
	// mm.push_back(Counter{}); // 错误：Counter 不可移动

	std::queue<Counter> q;
	// q.emplace(Counter{}); // 错误
	return 0;
}
