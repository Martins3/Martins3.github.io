# emplace_back

`emplace_back` 的核心只有一句话：

> 把构造对象所需的参数交给容器，让容器直接在尾部存储空间中构造对象。

假设有一个 `Person` 类型：

```cpp
#include <string>
#include <utility>
#include <vector>

struct Person {
    Person(std::string name, int age)
        : name(std::move(name)), age(age)
    {
    }

    std::string name;
    int age;
};

std::vector<Person> people;
```

## `push_back` 与 `emplace_back`

使用 `push_back` 时，传入的是一个已经构造好的 `Person` 对象：

```cpp
people.push_back(Person{"Alice", 20});
```

从语言语义上看，这个过程是：

1. 先在容器外构造临时的 `Person`。
2. 再把临时对象移动到 `vector` 尾部。
3. 销毁临时对象。

使用 `emplace_back` 时，传入的是构造 `Person` 所需的参数：

```cpp
people.emplace_back("Alice", 20);
```

容器把参数转发给 `Person` 的构造函数。其核心过程可以近似理解为：

```cpp
// 伪代码：在 vector 已经准备好的尾部内存上构造对象
::new (address) Person("Alice", 20);
```

因此可以这样记：

```cpp
push_back(对象);
emplace_back(构造对象的参数...);
```

## 日常应该如何选择

手中已经有对象时，使用 `push_back` 最自然：

```cpp
Person alice{"Alice", 20};

people.push_back(alice);            // 拷贝 alice
people.push_back(std::move(alice)); // 移动 alice
```

希望根据构造参数创建一个新对象时，使用 `emplace_back`：

```cpp
people.emplace_back("Bob", 30);
```

不要机械地把所有 `push_back` 都换成 `emplace_back`。下面两种写法通常都很合理：

```cpp
people.push_back(Person{"Carol", 40});
people.emplace_back("Carol", 40);
```

第一种会产生一个临时对象，但现代 C++ 中移动通常很便宜，有时编译器还可以进行优化。只有在这次额外移动的成本确实重要时，原地构造的性能优势才格外明显。

## `emplace_back` 只是转发构造参数

`emplace_back` 不会猜测程序员的意图。它只是寻找能够接收这些参数的构造函数并调用它。

例如：

```cpp
std::vector<std::vector<int>> data;
data.emplace_back(10, 3);
```

这相当于构造：

```cpp
std::vector<int>(10, 3);
```

结果是向 `data` 中加入一个包含 10 个 `3` 的 `vector`，而不是加入数字 `10` 和 `3`。

## 它可以调用 `explicit` 构造函数

```cpp
struct Number {
    explicit Number(int value) : value(value) {}

    int value;
};

std::vector<Number> numbers;

numbers.emplace_back(42);       // 可以：直接调用 Number(int)
// numbers.push_back(42);       // 错误：不允许从 int 隐式转换
numbers.push_back(Number{42});  // 可以：显式构造 Number
```

这让 `emplace_back` 很方便，但也意味着它有时会接受 `push_back` 拒绝的代码。调用处是否清晰仍然比少写一个类型名更重要。

## 原地构造不等于永远没有移动

`emplace_back` 直接构造的是新加入的元素。如果 `vector` 容量不足，容器仍然需要申请一块更大的内存，并把已有元素移动或拷贝过去：

```cpp
std::vector<Person> people;
people.reserve(100); // 预留空间，前 100 次插入不会因为容量不足而扩容

people.emplace_back("Alice", 20);
people.emplace_back("Bob", 30);
```

所以 `emplace_back` 避免的是“先在容器外构造新元素，再把新元素移进去”，并不能消除 `vector` 扩容导致的已有元素搬迁。

## 返回值和引用失效

从 C++17 开始，`emplace_back` 返回刚插入元素的引用：

```cpp
Person& alice = people.emplace_back("Alice", 20);
alice.age = 21;
```

但之后再次插入元素时，`vector` 可能扩容，使之前取得的指针、引用和迭代器失效：

```cpp
Person& alice = people.emplace_back("Alice", 20);
people.emplace_back("Bob", 30); // 可能触发扩容

// 如果发生过扩容，alice 已经失效，不能再使用
```

这不是 `emplace_back` 独有的问题，`push_back` 也遵守同样的 `vector` 引用失效规则。

既然引用会在下次扩容时失效，`emplace_back` 的返回值该怎么用？答案是：把它当作“刚插入元素的一次性别名”，只在它失效之前使用。常见的有三种姿势：

1. 提前 `reserve`，让后续插入不触发扩容，引用就一直有效：

```cpp
std::vector<Person> people;
people.reserve(2);

Person& alice = people.emplace_back("Alice", 20);
people.emplace_back("Bob", 30); // 容量足够，不会扩容

alice.age = 21; // alice 仍然有效
```

2. 拿到引用后立即使用，不跨过下一次插入：

```cpp
people.emplace_back("Alice", 20).age = 21; // 在同一个表达式里直接改
```

