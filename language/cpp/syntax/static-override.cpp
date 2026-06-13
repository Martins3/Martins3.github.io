#include <iostream>
using namespace std;
class Father {
public:
  static int age;
  static void get_name() { cout << "father" << endl; }
};

int Father::age = 12;

class Son : public Father {
public:
  static void get_name() { cout << "father" << endl; }
};

class A{
  class B{

  };
};





template <typename A> class GG {
public:
  static void do_something() { printf("hhh"); }
};

class Tester :public GG<int> {};

int main(int argc, const char *argv[]) {
  Son::get_name();
  Tester::do_something();
  printf("%d\n", Son::age);
  return 0;
}

/**
 * 1. 虽然静态方法没有重载,但是子类依旧可以访问
 * 2. 变量也是如此
 * 3. 静态方法和变量不存在动态绑定
 */
