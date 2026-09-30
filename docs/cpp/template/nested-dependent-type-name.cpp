#include <iostream>
// https://stackoverflow.com/questions/53405099/confused-about-c-nested-dependent-type-name
template <typename ElementType> class BST {
    private:
	class LinkNode {
	    public:
		ElementType data;
		LinkNode *left, *right;
		explicit LinkNode()
		{
		}
	};

    public:
	void some_func();
};

template <typename ElementType> void BST<ElementType>::some_func()
{
	// 这里是可以不用加 typename 的，因为无论 ElementType 取什么，BST<ElementType>::LinkNode 的类型都是知道的
	BST<ElementType>::LinkNode *m;
	std::cout << m << std::endl;
}

// 先看简单的例子，说明一下，C 不可以加 typename ，而 iter 在这里必须添加
template <typename C> void f(const C &container, typename C::interator iter)
{
	std::cout << iter << std::endl;
}

template <typename C> void f2(const C &container)
{
	// 同样的，这里必须加上
	// TODO 这里的问题是 : 既然在这里不去定义
	typename C::interator iter;
	std::cout << iter << std::endl;
}

template <typename ElementType> class BST2 {
	class LinkNode { /*...*/
	};

	using NodePtr = LinkNode *;

	template <typename T> void some_func(); // 额外的 template 参数
};

template <typename ElementType>
template <typename T>
void BST2<ElementType>::some_func()
{
	typename BST2<T>::NodePtr ptr; // (1)
	typename BST2<T>::LinkNode *ptr2; // (2)

	BST2<ElementType>::NodePtr ptr3; // (3)
	typename BST2<ElementType>::LinkNode *ptr4; // (4)
	// (1) 和 (2) 都必须有 typename 修饰
	// (3) 和 (4) 可以不用加
	// it's a result of name lookup for member of the current instantiation.
	// TODO 似乎是那么回事，但是没有完全理解，睡觉！
}

int main(int argc, char *argv[])
{
	BST<int> a;
	a.some_func();
	return 0;
}
