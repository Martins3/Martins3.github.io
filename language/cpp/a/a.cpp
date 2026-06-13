#include "h.hpp"
#include <iostream>

using namespace std;

class A {
public:
  int a;
  virtual void loop() { cout << "A" << endl; }
  virtual void read() { this->loop(); }
};

class B : public A {
  void loop() { cout << "B" << endl; }
};

int main(int argc, char *argv[]) {
  B *b = new B();
  A *a = b;
  a->read();
  return 0;
}
