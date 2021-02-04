# 词法

正规式　正规文法　有限状态自动机

DFA 和 NFA 之间的等价装换

DFA 的化简

# 文法
1. 使用递归下降其实 穷举的方法
2. 左递归会导致 递归下降无限循环，消除左递归也有对应的方法

提取左因子： 推迟替换非终结符出现的时间

自上而下的分析:

维护一个stack 来记录状态，也就是表驱动的预测分析器

follow 集合： 为了防止A 出现空串，是通过first集合求解 follow 集

通过 first 和 follow 建立表格: 列 放置 非终结符，行 放置 终极符， 利用first 集合 将推倒公式放在表项中间
当获取了prasing table 之后，那么需要利用stack, 其实就是不断的查表来确定使用哪一个表达式，当出现表达式就把终结符吃掉。


自顶向下 和 自底向上 :

自底向上 采用 LR 分析，从左向右分析，构建最右推倒。



文法分类
| type  | description                                                          |
|-------|----------------------------------------------------------------------|
| LL(1) | http://www.csd.uwo.ca/~moreno/CS447/Lectures/Syntax.html/node14.html |
| G1    | 整个推倒过程唯一，最简单的文法                                       |
| G2    | 和G1 差不多，只是右边不是由非终结符开始                              |



| def                 | ref                                                                                                |
|---------------------|----------------------------------------------------------------------------------------------------|
| context free        | https://en.wikipedia.org/wiki/Context-free_grammar                                                 |
| first   & follow    | https://www.cs.uaf.edu/~cs331/notes/FirstFollow.pdf                                                |
| leftmost derivation | https://cs.stackexchange.com/questions/54814/different-between-left-most-and-right-most-derivation |

动态语义分析:


# YACC/BISON
http://www.csd.uwo.ca/~moreno/CS447/Lectures/Syntax.html/node15.html

# llvm
LLVM中间表示实现方法
*  可以存为字节码文件形式并反汇编
*  不同指令通过不同C++类型来实现
    *  公有基类Instruction
    *  通过动态分配相关对象生成指令
    *  支持灵活的指令格式形式
    *  指令间的引用关系通过双向链表来保存
*  四元式/三元式等实现方法
    *  规范的范式统一实现
    *  可用静态数组来分配整个指令序列，相对效率更高


ppt 上列举了各种基本语句的分析方法。

控制流分析和基本块分析

LLVM/Clang依赖于全局优化来消除冗余

控制结点（必经结点）

寻找循环的算法

可归约性是流图的一个重要特性

根据深度优先搜索树可以将流图中的边分为三类
*  前向边(forward edges)：深度优先搜索树上, 前驱指向后 继的边
*  回边(back edges)：深度优先搜索树上, 后继指向前驱的 边(似乎添加支配的要求 循环中间终点dominate起点的边的内容)
*  交叉边(cross edges)：不属于前两种的边

后续遍历的反序是一种拓扑序

寻找循环的方法是首先需要提供回边的


自然循环(loop)：控制流图中单入口的强连通分量
*  入口点(entry point or loop header )唯一，且dominate
所有循环中其它点
*  不是所有强连通分量都是循环
> 应该指的是其他节点未必可以到达入口，但是入口可以到达所有点，其他所有点互相可以到达!

基本块的概念
*  定义：基本块是程序中最大限度顺序执行的语句
序列，其中只有一个入口和出口，入口是其第一
个语句，出口是其最后一个语句

> 说实话，没有看懂不可规约图的含义是什么？
> 1. 虽然规约图拥有良好的性质，但是实际上难以判断一个图是否是规约的
> 2. 而且不知道知道回边的判断 : 循环

> 下面所有循环的内容都是处理针对于自然循环的!
> 1. nloop 只有**一个**入口，而且可以到达所有的位置(只有一个可以到达所有位置的点!)
> 2. 确定back edge 很简单了 : 指向entry即可
> 3. 所以back edge 必定构成循环
> 4. **寻找循环的算法莫名奇妙** 采用2->9 的例子难道不是多一个entry 的吗？entry 的含义是什么 ?

# SSA
1. 静态单赋值

2. 构建成功basic block 的整个图形出来了 !
    1. 找到谁控制过我!
    2. 找到谁在控制我的最近的节点中间的赋值才是有效的
    3. 简单啊 : 向上询问宽度优先遍历，然后x 遇到赋值立刻停止,并且收集起来即可 !

# 机器无关代码优化和数据流分析
本章将讨论如何消除这样的低效率因素，具体包括了对各个基本块本身进行局部优化，以及更彻底的全局优化

同时还会对数据流分析中的 **可用表达式**、**活跃变量**、**到达定值**进行介绍

