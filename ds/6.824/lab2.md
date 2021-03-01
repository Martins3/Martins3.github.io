# https://pdos.csail.mit.edu/6.824/labs/lab-raft.html
> https://mr-dai.github.io/raft/ 写的相当不错啊

## Notes in lab instruction
The service expects your implementation to send an ApplyMsg for each newly committed log entry to the `applyCh` channel argument to `Make()`.

## [Raft Q&A](https://thesquareplanet.com/blog/raft-qa/)

## [Students' Guide to Raft](https://thesquareplanet.com/blog/students-guide-to-raft/)

## [Instructors' Guide to Raft](https://thesquareplanet.com/blog/instructors-guide-to-raft/)


写出来，进行测试，然后下一个。

# Notes
在 Raft 中，节点间通信由 RPC 实现，主要有 RequestVote 和 AppendEntries 两个 RPC API，其中前者由处于选举阶段的 Candidate 发出，而后者由 Leader 发出。

- [ ] committed 和 applied 的区别是什么 ?
  - lastApplied 的代码是什么意思 ?

- [x] term: Leader 的 Term ID 和 leaderId: Leader 的 ID 有什么区别吗 ?
  - term 是用于选举的，term 最大，那么他就是爸爸
  - id : 就是机器的 id

prevLogIndex: 在正在备份的日志记录之前的日志记录的 index 值
prevLogTerm: 在正在备份的日志记录之前的日志记录的 Term ID
> 很有道理，告诉 client 自己的 prevLog 的信息

日志由若干日志记录组成：每条记录中都包含一个需要在状态机上执行的命令，以及其对应的 index 值；除外，日志记录还会记录自己所属的 Term ID。

- [x] 解决该问题的关键在于在备份旧 Term 的日志时也要把当前 Term 最新的日志一并分发出去。
  - 实际上，真正的解决方法是进行判断的增加 term 的比较

- [ ] 证得前面 4 条性质后，最后一条 State Machine Safety 性质也可证得

> - [x] 到底是谁来回复 client ?
>   - 实验，似乎通过 isLeader，而 Raft 算法实际上定义更多

> - [ ] 什么情况下，leader 发送一个 rpc 结果收到的消息是存在 term id 更大的人 ?
> 猜测，比如 leader 和其他的任何人都断了联系，然后存在新的 leader 出现了

> - follower 通过 nextIndex 来描述自己期待 log 是哪一个，但是存在可能性，一个 follower 下线了一段实践，可以导致其严重落后于其他人
> - 一个 leader 每次只是将大多数的备份提醒，那么就可以 commit 了
> - leader crash 之后，如果任何人都是可以成为 candidate 进行选择，会导致 leader 中间的消息落后于其他人
> - leader 在备份日志的时候，可能崩掉了，根据上面的规则，只有持有最新的日志 follower 才可以继续

为了简化算法，Raft 限制了日志记录只会从 Leader 流向 Follower，同时 Leader 绝不会覆写它所保存的日志。

Raft 就能确保被选举为 Leader 的节点必然包含所有已经**提交的日志**。

leaderCommmit: Leader 已经提交的最后一条日志记录的 index 值

> 在这里，是比较的提交日志，对于未提交的日志
> 1. 首先，有的机器上可能看到没有提交的日志，因为 leader 一旦收到大多数 follower 的回复之后，就会可以回复他们说可以 commit
> 2. 通过 AppendEntries RPC 的参数 leaderCommmit, follower 就可以知道自己需要将消息 commit 到哪里了

> 虽然，不知道哪里简化了，只是在 election 的进行规定，就可以保证日志总是从 leader 到 follower 的, 真的很不错

> - [x] 应该是存在将多个 log 同时发送给 follower，然后等待 commit 吧
>    - 是的，为了提升性能


> - [x] 需要大多数投票，但是我怎么知道一共存在多少人, 如果忽然一堆人加入了，如何办 ?

> 每一个机器都持有状态机, 因为人人都可能成为 folloer

Leader 在一个 Term 中只会在一个 index 处创建一条日志记录，而且日志的位置不会发生改变。

