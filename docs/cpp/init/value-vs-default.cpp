#include <cassert>
#include <cerrno> #include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <iomanip>
#include <iostream>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <unistd.h>

// 按 C++20 编译，例如：
// g++ -std=c++20 -O2 -Wall -Wextra value-vs-default.cpp -o value-vs-default.out
//
// 这是 Linux + GCC/Clang 的机器内存实验：
// 1. 给每个对象准备独立、正确对齐的存储，并填入 0x78。
// 2. placement new 在指定地址构造对象，观察不同初始化形式的写入。
// 3. 通过 /proc/self/mem 读取内存快照，不对未初始化的 int 求值。
//
// 输出的 78787878 是本实验预填的字节，不是普通 T t; 的保证初值。
// 不能仅凭预填过存储，就认为新对象的未初始化成员可以作为 int 读取：
// https://cplusplus.github.io/CWG/issues/1997.html
// https://man7.org/linux/man-pages/man5/proc_pid_mem.5.html
static_assert(sizeof(int) == sizeof(std::uint32_t));

template<class T>
struct Storage {
	alignas(T) unsigned char bytes[sizeof(T)];

	Storage()
	{
		// 这里只管理存储；含 string 的对象要先析构，再让存储离开作用域。
		std::memset(bytes, 0x78, sizeof(bytes));
		// 让预填实际写入内存，不能因之后的生命周期变化而被优化掉。
		asm volatile("" : : "r"(bytes) : "memory");
	}
};

// placement new 的对象只调用析构函数，不能用普通 delete 释放其存储。
// 配合 unique_ptr，即使 snapshot() 抛出异常也会正常析构 string 成员。
struct DestroyOnly {
	template<class T>
	void operator()(T *object) const
	{
		std::destroy_at(object);
	}
};

template<class T>
using PlacedObject = std::unique_ptr<T, DestroyOnly>;

std::uint32_t snapshot(const void *address)
{
	// 编译器屏障：先完成待观察的写入，再让内核读取该地址。
	asm volatile("" : : "r"(address) : "memory");
	const int fd = open("/proc/self/mem", O_RDONLY | O_CLOEXEC);
	if (fd == -1)
		throw std::system_error(errno, std::generic_category(), "open /proc/self/mem");

	std::uint32_t result = 0;
	ssize_t count;
	do {
		count = pread(fd, &result, sizeof(result),
			      static_cast<off_t>(reinterpret_cast<std::uintptr_t>(address)));
	} while (count == -1 && errno == EINTR);
	const int read_errno = errno;
	close(fd);
	if (count == -1)
		throw std::system_error(read_errno, std::generic_category(), "pread /proc/self/mem");
	if (count != sizeof(result))
		throw std::runtime_error("incomplete memory snapshot");
	return result; // 读取的是内核已写入的 result，不是原对象的 int。
}

// 1. 基本类型
__attribute__((noinline)) void scalar_demo()
{
	Storage<int> sa, sb, sc;
	int *a = new (sa.bytes) int; // 对应 int a;：default initialization
	int *b = new (sb.bytes) int(); // value initialization：0
	int *c = new (sc.bytes) int{}; // 空列表 → value initialization：0

	// 堆上的存储也先填入 0x78，不依赖分配器是否复用已释放的块。
	auto sp = std::make_unique<Storage<int>>();
	auto sq = std::make_unique<Storage<int>>();
	auto sm = std::make_unique<Storage<int>>();
	int *p = new (sp->bytes) int;
	int *q = new (sq->bytes) int();
	int *m = new (sm->bytes) int{};

	assert(*b == 0 && *c == 0 && *q == 0 && *m == 0);
	std::cout << "scalar bytes: a=" << std::hex << snapshot(a)
		  << " b=" << snapshot(b) << " c=" << snapshot(c)
		  << " | p=" << snapshot(p) << " q=" << snapshot(q)
		  << " m=" << snapshot(m) << std::dec << '\n';
}

// 2. 类类型：默认构造函数是否 user-provided 决定 value initialization 是否先清零
struct A {
	int x;
	std::string name;
	A() = default; // 首次声明处 = default，不算 user-provided
};

struct B {
	int x;
	std::string name;
	B()
	{
	} // 空函数体，算 user-provided
};

struct C {
	int x;
	std::string name;
	C();
};

C::C() = default; // 首次声明之后才 = default，算 user-provided

