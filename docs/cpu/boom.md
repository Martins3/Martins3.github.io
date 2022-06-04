# BOOM 源码阅读


## 为什么

- TLB
- Cache
  - [ ] 是 nonblocking cache 吗?
- ROB

## 有什么好处
- 理解 likely 和 unlikely
- gcc 中的 cache size 的大小
- 辅助理解多核 CPU 的设计
- [ ] 编译器如何为了乱序多发射模型考虑的，什么叫做 LLVM 中的代价模型
- [ ] LLVM 似乎提供了给体系结构设计人员，需要提供 CPU 的参数信息就可以构建后端的
- [ ] 性能调优，使用 easy perf 中的信息来补充一下

## Question
- [ ] 如何进行地址计算的
- [ ] 如何实现两级 Cache 的
- [ ] cache 岂不是也是一个流水线的

- [ ] 应该看看国内的资料在说什么？
## 为什么

## 准备材料
- 计算机组成原理
- cs152
- R10000
- chisel
- 量化

## 下一步
- 多核 ?