3. 只把引用当作“刚插入的这个元素”来用，用完就丢：

```cpp
Person& newest = people.emplace_back("Alice", 20);
doSomethingWith(newest);
people.emplace_back("Bob", 30); // 从这里起不再使用 newest
```

如果确实需要长期引用某个元素，存它的下标而不是引用——扩容只会让引用、指针、迭代器失效，下标始终有效：

```cpp
std::size_t idx = people.size();
people.emplace_back("Alice", 20);
// ... 之后
people[idx].age = 21; // 下标不受扩容影响
```

一句话：返回值是“就地操作刚插入元素”的便利设施，不是可以长期持有的句柄。

## 最终心智模型

```cpp
// 我已经有一个对象，要把它放进容器
container.push_back(object);

// 我有构造对象所需的参数，要在容器中创建它
container.emplace_back(constructor_args...);
```

`emplace_back` 的重点是“在哪里构造”，不是“它永远比 `push_back` 快”。

## 什么时候 `emplace_back` 明显更好

### 1. 对象不可拷贝、不可移动

这是最硬性的理由：有些对象只能在容器内直接构造，`push_back` 根本编不过。

```cpp
#include <list>
#include <mutex>

struct Counter {
    std::mutex mtx; // std::mutex 不可拷贝、不可移动
    int value = 0;

    Counter() = default;
    Counter(const Counter&) = delete;
    Counter(Counter&&) = delete;
};

std::list<Counter> counters;
counters.emplace_back();                  // 可以：原地构造
// counters.push_back(Counter{});         // 错误：Counter 不可移动
```

注意这里用的是 `std::list` 而不是 `std::vector`：`vector` 是连续存储，扩容时必须把已有元素整体搬迁，因此元素必须可移动或可拷贝，不可移动的 `Counter` 放不进 `vector`（`vector` 的 `emplace_back` 在编译期就会实例化搬迁路径，即使 `reserve` 足够也编不过）。`std::list`/`std::deque` 这类不搬迁已有元素的容器才能直接存放不可移动对象；如果坚持要用 `vector` 的连续语义，可以存 `std::vector<std::unique_ptr<Counter>>`。

### 2. 移动本身很昂贵

`push_back(对象)` 与 `emplace_back(参数)` 的差别，本质是“多一次移动”。如果类型的移动很便宜（比如 `std::string`、`std::unique_ptr` 这类只是偷指针的类型），两者几乎没有差距；但如果是内含大块内联缓冲区的值类型，移动等价于把整个缓冲区再拷贝一遍，此时省掉这次移动就很有价值。

下面用一个“构造很廉价、移动很昂贵”的类型做基准测试，把这次移动的成本放大到肉眼可见：

```cpp
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <vector>

// 模拟内含大块内联缓冲区的值类型：
// 构造只写一个字节，而移动需要把整个数组拷贝一遍。
struct Block {
    std::array<char, 8192> data;

    Block() { data[0] = 'x'; }

    // noinline 只是防止编译器把这次移动优化掉，让差异真实可见（GCC/Clang）
    __attribute__((noinline)) Block(Block&& other) noexcept {
        data = other.data;
    }

    Block(const Block&) = delete;
    Block& operator=(const Block&) = delete;
    Block& operator=(Block&&) = delete;
};

constexpr std::size_t kCount = 500'000;

volatile char g_sink;

double push_back_case() {
    std::vector<Block> v;
    v.reserve(kCount);
    auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < kCount; ++i) {
        v.push_back(Block{}); // 构造临时对象 + 移动（拷贝 8192 字节）
    }
    auto end = std::chrono::steady_clock::now();
    g_sink = v.back().data[0];
    return std::chrono::duration<double, std::milli>(end - start).count();
}

double emplace_back_case() {
    std::vector<Block> v;
    v.reserve(kCount);
    auto start = std::chrono::steady_clock::now();
    for (std::size_t i = 0; i < kCount; ++i) {
        v.emplace_back(); // 只原地构造，没有那次移动
    }
    auto end = std::chrono::steady_clock::now();
    g_sink = v.back().data[0];
    return std::chrono::duration<double, std::milli>(end - start).count();
}

int main() {
    push_back_case(); // 预热，排除冷缓存影响
    emplace_back_case();

    double a = push_back_case();
    double b = emplace_back_case();
    std::printf("push_back   : %8.2f ms\n", a);
    std::printf("emplace_back: %8.2f ms\n", b);
    std::printf("ratio       : %.2fx\n", a / b);
}
```

在本机（GCC 15.2，`-O2`，C++17）的一次运行结果：

```text
push_back   :  1192.23 ms
emplace_back:   487.45 ms
ratio       : 2.45x
```

绝对数字随机器和编译器而异，但方向是稳定的：当移动的代价不可忽略时，`emplace_back` 省掉的那次移动就是实打实的收益。
反过来，对于移动只是偷指针的类型，`push_back` 与 `emplace_back` 通常看不出差别，此时更该按可读性选。

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
