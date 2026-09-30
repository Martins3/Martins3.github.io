// https://en.cppreference.com/w/cpp/language/dependent_name

#include <iostream>

void g(double)
{
	std::cout << "g(double)\n";
}

struct S {
	void f() const
	{
		g(1); // "g" is a non-dependent name, bound now
	}
};

void g(int)
{
	std::cout << "g(int)\n";
}

int main()
{
	g(1); // calls g(int)

	S s;
	s.f(); // calls g(double)
}
