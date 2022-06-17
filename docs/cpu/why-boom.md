# BOOM 源码阅读

- [超标量处理器设计](https://book.douban.com/subject/26293546/) 豆瓣 9.2
- 将小册子也看看

## 其他可以选择的项目
- [XiangShan](https://github.com/OpenXiangShan/XiangShan)
- [玄铁 C910](https://github.com/T-head-Semi/openc910) : 有 30 万行，一时不知道是不是统计错了

## 准备基础 Rocket Chip
- [Rocket chip](https://github.com/chipsalliance/rocket-chip)
- RISC-V 的"Demo"级项目——Rocket-chip : https://zhuanlan.zhihu.com/p/140360043

## 笔记
- BoomFrontendModule : Main Frontend module that connects the icache, TLB, fetch controller, and branch prediction pipeline together.

## 问题
- [ ] 既然采用 icache 和 dcache 是分开的，为什么只是在最上层分开？
- [ ] 存在对于 instruction 和 data 各自的 TLB 吗?
- [ ] chipyard 中如果可以运行 Linux Kernel 的话，那么岂不是也是存在内存控制器的，从 cache 不命中到
- [ ] Boom 中是如何使用状态机的

## 为什么
- TLB
- [ ] queue 是如何实现的?
  - [ ] 如何实现一个 cycle 进入，一个 cycle 出来的 queue 啊
- Cache
  - [ ] 是 nonblocking cache 吗?
    - do not block subsequent accesses to the other cache line
  - [ ] icache 和 dcache 都是如何和 LLC 链接的
- ROB
  - [ ] 到底什么是 ROM，为什么在隐式重命名的时候，推测寄存器的数值保存在其中
- Dispatch
  - 什么叫做 dispatch ?
- LSU
  - 理解一下，为什么 LSU 中如何实现 memory model 的?
    - [ ] 在 Boom 中无需考虑这些事情吗?
  - [ ]  /home/maritns3/hack/chipyard/generators/boom/src/main/scala/lsu/mshrs.scala 是做什么的?
- 总结一下为什么 LSU 非常复杂
  - 因为 out of order 之后，对于内存也是需要对比的，例如对于相同位置的 write 需要保持顺序

## 有什么好处
- 理解 likely 和 unlikely
- gcc 中的 cache size 的大小
- 辅助理解多核 CPU 的设计
- [ ] 编译器如何为了乱序多发射模型考虑的，什么叫做 LLVM 中的代价模型
  - 编译器会进行指令重排 ，循环展开
  - 编译器如何尽量减少 cache miss
- [ ] LLVM 似乎提供了给体系结构设计人员，需要提供 CPU 的参数信息就可以构建后端的
- [ ] 性能调优，使用 easy perf 中的信息来补充一下

## TODO
- 片上总线协议学习(1)——SiFive 的 TileLink 与 ARM 系列总线的概述与对比 : https://zhuanlan.zhihu.com/p/430486422

## Question
- [ ] 如何进行地址计算的
- [ ] 如何实现两级 Cache 的
- [ ] cache 岂不是也是一个流水线的
- [ ] Mixin 是什么东西
- [ ] 找到 page table walker 在什么地方

## [ ] 如何对于各种位置进行调试

- Differences between LazyModule and LazyModuleImp
  - [ ] https://stackoverflow.com/questions/67149769/differences-between-lazymodule-and-lazymoduleimp

## 准备材料
chisel 语言的准备:
- learn x in y minus : 简单的学习一下 scala
- chisel bootcamp : 学习 chisel

CPU 的基本设计原理:
- 计算机组成原理 : 学习静态 5 级流水线的设计
- 量化 : 补充材料
- cs152
- R10000

## 下一步
- 多核 ?
- 尝试回答这个问题: https://stackoverflow.com/questions/69223201/what-is-the-format-of-the-value-in-the-floating-point-register-in-the-riscv-boom
- 阅读软硬件融合

## 阅读材料
- [ ] ZHAO J, KORPAN B, GONZALEZ A, et al. SonicBOOM: The 3rd Generation Berkeley Outof­Order Machine[J], 2020.


[^1]: https://zhuanlan.zhihu.com/p/191660613
