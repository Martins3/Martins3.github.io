#include <bits/stdc++.h>
using namespace std;

class A {
    public:
	int a;
	int b;
};

int main(int argc, char *argv[])
{
	A a = { 1, 2 };
	A a1{ 1, 2 };
	// 不可以这样写
	// A a2 = (1, 2);
	A a3(1, 2);
	return 0;
}
