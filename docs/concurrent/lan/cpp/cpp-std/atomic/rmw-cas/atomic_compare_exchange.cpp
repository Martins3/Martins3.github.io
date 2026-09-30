// https://en.cppreference.com/w/cpp/atomic/atomic_compare_exchange#Notes
#include <atomic>

template <class T> struct node {
	T data;
	node *next;
	node(const T &data)
		: data(data)
		, next(nullptr)
	{
	}
};

template <class T> class stack {
	std::atomic<node<T> *> head;

    public:
	void push(const T &data)
	{
		node<T> *new_node = new node<T>(data);

		// put the current value of head into new_node->next
		new_node->next = head.load(std::memory_order_relaxed);

		while (!std::atomic_compare_exchange_weak_explicit(
			&head, &new_node->next, new_node,
			std::memory_order_release, std::memory_order_relaxed))
			;
	}
};

int main()
{
	stack<int> s;
	s.push(1);
	s.push(2);
	s.push(3);
}
