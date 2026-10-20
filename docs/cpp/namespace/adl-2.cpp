#include <iostream>

namespace geometry
{
struct triangle {};
}
namespace math
{
struct Point {};

// 由于 ADL ，这个可以直接被调用出来
void a(Point *a)
{
	std::cout << "it works" << std::endl;
}

void a(int)
{
	std::cout << "it works" << std::endl;
}

void b(int i, Point &b)
{
	std::cout << "it works" << std::endl;
}

void cc(geometry::triangle a, Point &b)
{
	std::cout << "in geometry" << std::endl;
}
}; // namespace math

namespace geometry
{
void b(int i, math::Point &b)
{
	std::cout << "in geometry" << std::endl;
}

void cc(triangle a, math::Point &b)
{
	std::cout << "in geometry" << std::endl;
}
} // namespace geometry

int main(int argc, char *argv[])
{
	struct math::Point p;
	struct geometry::triangle t;
	a(&p);
	math::a(12); // 必须有 math::

	b(12, p);
	geometry::b(12, p); // 可以显示添加上

	// 编译器的处理流程:
	// 参数的所属 namepace 是什么，然后改 namespace 中需要定义
	// 果然，正如我们想象的，每一个参数都可以参与，查找当前的
	// 所以，这里的名称就出现了 ambiguous 的警告
	// cc(t, p);
	return 0;
}
