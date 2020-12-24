# Linux Programming Interface: Chapter 16 : Extended Attribute

Extended Attribute
allow arbitrary metadata,
in the form of name-value pairs, to be associated with file i-nodes. 

## 16.1 Overview
EAs are used to implement access control lists (Chapter 17) and file capabilities (Chapter 39).

> 文件icon 的实现基础，awesome

An i-node may have multiple associated EAs, in the same namespace or in different
namespaces. The EA names within each namespace are distinct sets. In the user and
trusted namespaces, EA names can be arbitrary strings. In the system namespace,
only names explicitly permitted by the kernel 

> 演示了 setfattr 和 getfattr 的最基本的使用，但是Ubuntu 上可以运行，但是Manjaro 无法运行

## 16.2 Extended Attribute Implementation Details
> 介绍了C语言的调用的函数，最后还提供了一个编程例子
