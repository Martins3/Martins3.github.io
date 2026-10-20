#include <iostream>
struct A {
    int n;
    A() = default;
};

struct B {
    int n;
    B() : n(42) {}
};


int main (int argc, char *argv[]) {

	B b;
	std::cout << b.n  << std::endl;
	return 0;
}
