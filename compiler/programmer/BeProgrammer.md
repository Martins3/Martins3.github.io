---
title: 程序员的自我修养阅读笔记
date: 2018-07-19 17:23:07
tags: secret
---
> 这本书看的夹生熟，让我现在变的非常不爽 !

[riots approach anti cheat](https://engineering.riotgames.com/news/riots-approach-anti-cheat)

# 静态链接
Address and storage allocation
symbol resolution
relocation

1. 空间和地址分配
2. 符号解析和重定位

# ELF文件
段 + File Header

# 运行库
1. 获取glibc的[源代码](https://github.com/bminor/glibc)

# question
1. elf中的段表和OS中间的段表的关系是什么?
2. 4..4 指令修正方式没有看的太清楚
3. 强弱符号在链接的时候处理和想象的含有出入 4. elf文件头保存入口地址
4. it's there linker for other language ?
5.

## links
1. [速查手册](https://sourceware.org/binutils/docs/binutils/readelf.html)
2. [setjmp.h](https://en.wikipedia.org/wiki/Setjmp.h)
3. [strtok](https://www.geeksforgeeks.org/strtok-strtok_r-functions-c-examples/)

## TODO
https://gist.github.com/CMCDragonkai/10ab53654b2aa6ce55c11cfc5b2432a4
