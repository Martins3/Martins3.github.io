#include <iostream>
using namespace std;

struct A {
  virtual void foo(int i = 1) { cout << "A::foo" << i << endl; }
};
struct B : public A {
  virtual void foo(int i = 2) { cout << "B::foo" << i << endl; }
};
void test() {
  A a;
  B b;
  A *ap = &b;
  a.foo();
  b.foo();
  ap->foo();
}

int main (int argc, char *argv[]) {
	test();
	return 0;
}

