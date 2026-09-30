#include <iostream>
class X {
    public:
	X &operator+=(
		const X &rhs) // compound assignment (does not need to be a member,
	{ // but often is, to modify the private members)
		/* addition of rhs to *this takes place here */
		return *this; // return the result by reference
	}

	// TODO 啥，这个啥意思来着?
	// friends defined inside class body are inline and are hidden from non-ADL lookup
	//
	// TODO 这个可以返回一下
	friend X operator+(
		// TODO 测试下这个说法
		X lhs, // passing lhs by value helps optimize chained a+b+c
		const X &rhs) // otherwise, both parameters may be const references
	{
		lhs += rhs; // reuse compound assignment
		return lhs; // return the result by value (uses move constructor)
	}
};

int main(int argc, char *argv[])
{
	X a, b;
	a = a + b;
	return 0;
}
