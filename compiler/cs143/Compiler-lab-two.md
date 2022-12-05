---
title: Compiler lab two
date: 2018-04-31 16:34:33
tags: compiler
---

## pp2

### 阅读文档
1. In addition to the language constructs already specified for you, you’ll also extend the grammar to support C­style post­increment and decrement expressions along with switch statements.
2. first concentrate on getting the rules written and conflicts resolved before you add actions
3. To understand the conflicts being reported, scan the generated y.output file that identifies where the difficulties lie


### 阅读源代码
1. error
    1. 定义全部需要的报错函数
    2. 包含scanner使用部分函数
    3. 新增加
    void ReportError::UnderlineErrorInLine(const char *line, yyltype *pos) {
2. praser
    1. 头文件没有定义什么
    2. praser.y
        1. 简单复制到y.tab.c的yyparser()前面
        2. 定义 终结符 和　非终结符的类
        3. 规则定义
        4. 定义复制到k
3. ast系列 定义的非终结符的需要的节点类型


### 从pp1的调整
1. Makefile
2. 添加一个新函数
3. 修改scanner.l

### 修正对应关系
1. 同样是token, 含有类型声明的token需要需要使用class包装一下，否则只是简单的功能
   作用

### 获取所有ast node的构造函数的声明

### 注意事项
1. 使用自己的scanner
```
This function should take in a line number, then hand back the text of the line with that number.
const char* GetLineNumbered(int num);
The error reporting routines will use it to print out really shiny error messages.
```

https://stackoverflow.com/questions/6467166/bison-flex-print-erroneous-line
使用flex&bison中间，　将所有的行首先读入

https://stackoverflow.com/questions/25350337/how-can-i-get-all-character-until-eof-with-input-function-in-flex-lexer

http://dinosaur.compilertools.net/flex/flex_11.html

本来使用yyin, 但是没有初始化。

本来通过DoBeforEachAction的函数，但是发现flex和bison的工作原理和想想的完全不同



2. Be sure you understand how to use symbol locations and attributes in bison:


### 没有找到yylloc
注释

### 解决shift/reduce错误
1. 由于shift/reduce错误导致， parser.y
2. 添加优先级
3. 参考
https://www.gnu.org/software/bison/manual/html_node/Contextual-Precedence.html

### @n的含义
1.
Symbol: @ n
In an action, the location of the n-th symbol of the right-hand side of the rule. See Tracking Locations.
In a grammar, the Bison-generated nonterminal symbol for a mid-rule action with a semantical value. See Mid-Rule Action Translation.

2. @n　使用　$n 的原理相同

3. 分析构造函数中间的含有yylloc的

### 什么时候需要使用Join
<!-- 看一下文档 -->


### 坑
1. 在LAN中间，虽然在FunctionDecl中间包含的ident, 但是对应的实现已经变化成为了
   NameType, 所以应该添加新的非终结符作为划分, 由于FunctionDecl中间含有通配符+,
   identList删除

2. LValue 匹配不一致，而且LValue = ident难以转换的

### 新添加的Error为什么完全没有使用过
1.　在哪一个位置定义的
2.
