# https://pdos.csail.mit.edu/6.824/labs/lab-raft.html
> https://mr-dai.github.io/raft/ 写的相当不错啊


## 大坑
1. 正确处理 RPC 的方式是如果失败了，那么就丢弃这个消息，尝试构建下一个消息发送，而不是一致尝试
  - 消耗 CPU 过高 (remote server disconnected 之后，就是一个死循环了)
  - 即使 remote server reconnected 之后，还是无法连接上

```go
		if rn.longDelays {
			// let Raft tests check that leader doesn't send
			// RPCs synchronously.
			ms = (rand.Int() % 7000)
```
如果 heartbeat 采用同步的方式发送，如果上一个 heartbeat 由于 delay 没有发送出去，
那么下一个总是无法正确发送。

## Notes in lab instruction
The service expects your implementation to send an ApplyMsg for each newly committed log entry to the `applyCh` channel argument to `Make()`.

## [Raft Q&A](https://thesquareplanet.com/blog/raft-qa/)

## [Students' Guide to Raft](https://thesquareplanet.com/blog/students-guide-to-raft/)
- 没有考虑的问题 : 发送的 RPC 回复的 Term

## [Instructors' Guide to Raft](https://thesquareplanet.com/blog/instructors-guide-to-raft/)

写出来，进行测试，然后下一个。

# Notes
- [ ] 证得前面 4 条性质后，最后一条 State Machine Safety 性质也可证得

- [ ] 可能存在正在发送 vote 的时候，然后收到了 appendEntry

- https://stackoverflow.com/questions/61641052/go-vet-loop-variable-i-captured-by-func-literal

关于 debug 的两个建议 : 将 microseconds 打开 + 自己构建测试样例

- https://stackoverflow.com/questions/49231048/does-the-raft-consensus-protocol-handle-nodes-that-lost-contact-to-the-leader-bu
  - 如果一个 server 只是不能和 leader 通信，那么就会成为无情的 candidate 发送者，然后老是失败

- [x] 什么，为了查找 commit 需要发送 no-op，直接等待下一次即可啊 ?
  - https://stackoverflow.com/questions/49354345/raft-leaders-commit-a-no-op-entry-at-the-beginning-of-their-term
  - 现在测试框架不用这么操作

## 调试过程

## 2B
1. You will need to implement the election restriction (section 5.4.1 in the paper).
2. One way to fail to reach agreement in the early Lab 2B tests is to hold repeated elections even though the leader is alive. Look for bugs in election timer management, or not sending out heartbeats immediately after winning an election.
  - [ ] leader is alive ?
  - [ ] 当 leader is alive 的时候，其他人选举，如何 ? 被驳斥而已 !

## 2C 的思考

- [ ] 2B 的测试 CPU 不科学，是常规的好几倍
```c
➜  raft git:(main) ✗ go test -run 2B -race -cpuprofile cpu.prof
go tool pprof cpu.prof
```
https://golang.org/pkg/runtime/pprof/

- [ ] 上锁的规则应该重新想想，只有一把锁，persisit 不应该使用锁，在 RPC 的时候解除掉锁。

## 重新设计 lock
- [x] 仔细的重新读一遍上锁需知

- [ ] 仔细的阅读一下几个人的 blog

1. 只有在长时间等待的地方才会主动释放锁
2. apply 也是上锁的


## 2D 的记录
1. 评判 far behind ?
When a service falls far behind the leader and must catch up, the service first installs a snapshot and then replays log entries from after the point at which the snapshot was created.

[^1]: https://stackoverflow.com/questions/46376293/what-is-lastapplied-and-matchindex-in-raft-protocol-for-volatile-state-in-server
