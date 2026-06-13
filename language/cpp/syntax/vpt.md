```
#include <iostream>
typedef long long int lld;

class Base1 {
public:
  Base1() : base1_1(11) {}
  lld base1_1;
  virtual void base1_fun1() { std::cout << "Base1::base1_fun1()" << std::endl; }
};

class Base2 {
public:
  Base2() : base2_1(21) {}
  lld base2_1;
};

class Base3 {
public:
  Base3() : base3_1(31) {}
  lld base3_1;
  virtual void base3_fun1() { std::cout << "Base3::base3_fun1()" << std::endl; }
};

class Derive1 : public Base1, public Base3, public Base2 {
public:
  Derive1() : derive1_1(11) {}
  lld derive1_1;

  // 当删除下面的函数的时候，正确运行
  // 按照重载函数只是覆盖指针的原理，非继承的函数放到第一个父类的VPT的后面，
  // 这个显然是没有办法解释的
  virtual void base3_fun1() {
    std::cout << "Derive1::base1_fun1()" << std::endl;
  }

  virtual void base1_fun1() {
    std::cout << "Derive1::base1_fun1()" << std::endl;
 }

  virtual void derive1_fun1() {
    std::cout << "Derive1::derive1_fun1()" << std::endl;
  }
};

struct CBase2 {
  lld base2_1;
};

struct CBase1 {
  void **__vfptr;
  lld base1_1;
};

struct CBase1_VFTable {
  void (*base1_fun1)(CBase1 *that);
};

void base1_fun1(CBase1 *that) { std::cout << "base1_fun1()" << std::endl; }

struct CBase3 {
  void **__vfptr;
  lld base3_1;
};

struct CBase3_VFTable {
  void (*base3_fun1)(CBase3 *that);
};

void base3_fun1(CBase3 *that) { std::cout << "base3_fun1()" << std::endl; }

struct CDerive1 {
  CBase1 base1;
  CBase3 base3;
  CBase2 base2;

  lld derive1_1;
};

struct CBase1_CDerive1_VFTable {
  void (*base1_fun1)(CBase1 *that);
  void (*derive1_fun1)(CDerive1 *that);
};

struct CBase3_CDerive1_VFTable {
  void (*base3_fun1)(CDerive1 *that);
  void (*derive1_fun1)(CDerive1 *that);
};

void base3_derive1_fun1(CDerive1 *that) {
  std::cout << "base3_derive1_fun1()" << std::endl;
}

void derive1_fun1(CDerive1 *that) {
  std::cout << "derive1_fun1()" << std::endl;
}

void foo(Base1 *pb1, Base2 *pb2, Base3 *pb3, Derive1 *pd1) {
  std::cout << "Base1::\n"
            << "    pb1->base1_1 = " << std::hex << pb1->base1_1 << "\n"
            << "    pb1->base1_fun1(): ";
  pb1->base1_fun1();

  std::cout << "Base2::\n"
            << "    pb2->base2_1 = " << std::hex << pb2->base2_1 << std::endl;

  std::cout << "Base3::\n"
            << "    pb3->base3_1 = " << std::hex << pb3->base3_1 << "\n"
            << "    pb3->base3_fun1(): ";
  pb3->base3_fun1();

  std::cout << "Derive1::\n"
            << "    pd1->derive1_1 = " << std::hex << pd1->derive1_1 << "\n"
            << "    pd1->derive1_fun1(): ";
  pd1->derive1_fun1();
  std::cout << "    pd1->base3_fun1(): ";
  pd1->base3_fun1();

  std::cout << "over" << std::endl;
}

int main() {
  // CBase1 的虚函数表
  CBase1_VFTable __vftable_base1;
  __vftable_base1.base1_fun1 = base1_fun1;

  // CBase3 的虚函数表
  CBase3_VFTable __vftable_base3;
  __vftable_base3.base3_fun1 = base3_fun1;

  // CDerive1 和 CBase1 共用的虚函数表
  CBase1_CDerive1_VFTable __vftable_base1_derive1;
  __vftable_base1_derive1.base1_fun1 = base1_fun1;
  __vftable_base1_derive1.derive1_fun1 = derive1_fun1;

  CBase3_CDerive1_VFTable __vftable_base3_derive1;
  __vftable_base3_derive1.base3_fun1 = base3_derive1_fun1;
  __vftable_base3_derive1.derive1_fun1 = derive1_fun1;

  CDerive1 d1;
  d1.derive1_1 = 0x44444444;

  d1.base1.base1_1 = 0x11111111;
  d1.base1.__vfptr = reinterpret_cast<void **>(&__vftable_base1_derive1);

  d1.base2.base2_1 = 0x22222222;

  d1.base3.base3_1 = 0x33333333;
  d1.base3.__vfptr = reinterpret_cast<void **>(&__vftable_base3_derive1);

  Derive1 fuck;
  fuck.base1_1 = 0x11111111;
  fuck.base2_1 = 0x22222222;
  fuck.base3_1 = 0x33333333;
  fuck.derive1_1 = 0x44444444;
  char *fuck_p = reinterpret_cast<char *>(&fuck);
  printf("%p\n", fuck_p);

  char *p = reinterpret_cast<char *>(&d1);
  Base1 *pb1 = reinterpret_cast<Base1 *>(p + 0);
  Base2 *pb2 = reinterpret_cast<Base2 *>(p + sizeof(CBase1) + sizeof(CBase3));
  Base3 *pb3 = reinterpret_cast<Base3 *>(p + sizeof(CBase1));
  Derive1 *pd1 = reinterpret_cast<Derive1 *>(p);

  printf("%p %p\n", pd1, &d1);
  printf("%p %p\n", &(pd1->derive1_1), &(d1.derive1_1));
  printf("%lu %lu\n", sizeof(struct CDerive1), sizeof(struct Derive1));
  printf("%lu %lu %lu\n", sizeof(CBase1), sizeof(CBase2), sizeof(CBase3));
  printf("%lu\n", sizeof(void **));

  foo(pb1, pb2, pb3, pd1);

  return 0;
}
```
