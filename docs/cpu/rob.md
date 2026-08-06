# Reorder Buffer (ROB)

本文档聚合散落在各处笔记中关于 ROB 的内容，原始出处见文末参考。

## 为什么需要 ROB

乱序执行要支持推测(speculation)，必须把两件事分开:
1. 结果在指令之间的 bypass(让指令可以推测执行)
2. 指令的真正完成(更新架构状态)

核心思想是: **允许指令乱序执行，但强制它们按程序顺序提交，在指令提交之前，禁止任何不可撤销的动作(更新状态、触发异常)**。

历史上，乱序执行沉寂了很久，直到两个大问题被解决才成为主流:
1. 精确异常(precise traps): 不精确的异常让调试和 OS 代码变得复杂
2. 推测执行

ROB 就是为解决这两个问题而引入的硬件缓冲: 保存"已执行完但未提交"的指令的结果，同时也在推测指令之间传递结果。

## ROB 是什么

ROB 是一个循环队列:
- 在 allocation/dispatch 阶段按程序顺序分配 entry(入队)
- 指令提交时按顺序释放 entry(出队)
- 分支预测失败或异常时，清空错误路径上的 entry

每个 ROB entry 包含四个字段(CAQA 的定义):

| 字段 | 含义 |
| --- | --- |
| instruction type | 指令类型: branch(无目的结果)、store(目的是内存地址)、寄存器操作(ALU/load，目的是寄存器) |
| destination | 结果应写入的寄存器号(load/ALU)或内存地址(store) |
| value | 保存指令结果，直到指令提交 |
| ready | 指示指令已完成执行，value 就绪 |

ROB 在指令"执行完成"到"提交"之间持有结果。有了推测执行后，寄存器文件只有在指令提交时才被更新(此时才能确定该指令确实应该执行)。

ROB 和 Tomasulo 算法中的 store buffer 类似，为简单起见可以把 store buffer 的功能整合进 ROB。

## 指令经过 ROB 的四个阶段

| 阶段 | 触发条件 | 动作 |
| --- | --- | --- |
| Issue | 存在空闲 reservation station 和空闲 ROB 项 | 操作数若已在寄存器或 ROB 中就绪则送入 RS; 更新控制项; 将分配的 ROB 项编号发给 RS，用于之后给 CDB 上的结果打 tag |
| Execute | 操作数未就绪则监听 CDB | 可能花费多个周期; load 需要两步; store 在此阶段只做有效地址计算 |
| Write result | 结果可用时 | 把结果连同 ROB tag 放上 CDB，写入 ROB 以及所有等待该结果的 RS; 释放 RS |
| Commit | 指令到达 ROB 头部且结果就绪 | 更新寄存器并移除 ROB 项; store 类似但更新的是内存; 若到达头部的是预测错误的分支，则 flush ROB，从正确后继重新执行 |

要点:
- 因为每条指令在提交前都有 ROB 中的位置，所以用 ROB entry 号而不是 RS 号来给结果打 tag，这要求 RS 跟踪指令分配到的 ROB 项
- store 特殊处理: 若待存的值可用就写入该 store 的 ROB entry 的 value 字段; 不可用则监听 CDB 直到值广播出来
- 指令分发时预定的资源包括: issue queue entry、ROB entry 和 load/store queue entry，任何一个未就绪都会阻塞分发

## 基于 ROB 的重命名(隐式重命名)

当代处理器使用三种策略实现重命名: reorder buffer、rename buffer 和 merged register file。

基于 ROB 的方案中:
- ROB 保存未提交指令的结果，architecture register file 保存每个架构寄存器最新的已提交值
- 需要 rename table 指明架构寄存器的最新定义在 ROB 还是架构寄存器文件中; 若在 ROB 中，还要记录其在 ROB 中的位置
- 指令执行完，结果先放到 ROB; 提交时再复制到架构寄存器文件
- 缺点: 操作数在其生命周期中可能出现在两个位置，读取操作数变复杂

