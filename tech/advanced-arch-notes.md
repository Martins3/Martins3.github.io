1. 讲过寄存器 ?
2. 引言部分讲解过的 : Dennard Scalling, 图灵机，哈弗结构
3. 单核处理器设计基础 : 
4. pseudo-associative caches and skew-associative caches

flash: 读写擦，写只能是从 1 到 0, 擦就是将block 的数据都是写为 1。写的单位是 page, 擦除的单位是 block。擦除的速度相对慢。


ISA
Uarch(underline architecture)

design point
1. cost
2. performance
3. maximum power consumption
4. energy consumption
5. availability
6. reliability and correctness
7. time to market 


What are the elements of An ISA:
1. Instruction sequencing model
2. Instruction processing style.
    1. 0-address  : stack machine 分析过
    2. 1-address : accumulator
    3. 2-address : X86
    4. 3-address : MIPS
3. instruction :
    1. opcode
    2. operand specifier(addressing modes)
4. data types
    1. think compiler/programmer vs micro architecture
    2. concept of semantic gap

5. memory organization
    1. address space
    2. addressability : 一般是 bute addressable
    3. virtual memory

6. register
    1. how many
    2. size of each register

7. how to interface with IO device  : in/out 还是 io memory
8. privilege modes 
8. exception and interrupt handling
9. virtual memory
10. access protection

Orthogonal ISA : 寻址模式 * 操作 * 数据类型

Advantage of Complex instruction: 
1. denser encoding => smaller code size => better memory utilization, saves off-chip bandwidth, better cache hit rate
2. simpler compiler, no need to optimize small instruction as much

Disadvantage of complex instruction:
1. large chunks of work -> compiler has less opportunity to optimize(limited in fine-grained optimization it can do)
2. more complex hardware -> translation from a high level to control signals and optimization needs to be done by hardware.

semantic gap : 使用 X86 的字符串指令


RISC motivated by:
1. 2-8 principal
2. memory stall : 减少访存指令
3. simplifying the hardware
4. enabling the compiler to optimize the code better.

其实指令集的设计反映了硬件限制，比如
1. limited on-chip and off-chip memory
2. limited compiler optimization
3. limited memory bandwidth
4. need for specialization in important application


指令编码:
uniform decode and variable length


对其访问: 一般来说，RISC 不支持非对其访问，而X86为所欲为。


availability 和 reliability
1. 定义
2. principal of high-availability
3. what are some specific design optimization ?


MTTF : mean time to failure
MTTR : mean time to repairt
MTBF : mean time between failure = MTTF + MTTR 
Failure in time = failures per billion hours of operation = 10^9 / MMF

steady state availability = MTTF / (MTTF + MTTR)

各种故障分类。
disk : raid
memory : ECC chipkill
network : tcp
data center : 

## 9 Performance Engineering
many ways, many tools and the correct way to evaluation !

1. measurement
2. analysis
3. improvment
4. repeat

evaluation choices:
1. Real experiment
2. experiment using analytical models
3. experiments using a simulator

Simulation:
1. full system or user level
2. functional or timing (只是功能，或者对于时序进行模拟)
3. emulation-based simulator or instrumentation-based simulator
4. trace/event-driven simulartor or execution-driven simulator

randomness ?

中心极限定理

置信区间

## 10 功耗管理
1. power metrics
2. power comsumption in ICs
3. Typocal Power management
4. DC: Power Energy and Cooling

P = C * Vdd^2 * F (动态功耗) + Tsc * Vdd * Lpeak * F (短路电流功耗，和尺寸相关) + Vdd * Ileak (静态功耗,漏电流功耗)

power optimization:
1. circuits
2. architecture
3. compiler or app

clock gating 和 power gating

动态调频调压

LPDDR2

DC 的耗电

## 11 On chip Network : topology and flow control
1. topology : routing distance, diameter, average distance, 
2. flow control : 分片 ? 缓冲 ?
    1. buffer
        1. circuit switching : 建立路径，然后才使用 request acknowledgement use deallocation
        2. dropping : 当发生竞争的时候，随机丢掉一个
        3. misrouting : livelock
    2. bufferless:
        1. store-and-forward : 等整个包过来，产生延迟
        2. *virtual cur-through* : 还是按照包为单位。
        3. *wormhole* : ???
        4. *virtual-channel* : ???
