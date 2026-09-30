#include <iostream>

class Counter {
	int value = 0;

    public:
	Counter()
	{
		std::cout << "init" << std::endl;
	};
	Counter(Counter &&)
	{
		std::cout << "B" << std::endl;
	}
};

class M {
	int value = 0;

    public:
	M(M &&)
	{
		std::cout << "B" << std::endl;
	}

	// TODO
	// 1. 所有的构造函数都是可以没有返回值的吗?
	// 2. 编译器会自动的添加的内容都有哪些，这导致了我们需要使用 delete 函数的
	// 3. 移动赋值函数，是不是需要自动的来删除掉这些东西?
	// 4. 似乎 operator 的参数是可以不断变化的
	operator=(M &&)
	{
		std::cout << "B" << std::endl;
	}

};

// const-move.cpp: In function ‘int main(int, char**)’:
// const-move.cpp:33:13: error: use of deleted function ‘constexpr Counter& Counter::operator=(const Counter&)’
//    33 |         c = b;
//       |             ^
// const-move.cpp:7:7: note: ‘constexpr Counter& Counter::operator=(const Counter&)’ is
// implicitly declared as deleted because ‘Counter’ declares a move constructor or move
// assignment operator
//     7 | class Counter {
//       |       ^~~~~~~

int main(int argc, char *argv[])
{

	// 1. 由于 M 一定了一个构造函数，但是是移动构造函数，其他的构造函数都不存在
	//
	// const-move.cpp:43:11: error: no matching function for call to ‘M::M()’
	//    43 |         M m;
	//       |           ^
	// const-move.cpp:43:11: note: there is 1 candidate
	// const-move.cpp:21:9: note: candidate 1: ‘M::M(M&&)’
	//    21 |         M(M &&)
	//       |         ^
	// const-move.cpp:21:9: note: candidate expects 1 argument, 0 provided
	// M m;


	Counter c;
	Counter b;
	// c = b;


	return 0;
}
