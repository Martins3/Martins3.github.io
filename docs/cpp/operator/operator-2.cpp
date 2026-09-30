// User-defined conversion function
//
// 这个类型转换的例子，应该是很自然了
//
// https://en.cppreference.com/w/cpp/language/cast_operator
#include <iostream>
struct X {
	// implicit conversion
	operator int() const
	{
		std::cout << "implicit int()" << std::endl;
		return 7;
	}

	// explicit conversion
	explicit operator int *() const
	{
		std::cout << "explicit int()" << std::endl;
		return nullptr;
	}

	// Error: array operator not allowed in conversion-type-id
	//  operator int(*)[3]() const { return nullptr; }

	using arr_t = int[3];
	operator arr_t *() const
	{
		std::cout << "arr_t * ()" << std::endl;
		return nullptr;
	} // OK if done through typedef
	//  operator arr_t () const; // Error: conversion to array not allowed in any case
};

// 如果是调用的 static_cast ，就是调用显示转换的，所以
int main()
{
	X x;

	int n = static_cast<int>(x); // OK: sets n to 7
	int m = x; // OK: sets m to 7

	int *p = static_cast<int *>(x); // OK: sets p to null
	//  int* q = x; // Error: no implicit conversion

	int(*pa)[3] = x; // OK
}
