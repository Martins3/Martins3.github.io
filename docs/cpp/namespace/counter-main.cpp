#include "counter.hpp"
#include <iostream>

namespace {
int count = 100; // 与 counter.cpp 中的 count 是不同实体。
}

int main() {
    std::cout << "tutorial::next: " << tutorial::next() << '\n';
    std::cout << "tutorial::next: " << tutorial::next() << '\n';
    std::cout << "main translation unit count: " << count << '\n';
}
