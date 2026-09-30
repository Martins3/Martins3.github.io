// auto 的推导规则、参数位置的 auto，以及“什么时候不能用 auto”。
//
// 结论先行：
// 1) auto 的推导规则 == 函数模板实参推导（会丢掉引用和顶层 const）。
// 2) 参数/返回位置的 auto 不是“运行时任意类型”，而是“这里有一个模板形参”。
//    - C++14 起泛型 lambda: [](auto x){...} 的 operator() 是模板。
//    - C++20 起普通函数也可写 void f(auto x)，叫缩写函数模板(abbreviated
//      function template)，等价于 template <class T> void f(T x)。
//      所以“auto 不能用于函数传参”这个旧结论在 C++20 已经不成立。
// 3) 下面“编译期就不让写”的场景是真正不能使用 auto 的地方，见文件末尾清单。

#include <cassert>
#include <string>
#include <type_traits>
#include <vector>

// ---------------------------------------------------------------------------
// 参数位置的 auto 是模板形参
// ---------------------------------------------------------------------------
// C++20 缩写函数模板：等价于 template <class T> void take(T) 的一个隐式模板。
// 含义是“按值接收任意类型”，调用点才实例化，不是动态类型。
void take(auto x) { (void)x; }

// 泛型 lambda(C++14)：operator() 本身是模板。
inline constexpr auto add_one = [](auto x) { return x + 1; };

// ---------------------------------------------------------------------------
// auto 作返回类型时，默认按值推导
// ---------------------------------------------------------------------------
std::string g = "global";
auto           by_value() { return g; }          // std::string（拷贝一份）
auto&          by_ref() { return g; }            // std::string&
decltype(auto) by_decltype() { return (g); }     // 带括号 -> std::string&

// ---------------------------------------------------------------------------
// 代理对象(proxy)：auto 会把“代理”原样收下，这是最经典的坑
// （Effective Modern C++ Item 6：显式类型初始化惯用法）
// ---------------------------------------------------------------------------
struct Matrix {
    struct Proxy {  // 代理，代表“还没算出来的表达式”，而不是 double
        double value;
        friend Proxy operator+(Proxy a, Proxy b) {
            return Proxy{a.value + b.value};
        }
        // 需要时也能隐式转成 double
        operator double() const { return value; }
    };

    Proxy row[4]{{1}, {2}, {3}, {4}};
    Proxy operator[](int i) const { return row[i]; }
};

int main() {
    // (a) auto 丢掉引用和顶层 const —— 规则与模板推导一致
    std::string s = "hi";
    auto  a = s[0];  // char
    auto& b = s[0];  // char&
    static_assert(std::is_same_v<decltype(a), char>);
    static_assert(std::is_same_v<decltype(b), char&>);
    b = 'H';
    assert(s == "Hi");

    const int  ci   = 3;
    int* const cptr = nullptr;
    auto       x1   = ci;    // int：const 被丢掉
    auto       x2   = cptr;  // int*：顶层 const 被丢掉
    static_assert(std::is_same_v<decltype(x1), int>);
    static_assert(std::is_same_v<decltype(x2), int*>);

    // (b) 字符串字面量推成 const char*，不是 std::string
    auto lit = "hello";
    static_assert(std::is_same_v<decltype(lit), const char*>);

    // (c) 代理对象：sum 是 Proxy，不是 double
    Matrix m;
    auto   sum = m[0] + m[1] + m[2] + m[3];
    static_assert(std::is_same_v<decltype(sum), Matrix::Proxy>);
    // 想要 double 就显式写类型：显式类型初始化惯用法
    auto real_sum = static_cast<double>(m[0] + m[1] + m[2] + m[3]);
    static_assert(std::is_same_v<decltype(real_sum), double>);
    assert(real_sum == 10.0);

    // (d) std::vector<bool>::operator[] 也返回代理，不是 bool&
    std::vector<bool> vb{true};
    auto              bit = vb[0];
    static_assert(!std::is_same_v<decltype(bit), bool>);

    // (e) auto 从花括号初始化列表推导，规则很反直觉
    auto il  = {1, 2, 3};  // std::initializer_list<int>
    auto one{1};           // C++17 起：int
    static_assert(std::is_same_v<decltype(il), std::initializer_list<int>>);
    static_assert(std::is_same_v<decltype(one), int>);

    // (f) 数组会退化成指针，auto 存不住“数组类型”
    int  arr[3]{1, 2, 3};
    auto arr_decay = arr;  // int*
    static_assert(std::is_same_v<decltype(arr_decay), int*>);

    // 参数 auto / 泛型 lambda 正常工作
    take(1);
    take(2.0);
    assert(add_one(1) == 2);
    assert(add_one(1.5) == 2.5);
    assert(by_ref() == "global");
    return 0;
}

