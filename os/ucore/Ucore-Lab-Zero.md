---
title: ucore lab
date: 2018-04-19 12:14:12
tags:
---
> 你知道喝酒跟喝水的分别吗？酒，越喝越暖，水会越喝越寒

# lab zero

## AT&T assembly syntax review
[ref](http://csiflabs.cs.ucdavis.edu/~ssdavis/50/att-syntax.html)
1. 基本格式
```
nemonic	source, destinatio
```
2. All register names of the IA-32 architecture must be prefixed by a '%' sign
3. All literal values must be prefixed by a '$' sign.
4. 寻址方式
```
segment-override:signed-offset(base,index,scale)
```
5. adding a suffix - b/w/l - to the instruction to specify the data size
6. Branch addressing using registers or memory operands must be prefixed by a `*`. To specify a "far" control tranfers, a 'l' must be prefixed, as in 'ljmp', 'lcall'

## inlineasm
[ref](https://github.com/1184893257/simplelinux/blob/master/inlineasm.md)
1. 基本内联汇编
2. 扩展内联汇编

## C and Object oriented
[如何实现基本面向对象](http://address)](https://www.codementor.io/michaelsafyan/object-oriented-programming-in-c-du1081gw2)
[多态实现](https://stackoverflow.com/questions/351733/can-you-write-object-oriented-code-in-c)

## 双向循环链表


## 总结
1. 前面试验的代码可能对于其后的试验含有不良影响，项目结构中间会说明变化的地方，应该仔细的阅读
