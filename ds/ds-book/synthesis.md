# 分布式
## 1 分布式系统的特征

## 2 分布式系统设计的关注点
1. No clock synchronization algorithm can synchronize a system of n processes to within γ, for any
γ < (max-min)(1-1/n) 

> 找到 theorem 的证明  ? **算法证明**

2. Cristian : 使用外部时钟
> *时钟误差计算*

https://en.wikipedia.org/wiki/Network_Time_Protocol

3. berkeley

4. Lamport

5. 快照

## 3 交互处理

#### Part One

> 可能Pastry有点麻烦

> 两个应用
> 1. *ESM* : end system multicast
> 2. *scribe* 

> RPC 四种处理方法 

> chapter 4 中间分析其中 MOM
> *但是Stream-oriented communication 以及 后面的Qos的内容没有知道*

#### Part Two

排序组播
> 1. 排序为什么和组播关联到一起
> 2. 排序 ?
> Causal Consistency 好像和这个根本没有任何的关系啊!
> 在分析 Lamport 时钟,利息的例子的时候，提到了 total-order multicast


causality

## 4 故障处理

基本内容，内容一致。

*consensus* 问题
> 拜占庭

*checkpoint* 问题

## 5 数据处理

#### Part one
> peer-to-pear 为什么是在数据处理中间的 ?
讲解几个例子 :
> cdk5 : chapter 10 可能不够。

#### Part two
> 基本阅读过，但是理解不深刻。
> 各种一致性的问题

> Raft 是什么
## 6 中间件服务

**Enforcing causal communication**
> 在 vector lock 中间提到