__attribute__((noinline)) void class_demo()
{
	Storage<A> sa, sva, sma;
	Storage<B> sb, svb;
	Storage<C> sc, svc;
	PlacedObject<A> a{new (sa.bytes) A}; // default init
	PlacedObject<B> b{new (sb.bytes) B}; // default init
	PlacedObject<C> c{new (sc.bytes) C}; // default init

	static_assert(!std::is_aggregate_v<A>); // C++20：有用户声明的构造函数
	PlacedObject<A> m{new (sma.bytes) A{}}; // direct-list → value init → 先零初始化

	PlacedObject<A> pa{new (sva.bytes) A()}; // 非 user-provided：先零初始化
	PlacedObject<B> pb{new (svb.bytes) B()}; // user-provided：没有初始化 x 的写入
	PlacedObject<C> pc{new (svc.bytes) C()}; // 类外 = default：没有初始化 x 的写入

	// 无论 A/B/C、default/value，未指定初始化器的 name 都会调用 string()
	// 得到空字符串；同样的成员默认初始化对 int x 则不执行数值初始化。
	// A 的 value init 额外先零初始化，所以 pa->x 和 m->x 为 0。
	assert(pa->x == 0 && m->x == 0);
	assert(a->name.empty() && pa->name.empty() && m->name.empty());
	assert(b->name.empty() && pb->name.empty());
	assert(c->name.empty() && pc->name.empty());
	std::cout << "class bytes (default/value): A=" << std::hex
		  << snapshot(&a->x) << '/' << snapshot(&pa->x)
		  << " B=" << snapshot(&b->x) << '/' << snapshot(&pb->x)
		  << " C=" << snapshot(&c->x) << '/' << snapshot(&pc->x)
		  << " | A{}=" << snapshot(&m->x) << std::dec << '\n';

	const auto print_names = [](const char *type, const auto &def, const auto &val) {
		std::cout << type << " name: default=" << std::quoted(def->name)
			  << " (size=" << def->name.size() << ") | value="
			  << std::quoted(val->name) << " (size=" << val->name.size() << ")\n";
	};
	print_names("A", a, pa);
	print_names("B", b, pb);
	print_names("C", c, pc);
	std::cout << "A{} name=" << std::quoted(m->name)
		  << " (size=" << m->name.size() << ")\n";
	// PlacedObject 比 Storage 后声明，离开作用域时先析构对象，再释放存储。
}

// 3. 成员自身的初始化规则仍然生效
struct D {
	int x = 42; // 默认成员初始化器
	int y;

	D()
		: y(7)
	{
	} // 成员初始化列表
};

__attribute__((noinline)) void member_demo()
{
	D d; // default initialization
	D *p = new D(); // value initialization

	// x、y 都由构造函数/默认成员初始化器确定，两种初始化结果一致
	assert(d.x == 42 && d.y == 7 && p->x == 42 && p->y == 7);
	std::cout << "D: default x=" << d.x << " y=" << d.y
		  << " | value x=" << p->x << " y=" << p->y << '\n';

	delete p;
}

// 5. 静态存储期对象有额外的零初始化规则
int global; // 静态存储期：零初始化，得到 0

__attribute__((noinline)) void static_demo()
{
	static int s; // 静态存储期：零初始化，得到 0
	// int local; // 自动存储期：值不确定，不在此读取

	assert(global == 0 && s == 0);
	std::cout << "static: global=" << global << " s=" << s << '\n';
}

// 6. 数组：int a[3] 与 int b[3]{} 的区别
__attribute__((noinline)) void array_demo()
{
	Storage<int[3]> sa, sb;
	int *a = new (sa.bytes) int[3]; // default init
	int *b = new (sb.bytes) int[3]{}; // 空列表初始化：0, 0, 0

	assert(b[0] == 0 && b[1] == 0 && b[2] == 0);
	std::cout << "array bytes: " << std::hex << snapshot(a) << ' '
		  << snapshot(a + 1) << ' ' << snapshot(a + 2) << " | "
		  << snapshot(b) << ' ' << snapshot(b + 1) << ' '
		  << snapshot(b + 2) << std::dec << '\n';
}

int main()
{
	std::cout << "Memory prefill: 0x78 per byte; snapshots below are hexadecimal.\n";
	scalar_demo();
	class_demo();

	member_demo();
	static_demo();
	array_demo();
	return 0;
}
