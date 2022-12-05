# 将芯片设计相关的任何东西都放到这里吧

## 为什么选择 BOOM
- 重新构建一个语言来实现
- 在 scala 的基础上构建，更加有意思

https://developers.google.com/silicon

- https://github.com/T-head-Semi/openc910
  - https://zhuanlan.zhihu.com/p/456409077

- https://github.com/SpinalHDL/SpinalHDL
  - 所以，为什么需要搞出来 SpinalHDL 来啊


## 也许我们应该看看比较现代的架构的设计

- DSU 介绍 https://www.androidauthority.com/arm-dynamiq-need-to-know-770349/
- A76 wikichip https://en.wikichip.org/wiki/arm_holdings/microarchitectures/cortex-a76
- A77 wikichip https://en.wikichip.org/wiki/arm_holdings/microarchitectures/cortex-a77
- A77 介绍 https://www.anandtech.com/show/14384/arm-announces-cortexa77-cpu-ip
- Intel's Sandy Bridge Architecture Exposed https://www.anandtech.com/show/3922/intels-sandy-bridge-architecture-exposed/2
- AMD Zen Microarchitecture https://www.anandtech.com/show/10578/amd-zen-microarchitecture-dual-schedulers-micro-op-cache-memory-hierarchy-revealed
- A78 介绍 https://www.anandtech.com/show/15813/arm-cortex-a78-cortex-x1-cpu-ip-diverging
- A78 wikichip https://en.wikichip.org/wiki/arm_holdings/microarchitectures/cortex-a78
- A78 介绍 https://fuse.wikichip.org/news/3536/arm-unveils-the-cortex-a78-when-less-is-more/
- ARMv9 介绍 https://www.anandtech.com/show/16584/arm-announces-armv9-architecture

## A76
![](https://en.wikichip.org/wiki/File:cortex-a76_block_diagram.svg)

- [ ] 启动有专门的 AGU (Address Generate Unit)

## intel
- https://news.ycombinator.com/item?id=32145324
  - 居然真的有人将 intel 的 opcode 解码出来了

# 超标量处理器设计
- https://book.douban.com/subject/26293546/
