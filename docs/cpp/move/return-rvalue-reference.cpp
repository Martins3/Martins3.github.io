// 工程场景：从结果容器中取出任务，把所有权交给队列。
// std::optional<JobPtr> 的右值访问器返回 T&&：
//     JobPtr&& value() &&;
// 这里 T = JobPtr = std::unique_ptr<Job>。
// Abseil 的 StatusOr<T>::value() && 也使用这种接口：
// https://github.com/abseil/abseil-cpp/blob/master/absl/status/statusor.h

#include <iostream>
#include <memory>
#include <optional>
#include <queue>
#include <string>
#include <utility>

struct Job {
	std::string name;

	explicit Job(std::string name) : name(std::move(name)) {
		std::cout << "move" << std::endl;
	}

	void run() const
	{
		std::cout << "running job: " << name << '\n';
	}
};

using JobPtr = std::unique_ptr<Job>;

int main()
{
	// TODO 这的确是一个经典的例子用来理解当函数返回值来返回 rvalue 的
	// 不过这里的 make_unique 的返回值为什么是 std::optional 的
	std::optional<JobPtr> result = std::make_unique<Job>("generate report");
	std::queue<JobPtr> queue;

	// value() 返回 JobPtr&&，queue.push() 移动这个 unique_ptr，接过任务。
	// 返回引用本身不转移所有权，真正的移动发生在队列构造元素时。
	queue.push(std::move(result).value());
	queue.front()->run();
	queue.pop();
}