这就是隐式重命名: 物理寄存器数量和 ISA 寄存器数量相同，ISA 寄存器只保存已提交的值，推测状态的值由 ROB 保存，提交时 ROB 把值提交给 ISA 寄存器堆。Intel Core 2 使用此方案。

对比 rename buffer 方案: 约三分之一的指令不写寄存器文件，ROB 方案为其分配 entry 会浪费约三分之一空间; rename buffer 用独立结构保存 in-flight 指令的结果(IBM Power 3)。

## ROB vs Merged Register File

基于 ROB + RRF(retire register file)的架构状态管理:
- RRF 只存逻辑寄存器的值，数量与逻辑寄存器相同
- ROB entry 主要含两部分: 指令信息(指令类型、执行状态、结果的架构寄存器标识等)和产生的值
- 提交时 ROB 中的值被复制到 RRF，entry 出队
- 大多数基于 ROB 的处理器在 rename 之后、发射之前读操作数; 处于 allocation 阶段的指令需要被通知以更新 renaming table(值已到 RRF)

MRF 相对 ROB 方案的三个好处:
1. 指令结果不会改变位置(推测状态和架构状态存在同一位置)
2. ROB 方案需要为所有指令分配 entry，而约 25% 的 store/branch 指令不产生任何值
3. ROB 是中心化结构，所有指令都要从它那里读操作数，会让"发射之后读操作数"的 decoupled 架构变复杂

但 MRF 需要额外的队列来实现顺序提交，且物理寄存器释放更复杂(目前的策略是当后续指令写相同逻辑寄存器时才释放)。

## 从分支预测错误中恢复

恢复分前端和后端两部分:
- 前端: 清空中间 buffer，矫正分支预测器，更新 PC
- 后端: 清除错误指令在 memory buffer、issue queue、ROB 等结构中的影响，释放 renaming table 项、发射队列和物理寄存器等资源

基于 ROB 的架构(Intel Pentium Pro 风格): 等所有先于错误分支的指令都提交完成后才开始恢复。此时 RRF 中保存的就是错误预测之前的架构状态，恢复 renaming table 只需指向 RRF 中仍然有效的项。

## 实际实现

- **Intel Pentium Pro**: 经典的基于 ROB 的设计，确立了 ROB + RRF 的恢复方式
- **Intel Core 2**: 基于 ROB 的重命名
- **MIPS R10000 / PA8000**: store buffer 不存数据，提交时用专用端口从寄存器文件读
- **BOOM**: ROB entry 字段如下

| Entry | 含义 |
| --- | --- |
| valid | 该 Entry 是否有效 |
| busy | 该 Entry 的指令是否在执行中 |
| exception | 该 Entry 是否是一个异常 |
| `br_mask` | 该 Entry 的指令属于哪个处于预测中的分支上 |
| `rename_state` | 该 Entry 的逻辑目标寄存器和物理目标寄存器分别是什么 |

BOOM 中 PNR(point of no return)用于 RoCC 场景: 很多 RoCC 要求发来的指令必须是确定的、不能处于推测状态，因此 ROB 仅当 RoCC 指令过了 PNR 之后才将其发射到 RoCC。

## 一个例子: ROB 如何工作

用 CAQA 第 3 章的经典循环(假设每周期按序发射 1 条，L.D 执行 2 拍，MUL.D 4 拍，DADDIU/BNE 1 拍，分支预测 taken 且预测正确):

```asm
Loop: L.D    F0, 0(R1)     # F0 = Mem[R1]
      MUL.D  F4, F0, F2    # F4 = F0 * F2
      S.D    F4, 0(R1)     # Mem[R1] = F4
      DADDIU R1, R1, -8
      BNE    R1, R2, Loop
```

### 第 1 步: 按序分配(entry 入队)

