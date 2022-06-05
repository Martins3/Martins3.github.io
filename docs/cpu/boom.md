# BOOM 源码阅读


## 为什么

- TLB
- Cache
  - [ ] 是 nonblocking cache 吗?
    - do not block subsequent accesses to the other cache line
- ROB

## 问题
- [ ] 既然采用 icache 和 dcache 是分开的，为什么只是在最上层分开？
- [ ] 存在对于 instruction 和 data 各自的 TLB 吗?
- [ ] load queue / store queue 的功能是什么?
  - [ ] chipyard 中如果可以运行 Linux Kernel 的话，那么岂不是也是存在内存控制器的，从 cache 不命中到

## MIPS R10000 的设计
- stage 1 : 读入 4 条指令
- stage 2 : 对于 4 条指令进行译码，重命名和计算跳转的位置
- stage 3 : 将被重命名的指令放入到队列中，并且读取 busy-bit 来确定那个指令 ready 了

it determines memory address dependencies in the address queue.

## [BOOM 微架构学习——前端与分支预测](https://zhuanlan.zhihu.com/p/379874172)

## [BOOM 微架构学习(4)——指令的执行](https://zhuanlan.zhihu.com/p/259271229)

## [BOOM 微架构学习(3)——ROB 和指令发射](https://zhuanlan.zhihu.com/p/237232261)

## [BOOM 微架构学习(2)——译码单元与寄存器重命名](https://zhuanlan.zhihu.com/p/194075590)

## [BOOM 微架构学习(1)——取指单元与分支预测](https://zhuanlan.zhihu.com/p/168755384)

## [BOOM 微架构学习——详解重排序缓存](https://zhuanlan.zhihu.com/p/412828438)

## [BOOM 微架构学习——详解寄存器重命名技术](https://zhuanlan.zhihu.com/p/399543947)

## 有什么好处
- 理解 likely 和 unlikely
- gcc 中的 cache size 的大小
- 辅助理解多核 CPU 的设计
- [ ] 编译器如何为了乱序多发射模型考虑的，什么叫做 LLVM 中的代价模型
  - 编译器会进行指令重排 ，循环展开
  - 编译器如何尽量减少 cache miss
- [ ] LLVM 似乎提供了给体系结构设计人员，需要提供 CPU 的参数信息就可以构建后端的
- [ ] 性能调优，使用 easy perf 中的信息来补充一下

## Question
- [ ] 如何进行地址计算的
- [ ] 如何实现两级 Cache 的
- [ ] cache 岂不是也是一个流水线的

## 准备材料
- 计算机组成原理
- cs152
- R10000
- chisel
- 量化

## 下一步
- 多核 ?

## 阅读材料
- [ ] ZHAO J, KORPAN B, GONZALEZ A, et al. SonicBOOM: The 3rd Generation Berkeley Outof­Order Machine[J], 2020.
