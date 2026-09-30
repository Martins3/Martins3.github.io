// 这是一个普通头文件，用于演示 header unit：
// 可以用 import "header-unit.hpp"; 而不是 #include "header-unit.hpp"。
#pragma once

struct Point {
    int x;
    int y;
};

inline int manhattan(const Point& p) { return p.x + p.y; }
