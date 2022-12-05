#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <stack>
#include <sstream>
#include <climits>
#include <deque>
#include <set>
#include <utility>
#include <queue>
#include <map>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <string>
#include <cassert>

using namespace std;
int foo(){
  int tmp;
  scanf("%d", &tmp);
  return tmp;
}

int a = foo();

void bar(){
  printf("\n%s", "bar is called");
}

void bar2(){
  printf("\n%s", "bar is called");
}


int main(){
  atexit(bar);
  printf("%d\n", a);
  return 0;
}
