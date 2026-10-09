#include <iostream>

int value = 10;

namespace net
{
int open()
{
	return 1;
}
}

namespace fs
{
int open()
{
	return 2;
}
}

namespace demo
{
int value = 20;
int read()
{
	// 名字前面的 `::` 表示从全局命名空间开始
	return value + ::value;
}
}

namespace app::net
{
int port()
{
	return 8080;
}
}
namespace app::net
{
int timeout()
{
	return 30;
}
}
namespace an = app::net;

// 定义 namespace 的时候，不可以用 an
// namespace an { };

namespace library
{
namespace v1
{
int version()
{
	return 1;
}
}
inline namespace v2
{
int version()
{
	return 2;
}
}
}

int main()
{
	using std::cout;
	cout << "net::open / fs::open: " << net::open() << " / " << fs::open()
	     << '\n';
	cout << "demo::value + ::value: " << demo::read() << '\n';
	cout << "app::net port / timeout: " << an::port() << " / "
	     << an::timeout() << '\n';
	// 没有自动的穿透的功能，必须写完整的 app::net::port();
	// 这样写是错误的
	// app::port
	cout << "default / v1 / v2: " << library::version() << " / "
	     << library::v1::version() << " / " << library::v2::version()
	     << '\n';
}
