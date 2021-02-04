# [自学分布式](https://www.zhihu.com/question/23645117) of 15-440


## 关键资源
http://www.cs.cmu.edu/~dga/15-440/S14/syllabus.html

Distributed Systems: Principles and Paradigms


## Introduction
3 projects; 1 solo, 2 team + 2 mini-projects
- Distributed (internet-wide) bitcoin miner
- Building Tribbler (or something)
- Choose-your-own with consistent replication or distributed commit/consensus

## 2 Distributed Systems vs. Networks 
• Network links and LANs
> 复习计算机网络 MAC LAN ethernet 相关，bridge 如何填充转发表等

• Layering and protocols

• Internet design 
> IP Hourglass

## 3 Distributed Systems vs. Networks 
> @todo 继续复习计算机网络中间的内容
> 其实对于一个集群，就是在集群内部通信，真的需要使用TCP吗 ?

• Internet design 
• Transport protocols 
• Application design 

## 5 concurrency
> 1. 首先分析使用mutex实现queue, 很难正确高效的处理 deque 的时候，queue empty 的情况，并且引出condition
> 
Go promotes a different view of concurrency, where set up miniature
client/server structures within a single program.  Use "channels" as
mechanism for:
1. Passing information around
2. Synchronizing goroutines (和第一条的区别 ?)
3. Providing pointer to return location (like a "callback") (@todo)

ow about using channel to implement concurrent buffer:
* Acts as FIFO
* Allows concurrent insertion & removal

Shortcomings:
* Size bounded when initialized.  Cannot implement bounded buffer
* No way to test for emptiness.  When read from channel, cannot put back value at head position
* No way to flush
* No way to examine first element ("Front" operation)

Basic point:
* Channels are very low level.
* Most applications require building more structure on top of channels.

> 后面讲解了在 channel 的基础上构建，一共两个例子，也许在试验的时候

## 6 RPC

#### 6.1 RPC overview
- Few lectures ago: Abstractions for communication.
- Last time: Abstractions for computation.

Today, we'll put them together.
What programming abstractions work well to split work among multiple networked computers ? 

RPC goals:
• Ease of programming 
• Hide complexity
• Automates task of implementing distributed computation
• Familiar model for programmers (just make a function call)

But it's not always simple:
- Calling and called procedures run on different machines, with different address spaces
- Must convert to local representation of data
- Machines and network can fail 

RPC `stubs` do the work of marshaling and unmarshaling data 
• But how do they know how to do it? 
• Typically: Write a description of the function  signature using an IDL -- interface definition 
language. 

#### 6.2 RPC challenges
`4` properties of distributed computing that make achieving transparency difficult:
- Partial failures 
- Latency 
- Memory access
> @todo 4 条性质吗 ?

In distributed computing， if a machine fails, part of application fails。 *one cannot tell the difference between a machine failure
and network failure* **How to make partial failures transparent to client ?**
> 两种不同的失败的区别是什么 ?

几种处理的方法 :
1. At-least-once: Just keep retrying on client side until you get a 
response.
    - Server just processes requests as normal, doesn't remember anything. Simple!

2. At-most-once: Server might get same request twice...
    - Must re-send previous reply and not process request (implies: keep cache of handled requests/responses) 
    - Must be able to identify requests
    - Strawman: remember all RPC IDs handled. -> Ugh! Requires infinite memory.
    - Real: Keep sliding window of valid RPC IDs, have client number them sequentiall

https://stackoverflow.com/questions/44204973/difference-between-exactly-once-and-at-least-once-guarantees


Expose RPC properties to client, since you cannot hide them

synchronous and asynchronous RPC

Three classes of agents: 
1. `Request client`. Submits cracking request to server. Waits until server responds.
2. `Worker`. Initially a client. Sends join request to server. Now it should reverse role & become a server. Then it can receive requests from main server to attempt cracking over limited range.
3. `Server`. Orchestrates whole thing. Maintains collection of workers. When receive request from client, split into smaller jobs over limited ranges. Farm these out to workers. When finds bitcoin, or exhausts complete range, respond to request client

