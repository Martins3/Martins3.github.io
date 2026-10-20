#include <bits/stdc++.h>
using namespace std;

struct Base {
	explicit Base(int value)
	{
	}
};

struct Derived : Base {
	Derived()
		: Base(42)
	{
	}
};
int main(int argc, char *argv[])
{
	return 0;
}
