## emplace


##  `lower_bound` and `upper_bound`
the only difference between them are:
**lower\_bound**：Returns an iterator pointing to the first element in the range [first,last) which does not compare less than val.
**upper\_bound**:Returns an iterator pointing to the first element in the range [first,last) which compares greater than val.
通过iter的差值来找到具体的位置
当没有查询到的时候， 返回值为vector的v.end()
一个是第一个大于等于，一个第一个大于, both are the first one, only difference is who is bigger !

```
int bs_upper_bound(int a[], int n, int x) {
    int l = 0;
    int h = n; // Not n - 1
    while (l < h) {
        int mid = (l + h) / 2;
        if (x >= a[mid]) {
            l = mid + 1;
        } else {
            h = mid;
        }
    }
    return l;
}

int bs_lower_bound(int a[], int n, int x) {
    int l = 0;
    int h = n; // Not n - 1
    while (l < h) {
        int mid = (l + h) / 2;
        if (x <= a[mid]) {
            h = mid;
        } else {
            l = mid + 1;
        }
    }
    return l;
}
```

实现`lower_bound` 和　`upper_bound`的策略:

1. 唯一的不同，将等于的情况判定给谁

## 对象排序
### 重载小于号
1. 两个const 一个ref
2.  return <  将会的得到从小到大的排序

### 重载大于号
1. 和重载小于号相同
2. 借助泛型函数 greater<int>()

### 定义比较函数
1. 两个const ref

### 定义比较结构体
1. struct
2. 重载()
3. 利用struct 构造对象，不使用new
4. 建议使用inline
5. 调用：名字+括号

2. 特殊问题
当vector中间保存的是对象的指针和对象， 两者在Compare函数的区别是什么。
lower\_bound也是有效的吗？

## 为map 和 set 自定义比较函数

```
struct lex_compare {
    bool operator() (const int64_t& lhs, const int64_t& rhs) const {
        stringstream s1, s2;
        s1 << lhs;
        s2 << rhs;
        return s1.str() < s2.str();
    }
};

set<int64_t, lex_compare> s;
```

## STL
2. transform

3. erase unique
```
void sortUnique(vec<int> & vec) {
    sort(vec.begin(), vec.end());
    vec.erase(unique(vec.begin(), vec.end()), vec.end());
}
```

3. swap
    1. 如果的使用swap 的对象是局部的对象， 会有问题吗 ？
        void clear( std::queue<int> &q ){
            std::queue<int> empty;
            std::swap( q, empty );
        }
    2. swap 和 assign copy constructor 的关系是什么 ？

4. pair
    1. There is no difference between using make\_pair and explicitly calling the pair constructor with specified type arguments. std::make_pair is more convenient when the types are verbose because a template method has type deduction based on its given parameters.
    2. Aside from the implicit conversion bonus of it, if you didn't use make\_pair you'd have to do
    ```
    one = pair<int,int>(10,20)
    ```
    3. Pair can be assigned, copied and compared. The array of objects allocated in a map or hash\_map are of type ‘pair’ by default in which all the ‘first’ elements are unique keys associated with their ‘second’ value objects.
    4. pair and map:
    ```
    map<int, int> m;
    m.insert(pair<int, int>(1, 2));
    ```
5. copy

## priority\_queue
1. 默认为最大堆
2. std::priority\_queue\<int, std::vector\<int\>, std::greater\<int\>\> my\_min\_heap;
3. 注意: greater上面是没有名字
4. 注意， 排序规则是反过来的， 相对于sort而言。

priority queue 并没有办法删除其中的元素，除非使用使用 set :
https://stackoverflow.com/questions/19467485/how-to-remove-element-not-at-top-from-priority-queue

## set
1. 添加的元素是对象
    1. 对于对象排序

2. 对于 set 中间的元素进行修改的方法:
    1. 删除然后重新插入
    2. 直接修改 : 错误，其中的元素都是 const 的，根本没有办法修改

## vector
1. how to initialize a vector

```
vector<int> a;
vector<int> a(12, 100);
vector<int> a{12, 100};

int arr[] = { 10, 20, 30 };
int n = sizeof(arr) / sizeof(arr[0]);
vector<int> a(arr, arr + n);

vector<int> vect1{ 10, 20, 30 };
vector<int> vect2(vect1.begin(), vect.end());
```
2. reverse

```

std::reverse(myvector.begin(),myvector.end());    // 9 8 7 6 5 4 3 2 1
```

## map
1. 对于key 会实现默认的排序
2. erase 删除可以使用的指针，也可以使用key

```
  it=mymap.find('b');
  mymap.erase (it);                   // erasing by iterator
  mymap.erase ('c');                  // erasing by key
```

## functional
C++11 `std::function` 是一种通用、多态的函数封装，它的实例可以对任何可以调用的目标实体进行存储、复制和调用操作，它也是对 C++中现有的可调用实体的一种类型安全的包裹（相对来说，函数指针的调用不是类型安全的），换句话说，就是函数的容器。
当我们有了函数的容器之后便能够更加方便的将函数、函数指针作为对象进行处理。
```
#include <functional>
#include <iostream>

int foo(int para) {
    return para;
}

int main() {
    // std::function 包装了一个返回值为 int, 参数为 int 的函数
    std::function<int(int)> func = foo;

    int important = 10;
    std::function<int(int)> func2 = [&](int value) -> int {
        return 1+value+important;
    };
    std::cout << func(10) << std::endl;
    std::cout << func2(10) << std::endl;
}
```

使用`bind`
```
int foo(int a, int b, int c) {
    ;
}
int main() {
    // 将参数1,2绑定到函数 foo 上，但是使用 std::placeholders::_1 来对第一个参数进行占位
    auto bindFoo = std::bind(foo, std::placeholders::_1, 1,2);
    // 这时调用 bindFoo 时，只需要提供第一个参数即可
    bindFoo(1);
}
```

## 大数
```
string findSum(string str1, string str2){

    if (str1.length() > str2.length())
        swap(str1, str2);

    // Take an empty string for storing result
    string str = "";

    // Calculate lenght of both string
    int n1 = str1.length(), n2 = str2.length();

    // Reverse both of strings
    reverse(str1.begin(), str1.end());
    reverse(str2.begin(), str2.end());

    int carry = 0;
    for (int i=0; i<n1; i++) {
        // Do school mathematics, compute sum of
        // current digits and carry
        int sum = ((str1[i]-'0')+(str2[i]-'0')+carry);
        str.push_back(sum%10 + '0');

        // Calculate carry for next step
        carry = sum/10;
    }

    // Add remaining digits of larger number
    for (int i=n1; i<n2; i++) {
        int sum = ((str2[i]-'0')+carry);
        str.push_back(sum%10 + '0');
        carry = sum/10;
    }

    // Add remaining carry
    if (carry)
        str.push_back(carry+'0');

    // reverse resultant string
    reverse(str.begin(), str.end());

    return str;
}
```

## 判断一个数字是否为平方数
```
int m=floor(sqrt(n)+0.5);
if(m*m==n)
    return true;
```
