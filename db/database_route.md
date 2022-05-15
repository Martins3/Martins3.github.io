- [ ] 首先复习一下单机数据库的 acid 之类的
- 然后看看 tidb 的分布式数据库

- [How Does a Database Work?](https://cstack.github.io/db_tutorial/)
  - 划分为 part 13 分析 SQL 解析和 B tree 之类的操作
- [A More Human Approach To Databases](https://ccorcos.github.io/filing-cabinets/) : 画图的方式描述数据库的执行流程，很短的一篇文章
- [toydb](https://github.com/erikgrinaker/toydb)
  - 又是实现了 rust 的，还是把 mit 的实验搞清楚吧
- [Cmu15445 课程介绍](https://www.qtmuniao.com/2021/02/15/cmu15445-introduction/#more)

- https://15445.courses.cs.cmu.edu/fall2021/
  - https://www.bilibili.com/video/BV1bQ4y1Y7iT

## 两本教科书
- database system concepts
- [数据密集型应用](https://vonng.github.io/ddia/#/part-i) 的中文翻译，可以快速阅读

- Relational Database
- Storage
  - A:
    - different ways to track pages
    - different ways to store pages
    - different ways to store tuples
  - B:
    - log structures : level compaction
- Execution
- Concurrency Control
- Recovery
- Distributed Database

- Query Planning
- Operator Execution
- Access Methods
- Buffer Pool Manager
- Disk Manager

## 高级
- [Non-Volatile Memory Database Management Systems](https://www.morganclaypool.com/doi/10.2200/S00891ED1V01Y201812DTM055)

## 问题
- 什么是时序数据库


## leveldb
- [ ] skiplist ?
  - [ ] 在 leveldb 用来管理什么的？
- cache
  - [ ] shared lru cache

- [ ] 日志压缩

- [leveldb](https://www.qtmuniao.com/2020/07/03/leveldb-data-structures-skip-list/) :  C++ 数据库 Jeff Dean 1000 行 :star:
    - https://medium.com/databasss/on-disk-io-part-1-flavours-of-io-8e1ace1de017 : 数据库教程
    - https://www.bilibili.com/video/BV16B4y1N7Qh
