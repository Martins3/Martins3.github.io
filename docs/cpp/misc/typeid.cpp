// # cpp typeid
// <!-- c47f8778-e0b6-4c4c-a88d-691aab5d4d2c -->
//
// typeid 是关键字
#include <bits/stdc++.h>
#include <typeinfo>

auto add(int a, int b) -> int
{
	return a + b;
}

double add2(int a, int b)
{
	return a + b;
}

struct Node  {
	int a;
	int b;
};

void add3(struct Node * m){

}

int main(int argc, char *argv[])
{
	auto i = 5; // i 被推导为 int
	auto arr = new auto(10); // arr 被推导为 int *
	std::cout << typeid(i).name() << std::endl;
	std::cout << typeid(arr).name() << std::endl;
	std::cout << typeid(*arr).name() << std::endl;
	std::cout << typeid(add).name() << std::endl;
	std::cout << typeid(add2).name() << std::endl;
	std::cout << typeid(add3).name() << std::endl;
	delete arr;
	return 0;
}