3. router mircoarchitecture
4. routing

## 11 On chip Network : router and route algorithm
what's in a router ?
1. logic : state machine, arbiters, allocator
2. memory : buffer
3. communication : switchs


使用 virual-channel router 作为例子: 分析流水线的构建和优化


properties of routing algorithm:
1. deterministic /oblivious
2. adaptive
3. minimal
4. deadlock-free

转弯模型 => cdg 是否存在环 

或者增加更加多的资源，VC (virtual channel)

randomized routing : valiant，首先选择一个随机节点作为中间节点，可以限制中间节点的选择范围进行优化。

> *有一说一，virtual channel 不是很理解*
https://ris.utwente.nl/ws/portalfiles/portal/5127831/00000102.pdf


## 12 cache coherence
似乎有需要把那个论文掏出来看看

communication model:
1. shared memory
2. message passing

A read should return the most recently written value:

Coherence : what values can a read return ?
concerns read/write to a single memory location

Consistency : when do writes become visible to reads ?
concerns reads/writes to multiple memory location 

A cache coherence protocal controls cache contents to avoid stale cache line

coherence protocals must enforce two rules:
1. write propagation : writes eventually become visible to all processors
2. **write serialization** :writes to the same location are serialized(all the processor see them in the same order)
> 为什么需要这个要求，是不是太严格了 ?
> prcessor A B
> A 对于 M 位置写入 1 2
> 如果 B 看到的是顺序是 2 1
> 可能会存在问题

基于侦听: bus 提供序列化, write invalid / write update
- valid / invalid
- MSI
    1. cache intervention : 内存中间的数据可能是陈旧的，
    2. 新增状态 E state : exclusive and clean => MESI : *private read-write data* 第 21 页ppt，感觉非常没有道理啊! E 状态为什么可以增强 ?
    3. M : modified and dirty : dirty 相对于 memory 来说的
    4. O : shared and responsibility to write back

split-transaction and popilined buses : 导致大量的暂态

scaling cache coherence : bus 无法 scale

虚假共享, bus occupancy :
1. load-reserve
2. store-condition 

## 14 memory consistency
顺序一致性 : 同一个进程的操作是满足顺序的其顺序，一般不会满足。
    - 注意一个问题，保持程序顺序，是指其他处理器观察到的，貌似自己顺序是否满足程序顺序，自己必然知道什么时候会出现问题。

可线性化 : 所有的操作瞬间为一个点，上锁可以保证。


举几个例子，其实就是乱序执行指令，当前后两个指令没有关联的时候，但是指令的提交不是按照指令顺序提交的吗 ?
- Store Buffer 的例子 : 简单
- load store bypassing : *莫名其妙* 什么叫做添加的 load 操作没有影响啊
- interleaved memory system => non-fifo store system
- non-blocking cache => 前面的 load 失败，然后进行下一个 load
- speculative exection => load 越过 load
- address speculative => 一个程序中间的load store 使用的地址没有关系，所以可以混乱顺序。
- store atomicity => 四个处理器的例子，两个写，两个读，但是两个读看到的写的顺序不一致。store 不是 atomicity 的，而导致观察到顺序不一致。
- causality => 三个处理器的例子，看似存在因果关系，但是 load/load 是乱序执行的，其实即使是考虑到提交，load 和 load 的乱序执行还是会产生问题，因为第二个 load 首先执行，获取到一个老数值，第一个 load 之后执行，获取到是新的数值，所以最后的效果还是乱序执行的。


release consistency 

RMO 是否等于 WO

## 15 vector
1. vector 同样使用流水线
2. vector memory system
3. vector conditional execution : 掩码，操作照样进行，但是不会写回。或者提前判断
4. vector scatter/gather

## 16 GPU
SIMT:
- Many threads each with provate architectural state
- Group of threads that issue together called a warp
- All threads that issue together execute same instruction
- Entire pipeline is an SM or streaming multiprocessor
- each SM support 10s of warp


memory types:
1. per thread memory : small, fixed size memory
2. scratchpad shared memory : small, fixed size memory , threads share data
3. global memory

all load are gathers, all stores are scatter.

serialized cache access : 首先比较 tag 然后再去访问数据

handle branch divergence: push pop 掩码，所有的branch 全部执行

*那么，block 和 warp 的关系是什么*

## 19 
