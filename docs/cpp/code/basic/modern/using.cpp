#include <vector>
class A {
    public:
	int a;
	typedef int type;
};

template <typename T> class A2 {
    public:
	typedef std::vector<T> type;
};
int main(int argc, char *argv[])
{
	// 先看最简单的，class 中是可以定义类型的
	using abc = A::type;

	using xyz = A2<int *>::type;

	// a type alias block_ptr_t that represents a pointer to a block of int values.
	using block_ptr_t = std::pointer_traits<int *>::template rebind<int *>;

	int a;
	int *b = &a;
	block_ptr_t block_ptr = &b;

	return 0;
}