1. Define APIs between modules 
- Split application based on function, ease of development, and ease of maintenance
- *Don’t worry whether modules run locally or remotely*
2. Decide what runs locally and remotely 
- Decision may even be at run-time
3. Make APIs bullet proof
- Deal with partial failures 


## 7 Distributed File Systems 1

Design choices and their implications:
- Caching
- Consistency
- Naming
- Authentication and Access Control 


AFS简单介绍 :
- `Cells` correspond to administrative groups.
- Cells are broken into volumes (miniature file systems)
- Client machine has cell-server database 
  - protection server handles authentication 
  - volume location server maps volumes to servers

So, uh, what do we cache? 
- Read-only file data and directory data ! easy 
    - Data written by the client machine ! when is data  written to the server? What happens if the client  machine goes down? 
    - Data that is written by other machines ! how to know  that the data has changed? How to ensure data  consistency? 
    - Is there any pre-fetching? 
- And if we cache... doesnt that risk making things inconsistent? 

Client Caching in NFS v2
- Cache both clean and dirty file data and file attributes
- *File attributes in the client cache expire after 60 seconds* (file data doesn’t expire)
- File data is checked against the modified-time in file attributes (which could be a cached copy)
    - Changes made on one machine can take up to 60 seconds to be reflected on another machine
- Dirty data are buffered on the client machine until file close or up to 30 seconds.
    - If the machine crashes before then, the changes are lost 

Advantage: No network traffic if open/read/write/ close can be done locally. 
- But…. Data consistency guarantee is very poor 
    - Simply unacceptable for some distributed applications 
    - Productivity apps tend to tolerate such loose consistency
- Generally clients do not cache data on local disks 


- Distributed filesystems almost always involve a 
tradeoff: consistency, performance, scalability. 
- We've learned a lot since NFS and AFS (and can 
implement faster, etc.), but the general lesson 
holds. Especially in the wide-area. 
- We'll see a related tradeoff, also involving 
consistency, in a while: the CAP tradeoff. 
Consistency, Availability, Partition-resilience

- Client-side caching is a fundamental technique to 
improve scalability and performance 
    - But raises important questions of cache consistency 
- Timeouts and callbacks are common methods for 
providing (some forms of) consistency. 
- AFS picked close-to-open consistency as a good 
balance of usability (the model seems intuitive to 
users), performance, etc. 
    - AFS authors argued that apps with highly concurrent, shared access, like databases, needed a different  model 

Key to Simple Failure Recovery 
- Try not to keep any state on the server 
- If you must keep some state on the server 
- Understand why and what state the server is keeping 
- Understand the worst case scenario of no state on the  server and see if there are still ways to meet the  correctness goals 
- Revert to this worst case in each combination of failure  cases 


> 问题:
> 2. caching 的策略会导致很多问题，但是ppt 讲解过于迷惑
> 3. 


## 8 Distributed File Systems 2
1. file access consistency
2. name space construction
3. Security in distributed file systems 

> 首先，复习

Other types of DFS:
- Coda – disconnected operation
- LBFS – weakly connected operation 

> 分析Coda

Coda replica control :
1. Pessimistic : Disable all partitioned writes ( Require a client to acquire control of a cached object prior to disconnection)
2. Optimistic : Assuming no others touching the file 

Pessimistic replication control protocols 
guarantee the consistency of replicated in the 
presence of any non-Byzantine failures 
- Typically require a quorum of replicas to allow access 
to the replicated data 
- Would not support *disconnected* mode

Optimistic replica control allows access in 
every disconnected mode 
- Tolerates temporary inconsistencies 
- Promises to detect them later 
- Provides much higher data availability


