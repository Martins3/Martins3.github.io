- [ ] 首先复习一下单机数据库的 acid 之类的
- 然后看看 tidb 的分布式数据库

- [How Does a Database Work?](https://cstack.github.io/db_tutorial/)
  - 划分为 part 13 分析 SQL 解析和 B tree 之类的操作
- [A More Human Approach To Databases](https://ccorcos.github.io/filing-cabinets/) : 画图的方式描述数据库的执行流程，很短的一篇文章
- [toydb](https://github.com/erikgrinaker/toydb)
  - 又是实现了 rust 的，还是把 mit 的实验搞清楚吧
- [Cmu15445 课程介绍](https://www.qtmuniao.com/2021/02/15/cmu15445-introduction/#more)
  - https://github.com/risingwavelabs/risingwave : 据说这个更加好用

- https://15445.courses.cs.cmu.edu/fall2021/ : 核心关键，是实现单机数据库理解的基础
  - https://www.bilibili.com/video/BV1bQ4y1Y7iT

- https://github.com/pingcap/awesome-database-learning

## sqlite
- https://www.sqlite.org/docs.html : official doc
- https://news.ycombinator.com/item?id=32250426 : sqlite 的源码分析
- https://fly.io/blog/sqlite-internals-wal/ : How SQLite Scales Read Concurrency
- https://liyafu.com/2022-07-31-sqlite-untold-story/ : sqlite 背后的趣事
- https://github.com/sudeep9/mojo : 分析,学习 sqlite 的小工具 ?
- https://news.ycombinator.com/item?id=32684424 : 比较 duckdb 和 sqlite
- https://news.ycombinator.com/item?id=32675861 : SQLite: Past, Present, and Future
- https://news.ycombinator.com/item?id=32478907 : sqlite is not a toy database
- https://news.ycombinator.com/item?id=32539360
- https://fly.io/blog/
- https://www.codedump.info/post/20220904-weekly-24/

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

## 所有资料的整理
https://github.com/huachaohuang/awesome-dbdev
