// 3.cpp —— 依赖名字（dependent name）与 typename
//
// 原问题：
//   1) using ReboundType = X<T>;                        // 这里为什么是正确的？
//   2) using MemberType2 = typename X<T>::MemberType;   // 这里的 typename 是做什么的？
//
// 一句话回答：
//   * X<T> 整体是一个 type-id（一个类模板特化类型）。编译器看到“模板名 X”
//     紧跟“<T>”就知道这是模板实参列表，X<T> 必然是类型；虽然 T 是依赖的，
//     但“它一定是类型”没有歧义，也不涉及 "::" 成员查找，所以不需要 typename。
//   * typename 只服务于一种场合："::" 左边的 nested-name-specifier 是依赖的。
//     X<T>::MemberType 里 X<T> 依赖 T，编译器无法仅凭语法判断 MemberType 是
//     嵌套类型（typedef/using 别名）、static 数据成员、静态函数还是枚举。标准规定：这种情况下
//     默认按“非类型”理解，除非显式用 typename 说明它是类型。
//     所以 typename 是“消歧义标记”，不是“引入类型”。
//
// 见下面 "可运行 demo"，配合输出一起看就清楚了。

#include <iostream>
#include <string>
#include <type_traits>

// 打印一个类型名：__PRETTY_FUNCTION__ 里会带上模板实参，比 typeid().name() 好读
template <typename T> const char *raw_name()
{
	return __PRETTY_FUNCTION__;
}

template <typename T> std::string type_name()
{
	std::string s = raw_name<T>();
	const std::string key = "T = ";
	auto begin = s.find(key);
	if (begin == std::string::npos)
		return s;
	begin += key.size();
	auto end = s.find_first_of(";]", begin);
	return s.substr(begin, end - begin);
}

// ================= 可运行 demo =================

// X<T> 里有一个成员类型 MemberType，还有一个数据成员 value
template <typename T> struct X {
	using MemberType = T;
	T value{};
};

template <typename T> struct Y {
	// (1) 不需要 typename：X<T> 是一个完整的 type-id，没有 "::" 成员查找。
	//     ReboundType 就是 X<T> 本身，也就是 allocator 里的 rebind 惯用法。
	using ReboundType = X<T>;

	// (2) 需要 typename：X<T> 是依赖的 nested-name-specifier，MemberType 既可能
	//     是类型也可能是静态成员/函数，标准默认它不是类型，typename 用于声明
	//     “我保证它是类型”。这里 MemberType2 就是 T。
	using MemberType2 = typename X<T>::MemberType;
};

// 两个断言把上面的话钉死：
//   Y<int>::ReboundType   是 X<int>，不是 int
//   Y<int>::MemberType2   是 int，不是 X<int>
static_assert(std::is_same<Y<int>::ReboundType, X<int>>::value,
	      "ReboundType 应该等于 X<int>");
static_assert(std::is_same<Y<int>::MemberType2, int>::value,
	      "MemberType2 应该等于 int");
static_assert(std::is_same<Y<int *>::ReboundType, X<int *>>::value,
	      "ReboundType 随 T 变化：Y<int*>::ReboundType == X<int*>");

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	std::cout << "== (1) ReboundType = X<T>，不需要 typename ==\n";
	std::cout << "  " << type_name<Y<int>::ReboundType>() << "\n";
	std::cout << "  " << type_name<Y<double>::ReboundType>() << "\n";

	std::cout << "== (2) MemberType2 = T，需要 typename ==\n";
	std::cout << "  " << type_name<Y<int>::MemberType2>() << "\n";
	std::cout << "  " << type_name<Y<double>::MemberType2>() << "\n";

	std::cout << "== 实际用起来 ==";
	Y<int>::ReboundType a; // a 是 X<int>
	a.value = 42;
	std::cout << " a.value = " << a.value;
	Y<int>::MemberType2 b = 7; // b 是 int
	std::cout << ", b + 1 = " << b + 1 << "\n";

	return 0;
}

// ============ 反面教材（打开注释即可看到编译错误）============
//
// 1) X<T> 不涉及 "::" 成员查找，所以不存在歧义；在它前面加 typename 是语法错误，
//    因为 typename 后面必须跟 qualified-id：
//        using Bad = typename X<T>;   // error: expected nested-name-specifier
//
// 2) 依赖名字的语义检查推迟到实例化时进行。若 X<T> 里根本没有 MemberType，
//    只要 Y<T> 没被实例化就不报错：
//        template <typename T> struct X2 {};
//        template <typename T> struct Y2 {
//            using M = typename X2<T>::MemberType;   // 定义时不报错
//        };
//        Y2<int> y;   // 只有这里才报: no type named 'MemberType' in 'struct X2<int>'
//
// 3) C++20 起（P0634R3 "Down with typename!"），在“只可能放类型”的位置可以省略
//    typename，例如 decl-specifier-seq 开头的 typedef，以及 alias-declaration 的 type-id：
//        template <typename T> struct Z { using M = X<T>::MemberType; };  // C++20 OK
//    C++17 会报: need 'typename' before 'X<T>::MemberType' because 'X<T>' is a
//    dependent scope。为可读性，多数代码库仍然保留 typename。
