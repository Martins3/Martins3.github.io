#include <iostream>
#include <shared_mutex>

// https://stackoverflow.com/questions/19559503/call-to-implicitly-deleted-copy-constructor-in-llvm
class R {
	std::shared_timed_mutex mut;
	int a;

    public:
	// TODO R & operator=(const R &other) 也是一个修改方法
	R operator=(const R &other)
	{
		std::cout << "operator = " << std::endl;
		// 如果取消掉 std::shared_timed_mutex 的注释，这里会报错
		// TODO 如果 operator = 的返回都需要拷贝，有必要吗?
		// call to implicitly deleted copy constructor of 'Uni'
		return *this;
	}

};

int main()
{
	R l;
	R r;
	l = r;
}
