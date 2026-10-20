#include <iostream>
#include <type_traits>

int main()
{
	// 用法：std::remove_pointer_t<类型>，在编译期去掉一层指针。
	using A = std::remove_pointer_t<int *>;       // int
	using B = std::remove_pointer_t<const int *>; // const int，保留对象的 const
	using C = std::remove_pointer_t<int **>;      // int*，只去掉一层
	using D = std::remove_pointer_t<int>;         // int，非指针类型保持不变

	// 用 is_same 检查得到的类型，以下四行都输出 true。
	std::cout << std::boolalpha;
	std::cout << "int* -> int: " << std::is_same<A, int>::value << '\n';
	std::cout << "const int* -> const int: "
		  << std::is_same<B, const int>::value << '\n';
	std::cout << "int** -> int*: " << std::is_same<C, int *>::value << '\n';
	std::cout << "int -> int: " << std::is_same<D, int>::value << '\n';

	// A 已经是 int，可以直接用来声明普通变量。
	A value = 42;
	std::cout << "value: " << value << '\n';

	// C++14 的 remove_pointer_t<T> 是 typename remove_pointer<T>::type 的简写。
	// 它只转换类型，不会解引用指针，也不会释放内存。
}
