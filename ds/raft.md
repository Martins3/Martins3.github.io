# raft
[看看blog](https://mr-dai.github.io/raft/)
这篇论文详细介绍了斯坦福大学研究人员为解决 Paxos 难度过高难以理解而开发出的一个名为 Raft 的分布式共识算法。

我们很容易得出结论：在 Replicated State Machine 中，分布式共识算法的职责就是按照固定的顺序将指定的日志内容备份到集群的其他实例上。

先我们先来说说 Raft 所提供的性质：
- Election Safety（选举安全）：在任意给定的 Term 中，至多一个节点会被选举为 Leader
- Leader Append-Only（Leader 只追加）：Leader 绝不会覆写或删除其所记录的日志，只会追加日志
- Log Matching（日志匹配）：若两份日志在给定 Term 及给定 index 值处有相同的记录，那么两份日志在该位置及之前的所有内容完全一致
- Leader Completeness（Leader 完整性）：若给定日志记录在某一个 Term 中已经被提交（后续会解释何为“提交”），那么后续所有 Term 的 Leader 都将包含该日志记录
- State Machine Safety（状态机安全性）：如果一个服务器在给定 index 值处将某个日志记录应用于其上层状态机，那么其他服务器在该 index 值处都只会应用相同的日志记录

在 Raft 中，节点间通信由 RPC 实现，主要有 RequestVote 和 AppendEntries 两个 RPC API，其中前者由处于选举阶段的 Candidate 发出，而后者由 Leader 发出。

* ***Leader 选举***

Follower 发起选举时会对自己存储的 Term ID 进行自增，并进入 Candidate 状态。随后，它会将自己的一票投给自己，并向其他节点并行地发出 RequestVote RPC 请求。

*COME HREE*
- [ ] That's fucking easy, but we should check this articles words by words 



# https://raft.github.io/

Each server has a state machine and a log. The state machine is the component that we want to make fault-tolerant, such as a hash table.
Each state machine takes as input commands from its log. 
consensus algorithm is used to agree on the commands in the servers' logs.

顺着这个教程走一遍 : http://thesecretlivesofdata.com/raft/

一个 node 存在三种状态：
1. follower
2. leader
3. candidate

* **Log Replication**
- 开始的时候都是处于 follower 的状态
- 如果一个 follower 没有接收到 leader 的消息，那么就可以转换为 candidate
  - [ ] 转换的标准是什么，一段实践没有接收到消息吗 ?
- candidate 可以发送消息给其他的节点说自己想要成为 leader, 然后其他人发送选票给他
- 如果一个 candidate 收到了大多数的选票，那么表示其成为了 leader
  - [ ] 如果多个 candidate 发起 vote, 如何保证一个 follower 只是投票一次
    - 只有在自己没有收到 master 的消息的时候才会 vote 吧
  - [ ] 很有可能所有的节点同时发送消息说自己成为 candidate 了
    - 成为 candidate 之后，还需要 vote 吗?
  - [ ] 如果两个 candidate 的票数恰好相等，如何 ?
  - 投票就是先到先得
  - 如果出现 follower 的在同时死掉了一些，让永远都不存在高于半数的票，如何 ?
- 之后，所有的请求都需要经过 leader
- 请求首先会被放到 leader 的 log 中间，只有 log entry 被 commit 之后，数值才会被修改
- leader 需要将 log entry 发送给 follower
- leader 等待，直到收到超过半数的 follower 的 ack, 才可以将 log entry commit 掉
- 最后，leader 告诉 client 可以 commit 了

* **Leader sleection**
- 存在两个 timeout : election timeout, heartbeat timeout
- election timeout : 一个 follower 在这个时间段里面没有接受到消息，那么其将会成为 candidate, 随机设置为 150ms ~ 300ms
- follower 发送完投票之后，就重置 election timeout
- 选举可能出现两个 candidate 正好平分数据，那么，因为没有获取 majority, 那么等待一段时间可以重新选举。


