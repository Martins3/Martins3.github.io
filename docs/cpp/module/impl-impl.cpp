// module implementation unit：注意开头是 module（没有 export），
// 表示本文件属于 hello 模块，负责补齐声明里的实现。
//
// 注意：这里没有 #include <iostream>。GCC 在 implementation unit 里
// #include <iostream> 会报 "conflicting declaration of std::type_info"，
// 实现单元里要输出建议用 import <iostream>（header unit）或 import std。
module hello;

int greet() { return 42; }
