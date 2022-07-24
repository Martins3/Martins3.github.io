# 并行程序设计

> 从量化分析方法的中关于并行的总结全部放到这里来。

- [ ] 如果 Guest 和 Host 如何同步？
  - 如果 Guest 对于一个物理内存原子操作，那么 Host 也是无法修改的吗?
    - 会不会在一些边界上制造问题

## 这个 kernel locking 的内容非常老了，应该整理一个类似的内容出来
https://www.kernel.org/doc/htmldocs/kernel-locking/index.html
