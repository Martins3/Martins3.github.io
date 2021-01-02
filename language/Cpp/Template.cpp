#include <algorithm>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstring>
#include <deque>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <stack>
#include <string>
#include <utility>
#include <vector>

using namespace std;

// how to use it !
//      how to define class
//      how to define method and function
//
// The problem it leads to and how to solve it !
//
// How to conflicts with exsiting objects !
//
// If a class has more than one parameter, can template generate template !
//

template <class T> T foo(T a) { return a; }

template <int N> void compare(const char (&a)[N]) {
  printf("the len %d :   %s\n", N, a);
}

template <typename T> class Foo {
  T t;

public:
  // 这两个 typedef 想要表达什么？
  typedef T value_type;
  typedef typename vector<T>::size_type size_type;
  value_type bar();
};

template <typename T> T Foo<T>::bar() { return t; }

// this factorial code is amazing !
template <unsigned int n> struct factorial {
  enum { value = n * factorial<n - 1>::value };
};

template <> struct factorial<0> {
  enum { value = 1 };
};

int main() {
  struct factorial<10> a;
  printf("%d\n", a.value);
  return 0;
}