// ---------------------------------------------------------------------------
// 真的“不能用 auto”的清单（以下每一行都编译不过，附 clang++/g++ 的报错）
// ---------------------------------------------------------------------------
//
// 1. 没有初始化器，无从推导
//      auto x;                       // error: declaration ... requires an initializer
//
// 2. 声明数组 / 用 auto 做数组元素类型
//      auto a[3] = {1, 2, 3};        // error: cannot deduce ... 'auto[3]'
//      auto p = new auto[3];         // error: cannot allocate array of 'auto'
//
// 3. 花括号列表元素个数为 0 或 >1 时不能直接推导
//      auto x{};                     // error: initializer ... is empty
//      auto y{1, 2, 3};              // error: initializer ... multiple expressions
//
// 4. 返回类型推导要求所有 return 语句推出同一类型
//      auto f(bool b) { if (b) return 1; return 2.0; }
//      // error: 'auto' deduced as 'double' here but 'int' earlier
//
// 5. 递归调用时，返回类型必须先推导出来，否则不能调用自己
//      auto fib(int n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }
//      // error: function 'fib' with deduced return type cannot be used
//      //        before it is defined
//    注意：先返回一次、再递归是合法的（此时已经推成 int）：
//      auto ok(int n) { if (n < 2) return n; return ok(n - 1); }
//
// 6. 虚函数不能有推导返回类型
//      struct B { virtual auto f() { return 1; } };
//      // error: function with deduced return type cannot be virtual
//    同理，virtual 也不能用 auto 形参（会变成成员函数模板）：
//      struct B { virtual void g(auto) {} };
//      // error: 'virtual' cannot be specified on member function templates
//
// 7. 非静态数据成员不能是 auto（静态的可以）
//      struct S { auto x = 5; };     // error: 'auto' not allowed in non-static
//      struct T { static inline auto x = 5; };  // OK
//
// 8. auto 不能出现在模板实参里
//      std::vector<auto> v;          // error: 'auto' not allowed in template argument
//
// 9. 不能从重载集/函数模板名推导函数指针
//      auto f = std::max;            // error: incompatible initializer of type
//                                    //        '<overloaded function type>'
//
// 10. 不能给“无目标类型”的位域绑定引用
//      struct S { int b : 3; }; S s; auto& r = s.b;
//      // error: non-const reference cannot bind to bit-field
//
// 11. 类型别名、catch、sizeof、基类等位置不允许
//      using T = auto;               // error: 'auto' not allowed in type alias
//      catch (auto e) {}             // error: 'auto' not allowed in exception decl
//      sizeof(auto);                 // error: 'auto' not allowed here
//      struct D : auto {};           // error: expected class name
//
// 12. main 的返回类型必须是 int
//      auto main() { return 0; }     // error: 'main' must return 'int'
//
// 历史版本限制（现代编译器已放开）：
//      void f(auto);                 // C++17 及以前: error: 'auto' not allowed
//                                    // in function prototype；C++20 OK
//      [](auto x){...}               // C++11 不行，C++14 起为泛型 lambda