Puts scalability and availability before 
data consistency 
- Unlike NFS
- Assumes that inconsistent updates are very infrequent 
- Introduced disconnected operation mode and file hoarding

> 接下来讲解 LBFS 的内容，看不懂了

## 9 Time
#### 9.1 Need for time synchronization
> 绝对时间其实没有意义，但是时间相对于顺序不希望被改变



#### 9.2 Time synchronization techniques
Cristian’s Time Sync

The Berkeley Algorithm 

Network Time Protocol 
#### 9.3 Lamport Clocks
Capture just the happens before relationship between events:
- Discard the infinitesimal granularity of time
- Corresponds roughly to causality


Logical time and logical clocks.


#### 9.4 Vector Clocks 
Vector clocks **overcome** the shortcoming of Lamport logical clocks.

## 10 Distributed Mutual Exclusion
Whereas multithreaded systems can use shared memory, we assume that processes can only coordinate message passing.

Requirements:
1. Safety. At most one process holds the lock at a time
2. Fairness. Any process that makes a request must be granted lock
   - Implies that system must be deadlock-free
   - Assumes that no process will hold onto a lock indefinitely
   - Eventual fairness: Waiting process will not be excluded forever
   - Bounded fairness: Waiting process will get lock within some
   bounded number of cycles (typically n)

Other possible goals
 1. Low message overhead
 2. No bottlenecks
 3. Tolerate out-of-order messages
 4. Allow processes to join protocol or to drop out
 5. Tolerate failed processes
 6. Tolerate dropped messages
For today, we will only consider goals 1-3. I.e., assume:

Ricart & Agrawala’s algorithm

Lamport’s Distributed Mutual Exclusion

Alternative organization: Token ri

## 11 Errors and Failures
- Hard errors: The component is dead.
- Soft errors: A signal or bit is wrong, but it doesn’t mean the component must be faulty

> 应该不是很难的那种，东西。

## 12 Fault Tolerance - Detecting and Correcting Local Faults
In the context of today's consistency mechanisms,
for fault tolerance, most systems use a form of logging,
where they write information down *before* operating on it,
to recover from simple failures.  We'll talk about logging and recovery in a few lectures.
> @todo fault 到底指的是什么情况，举个例子? 为什么可以靠日志实现fault tolerance ? 其他的方法还有什么 ?

Database researchers laid the background for reasoning about how to
process a number of concurrent events that update some shared global
state.  They invented the concept of a "transaction" in which a
collection of reads and writes of a global state are bundled such that
they appear to be single, indivisible operation. 

Desirable characteristics of transaction processing captured by acronym ACID.

- Atomicity: Each transaction is either completed in its entirety, or
aborted.  In the latter case, it should have no effect on the global state.

- Consistency: Each transaction preserves a set of invariants about the
global state.  The exact nature of these is system dependent.
*Warning*:  The "C" here makes a great acronym, but the term
"consistency" is used differently in distributed systems when
referring to operations across multiple processors.  Beware this
gotcha -- this "C" is an application or database-specific
idea of maintaining the self-consistency of the data.

- Isolation: Each transaction executes as if it were the only one with
the ability to read and write the global state.

- Durability: Once a transaction has been completed, it cannot be "undone".


This general scheme is known as two-phase locking.
More precisely, as strong strict two-phase locking.

General 2-phase locking
Phase 1.  Acquire or escalate locks (e.g., read lock to write lock)
Phase 2.  Release or deescalate locks

Strict 2-phase locking
During Phase 2.  Release WRITE locks only at end of transactions

Strong strict 2-phase locking
During Phase 2.  Release ALL locks only at end of transactions.  This
is the most common version.  Required to provide ACID properties.

General rule: Always acquire locks according to some consistent global
ordering.

> 可以证明，两段封锁协议永远不会产生dead lock.

Why not always use strong strict 2PL, then?
Because in the general sense, a transaction may not know what locks it needs in advance!

