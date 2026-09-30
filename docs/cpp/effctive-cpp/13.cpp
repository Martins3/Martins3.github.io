// 13 以对象管理资源

// 似乎只是 unique_ptr 可以放到 container 中而已，没有其他的问题
// https://stackoverflow.com/questions/3697686/why-is-auto-ptr-being-deprecated
#include <iostream>
#include <memory>

class M {
    public:
	int x = 12;
	M()
	{
		std::cout << "M constructor" << std::endl;
	}

	~M()
	{
		std::cout << "M destructor" << std::endl;
	}
};

int main(int argc, char *argv[])
{
	// auto_ptr 的标准错误类型，ap2 被 move 到 ap3 之后，但是还是可以使用
	// std::auto_ptr<M> ap2(new M);
	// std::auto_ptr<M> ap3(ap2);
	// std::cout << ap2->x << std::endl;

	std::unique_ptr<M> s(new M);
	std::unique_ptr<M> s2(std::move(s)); // 必须 explicit 的移动
	std::unique_ptr<M> s4(s2.release());
	/* std::unique_ptr<M> s3(s); // 错误 */

	std::cout << s->x << std::endl; // 这里会报错，看来没有想象的 nb
	std::cout << s4->x << std::endl;
	return 0;
}
