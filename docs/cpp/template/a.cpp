#include <cstdio>
#include <atomic>
#include <iostream>

int m(int m)
{
	int n;
	// scanf("%d", &n);
	return n + 10;
}

template <int N> inline int length = m(N);

void test1()
{
	// 1. length 居然是可以动态执行的，好吧，我本来以为只能是 constexpr
	// 2. 变量模板本身不能声明成函数内的局部变量，所以当 length 初始化的时候，
	// 是全局变量的初始化，这个函数 length 对于 m 的调用会是在 main 之前的!
	//
	//   编译程序
	//       编译 test1() 的函数体
	//       实例化 length<3>
	//       生成变量存储及调用 m(3) 的初始化代码
	//       此时不执行 scanf()
	//
	//   运行生成的程序（你目前观察到的行为）
	//       执行全局变量的初始化代码
	//           调用 m(3)
	//               scanf() 等待输入
	//               返回输入值 + 10
	//           保存到 length<3>
	//       进入 main()
	//           调用 test2()

	std::cout << length<3> << std::endl;
}

template <class Event> inline std::atomic<unsigned> event_count{ 0 };

struct Login {};
struct Logout {};

void on_login()
{
	++event_count<Login>;
	std::cout << "login" << event_count<Login> << std::endl;
}

void on_logout()
{
	++event_count<Logout>;
	std::cout << "logout" << event_count<Logout> << std::endl;
}

void test2()
{
	on_login();
	on_logout();
	on_logout();
	on_logout();
	on_logout();
}

// 对于 template 的结果最经典的原理就是在:
//
//  // 变量模板的写法：直接取得结果
//   static_assert(std::is_same_v<int, int>);
//
//   std::is_same_v 就是标准库提供的变量模板，其定义可以理解为：
//
//   template<class T, class U>
//   inline constexpr bool is_same_v = std::is_same<T, U>::value;
int main(int argc, char *argv[])
{
	test2();
	return 0;
}
