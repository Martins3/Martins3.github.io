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
#include <unordered_set>
#include <unordered_map>

using namespace std;

int main(){
  vector<int> x;
  x.push_back(1);
  auto i = x.begin();
  for (; i < x.begin() + 10000; ++i) {
    cout << *i << " ";
  }
  cout << x.capacity() << endl;
  return 0;
}
