#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
using namespace std;

// # 右值引用
// <!-- a1b54185-cf7d-478b-ae36-76ccf9a3d634 -->
//
// 普通左值引用：
//
// std::string& ref = s;
//
// 只能绑定到普通左值。
//
// 右值引用：
//
// std::string&& ref = std::string("hello");
//
// 主要用于绑定右值。于是函数可以区分：
//
// void consume(const std::string& value)
// {
//     // value 的所有者可能还需要它，不能掏空
// }
//
// void consume(std::string&& value)
// {
//     // 调用者给了右值，可以接管 value 的资源
// }
//
// 调用时：
//
// std::string s = "hello";
//
// consume(s);            // 调用 const std::string&
// consume(std::move(s)); // 调用 std::string&&
// consume("temporary");  // 临时对象，也可以调用 std::string&&
//
// 右值引用的意义不是“引用临时对象”这么简单，而是：
// 它给重载系统提供了一种信号：该对象的资源是否允许被复用。


struct RValue {
	explicit RValue(string value = "hello!!!")
		: id(++next_id)
		, sources(std::move(value))
	{
		cout << "construct       #" << id << " sources='" << sources << "'\n";
	}

	// TODO 原来这里的 noexcept 是可以去掉的
	RValue(RValue &&a) noexcept
		: id(++next_id)
		, sources(std::move(a.sources))
	{
		cout << "move construct  #" << id << " <- #" << a.id
		     << ", source now '" << a.sources << "'\n";
	}

	RValue(const RValue &a)
		: id(++next_id)
		, sources(a.sources)
	{
		cout << "copy construct  #" << id << " <- #" << a.id << '\n';
	}

	RValue &operator=(RValue &&a) noexcept
	{
		sources = std::move(a.sources);
		cout << "move assign     #" << id << " <- #" << a.id
		     << ", source now '" << a.sources << "'\n";
		return *this;
	}

	RValue &operator=(const RValue &a)
	{
		sources = a.sources;
		cout << "copy assign     #" << id << " <- #" << a.id << '\n';
		return *this;
	}

	int id;
	string sources;
	static inline int next_id = 0;

	~RValue()
	{
		cout << "destroy         #" << id << " sources='" << sources << "'\n";
	}
};

void title(const char *value)
{
	cout << "\n=== " << value << " ===\n";
}

// 这里只观察重载选择，本身并没有复制或移动对象。
void observe(const RValue &value)
{
	cout << "observe(const RValue&): #" << value.id << " is an lvalue\n";
}

void observe(RValue &&value)
{
	cout << "observe(RValue&&):       #" << value.id << " is an rvalue\n";
}

void value_category_demo()
{
	title("1. && 让重载识别右值");
	RValue value("named object");

	observe(value);
	observe(std::move(value));
	observe(RValue("temporary"));

	// FIXME 不是 value 已经被 move 了，为什么还可以访问?
	cout << "after observe(std::move(value)), value.sources is still '"
	     << value.sources << "'\n";
}

void take_rvalue_reference(RValue &&value)
{
	cout << "parameter type is RValue&&, but expression 'value' is: ";
	observe(value);

	cout << "std::move(value) changes the expression category to:     ";
	observe(std::move(value));

	cout << "now construct another object to perform the real move:\n";
	RValue taken(std::move(value));
}

void named_rvalue_reference_demo()
{
	title("2. 有名字的右值引用表达式是左值");
	RValue source("resource owned by source");
	take_rvalue_reference(std::move(source));
	cout << "back in caller, source.sources is now '" << source.sources << "'\n";
	cout << "注意：标准只保证 moved-from 对象有效，不保证 string 一定为空。\n";
}

void copy_and_move_demo()
{
	title("3. std::move 只是许可，构造/赋值才真正移动");
	RValue source("expensive resource");

	RValue &&alias = std::move(source);
	static_assert(is_rvalue_reference_v<decltype(alias)>);
	static_assert(is_lvalue_reference_v<decltype((alias))>);
	cout << "creating an && alias did not move: source.sources is '"
	     << source.sources << "'\n";

	// alias 虽然声明类型是 RValue&&，但它有名字，因此表达式 alias 是左值。
	RValue copied(alias);
	RValue moved(std::move(alias));

	RValue copy_target("old copy target");
	copy_target = copied;

	RValue move_target("old move target");
	move_target = std::move(copied);
}

template <typename T>
void forward_badly(T &&value)
{
	cout << "T is "
	     << (is_lvalue_reference_v<T> ? "an lvalue reference; " : "not a reference; ")
	     << "passing the named parameter directly -> ";
	observe(value);
}

template <typename T>
void forward_perfectly(T &&value)
{
	cout << "T is "
	     << (is_lvalue_reference_v<T> ? "an lvalue reference; " : "not a reference; ")
	     << "std::forward<T>(value) -> ";
	observe(std::forward<T>(value));
}

void forwarding_reference_demo()
{
	title("4. T&& + 类型推导 = 转发引用");
	RValue value("forward me");

	cout << "pass lvalue:\n";
	forward_badly(value);
	forward_perfectly(value);

	cout << "pass rvalue:\n";
	forward_badly(RValue("temporary for bad forwarding"));
	forward_perfectly(RValue("temporary for perfect forwarding"));
}

vector<RValue> getV()
{
	// reserve(1) 后添加第二个元素会触发扩容。RValue 的移动构造是 noexcept，
	// 因此 vector 可以安全地把已有元素移动到新内存。
	// 现代 C++ 中 return x 通常触发 NRVO；即使没有 NRVO，也只需移动
	// vector 自身的内部指针，不会逐个移动其中的 RValue。
	vector<RValue> x;
	x.reserve(1);
	x.emplace_back("first vector element");

	cout << "capacity is full; adding another element:\n";
	x.emplace_back("second vector element");
	return x;
}

void vector_demo()
{
	title("5. noexcept move 让 vector 扩容时转移元素");
	vector<RValue> values = getV();
	cout << "returned vector size: " << values.size() << '\n';
}

int main()
{
	// value_category_demo();
	// named_rvalue_reference_demo();
	// copy_and_move_demo();
	forwarding_reference_demo();
	// vector_demo();

	return 0;
}
