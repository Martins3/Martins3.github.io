# cap

[终极解释](https://medium.com/swlh/cap-theorem-in-distributed-systems-edd967e7bdf4)

[CAP 理论常被解释为一种“三选二”定律，这是否是一种误解？](https://www.zhihu.com/topic/21215889/top-answers)
> 综上，CAP应该描述成，当发生网络分区的时候，如果我们要继续服务，那么强一致性和可用性只能2选1。

[JavaGuide : CAP理论](https://github.com/Snailclimb/JavaGuide/blob/master/docs/system-design/distributed-system/CAP%E7%90%86%E8%AE%BA.md)
> 如果系统没有发生“分区”的话，节点间的网络连接通信正常的话，也就不存在 P 了。这个时候，我们就可以同时保证 C 和 A 了。
> 
> 总结：如果系统发生“分区”，我们要考虑选择 CP 还是 AP。如果系统没有发生“分区”的话，我们要思考如何保证 CA 。


[JavaGuide : Base 理论](https://github.com/Snailclimb/JavaGuide/blob/master/docs/system-design/distributed-system/BASE%E7%90%86%E8%AE%BA.md)

> BASE 是 Basically Available（基本可用） 、Soft-state（软状态） 和 Eventually Consistent（最终一致性） 三个短语的缩写。
> 
> 软状态指允许系统中的数据存在中间状态（CAP 理论中的数据不一致），并认为该中间状态的存在不会影响系统的整体可用性，即允许系统在不同节点的数据副本之间进行数据同步的过程存在延时。
> 
> ACID 是数据库事务完整性的理论，CAP 是分布式系统设计理论，BASE 是 CAP 理论中 AP 方案的延伸。
>
> 也就是牺牲数据的一致性来满足系统的高可用性，系统中一部分数据不可用或者不一致时，仍需要保持系统整体“主要可用”。
> 
> BASE 理论本质上是对 CAP 的延伸和补充，更具体地说，是对 CAP 中 AP 方案的一个补充。