> 数据流分析

以及公共子表达式删除、死代码删除、循环不变式外提

在这里的 DAG 中，叶结点表示变量初始值或常数，内部结点表示基本块内运算语句，边表示了操作间的前驱和后继的关系，当新的节点将
被加入 DAG 时，检查是否存在已有节点和新节点有相同的操作符和子节点
> 为什么可以表示为DAG 循环形成的环怎么办？因为是局部优化吧 !


https://en.wikipedia.org/wiki/Directed_acyclic_graph



# 指针分析
指向多个元素的时候，不能去除，不然会失去语义!(精度下降)

> T　为什么是全集 ?

> 多个等于，graph 上如何表示的 ?

程序点(program point)

过程内　过程间
指向集合


上下文: 分析函数，不同的调用点，其分析采用不同的方式。
1. 使用函数调用chain 环使用一个节点表示
2. cloning-based 方法 : 相当于inline 整个函数体，数量爆炸 -> 减少路径数目(BDD消除) 限定调用路径长度
    3. BDD : 节点编号， ....(? ? ? ?)

3. summary-based 方法 : 难以精确的计算summary 信息

4. cfl- reachability

https://www.cs.purdue.edu/homes/peugster/EventJava/Dyck-Trees.pdf
https://cs.stackexchange.com/questions/53903/how-is-cfl-reachability-solvable-in-exponential-time-and-space
https://research.cs.wisc.edu/wpis/papers/tr1386.pdf

过程间数据流分析:

上下文敏感和路径敏感不同 ?

# 考试
1. 可以上网查资料
2. slides 中间找到答案。




## 前段
1. 第二节　第三节

> 1. 重写规则 : derivation
> 2.

正规式　正规文法　有限状态自动机 处理的问题是什么 ?
三者都是描述生成的语言。

0123型文法，其要求逐渐增加。

自上而下的分析 :
1. 消除左递归 (不然不能处理，无法建立FIRST 和 FOLLOW集合)
    1. 左递归分类
    2. 消除的方法
2. First集合 和 Follow集合 的构造

控制流分析
- 控制流图，基本块
  - 基本块的确定，条件语句到达的位置和下一条语句。
- 控制流图的遍历
  - 控制节点用于确定循环
  - 其他控制节点都是控制 immediate dominate 的节点的(反证法)
  - 写出对应的控制树的方法 : 如何确定谁dominate 我 (如果其dominate所有的pred即可，有的节点看上去含有两个入口，但是问题是从root 到 m 都会经过 n，那些入口其实是用来构成循环的)
  - back edge : a dominate b 同时 b 指向 a
    - 给定一个back edge 就是可以构造出来一个 natural loop 的情况。方法 : 从 d -> n, 然后在反向图上dfs。
    - 为什么强调单一入口节点 : 不然和自然循环构造算法相冲突了。 单一入口不是一个简单命名，而是真真切切的要求。
- 支配节点，后支配节点，支配树
- 自然循环的识别

- 计算 DF 的方法: 节点N, 我无法dominate 但是可以dominate 其 直接控制节点。
  - 根据DF 的含义 : 将 DF 的内容划分为两种，其实就是eac中间的算法。(499页)
    - 如果该节点是pred 并且该节点无法 dominate 
    - 如果你也 immediate dominate 我，那么可以收割我的 DF节点，当然除非你 dominate 了我的DF
    - 节点的开始位置限制为 joint

  - iterated : 不是在计算节点的 dominance frontier，而是在分析对于一个变量需要设置 phi 的位置。
    - alreadyList : 防止重复添加 phi
    - everOnWorkList : 防止重复操作
    - workList : DF也全部加入到
    - 算法原理 : 递归的添加DF 的 DF
  - rename : 
    - 对于定义的rename 很简单的，遇到就是序号增加。
    - 对于 dominance tree 进行递归调用的，遇到新的定义入stack 然后
    - 对于每一个 sucessor 定义的phi 参数更新
    - 遇到新的定义，无论是phi还是普通的赋值，都需要使用序号增加，进入stack
    - 说明一下其中的合法性 : phi 的参数来自于succ, rename 之前已经插入了phi 节点。重命名，该变量如果不使用，那么就会是phi


- 数据流
  - 具体三个例子
    - 活跃变量 : 该变量在此处依旧会被使用，用于确定变量初始化
        - 后，交集
    - 可用表达式 : 
        - 前，并集
    - 到达定义 : 
        - 前，并
  - May 和 Must 就是表示交集和并集。
  - 单调性和分布性，只是强度不同

Anderson 算法:
1. 当出现赋值语句，指向集包含
2. 箭头的作用