> 任何 leader 只会在一个 term 在一个 index 位置只会创建一个记录
> 1. 首先，系统中间只会存在一个 leader, 因为当一个系统 leader 就算是被完成孤立，虽然他没有放弃自己 leader 的身份，但是结果是消息无法 commit 出去
> 2. 一旦在另一个网络分区，新的 leader 形成，成立新的 commit，那么立刻放弃 leader 身份，并且更新其他的 commit 消息过来
> 3. 所以，如果一个网络中间，一个消息 commit 的时候，对应的 term 总是唯一的, 而且只会在一个 index 中间使用
> 4. 其实这也说明一个问题，当 index 相同的时候，一个 follower 的 log 未必相同，因为可能
> 
> - [x] 如果一个 follower 被下线了很长实践，然后忽然接收到 AppendEntries 的时候，需要怎么让 server 继续通知之前没有发送的消息
> - leader 持有了 nextIndex 和 matchIndex :
>   - 通过 matchIndex 可以让 server 消息可以 commit 到哪一个位置了, 大多数人的决定
>   - 发送给 client 从 nextIndex 之后的消息，如果失败就递减，直到成功
> 
> 更新的时候需要提供之前的 index 和 item 数值，如果不匹配，那么拒绝更新
> 这是一个递归证明，因为当其中的数值总是保证的，
> 
> 如果存在两个 item 在同一个 index 的位置，那么至少有一个是没有 commit 的
> 
> 一个 log 无论是否已经 commit 了，知道被 follower 接受，其 item 就一定是被选择出来的 leader。只有当选的 leader ，才可以发送 log
> 系统同时最多一个 leader, 每个 leader 一个 term 标志

若日志中不包含index 值和 Term ID 与 prevLogIndex 和 prevLogTerm 相同的记录，返回 false

> - [x] 消息中间为什么需要携带 term ?
>   - 如果不增加，一个 follower 接受了一个临时 leader 的消息，没有办法区分这个消息是否被清理

> 随着机器数量的增加，server 需要等待的时间变长了，实际上，有必要吗 ?
> 更多的只读，部分是

切换时，Leader 会创建特殊的配置切换日志，并利用先前提到的日志备份机制通知其他节点进行配置切换。对于这种特殊的配置切换日志，节点在接收到时就会立刻切换配置，不会等待日志提交，因此 Leader 会首先进入 C_old_new 配置

> 总的来说，进行两次切换，首先切换到 old_new, 然后切换到 new
> - [ ] 当 old_new 被发送出去，那么什么时候，才可以发送 new 
> - [ ] 在 old_new 的时候可以做什么 ?
>
> 即使是 leader crash 掉了，也需要保证选举出来的 leader 必然在 old_new 中
> 
> 对于状态切换是否成功的标志是什么 ?
> 1. 当然，使用新的配置来判断
> 2. 判断标准如果是所有人都知道了，此时有机器挂了，那么 GG, 这件事永远做不了
> 3. 在当前配置下，大多数机器都是好的，那么就可以了切换成功了，但是此时，部分机器是 old, 部分是 old_new
>   - 但是可以保证，大多数的是 old_new 的状态
>   - 如果 leader 在发送过程中间挂了，新的选出来在哪里无所谓的
>   - 可以进行更新 new 的 leader (称之为在 old_new 装换 commit 掉的) 正式通告前一定是在 old_new 中间的，而且 odd 的状态的机器一定无法选举出来新的 leader
>   - 然后处于 old_new 状态的发起新的装换，k
>
> - 会不会出现，有的机器永远都是处于没有切换的状态，然后一下次成为 leader
>   - 不会的，因为想要成为 leader 首先就要将自己 log commit 掉
> 
> 配置切换为什么不需要提交? 只是为了让切换快点进行吧
> 
> 可能机器根本就不在其中，但是可以假装自己在其中，然后直接挂机即可，因为此时 new 的状态可以构建新的 leader 出来
>
> 被移除的过程 : server 不在发送消息给他们

> 快照的时候 : 
term: Follower 的当前 Term ID。Leader 可根据该值判断是否要降级为 Follower。

> 那么 client 什么时候自己制作自己的 snapshot 啊 ?

仔细想想，那些事情会同时发生 ?
1. 回复 vote
2. 回复 append ? ()
3. 正在faqi  vote

## 编程注意点
1. 但如果 Leader 在日志记录备份至大多数节点之前就崩溃了，后续的 Leader 会尝试继续备份该日志。然而，此时的 Leader 即使在将该日志备份至大多数节点上后都无法立刻得出该日志已提交的结论。

- [ ] 可能存在正在发送 vote 的时候，然后收到了 appendEntry



## 编程记录
- https://stackoverflow.com/questions/47827379/in-raft-leader-election-how-live-leader-response-to-requestvote-rpc-from-a-candi : 从这个回答来看，似乎需要可以自己调用 rpc vote 自己，但是我感觉没必要 ?

处理 network partition:
- [ ] https://stackoverflow.com/questions/45145500/how-do-raft-guarantee-consistency-when-network-partition-occurs

