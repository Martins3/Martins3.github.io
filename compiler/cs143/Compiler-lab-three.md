---
title: Compiler Lab Three
date: 2018-04-31 16:34:33
tags: compiler
---
# Chaos
1. 由于没有明确报错的顺序，所以必定使用Check Point

# 阅读文档
1. 可以多遍
2. 只要检语义规则，报告错误
3. checkpoint 用于防止走入到死胡同中间
4. PP3 makes the first two programming projects seem like elementary school math tests
5. 首先确定semantic rules for the decaf language

## Implemention Hints
1. 处理scoping
2. Hashtable map of names to declaration
3.

## semantic analyze
1. type checking, label checking and flow control check
2. Implementing the semantic actions in a tableaction
driven LL(1) parser requires the addition of
a third type of variable to the productions and the
necessary software routines to process it
3. Symbol Tables is the Key data structure during semantic analysis, code generation
4. Generic Type Checking Algorithm
    1.  going down, create any nested symbol table & context needed
    2.  recursively type check child subtree
    3.  on the way back up, check that the children are legal in the context
    of their parents
5. each ast node class defines it's own type check method, which fills in the
   specifics of this recursively Algorithm;

[https://www.tutorialspoint.com/compiler_design/compiler_design_symbol_table.htm]




# 阅读代码

## 注释

    /* pp3: here is where the semantic analyzer is kicked off.
     *      The general idea is perform a tree traversal of the
     *      entire program, examining all constructs for compliance
     *      with the semantic rules.  Each node can have its own way of
     *      checking itself, which makes for a great use of inheritance
     *      and polymorphism in the node classes.
     */
## error
1. 两个辅助private函数
```
  static void UnderlineErrorInLine(const char *line, yyltype *pos);
  static void OutputError(yyltype *loc, string msg);
```
2. 用于实现的其他的函数
3. Each node can have its own way of checking itself, which makes for a great use of inheritance
and polymorphism in the node classes. 清华关于这一个问题分析如下
    1. 建立AST 完成之后，会遍历多次，建立符号表 类型检查 TAC生成
    2. 一种方法是Node节点添加一个抽象method， 然后为所有类添加自己类建立符号表的
       操作

## Visitor模式
    2. Visitor模式支持双重指派的
    1. 具体做法
        2. 对于每一个类A, 都含有的一个抽象方法visitA
        3. 每次遍历的工作都是对应的class都是继承抽象类visitor, 对于visitA进
            行重载，
    3. 具体操作:　cpp实现
    4. BuildSymbol的使用数据放置　形成局部变量
    5. 交叉引用的处理

## 重新理解动态绑定
1. 使用动态绑定首先要求，所有的非终结符可以独立的处理自己的业务

## panic模式的处理

# 阅读语言定义

## 重名的定义是什么
1. NamedType的作用：语义上面只有的identifier, class 和 interface 的类型
2. 数组类型判断需要一致向下进行动态绑定的查找
3. NewArray 是什么 :
    1. 词法分析
    "NewArray"       {return T_NewArray;}
    "[]"   {return T_NewArray;}
    2. is bug ?
4. 使用名称在开作用域中间查询，找到Node * 类型　，首先检查类型
5. 同名符号用屏蔽策略
## expr的类型获取
1. 类型stack 处理
2. LValue的类型(tricky)
3. 只会对于含有运算符的报告类型不一致错误
4. Call的分析　提供返回值类型

所有的Expr全部返回类型，coupundExpr首先检查类型

this的返回，当前的class Type, 用于调用的时候检查类型, 由于实际上调用者是Expr,
This 需要构造类型，

没有指针，　使用类似于Java

**NULL type**　是LAN 和　定义含有不一致的位置




## class关键字
所有的函数都是public的，当调用其他的class的函数的时候，查询第一个, 找到class
decl, 然后从class 的　fundecl 中间找到对应的函数名，最后检查参数列表是否一致

由于variable的访问都是private, "." 的访问需要小心

interface没有继承和implements,　implements 中间只有函数，　所有函数必须实现(当
extends 中间含有interface 中间的函数，处理逻辑是什么)

(interface 中间定义重载函数的处理)

对于继承的处理方法是，首先将父类全部函数声明和变量添加进来，逐步添加的过程中间发
现冲突报错，添加本地的变量和函数申明，最后检查interface 的声明


## new 关键字
1. 检查class Type 是否存在, 也可以是Interface name
2. new Array 的参数检查，必须是整数(负数是否检查)
3. 继承时候的作用域的处理，首先继承的时候，需要将父类的声明copy 下来，从base
   class 中间找到函数，然后逐渐向下查询(如果在extends　的过程中间的找到错误，
   报错顺序)
4.


## this关键字
1. 访问的时候可以实现实现访问当前的class 区间，this-> 一定需要指出class 里面作用
   ，　而不是访问到任何作用域的变量
2. 添加一个局部变量的用于标志哪一个hashTable是class Decl的HashTable
3. 如果该变量的数值为负数，那么表示当前的位置不是class 中间，此时this 关键字禁用
4.



### 函数
1. 参数列表中间的声明
2. Formal parameters are declared in a separate scope from the function’s local
variables (thus, a local variable can shadow a parameter).
    所以，　相当于外部的一个空间
3. A function's return type can be any base, array, or named type. void is used to
indicate the function returns **no value**

class　中间的函数和不是class 中间的函数的区别 不是函数的业务，
this的使用this 关键字自行检查

函数参数列表　调用者自行处理　和 进入到class

函数是一定需要含有一个返回值吗 ? 在if 中间，有的地方有返回值，有的地方没有返回值，如何处理?

出现了返回值，那么返回值的类型必须一致，没有出现就算了

### 理解Stmts
1. Stmts 也是一个简单的程序块，会有局部变量
2. 找不到变量创建者




## hashtable
1. a more easy STL map
2. key string  value any thing, 最好是 pointer
3. 提供几个简单的函数
4. mmap 而不是 map


# 试验思路　ast 然后实现对于各种错误静态分析
1. semantic 的终结任务是检查错误，使用的方法是遍历树
2. 首先检查重名，然后对于每一个decl 逐个分析
3. 处理的问题:
    1. decl : 有无重复
    2. expr : reference correctly !


## 细节
1. 如何遍历
2. 什么时候建立表格
3. 报告什么错误

## 初始版的三个错误
1. 作用域
2. 子类　父类的处理


## 自动化处理
```
:%astyle 格式化文件首先
```

## 梳理一下
1. 变量声明，会添加到开作用于中间，但是变量类型错误　！
2. 只是的不会继续报错，但是需要对于访问需要继续

# 问题存档
1. 语义分析的完成的任务是什么?
2. 语义分析的过程中间需要处理什么事情?
3. 检查program的打印过程， 查看遍历的原则是什么?
4. 是否支持ident的名字　相同
5. 向下分析　?
6. HashTable 如果使用类型都是

7. 检查错误，是首先分析全部
