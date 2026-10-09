#include <forward_list>
#include <list>
#include <iostream>
#include <memory>

template <typename T> class Node {
    public:
	T val;
	std::shared_ptr<Node> next;
	Node(T a)
		: val(a)
	{
		std::cout << "create : " << val << std::endl;
	}
	~Node()
	{
		std::cout << "remove : " << val << std::endl;
	}
};

class List {
	std::shared_ptr<Node<int> > head;

    public:
	void add(int val)
	{
		auto m = std::make_shared<Node<int> >(val);
		if (head) {
			// 现在 head 中资源被放到两份
			// 这里，资源和 std::make_shared 明显区分了
			m->next = head;
		}
		head = std::move(m);
	}

	void print()
	{
		auto m = head;
		while (m) {
			std::cout << m->val << std::endl;
			m = m->next;
		}
	}

	void remove(int x)
	{
		auto m = head;
		auto p = head;
		if (m->val == x) {
			head = head->next;
			return;
		}

		m = head->next;
		while (m) {
			if (m->val == x) {
				p->next = m->next;
				return;
			}
			p = m;
			m = m->next;
		}
	}
	// 需要注意，这里的链表析构居然是自动的
	// 因为当 List 析构的时候，会移除掉 reference
};

void test1()
{
	List a;
	a.add(1);
	a.add(2);
	a.add(3);
	a.print();

	a.remove(2);
	a.print();

	a.remove(1);
	a.print();

	a.remove(3);
	a.print();
}

void test2(){
	List a;
	a.add(1);
	a.add(2);
	a.add(3);
	a.remove(4);
	a.print();
}

int main(int argc, char *argv[])
{
	test2();
	return 0;
}
