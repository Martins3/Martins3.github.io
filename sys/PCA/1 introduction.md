# 1.1 Introduction
[VLSI](https://en.wikipedia.org/wiki/Very_Large_Scale_Integration)Very Large Scale Integration
In particular, the basic issues of locality, bandwidth, latency, and synchronization arise at many levels of the design of parallel computer systems.

A parallel computer is a collection of processing elements that cooperate and communicate to solve large problems fast.

However, this simple definition raises many questions. How large a collection are we talking about? How powerful are the individual processing elements and can the number be increased in a straight-forward manner? How do they cooperate and communicate? How are data transmitted between processors, what sort of interconnection is provided, and what operations are available to sequence the actions carried out on different processors? What are the primitive abstractions that the hardware and software provide to the programmer? And finally, how does it all translate into performance?

## 1.2 Why Parallel Architecture





# 辅助资料

### [MOESI](https://en.wikipedia.org/wiki/MOESI_protocol)
In addition to the four common MESI protocol states, there is a fifth **Owned** state representing data that is both modified and shared.
This avoids the need to write modified data back to main memory before sharing it.
While the data must still be written back eventually, the write-back may be deferred.

Unlike the MESI protocol, a shared cache line may be dirty with respect to memory; if it is, some cache has a copy in the Owned state, and that cache is responsible for eventually updating main memory If no cache hold the line in the Owned state, the memory copy is up to date

五种状态
1. Share: 多个copy, 有权修改
2. Own: 多个copy, 无权修改
3. modified: 只有一个copy, dirty
4. invalid: 无效，相当于不存在
5. Exclusive: 只有一个copy, clean

状态相容图:

![ffff](./img/amd-cache-opteron/moesi.png)

状态转换图:

![ffff](./img/amd-cache-opteron/amd-1.png)


### [MESI](https://en.wikipedia.org/wiki/MESI_protocol)
The MESI protocol is an Invalidate-based which support write-back caches. 

```
那么invalidate-based 是什么意思啊 ？
有什么协议不支持write-back 的吗？
```

四种状态:
1. modified: one copy, dirty
2. exclusive: one copy, clean
3. shared: multiple copy, clean
4. invalid:

状态相容图:
![](./img/amd-cache-opteron/mesi.png)


状态的装换来自两种:
Processor Requests to Cache includes the following operations:
1. PrRd: The processor requests to read a Cache block.
1. PrWr: The processor requests to write a Cache block

Bus side requests are the following:
1. BusRd: Snooped request that indicates there is a read request to a Cache block made by another processor
1. BusRdX: Snooped request that indicates there is a **write request** to a Cache block made by another processor which doesn't already have the block.
1. BusUpgr: Snooped request that indicates that there is a write request to a Cache block made by another processor but that processor already has that Cache block resident in its Cache.
1. Flush: Snooped request that indicates that an entire cache block is written back to the main memory by another processor.
1. FlushOpt: Snooped request that indicates that an entire cache block is posted on the bus in order to supply it to another processor(Cache to Cache transfers).

### [Snooping](https://en.wikipedia.org/wiki/Bus_snooping)
For the snooping mechanism, a **snoop filter** reduces the snooping traffic by maintaining a **plurality** of entries, each representing a cache line that may be owned by one or more nodes. When replacement of one of the entries is required, the snoop filter selects for the replacement the entry representing the cache line or lines owned by the fewest nodes, as determined from a presence vector in each of the entries. A temporal or other type of algorithm is used to refine the selection if more than one cache line is owned by the fewest number of nodes.


There are two kinds of snooping protocols depending on the way to manage a local copy of a write operation: 
1. Write-invalidate: There are two kinds of snooping protocols depending on the way to manage a local copy of a write operation

2. When a processor writes on a shared cache block, all the shared copies of the other caches are updated through bus snooping
[Dragon](https://en.wikipedia.org/wiki/Dragon_protocol)和[Firefly](https://en.wikipedia.org/wiki/Firefly_(cache_coherence_protocol)

Implementation:




Benefit: fast
Drawback: limited scalability, so larger cache coherent NUMA (ccNUMA) systems tend to use directory-based coherence protocols.

Snoop filter:

the cache tag lookup by the snooper is usually an unnecessary work for the cache who does not have the cache block. But the tag lookup disturbs the cache access by a processor and incurs additional power consumption.

A snoop filter determines whether a snooper needs to check its cache tag or not.

A snoop filter is based on a directory based structure and **monitors** all coherent traffics in order to keep track of the coherency states of cache blocks. It means that the snoop filter knows the caches that have a copy of a cache block. Thus it can prevent the caches that do not have the copy of a cache block from making the unnecessary snooping. There are two types of filters depending on the location of the snoop filter. One is a **source filter** that is located at a cache side and performs filtering before coherent traffics reach the shared bus. The other is a **destination filter** that is located at a bus side and blocks unnecessary coherent traffics going out from the shared bus. The snoop filter is also categorized as **inclusive** and **exclusive**. The inclusive snoop filter keeps track of the presence of cache blocks in caches. However, the exclusive snoop filter monitors the absence of cache blocks in caches. In other words, a hit in the inclusive snoop filter means that the corresponding cache block is held by caches. On the other hand, a hit in the exclusive snoop filter means that no cache has the requested cache block.[4]

### [Directory](https://en.wikipedia.org/wiki/Directory-based_coherence)

Full bit vector format, for each possible cache line in memory, a bit is used to track whether every individual processor has that line stored in its cache.

Distributed shared memory (DSM) a.k.a. Non-Uniform Memory Access (NUMA)

A directory node keeps track of the overall state of a cache block in the entire cache system for all processors. It can be in three states :

1. Uncached (U): No processor has data cached, memory up-to-date .
2. Shared (S): one or more processors have data cached, memory up-to-date. In this state directory and sharers have clean copy of the cached block.
3. Exclusive/Modified (EM): one processor (owner) has data cached; memory out-of-date. Note that directory can not distinguish a block cached in an exclusive or modified state at the processor as processors can transition from an exclusive state to modified state with out any bus transaction.
```
我想知道，是不是directory protocal 绝对不会使用broadcast的。
```
