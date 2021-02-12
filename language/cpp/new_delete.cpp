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

class Apple{
public:
    int num;
    void * operator new (size_t){
        printf("%s\n","why we should use new now that we have malloc" );
        return malloc(sizeof(Apple));
    }

    void * operator new[] (size_t){
        printf("%s\n","They are two different operator" );
        return malloc(sizeof(Apple));
    }

    void operator delete(void *) noexcept{
        printf("%s\n", "In fact i don't know the right to free");
    }

    void operator delete[](void *) noexcept{
        printf("%s\n", "What is the fucking meaning of it !");
    }
};

void A(){
    Apple * a = new Apple[10];
    for (int i = 0; i < 10; i++) {
        a[i].num = i;
    }
    delete [] a;

    Apple * b = new Apple;
    delete b;
}

void B(){
    // int * throw = new int[10000000000000];
    int * no_throw = new(nothrow) int[10000000000000];
    if(no_throw == NULL){
        printf("%s\n", "malloc failed, but no throw");
    }
}


int main(){
    A();
    return 0;
}
