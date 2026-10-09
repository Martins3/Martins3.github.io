// 和想象的不一样，构造函数是非常容易构建出来递归死循环的
#include <bits/stdc++.h>
using namespace std;

class A {
    public:
	static int m;
	A()
	{
		std::cout << "default constructor : " << m << std::endl;
		if (m++ > 10)
			return;
		A a;
	}
};

int A::m = 0;
int main(int argc, char *argv[])
{
	A a;
	return 0;
}