- [ ] 如果啊 server 被隔离，就会不断的 RequestVote ，然后不断的增加 term, 怎么办 ?


## 编程要点
- https://stackoverflow.com/questions/61641052/go-vet-loop-variable-i-captured-by-func-literal


Yes -- a majority is sufficient. It would be a mistake to wait
longer, because some peers might have failed and thus not ever reply.

## 有意思的思考
- https://stackoverflow.com/questions/49231048/does-the-raft-consensus-protocol-handle-nodes-that-lost-contact-to-the-leader-bu
  - 如果一个 server 只是不能和 leader 通信，那么就会成为无情的 candidate 发送者，然后老是失败

- term 没有必要是连续的

- 由于 network partition 的存在，一个小的局域网中间可能会拉起来一个很高的 term
  - 然后网络修复之后，开始广播这个 temr, 导致 leader 变成 follower，虽然你的 term 很高，但是并不能成为 leader, 因为你的 log 不够新
  - 然后系统重新进行选举

- 如果 log 更新，那么 commit 的 log 一定更新
  - 基于日志的两条性质，以及 AppendEntries 的属性，都是针对于 log 的而不是 commit 的 log
  - 比较 log 是不是更新，也不是说非要是 commit 的

- 检测 heartbeat flag 的含义 reset 的含义根本不同:
  - 意味至少睡眠的时间为 Election timeout

## 调试过程
1. 测试三说明其实还是存在 bug 的

2. 存在有的时候找不到 parent 的时候:
```
Test (2A): initial election ...
  ... Passed --   4.0  3   46    5024    0
Test (2A): election after network failure ...
--- FAIL: TestReElection2A (9.99s)
--- FAIL: TestReElection2A (9.99s)
    config.go:386: expected one leader, got none
Test (2A): multiple elections ...
  ... Passed --  11.1  7  720   48968    0
FAIL
exit status 1
FAIL    6.824/raft      25.103s
Test (2A): initial election ...
  ... Passed --   4.0  3   48    5238    0
Test (2A): election after network failure ...
--- FAIL: TestReElection2A (6.54s)
    config.go:386: expected one leader, got none
Test (2A): multiple elections ...
--- FAIL: TestManyElections2A (11.95s)
    config.go:386: expected one leader, got none
FAIL
exit status 1
FAIL    6.824/raft      22.492s

```

```
disconnect(0)
disconnect(1)
disconnect(2)
connect(0)
connect(1)
connect(2)
Test (2A): election after network failure ...
2021/02/22 22:52:51 0 start elections
2021/02/22 22:52:51 1 voted 0
2021/02/22 22:52:51 2 voted 0
disconnect(0)
2021/02/22 22:52:53 2 start elections
2021/02/22 22:52:53 1 voted 2
connect(0)
disconnect(2)
disconnect(0)
2021/02/22 22:52:55 1 start elections
2021/02/22 22:52:55 0 start elections
connect(0)
--- FAIL: TestReElection2A (11.99s)
    config.go:386: expected one leader, got none
```

似乎，我们是经受不起碰撞的

```
disconnect(0)
disconnect(1)
disconnect(2)
connect(0)
connect(1)
connect(2)
Test (2A): hello this is my test ...
2021/02/22 23:40:02.853859 0 start elections
2021/02/22 23:40:02.854167 2 start elections
2021/02/22 23:40:02.856141 1 voted 0
2021/02/22 23:40:02.856187 wait for all the followers
2021/02/22 23:40:02.856514 wait for all the followers
disconnect(0)
2021/02/22 23:40:04.862403 2 start elections
2021/02/22 23:40:04.863979 1 start elections
2021/02/22 23:40:08.159054 wait for all the followers
--- FAIL: TestMy2S (7.49s)
    config.go:386: expected one leader, got none
FAIL
exit status 1
FAIL	6.824/raft	7.510s
```
是的，碰撞会出现问题，而且等待 follower 返回的 timeout 时间很恐怖

关于 debug 的两个建议 : 将 microseconds 打开 + 自己构建测试样例


## 2B
1. You will need to implement the election restriction (section 5.4.1 in the paper).
2. One way to fail to reach agreement in the early Lab 2B tests is to hold repeated elections even though the leader is alive. Look for bugs in election timer management, or not sending out heartbeats immediately after winning an election.
  - [ ] leader is alive ?
  - [ ] 当 leader is alive 的时候，其他人选举，如何 ? 被驳斥而已 !


- [x] log 的开始位置是 1 对应的初始化

- [x] raft 中间，只能顺序的 commit 吗 ?
  - 从代码上，没有必要仔细的考虑
  - 实际上，因为要求保证上一条 log 存在，所以一定保证

