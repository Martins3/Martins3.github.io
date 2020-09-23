---
title: Modern Cpp
date: 2018-04-19 18:09:37
tags: cpu
---
> 待补充


## [stackoverflow top question](https://github.com/EthsonLiu/stackoverflow-top-cpp)

> https://stackoverflo.com/questions/4172722/what-is-the-rule-of-three : 
> 大多数情况下，你都没有必要自己手写一个管理资源的类，因为 std:: 基本上都给你实现好了。只要避免使用原始指针，那么“三/五法则”你基本上也用不到。（译注：这点可以从智能指针就可以看出来)

问题:
1. 是不是，只有使用 智能指针，就不需要自己手写
2. 5 法则 : move ctor 和 move assignment 是什么 ?

> https://stackoverflow.com/questions/4421706/what-are-the-basic-rules-and-idioms-for-operator-overloading
> - 运算符两边的操作数至少有一个是自定义的类型
> - 和其它函数一样，运算符重载既可作为成员函数，也可作为非成员函数。

> https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom


> https://gcc.gnu.org/onlinedocs/cpp/Predefined-Macros.html
> 


## TODO
我感觉 c++ 足够强，不用使用 : filesystem : https://en.cppreference.com/w/cpp/filesystem