For reliability, typically split transaction into phases:

1. Preparation. Figure out what to do and how it will change state,
   without altering state.  Generate L: Set of locks, and U: List of updates
2. Commit or abort.
   - If everything OK, then update global state.
   - If transaction cannot be completed, leave global state unchanged. In either case, release all locks

**Same general idea, but state spread across multiple servers.  Want to
enable single transaction to read and modify global state and maintain
ACID properties.**

General idea:
1. Client initiates transaction.  Makes use of "coordinator" (could be self). 
2. All relevant servers operate as "participants".
4. The coordinator assigns a unique Transaction ID (TID) for the transaction.
> coordinator ? participants ? and TID ?



* ***Two phase commit***:

Split each transaction into two phases:
1. Prepare & vote.
   - Participants figure out all state changes
   Each determines if it will be able to complete transaction and
   communicates with coordinator
2. Commit.
   - Coordinator broadcasts to participants whether to commit or abort
   If commit, then participants make their respective state changes

Implemented by set of messages between coordinator & participants:

1. 
    - A: Coordinator sends "CanCommit?" query to participants
    - B: Participants respond with "VoteCommit" or "VoteAbort" to coordinator

2. 
    - A: If any participant votes for abort, the entire transaction must be aborted.
     Send "DoAbort" messages to participants.  They release locks.
    - B: Else, send "DoCommit" messages to participants.  They complete transaction

> 这个数据库的ACID存在任何的相似性吗？


Future pointer:  This requires that nodes be able to abort or carry
forward their changes.  This is accomplished with _logging_.
> log，可以将操作撤回，为什么需要对于操作进行撤回啊.

> 锁，在分布式系统中间，已经变成了分布式的锁，但是似乎每一个人还是对于内容进行的 !
What about locking?

Locks held by individual participants
* Acquired at start of preparation process
* Released as part of commit or abort.

Distributed deadlock:
* Possible to get cyclic dependency of locks by transactions across
  multiple servers
* Manifested in 2PC by having one of the participants unable to
  respond to a voting request (because it is still waiting to lock its
  local resources).
* Most often handle with timeout.  Participant times out and then votes
  to abort.  The transaction must then be retried.
  - Eliminates risk deadlock
  - Introduces danger of LIVELOCK: Keep retrying transaction but never succeed

## 13 RAID
> just RAID

## 14 Crashes & Recovery

1. Shadow pages.  When start writing to page, make a copy (called the
"shadow").  All updates on this one.  Retain original copy.
    - Abort: Discard shadow page
    - Commit: Make the shadow page become the real page.  Must then update any pointers to data on this page from other pages.  Requires recursive updating

2. Write-Ahead Logging (WAL): Create log file recording every
operation performed on database.  Consider update to be reliable when
log entry stored on disk.  Keep updated versions of pages in memory
(page cache). When have crash, can recover by replaying log entries to
reconstruct correct state.

View log file as sequence of entries.  Each numbered sequentially with
log sequence number (LSN).  (Typically LSN is the address of the first byte in the entry.)
*Database organized as set of fixed size PAGES,
each of which may have multiple DB records.  Manage storage at page
level.*


Recovery requires passing over log file 3 times:
1. Analysis.  Reconstruct transaction table & dirty page table.  Get
copies of all of these pages at beginning.  Figure out what is
required for other 2 phases.
2. Recovery.  Replay log file forward, making all updates to the dirty
pages.  Bring everything to state that occurred at time of crash.
This involves even executing those transactions that were not yet
committed at time of crash.  (Required to get consistent state of
locks.)
3. Undo.  Replay log file backwards, reverting any changes made by
transactions that had not committed at time of crash.  This is done
using the PrevLSN pointers for the uncommitted transactions.  Once
reach a Begin transaction entry, write an End transaction record to
log file.

Other (in memory) data structures:
- **Transaction table** : Records all transactions that have not yet been
recorded permanently to disk. PrevLSN field to serve as head of list
for that transaction.