- [ ] 切换 leader，统计数量是不可靠的
  - [ ] 让发送新的消息来实现之前的消息自动被处理掉
  - 正确的描述应该是 : 当 leader 在广播一个 term != currentTerm 的 log 时候，并不能得出结论
  - 现在的代码方式是，首先 广播一个 latest 的消息，然后一直等待 latest 的消息确认

- **除非不是 leader, 那么一个 server 的 log 是不可能发生抛弃，丢失，别其他 server 修改, 因为选举算法保证，选出来的就一定持有最新的 log**

- [ ] 当成为一个新的 leader 的时候，我怎么知道 commitIndex 已经到了哪里了 ?

- [x] 在此之前，如何初始化 leader 的 nextIndex 和 matchIndex ?
  - nextIndex 用于实现访问 appendEntry 的构造，最开始的 leader 的 logIndex，让 server 发送最新的 log 出去
  - leader 的 commitIndex 开始的时候，设置为 0, 单调递增的，但是并不是说，非要从 0 开始初始化的
    - commitIndex 的更新法则 : 如果 appendEntry 的时候返回为 true 了
  - 在 appendEntry 的时候，当大多数人都承认了某一个，那么就可以直接开始了
  - 在使用 matchIndex 来更新 leader 的 commitIndex 的

- [x] 好的了，但是我怎么感觉不需要这两个数组 ? [^1]
  - 因为消息是异步的，消息连续到来，根本不用上一个消息是否搞定，就需要发送下一个消息，nextIndex 表示应该发送的消息，如果消息不成功就后退
  - 在正常情况下，commitIndex 和 nextIndex 实际上，没有区别
    - 正常回复即为代表该消息在 client 成功 commit 了
  - 什么时候算作不正常的情况，感觉甚至可以不需要 nextIndex 了
    - 因为 nextIndex 的信息保存无线循环的函数了
    - 对于每一个成功的 log，在数组中间都需要更新一下, 更新那个数组呀 ?
      - 应该是两个同时更新了
      - 如果是 leader crash, commitIndex 瞬间提升过去
    - 应该是当 leader 迟迟没有返回的时候

- [x] 因为发送失败，导致发送需要后退，那么是接着发送所有的 log 的，还是一个个的发送 ?
  - 一起发送，但是如果这个 server 缺失的太少，其实可以存在一些调整策略
  - 只要保证发送的东西，

- [ ] 什么，为了查找 commit 需要发送 no-op，直接等待下一次即可啊 ?
  - https://stackoverflow.com/questions/49354345/raft-leaders-commit-a-no-op-entry-at-the-beginning-of-their-term
  - 但是如果下一次的 entry 迟迟不到来，怎么办法 ?
  - 所以，其实可以构建一个 nil 强行将其发送过去

## 2C 的思考
- raft 保证的是，当 server crash 之后，恢复了，然后系统就可用了

- Updated on stable storage before responding to RPCs
- Raft’s RPCs typically require the recipient to persist information to stable storage

- raft 是可以对抗 disk 也 fail 的，但是该节点是重新加入的方式


为什么需要 persist 状态，或者说，为什么需要一个 server 恢复的时候，保证其 log 是存在的 ?
- votedFor : crash 然后恢复，那么会出现两次投票

- [x] 并不是感觉 log 必须存在
  - 其实不可以，多个 log 消失，那么可能被 commit 的消息永远都是不存在的

- [x] 那么为什么需要保存 currentTerm 的内容 ? 
  - 假设不保存，然后 currentTerm 在恢复之后，内容清空
  - 至少，立刻违背了一个原则, 那就是相同的 term 出现过多个 server
  - 这个会延伸很多的错误出现

- [x] 对于 nextIndex 需要进行优化 ?

还存在的问题

Figure 8 可能需要一个 empty 的消息来实现快速确认。

## 一些测试小程序
1. buffered chan 和想法根本不同，其实是不需要 buffered 的
```go
package main

import (
	"fmt"
	"time"
)
var done = make(chan bool)
var msgs = make(chan int)

func produce() {
	for i := 0; i < 10000000; i++ {
		msgs <- i
	}
}

func consume() {
	for {
		msg := <-msgs
		time.Sleep(100 * time.Millisecond)
		fmt.Println("Consumer: ", msg)
	}
}

func main() {
	go produce()
	go consume()
	for true {
	}
}
```

[^1]: https://stackoverflow.com/questions/46376293/what-is-lastapplied-and-matchindex-in-raft-protocol-for-volatile-state-in-server
