#include <bits/stdc++.h>
using namespace std;
int main(int argc, char *argv[])
{
	std::string s = "a12f3fafa";
	try {
		int n = std::stoi(s);
		std::cout << n << std::endl;
		// cpp 中最经典的从 string 获取到整数的方法
		// 如果转换失败，抛出异常，如果我们没有处理异常，最后程序就会直接结束
		// terminate called after throwing an instance of 'std::invalid_argument' what():  stoi
	} catch (const std::exception &e) {
		std::cout << "转换异常 : " << e.what() << std::endl;
	}

	return 0;
}