- **Dirty Page Table** : List of all pages held in memory that have not yet
been written to disk.

Committing a transaction:
- Not difficult.  Simply make sure log file stored to disk up through commit entry.  Don't need to update actual disk pages, since log file gives instructions on how to bring them up to date.
- Can keep "tail" of log file in memory.  These will contain most recent entries, but none of them are commits.  If have a crash and the tail gets wiped out, then the partially executed transactions will lost, but can still bring system to reliable state.

Aborting a transaction:
- Locate last entry from transaction table.  Undo all updates that have occurred thus far.  Use chain of PrevLSN entries for transaction to revert in-memory pages to their state at beginning of transaction. Might reach point where page stored on disk requires undo operations. Don't change disk pages.  (Come back to this later)

> 整体，感觉和数据库的恢复很类似

## 15 Consistent hashing and name-by-hash 
Two uses of hashing that are becoming wildly popular in distributed systems:
- *Content-based naming* (应该指的重复内容减少存储)
- *Consistent Hashing of various forms*

Many file systems split files into blocks and store each block on a disk.

Hashes used for:
1. Splitting up work/storage in a distributed fashion 
2. Naming objects with self-certifying properties 
    - Key applications 
    - Key-value storage 
    - P2P content lookup 
    - Deduplication 
    - MAC 


## 16 Replication

简单的介绍了primary-backup，然后分析 paxos

## 17 MapReduce
> skip

## 18  Parallel File Systems: HDFS & GFS
> skip

## 20 DNS and CDNs
> skip

## 21 CDN & Peer-to-Peer 
> skip 可能和作业有关的

## 24 Virtual Machines
> 关键

## Byzantine Fault Tolerance
> 关键

## Security Protocols
> to read


## 补充资料

https://tour.golang.org/concurrency/2

## todo
1. 14.go 和 15.go 
2. http://www.cs.cmu.edu/~dga/15-440/S14/lectures/05-concurrency.txt

https://pdos.csail.mit.edu/6.824/schedule.html
https://www.v2ex.com/t/574537
https://github.com/cmu-440-f19/P0


乱七八糟的Lamport时钟内容:
> 在 Lamport 时钟，利用 C 数值的大小关系其实并不能描述其发生的先后关系的
> The problem is that Lamport clocks do not capture **causality**. In practice, causality is captured by means of vector clocks.
> 这他妈的是因果啊 !
> event id : id + local counter 取值
> merge :  
> 关系的比较方法是什么 : 每一个事件都是持有一个历史的，如果其在前面发生，那么历史具有包含关系。
> 利用VC实现 : VCi 数组，i 表示其中的关于该存在的内容，而j表示i对于Pj 上发生的事件的knowledge
> event happend : counter ++
>
> 当进程i发送消息的时候，send msg 将会持有VCi
> 接收到消息，merge : max for each one , 然后刷新数值
> 向量时钟的比较 : 全面压制才可以的

> Note, by the way, that without knowing the actual information contained in messages, it is not possible to state with certainty that there is indeed a causal relationship, or perhaps a conflict.
> 只是推测信息
> vector clock的内容位置 : 317

> checkpoint 内容 : 493

> 非一致割集和一致割集的作用，确定snapshot 状态的定义



乱七八糟的Lamport互斥内容:

> Lamport 互斥算法
> 含有假设 : messages passing FIFO 以及 reliable
> 描述事件先后顺序的方法 : 
> process : 维护一个local request queue，在queue 中间排序为total order
> 三种类型的消息 : request reply 和 release

> 操作方法 :
> 1. 释放资源: 将所有process 的queue 中间的该进程的queue 的内容去掉。
> 2. 获取到锁 : 当自己的request在，
> 3. 试图获取锁 : request 和 reply

> 问题 : 什么是total order ?
> reply 的作用是什么 ?
