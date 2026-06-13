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
    tuple<int, string> a(10, "fuck");
    size_t len = tuple_size<tuple<int, string> >::value;
    printf("%ld\n", len);
    return 0;
}
