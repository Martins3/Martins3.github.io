## [BOOM 微架构学习(1)——取指单元与分支预测](https://zhuanlan.zhihu.com/p/168755384)

## [BOOM 微架构学习(2)——译码单元与寄存器重命名](https://zhuanlan.zhihu.com/p/194075590)
所谓寄存器重命名即是将每条指令使用的 ISA 寄存器映射到物理寄存器上的过程。
寄存器重命名的主要目的是解决流水线中的 WAW 和 WAR 依赖（RAW 依赖是真依赖，无法通过寄存器重命名的方法解决）。
之所以能通过寄存器重命名方式解决 WAW 和 WAR 依赖，是因为这两种依赖本质上是由于这两种情况导致的：
1. 指令集规定的寄存器数量不足，导致可用的寄存器“标号”过少，编译器只能重复使用相同的寄存器，导致出现本不应该出现的依赖
2. 在循环中，每次使用的都是同一个寄存器，但实际上每次循环中该寄存器的值直接并不一定存在关联

*总结的太好了*

- 在重命名的同时，每个指令的寄存器指示符指向所使用的物理寄存器。

- 和显式重命名相对的就是隐式重命名，在隐式重命名中物理寄存器的数量和 ISA 寄存器数量相同， ISA 寄存器只保存已经提交的指令的值，而不包括处于“推测”状态的值，这部分内容交由 ROB 保存。 当指令提交时，ROB 将值提交给 ISA 寄存器堆。

- BOOM 的重命名模块内部包括一个重命名映射表，用于记录映射关系
- 一个 Busy Table 用于记录每个寄存器是否处于“脏”状态
  - Busy Table 中维护着每个物理寄存器的"Busy"状态（即该寄存器当前是否可读），当某个寄存器被分配作为 pdst 时，BusyTable 的对应表项会置位，当该指令写回时，BusyTable 对应表项清空。
  - 当且仅当某条指令所有操作数均为清空状态时，该指令才能发射。
- 一个 Free List 用于记录哪些物理寄存器可用。

重命名映射表里包括了
- 一个 `map_table`（即从逻辑寄存器到物理寄存器的映射，实现方式为一个寄存器向量）
- 一个 `br_snapshots` 用于记录每个推测分支的 `map_table` 快照（每个分支有一个完整的`map_table`）
- 一个 `remap_table` 用于记录修改 map 中间值（下一个时钟周期将会更新至`map_table`）。

重命名映射表分为
- 查询逻辑 : 查询逻辑即输入 ISA 寄存器，根据 `map_table` 输出对应的物理寄存器；
- 更新逻辑 : 更新逻辑即 `map_table` 的更新，输入待更新的 `ISA` 寄存器及其对应的新物理寄存器，就会更新对应的表项；
- 快照保存逻辑 : 快照保存逻辑当流水线进入新的推测分支时，会记录当前的 `map_table` 状态；
- 恢复逻辑 : 恢复逻辑为当流水线发生分支预测错误时，会恢复进入该分支时记录的 `map_table` 状态。

> 所谓的 busyTable 也就是是否 ready 而已

## [BOOM 微架构学习(3)——ROB 和指令发射](https://zhuanlan.zhihu.com/p/237232261)

| Entry          | 含义                                                |
|----------------|-----------------------------------------------------|
| valid          | 该 Entry 是否有效                                   |
| busy           | 该 Entry 的指令是否在执行中                         |
| exception      | 该 Entry 是否是一个异常                             |
| `br_mask`      | 该 Entry 的指令属于哪个处于预测中的分支上           |
| `rename_state` | 该 Entry 的逻辑目标寄存器和物理目标寄存器分别是什么 |

- [ ] 这里关于异常的处理让我感到疑惑
  - [ ] 为什么会在 ROB entry 存放一个指令说明其是否存在一个异常的，难道异常不是在执行的时候才可以确定吗? 为什么在 ROB 中就可以确定了

