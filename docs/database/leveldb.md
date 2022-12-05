## leveldb
- [ ] 环境搭建
  - diff 一下
  - LSM tree 是什么?
- [ ] sstable 需要的 bloom filter 的计算 hash 是存储在哪里的

- [ ] skiplist ?
  - [x] 在 leveldb 用来管理什么的？
- cache
  - [ ] shared lru cache

- 使用 LRU 的使用位置
  - https://zhuanlan.zhihu.com/p/149795559
  - table cache
  - LRU cache

- [ ] 日志压缩
  - sstable 的压缩，但是如何读取啊
  - [ ] 如何压缩，如何解压缩的

- [ ] WAL log 相关的代码在什么位置
- [ ] Version 是做什么的


- [ ] snappy 压缩 ?

- [leveldb](https://www.qtmuniao.com/2020/07/03/leveldb-data-structures-skip-list/) :  C++ 数据库 Jeff Dean 1000 行 :star:
    - https://www.bilibili.com/video/BV16B4y1N7Qh


- https://github.com/dermesser/leveldb-rs/tree/master/src

- WriteBatch

## 关键参考
https://www.zhihu.com/column/c_1282795241104465920