前 5 条指令依次发射，各拿到一个 ROB entry。寄存器文件里 `RegisterStat[F0].Reorder = 1`、`RegisterStat[F4].Reorder = 2`，表示 F0/F4 的最新定义在 ROB #1 / #2:

| ROB# | 指令 | 目的 | Value | Ready |
| --- | --- | --- | --- | --- |
| 1 (head) | L.D F0, 0(R1) | F0 | -- | no |
| 2 | MUL.D F4, F0, F2 | F4 | -- | no |
| 3 | S.D F4, 0(R1) | Mem[R1] | -- | no |
| 4 | DADDIU R1, R1, -8 | R1 | -- | no |
| 5 (tail) | BNE R1, R2, Loop | -- | -- | no |

此时 MUL.D 在 RS 中等待，它拿不到 F0 的真实值，只记下 tag: "我的操作数来自 ROB #1"。S.D 同理，它需要两个东西: 地址(执行时算出，写入 entry 的 destination)和待存的值(F4，即 ROB #2 的结果)。

### 第 2 步: 执行与写回(乱序完成，顺序不动)

- L.D 先完成，把 `Mem[R1]` 连同 tag `#1` 放上 CDB: 写入 ROB #1 的 Value 并置 Ready，同时喂给等待中的 MUL.D
- DADDIU、BNE 也不等别人，先后在 ROB #4/#5 中写好结果、置 Ready -- 注意 BNE 只是"确认预测正确"，不产生值
- MUL.D 拿到 F0 后算 4 拍，广播 tag `#2`: ROB #2 Ready，S.D 的 Value 字段也在这时被更新(它一直在监听 CDB 等 F4)

可能出现的状态( head 还没走，后面的全好了 ):

| ROB# | 指令 | Value | Ready |
| --- | --- | --- | --- |
| 1 (head) | L.D | Mem[R1_old] | yes |
| 2 | MUL.D | F0*F2 | yes |
| 3 | S.D | F0*F2 | yes |
| 4 | DADDIU | R1_old-8 | yes |
| 5 (tail) | BNE | -- | yes |

### 第 3 步: 按序提交(entry 出队)

提交严格从 head 开始，即使 #2~#5 早就 Ready 也不能插队:

1. **L.D 提交**: Value 从 ROB #1 复制到架构寄存器文件的 F0，#1 出队
2. **MUL.D 提交**: F4 写入架构寄存器文件，#2 出队
3. **S.D 提交**: 此时才真正写内存 `Mem[R1] = F4` -- store 对内存的修改是不可撤销的，所以必须等到提交这一刻(它已确定不在任何错误路径上)
4. **DADDIU 提交**: R1 更新
5. **BNE 提交**: 预测正确，直接出队，无事发生; 若预测错误，见下一步

因为预测正确且 ROB 还有空位，第二轮循环的 5 条指令早已在 tail 后面继续入队，推测执行不会断档。

### 第 4 步: 预测错误时的 flush

假设 BNE 实际不跳转(预测错误)。当 BNE 到达 head 并发现错误时:

- ROB 中 BNE 之后的所有 entry(第二轮循环的指令)全部作废，tail 指针回卷
- 架构状态不需要"回滚": 错误路径上的指令从未提交过，架构寄存器文件和内存里根本没有它们的痕迹 -- 这正是"不可撤销动作延迟到提交"换来的
- 推测的寄存器值(ROB 中的 Value)随 entry 一起丢弃; 用 rename table 恢复映射(Pentium Pro 风格是等错误分支之前的指令全部提交后，用 RRF 中的值重建)
- PC 指向正确的后继，重新取指

一句话总结: **ROB 让"执行"可以乱序、可以猜，但让"生效"永远按程序顺序、永远可撤销**。

## 为什么 ROB 能实现精确异常

**精确异常(precise exception)**的定义: 异常发生时，处理器的状态必须等价于"指令按程序顺序逐条执行到出错指令"的状态:

- 出错指令**之前**的所有指令已经全部完成、更新了架构状态
- 出错指令本身及**之后**的所有指令完全没有修改架构状态
- 保存的 PC 精确指向出错的那条指令

这样 OS 处理完异常后可以透明地恢复执行，调试器看到的状态也是自洽的。

### 没有 ROB 时: 不精确异常

乱序执行下，指令完成的顺序和程序顺序不一致。考虑:

```asm
DIV.D  F0, F2, F4     # 慢，要执行几十拍，第 30 拍才发现除零
ADD.D  F10, F10, F8   # 快，第 3 拍就完成了
SUB.D  F12, F12, F14  # 快，第 4 拍就完成了
```

如果结果一出来就直接写寄存器文件，那么当 DIV.D 在第 30 拍触发除零异常时，排在它**后面**的 ADD.D/SUB.D 早已改写了 F10、F12。此刻的架构状态是"后面指令生效了、前面指令没生效"的缝合怪，任何顺序执行都产生不出这个状态:

- OS 异常处理程序无法假设 F10/F12 还是旧值
- 处理完想重新执行? DIV.D 的操作数可能已经被后续指令覆盖，连重启点都找不到

这就是不精确异常(早期 MIPS R8000、Alpha 的浮点异常就是这个风格)，只能靠软件约定(比如异常后不许依赖寄存器内容)凑合。

### 有 ROB 时: 异常只是"到达队头的一种提交结果"

关键点: 异常在**执行时被发现**，但只在**提交时被生效**。两个时刻分离:

1. **执行时**: DIV.D 的除零在执行阶段就检测到了，此时只是在 ROB entry 里记下 exception 标记(这回答了 BOOM 笔记里的疑问: ROB entry 中的 exception 位不是说"这条指令注定异常"，而是"执行完发现它异常了，先记账，到提交时再处理")。指令照常标记 Ready，流水线不停
2. **提交时**: DIV.D 到达 ROB 队头，发现带 exception 标记，于是: 清空它和所有更年轻的 entry(ADD.D/SUB.D 的结果一直待在 ROB 里，从未写进寄存器文件，直接随 entry 丢弃)，PC 精确设为 DIV.D 的地址，跳转异常处理程序

因为架构状态只在队头按序更新，异常发生时的寄存器/内存状态天然满足精确异常的定义。对比分支预测错误的 flush，机制完全同构: 分支预测错误是"赌错了，撤销"，异常是"确定走不下去了，撤销" -- ROB 用同一套"延迟生效 + 按序提交"同时解决了这两个历史难题。

### 一个细节: 异常之前的推测指令

注意 DIV.D 在 ROB 里排队等提交期间，ADD.D/SUB.D 的结果是可以被更后面的指令通过 CDB 正常使用的 -- 反正真到了异常那一刻，这些消费者也都在 ROB 里、也在被清空的范围内。推测世界内部自洽，只有提交才通向架构世界。

## 待解决的问题

- 什么叫做 architectural? 为什么只有 ROB 方案需要 ROB pointer，architecture register file 就不需要?
- 大多数基于 ROB 的处理器在 rename 之后、发射之前读操作数，allocation 和 data read 为什么拆成两个阶段，为什么不在重命名阶段读取?
- ROB 是中心化结构，为什么不把 ROB 分散开来?
- 物理寄存器的释放和分配的具体过程是什么样子的?

## 参考

- [Computer Architecture: A Quantitative Approach 第3章笔记](sys/Quantitative/3.md)
- [Quantitative 速览](sys/Quantitative/synthesis/3.md)
- [超标量处理器设计 第5章 Allocation](sys/SLCA/5.md)
- [超标量处理器设计 第7章 写回](sys/SLCA/7.md)
- [超标量处理器设计 第8章 提交](sys/SLCA/8.md)
- [BOOM 微架构学习笔记](boom/doc.md)
- [BOOM 问题清单](boom/why-boom.md)
- [CS252 课程笔记](sys/cs252/lecture.md)
- [MIPS R10000](mipsR10000.md)

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
