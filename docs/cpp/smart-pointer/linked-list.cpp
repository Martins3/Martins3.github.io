#include <iostream>
#include <memory>
#include <thread>
#include <utility>

template <typename T> class SharedList {
	struct Node {
		T value;
		std::shared_ptr<Node> next;

		explicit Node(T v)
			: value(std::move(v))
		{
			std::cout << "create : " << value << std::endl;
		}

		~Node()
		{
			std::cout << "delete : " << value << std::endl;
		}
	};

	std::shared_ptr<Node> head_;

    public:
	// TODO 禁用了浅拷贝，那么就必须定义 default 吗?
	SharedList() = default;
	// 禁止浅拷贝，否则两个链表对象会共享同一批节点。
	SharedList(const SharedList &) = delete;
	SharedList &operator=(const SharedList &) = delete;

	// 允许移动：把整条链表的所有权转移给另一个对象。
	SharedList(SharedList &&) noexcept = default;

	SharedList &operator=(SharedList &&other) noexcept
	{
		if (this != &other) {
			clear();
			head_ = std::move(other.head_);
		}
		return *this;
	}

	~SharedList()
	{
		clear();
	}

	bool empty() const noexcept
	{
		return !head_;
	}

	// 头插。
	void push_front(T value)
	{
		auto node = std::make_shared<Node>(std::move(value));

		node->next = std::move(head_);
		head_ = std::move(node);
	}

	// 尾插。
	void push_back(T value)
	{
		auto node = std::make_shared<Node>(std::move(value));

		if (!head_) {
			head_ = std::move(node);
			return;
		}

		auto current = head_;
		while (current->next) {
			current = current->next;
		}

		current->next = std::move(node);
	}

	// 删除头节点，返回是否成功删除。
	bool pop_front() noexcept
	{
		if (!head_) {
			return false;
		}

		auto removed = std::move(head_);
		head_ = std::move(removed->next);

		// removed 析构时，其 next 已为空，只释放当前节点。
		return true;
	}

	// 删除第一个值等于 value 的节点。
	// TODO 这个写的非常好，可以仔细看看，相当于 shared_ptr 就是用于 pointer
	// 但是 pointer 是引用计数的
	bool erase_first(const T &value)
	{
		std::shared_ptr<Node> previous;
		auto current = head_;

		while (current) {
			if (current->value == value) {
				if (previous) {
					previous->next =
						std::move(current->next);
				} else {
					head_ = std::move(current->next);
				}

				// current 保证节点在重新接链时仍然存活。
				// 返回时 current 析构，被删除的节点随之释放。
				return true;
			}

			previous = current;
			current = current->next;
		}

		return false;
	}

	// 查找是否存在指定值。
	bool contains(const T &value) const
	{
		std::shared_ptr<const Node> current = head_;

		while (current) {
			if (current->value == value) {
				return true;
			}
			current = current->next;
		}

		return false;
	}

	// 遍历并打印。
	void print(std::ostream &out = std::cout) const
	{
		std::shared_ptr<const Node> current = head_;

		while (current) {
			out << current->value << " -> ";
			current = current->next;
		}

		out << "nullptr\n";
	}

	// 逐个释放，避免沿 next 递归析构整条链。
	void clear() noexcept
	{
		while (pop_front()) {
		}
	}
};

int main()
{
	SharedList<int> list;

	list.push_back(10);
	list.erase_first(10);
	std::this_thread::sleep_for(std::chrono::milliseconds(1000000));
	// list.push_back(20);
	// list.push_front(5);
	// list.print(); // 5 -> 10 -> 20 -> nullptr

	// list.erase_first(10);
	// list.print(); // 5 -> 20 -> nullptr

	// list.pop_front();
	// list.print(); // 20 -> nullptr

	// std::cout << std::boolalpha << "contains 20: " << list.contains(20)
	// 	  << '\n'
	// 	  << "contains 10: " << list.contains(10) << '\n';

	// auto moved = std::move(list);

	// std::cout << "source empty: " << list.empty() << '\n';
	// moved.print(); // 20 -> nullptr

	// moved.clear();
	// std::cout << "empty: " << moved.empty() << '\n';
}
