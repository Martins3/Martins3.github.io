// 14 在资源管理类中，小心 copying 的行为

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
// TODO 这个函数的参数到底是什么？为什么 ccls 检查不出来，
// 非要编译器的 static assert 啊!
void wowo(const M *m)
{
	std::cout << "wowo : " << m->x << std::endl;
}
int main(int argc, char *argv[])
{
	std::shared_ptr<M> s(new M, wowo);
	return 0;
}
