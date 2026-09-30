// import "..." 导入一个 header unit（预编译好的头文件），
// 效果接近 #include，但走的是 module 的编译模型。
import "header-unit.hpp";
#include <iostream>

int main() {
    Point p{3, 4};
    std::cout << manhattan(p) << '\n';
}
