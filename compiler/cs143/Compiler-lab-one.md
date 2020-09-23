---
title: Compiler lab one
date: 2018-04-29 16:34:33
tags: compiler
---

# pp1

## 阅读文档
1. [decaf](https://web.stanford.edu/class/archive/cs/cs143/cs143.1128/handouts/030%20Decaf%20Specification.pdf) specification
    1. strtol 和 strtod 实现对于int 和　double　获取, 也可以使用c++ stringstream
    2. 不用分析数值的overflow问题
    3. unterminated comment 需要报错
    4. Each single­character operator has a token type equal to its ASCII value, while the multicharacter operators have named token types associated with them
    5. starter只有scanner需要修改
    6. Take care that comment characters inside string literals don't get misidentified as
comments.
    7. If a file ends with an unclosed multi­line comment, report an error via a call
to one of the methods in the ReportError class

2. In order to match our output exactly (which is important for our testing code), please use the standard error messages provided in **errors.h**

3. what to modify
    1. skip over white space;
    2. recognize all keywords and return the correct token from scanner.h;
    3. recognize punctuation and single­char operators and return the ASCII value as
    the token;
    4. recognize two­character operators and return the correct token;
    5. recognize int, double, bool, and string constants, return the correct token and set
    appropriate field of yylval;
    6. recognize identifiers, return the correct token and set appropriate fields of yylval;
    7. record the line number and first and last column in yylloc for all tokens;
    8. and report lexical errors for improper strings, lengthy identifiers, and invalid
    characters

4. panic mode的方式
    1. For each character that cannot be matched to any token pattern, report it and continue parsing with the next character.
    2. If a string erroneously contains a newline, report an error and continue at the beginning of the next line.
    3. If an identifier is longer than the Decaf maximum (31 characters), report the error, truncate the identifier to the first 31 characters (discarding the rest), and continue.

## 阅读框架代码
开始之前一定需要一定阅读一下几个文件，其他的文件是配置文件，没有必要访问
1. error.h/c 定义出现错误的打印函数
2. main.c 定义函数　PrintOneToken　用于处理当正确读入一个Token之后的当前动作的信息
3. location.h 处理位置信息，定义yyltype类型
4. scanner.h 定义了TokenType
5. utility.h/c 试验辅助函数，用于debug
5. scanner.l 需要填写的函数



## 主要部分



## 特别注意的地方
1.  string must start and end on a single line; it cannot be split over multiple lines
2. \* 和 - 对于flex 含有特殊含义
3. String constants do not allow C­style escape sequences•
4. A tab character accounts for 8 columns
5. you need to be sure that your scanner reports the various lexical errors.
6. For those of you using vim as your primary editor, be aware that vim automatically appends a newline to the end of any file that you create
7. 使用diff -w 来比较


## Bugs Log
1. 行号初始化错误
```
    yylloc.first_line = 1;
    yylloc.first_column = 0;
    yylloc.last_line = 0;
    yylloc.last_column = 1;
```
2. 数字中间不包含signed, 但是在double的scientific会包含使用
3. 对于长度超过31的字符串没有正常的报错，但是实际上发现是由于重定向没有处理
   stderr
4. 未处理未识别符号
5. 由于不同的操作系统对于换行符号处理有10 13 或者(10)(13), 所以对于
6. badpre.frag测试出现问题，但是不知道为什么a和; 都是被忽视了
