// https://stackoverflow.com/questions/610245/where-and-why-do-i-have-to-put-the-template-and-typename-keywords
#include <bits/stdc++.h>
using namespace std;

template <typename T> void f_tmpl()
{
	// 对于这句话，无论如何，必须添加 typename
	// 这个表达式，T::G m 必须是一个声明，
	// 那么这个 typename 是不可以去掉的
	typename T::G m;
}

struct X {
	using G = std::string;
};

struct Y {
	using G = float;
};

int main(int argc, char *argv[])
{

	return 0;
}