发射队列存储着已经派遣（进入 ROB）但还没执行的指令。当某个指令的全部操作数均 ready 时，该指令即可请求发射；发射后，该指令即可从发射队列中移除出去。

在 BOOM 中存在着三个发射队列（整数指令、浮点指令和访存指令），不同类型的指令会放入不同的发射队列中。

目前 PNR 仅仅用于 RoCC 的场景下。很多 RoCC 要求发来的指令必须是确定的，不能是处于“推测”状态下的。因此 ROB 仅当 RoCC 指令过了 PNR 之后，才将其发射到 RoCC 上。

- [ ] 什么是 RoCC

## [BOOM 微架构学习(4)——指令的执行](https://zhuanlan.zhihu.com/p/259271229)

## [BOOM 微架构学习——前端与分支预测](https://zhuanlan.zhihu.com/p/379874172)
- 译码之后的指令放到 Fetch Buffer（Fetch Buffer 是一个将前后端解耦的指令队列）中供后端使用
- Fetch Target Queue 保存着从 I-Cache 取得的指令的地址以及地址对应的分支预测信息
  - 因此，在执行的时候阶段进行对比，如果预测错误，那么就刷新流水线。

- F3 阶段处理跳转指令。
- F4 阶段处理 short forward branch 指令

## [BOOM 微架构学习——详解重排序缓存](https://zhuanlan.zhihu.com/p/412828438)

## [BOOM 微架构学习——详解寄存器重命名技术](https://zhuanlan.zhihu.com/p/399543947)

## Official Documentation

### The Load/Store Unit (LSU)

When issued, “uopLD” calculates the load address and places its result in the LDQ.
Store instructions (may) generate *two* UOP s, “uopSTA” (Store Address Generation) and “uopSTD” (Store Data Generation).
  - The STA UOP calculates the store address and updates the address in the STQ entry.
  - The STD UOP moves the store data into the STQ entry.

Each of these UOP s will issue out of the Issue Window as soon their operands are ready. See Store Micro-Ops for more details on the store UOP specifics.

Entries in the Store Queue are allocated in the Decode stage ( stq(i).valid is set).
A “valid” bit denotes when an entry in the STQ holds a valid address and valid data (stq(i).bits.addr.valid and stq(i).bits.data.valid).
Once a store instruction is committed, the corresponding entry in the Store Queue is marked as committed. The store is then free to be fired to the memory system at its convenience.
Stores are fired to the memory in program order.

- [ ] 之所以制作出来一个 store queue ，是不是为了让 store 操作总是按照 program order 的

The RISC-V WMO memory model requires that loads to the same address be ordered

- read 可以提前到 write 前面

- [ ] 为什么不直接说，到所有的位置上都是必须 order 的

A thread can read its own writes early.
- [ ] 如果理解成为 : 可以提前更新的数值，那么如果

通过 Memory Ordering Failures 防止:
- store 的时候会检查已经被执行的 load 的地址是否相等
  - 如果有，那么刷新流水线
- 注意，这是程序员看不到的东西

- [x] 既然存在 load queue，为什么还会存在 read 可以乱序的问题
  - 只是叫做 queue 而已吧


Once a store instruction is committed, the corresponding entry in the Store Queue is marked as committed. The store is then free to be fired to the memory system at its convenience.

- [ ] 显然，顺序提交的 store 操作，显然不能最后对于寄存器的更新是相同的

对同一地址，写不超前、读 CoRR、原子操作不乱序 [^1]
- 实际上，显然在内存的读写上，相同的位置上, 只能进行 RAR 的重排
  - 寄存器中的 WAW 和 WAR 都是可以重命名的，第一，因为存在 ROB 来映射，第二，因为存在寄存器是私有的
- 读 CoRR 。对于同一地址的两个读，只要后一个 load 不到更老的值，就不约束两者的内存序 [^1]
