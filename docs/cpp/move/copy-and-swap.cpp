// https://github.com/ethsonliu/stackoverflow-top-cpp/blob/master/question/016%20-%20copy-and-swap%20%E6%98%AF%E4%BB%80%E4%B9%88.md
// 解释的太好了
#include <algorithm> // std::copy
#include <cstddef> // std::size_t
#include <iostream>
#include <tuple>
#include <vector>

class DumbArray {
    public:
	DumbArray(std::size_t size = 0)
		: mSize(size)
		, mArray(mSize ? new int[mSize]() : nullptr)
	{
		std::cout << "(default) constructor" << std::endl;
	}

	DumbArray(const DumbArray &other)
		: mSize(other.mSize)
		, mArray(mSize ? new int[mSize] : nullptr)
	{
		// note that this is non-throwing, because of the data
		// types being used; more attention to detail with regards
		// to exceptions must be given in a more general case, however
		std::copy(other.mArray, other.mArray + mSize, mArray);
		std::cout << "copy-constructor" << std::endl;
	}

	~DumbArray()
	{
		std::cout << "destructor" << std::endl;
		delete[] mArray;
	}

	friend void swap(DumbArray &first, DumbArray &second)
	{
		// TODO 什么是 ADL
		// enable ADL (not necessary in our case, but good practice)
		using std::swap;

		// by swapping the members of two objects,
		// the two objects are effectively swapped
		swap(first.mSize, second.mSize);
		swap(first.mArray, second.mArray);
	}

	// 这里的 = 自动的调用了 copy-constructor ，swap 之后，
	// copy-constructor 构建的对象会自动删除
	DumbArray &operator=(DumbArray other)
	{
		swap(*this, other);
		std::cout << "operator =" << std::endl;
		return *this;
	}

    private:
	std::size_t mSize;
	int *mArray;
};

static void basic(){
	std::vector<int> a{1,2};
	// 没想到吧，这个 std::swap 是可以直接作用于 a[0] 的
	// 这是 C 语言用户难以理解的
	std::swap(a[0], a[1]);
	// 这是不可以的，因为 1 和 2 都是 prvalue
	// std::swap(1 , 2);
}

int main(int argc, char *argv[])
{
	DumbArray a;
	DumbArray b;
	a = b;

	std::cout << "end" << std::endl;
	return 0;
}
