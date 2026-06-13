---
title: Compiler lab four
date: 2018-04-29 16:34:33
tags: compiler
---

http://spimsimulator.sourceforge.net/

# 阅读文档
traverse the abstract syntax tree, stringing together the appropriate TAC instructions for each subtree—to assign a variable, call a function, or whatever is needed

Your job is to add **Emit** instructions to the nodes, implemented using the same virtual method
business you used for **Print** and **Check** in previous programming projects.
You can decide how much of your PP3 semantic analysis you want to
--> 的确是制作类似的工具处理

pp3 没有修改的必要

进一步的简化操作
We have removed doubles from the types of Decaf for this project. In the
generated code, we treat bools just like ordinary 4­byte integers, which evaluate
to 0 or not 0. The only strings are string constants, which will be referred to via
pointers. Arrays and objects (variables of class type) are also implemented as
pointers. That means all variables/parameters are 4 bytes in size—that
simplifies calculating offsets and sizes.


Before you begin, go back over the TAC instructions and examples in the TAC
Handout to ensure you have a good grasp on TAC. Also read the comments in
the starter files to familiarize yourself with the CodeGenerator, Tac, and Mips
classes we provide
# 理论知识

There are a number of standard techniques for structuring executable code that are widely used **what**


Compiler is responsible for:
–  Generating code
–  Orchestrating use of the data area
## Management of run-time resources
Correspondence between
–  static (compile-time) and
–  dynamic (run-time) structures
## Storage organization

activation
lifetime of variables

The information needed to manage one procedure activation is called an activation record (**AR**) or frame

重新理解一下stack frame到底说的是什么 ？ 为什么使用Location 和 Instruction 就可
以解释了



## 代码
### MIPS class
用于实现从TAC 到 MIPS源代码的生成


### Tac
location class and instruction clas
Location :

Instruction :
instruction 下面定义的子类到底是用来做什么 ？

似乎缺少关键一步，是如何遍历对应的tree的
Tac 中间多出来的部分就是各种类的定义


### code generation
**codeGen 和 tac 的关系是什么**
The CodeGenerator class has a variety of methods that can be called to create
**TAC instructions and append them to the list so far**. **Each instruction is an object
of one of the subclasses declared in tac.h**. The CodeGenerator supports some
basic instructions, but you will need to augment it to generate instruction
sequences for the fancier operations (array indexing, dynamic dispatch, etc.)

Code generation 中间是链接Tac 中间的类文件而已


## 收集的问题
1. Vtable 创建的位置什么?
    1. 查询类是如何处理
2. 成员函数的放置位置 和 普通函数的放置位置有什么不同?
3. emit specific 和 emit 的关系是什么 ？
4. 为什么需要返回location 来



## 通用思路是什么
遍历的过程中间持续维护的变量是什么

遍历的过程中间， 逐渐将Instruction添加到对应的文件中间

## Plans
1. 星期天之前的完成代码
2. 之后完成组成原理的部分

1. 更加多细节
完成代码迁移，阅读文档


规划一下，到底有几个部分
1. expr
    1. 计算位置， 放入更加多的内容
2. class
3. 跳转语句的处理


## GenCode 辅助代码生成的
1. 无参数的范式代码
    1. 函数返回
2. 有参数的范式代码
    1.





处理的问题 :
    1. Fn 和 call 添加class 的相关处
    2. 分析访问成员变量的
    3. 分析数组的问题
    4. 函数的调用的时候预分配的空间的大小

还是需要处理的问题:
1. 函数调用和 fieldAccess的base 调用可以分析一波
2. this 的问题
3. 函数的局部变量计算比想象的简单



LValue expr.ident 和 ident 的， 什么时候load 和 store 的处理， 被之前处理的过于
复杂

1. 添加其他类型的检查
2. 搞清楚标签的问题
    1. label 的处理是 函数发生跳转的关键
3. 继承的实现
4. 接口的实现


## 函数
stack

fp + 4 : 第一个参数
fp
fp - 4
fp - 8 : 第一个局部变量


1. 怀疑第一个offset含有问题。
2. 为什么first field 是4， class 中间是如何规划的


几个问题:
    1. 偏移量的设置参数的解释
    2. 为什么函数需要计算使用空间的大小
    3. 函数寄存器的使用的使用规定是什么
    4. 框架的源代码




GenCode.c
    1. 如何返回location的
        1. 那些需要返回location的
        2. localOffSet 是如何确定的 ？
    2. Location 类的子类
        1. 为什么需要创建出来这些子类来
        3. sotre的使用位置

为什么需要维护sp指针:
@符号的使用， 为什么

为什么 ! 和 <  != 都是没有实现的 var1 和 var5 中间

没有实现runtime error的检查处理




# 检查出来的bug
1. nameScope 没有任何意义，通过所有的变量检查出来该变量VarDecl
2. 获取类型的代码完全的没有正确性可言
    1. 如果是this 语句的时候
        1. 没有this
        2. 含有this
3. 中间变量真的没有必要的保护吗 ?
    1. 为什么会创建出来中间变量， 但是该的中间变量的没有放置的位置 ？
    2. 函数调用的时候，变量空间并不是满足的，


# 回忆到的问题
1.  为什么Call没有区分store 和 access的区别
    1. 函数返回值 是右值， 所以是没有办法处理
2. 如何访问parent class中间的变量， 如果含有同名的变量， 如何处理。
