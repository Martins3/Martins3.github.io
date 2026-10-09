#include <iostream>
#include <utility>

namespace geometry {
struct Point {
    int x;
    int y;

    // 非成员 hidden friend，可以由 ADL 找到。
    friend bool operator==(const Point& a, const Point& b) {
        return a.x == b.x && a.y == b.y;
    }
};

int sum(Point p) { return p.x + p.y; }

void swap(Point& a, Point& b) noexcept {
    std::cout << "geometry::swap\n";
    std::swap(a.x, b.x);
    std::swap(a.y, b.y);
}
}

template <class T>
void exchange(T& a, T& b) {
    using std::swap;
    swap(a, b); // 默认候选 + ADL；Point 会选择非模板的 geometry::swap。
}

int main() {
    geometry::Point a{3, 4};
    geometry::Point b{8, 9};
    std::cout << "sum via ADL: " << sum(a) << '\n';
    std::cout << std::boolalpha << "hidden friend equality: "
              << (a == geometry::Point{3, 4}) << '\n';
    exchange(a, b);
    std::cout << "after ADL swap: " << a.x << ',' << a.y << " / "
              << b.x << ',' << b.y << '\n';
    std::swap(a, b); // 限定调用，不会调用 geometry::swap。
    std::cout << "after std::swap: " << a.x << ',' << a.y << " / "
              << b.x << ',' << b.y << '\n';
    int x = 1;
    int y = 2;
    exchange(x, y); // 基本类型没有关联命名空间，使用 std::swap。
    std::cout << "int swap: " << x << " / " << y << '\n';
}
