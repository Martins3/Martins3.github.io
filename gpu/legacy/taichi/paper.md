# 方针
1. 确定每一段的内容是什么东西 ?

# 读
sparse multi-level data structures

A voxel is a unit of graphic information that defines a point in three-dimensional space.

stencil : 在图形渲染中间的含义 ?

A voxel is a unit of graphic information that defines a point in three-dimensional space.

We propose a new programming model that **decouples** data structures from computation
1. 如何 decouple ?
2. data structures 的细节到底是什么 ?

Accessing a multi-level sparse array is significantly more involved than accessing a dense array.
> more involved 的含义是什么 ?

所做的贡献:
1. A data structure description *mini-language*, which provides several elementary data structure components that can be composed
to form a wide range of sparse arrays with static hierarchies


We achieve this by abstracting data structure access with **Cartesian indexing**, while the actual data structures define the mapping from the
index to the actual memory address

We facilitate compiler optimizations and simplify memory allocation by assuming the **hierarchy** to be fixed at compile time.

More discussions on hints for scratchpad optimization 

After writing the computation code, the user needs to specify the internal data structure hierarchy. Specifying a data structure includes
choices at both the macro level, dictating how the data structure
components nest with each other and the way they represent sparsity, and the micro level, dictating how data are grouped together
(e.g. structure of arrays vs. array of structures).
当理解如何计算之后，描述数据结构内部的架构，从而实现自动装换其中的，

kernel 是什么含义啊 ?

**跳过代码教学**

Hierarchical data structures provide an efficient representation for
sparse fields but have high access costs due to their complexity,
especially when parallelism is desired. Our compiler reduces access
overhead from three typical sources:
使用层级架构的数据结构易于呈现数据，但是存在三种情况，同时采用三种对应策略

这是给几个的总结:
optimizing for locality (Sec. 4.1),
minimizing redundant accesses using access coherency (Sec. 4.2), 
automatically parallelizing (Sec. 4.3) and allocating memory (Sec. 5.2).

## INTRODUCTION
First, naively traversing the hierarchy can take one or two orders of
magnitude more clock cycles than the essential computation. 
Second, we need to ensure load-balancing for efficient parallelization. 
Third, we need to allocate memory and maintain sparsity when accessing
inactive elements.


## 2 GOALS AND DESIGN DECISIONS
做出的贡献 : We summarize our contributions as follows 

设计目标 : The four high-level goals are as follows

根据这样的设计规则，提出的设计原则是什么!



## THE TAICHI PROGRAMMING LANGUAGE
#### 3.1 Defining Computation
使用 Lapace 作为例子，在计算上的优化

We adopt the Single-Program-Multiple-Data paradigm. Our language is similar to other SPMD languages such as ispc and CUDA,
with three additional components: 1) parallel sparse For loops, 2)
multi-dimensional sparse array accessors, and 3) compiler hints for
optimizing program scheduling.

#### 3.2 Describing Internal Structures Hierarchically
macro level : 数据如何嵌套
micor level : 数据层次


Kernels are defined as iterations over leaf elements (i.e., voxels or pixels), independent
of the internal data organization. Leaf blocks, immediate blocks of leaf
elements, are the smallest quantum of storage and computation tasks.

## DOMAIN-SPECIFIC OPTIMIZATIONS
Our compiler reduces access overhead from three typical sources:
1. Out-of-cache access : 
2. Data structure hierarchy traversal : 
3. Instance activation : 为什么 instance activation 需要上锁的工作

**为什么称之为 domain-specific optimization**
#### Scratchpad Optimization
*应该搜索一下更多的 scratchpad 的信息，否则和 cache 有什么区别啊*


Fortunately, oftentimes it is possible to determine bounds using domain
knowledge from the data. Therefore we provide an AssumeInRange
construct for specifying the bounds of individual variables

Apart from the Cache construct that provides shared memory
usage hints, the CacheL1(V) construct for the GPU backend instructs
the compiler to issue `__ldg` intrinsics to force data loads from the
global variable V into GPU L1 cache. 
> 那么数据的分析是什么 ?

#### Removing Redundant Accesses
> 这就是一直都理解的思想

#### Automatic Parallelization and Task Management
> 任务平均分配

Our strategy is to generate a task list of leaf blocks, **which flattens
the data structure into a 1D array**, circumventing the irregularity of
incomplete trees. Importantly, we generate a task per block instead
of a task per element (Fig. 5), to amortize the generation cost.


按照每一个 root-to-leaf 创建一个:
The lists are generated in a layer-by-layer manner: *starting from the root node, the queue of active parent nodes is used
to generate the queue of active child nodes.* A global atomic counter is used to keep track of the current queue head.

## COMPILER AND RUNTIME IMPLEMENTATION

Our intermediate representation follows the static single assignment design and is similar to LLVM 
**和LLVM的关系是什么**，采用了不同的中间表示层，**有没有利用过LLVM的内容，以及如何利用**

We split the simplification into two phases. The first phase greatly
reduces and simplifies the number of instructions and makes it easier
for the second simplification phase. 

在 5.1 中间，重点分析了*如何*利用编译的方法实现，访问的transver 的简化。
5.2 中间，为什么给每一个node 都是确定一个 alloccator 啊
5.3 在 CPU 中间实现向量化操作

The frontend kernel code is lowered to an intermediate representation
before being compiled into standard C++ or CUDA code.
Key components of our compiler and runtime are a two-phase simplifier for
reducing instructions and removing redundant accesses, an access
lowering transform, a customized memory management system for
memory allocation and garbage collection, and a CPU loop vectorizer. The compilation workflow is summarized in Fig. 10
> 首先，翻译为中间层，然后翻译为 C++ 或者 CUDA code
> two-phase simplifier 实现的目的 : reducing instructions and removing redundant accesses

Our intermediate representation follows the **static single assignment** design and is similar to LLVM

#### 5.1 Simplification
基本都是常规的 simplification


The stages of moving down a single hierarchy in the data structure are as follows.

Central to data structure access simplification are what we call micro access instructions: OffsetAndExtractBit, SNodeLookup, GetCh, and IntegerOffset. 
> 和数据访问相关的，当从 hierarchy 从上向下访问的时候，将相似的进行合并

The stages of moving down a single hierarchy in the data structure are as follows
1.  First, offsets at each dimension are computed, along with the starting and ending position of each index represented as bit masks (OffsetAndExtractBit). 
2. Next, the extracted multi-dimensional
indices are flattened into a linear offset (Linearize). Then a pointer
to the item in the data structure is fetched from the current level
of the data structure using the linear offset, along with a check
of whether the node is active or not (SNodeLookUp).
3. Then a pointer to the item in the data structure is fetched from the current level
of the data structure using the linear offset, along with a check
of whether the node is active or not (SNodeLookUp).
> 这一步需要注意的是，SNodeLookUp 获取的数据是否真的存在
4. Finally the corresponding field in the item is fetched (GetCh).

In cases where two micro access instructions of the same type
lead to a compile-time-known non-zero offset, we replace the second micro access instruction with an IntegerOffset instruction,
representing the relationship between the two accesses in bytes,
avoiding data structure traversals.
> 上面关于指令的陈述，应该是涵盖了所有的关于的认知吧!

#### 5.2 memory management
> 每一个 node 都有各自的类型

The benefit of having multiple allocators is that each allocator
only needs to allocate memory segments of a fixed size, which greatly simplifies and accelerates the process.
> 第一个，为什么每一个分配内存只是按照 memory segment，并且这种方法可以大大 简化和加速 process

## EVALUATION AND APPLICATIONS
这个部分，每一个小节都是分析一个例子

physical simulation, rendering, and 3D deep learning : 居然还可以 3D deep learning !


#### 6.1
Least Squares Material Point : elastoplastic continuum simulation
However, when the simulation progresses and the spatial
distribution of particles changes, our performance drops drastically
(Table 2, row “SOA”), especially when simulating liquids.


Gao et al. [2018] implemented a highperformance Moving Least Squares Material Point Method [Hu et al.
2018] solver on GPU with intensive manual optimization, including
> 高明使用了大量的优化，而 taichi 的措施是:
> 对于其中的优化1 和 优化2 可以很容易实现，3 太底层，4 太麻烦

It took us a few attempts, but thanks to the easy data structure exploration supported by our language, we eventually surpassed their
performance by 18%. 

The data structure code for the high-performance data structure
we found for MPM is illustrated in Figure 13.
For particles we use array of structures, for grids we use structure of arrays, and each block
maintains a list of indices of its contained particles
> 在这一个例子中间，具体的说明了数据结构是什么样子，相对于 Gao 的算法，是存在简化的

Our implementation has only four kernels: sort particle indices
to their containing blocks, particle to grid (P2G), grid normalization,
and grid to particle (G2P)
> 相比，Gao的使用20kernel，主要用于维护数据

In the P2G and G2P kernels, we use the `AssumeInRange` construct
to hint to the compiler the spatial relationship between blocks and
their containing particles. 
> block 和 其中 particle 的优化，其中的配图是什么意思
> 1. 使用了 stagger particlegrid ownership optimization 
> 2. 


https://stats.stackexchange.com/questions/380040/what-is-an-ablation-study-and-is-there-a-systematic-way-to-perform-it


#### 6.2
A large scale sparse grid-based finite element solver was presented by Liu et al. [2018] for high-resolution topology optimization

conjugate gradient 梯度算法 :
https://www.cs.cmu.edu/~quake-papers/painless-conjugate-gradient.pdf

We reproduced their algorithm in our language. Our compiler is
especially good at compute-bound tasks, as our access optimization
and auto-vectorization significantly reduce the number of instructions. 
> access optimization 和 auto-vectorization 可以自动间

#### 6.3
Large-scale Poisson equation solving has extensive use in graphics, including fluid simulation, image processing and mesh reconstruction
> 这是一种方法，可以用于各种工具中间

We implemented a simplified version of the reference implementation, with the following differences:
> 后面说明了几个不同点，但是，为什么需要采用这种方法


Our solver automatically generalizes to an irregular and sparse
case, while the reference implementation deals with only dense grids.

https://en.wikipedia.org/wiki/Dirichlet_distribution


The same solver can potentially be used for other graphics applications such as panorama image stitching [Agarwala 2007] or mesh
reconstruction [Kazhdan et al. 2006].
> Large-scale Poisson equation solving 技术还可以用于其他的方面

#### 6.4
We implemented a 3D convolution layer, operating on multi-channel sparse voxels.
其中还说明了，具体的内容配置

#### 6.5
Our domain-specific optimizations 只有 5% 的增加，因为 access pattern 是 incoherent in volume rendering.
> **有点迷惑**

## 7 LIMITATIONS
Low arithmetic intensity tasks


对于 *material point method*，*finite element method*，这种 compute bound，access optimizer 占据核心位置。

multigrid Poisson solver 这种 memory bound 的，那么没有什么帮助。

> In these cases reducing instructions no longer helps.
> **难道不是对于 memory bound 的** 是最容易处理的吗 ?

For the *volume rendering* 的，这种动态局部性信息的，没有办法在编译的知道


## 8 RELATED WORK
array compiler : 其实并不存在这种类型的 compiler，只是之前在为了处理高效访问 array，都是做了那些工作的!
1. 将 dense image processing 和 lower level scheduling 拆分开来
2. linear algebra operation, graph operation
3. vector 
4. Physical Simulation Languages


Data oriented:
Hierarchical Sparse Grids in Graphics


All these compilers focus on dense data structures and do not **model sparsity**. 

Our language decouples algorithms from the internal organization of sparse data structures, allowing programmers to quickly switch between data organizations to achieve high performance.
> 高效部署，快速测试

Physical Simulation Languages.　=> 存在几个 domain specific 的 language =>

Our language facilitates data-oriented design and shares the same philosophy through decoupling of data structures and computation. => **what's data oriented**

Computer graphics, especially in the field of physical simulation,
has a long history of using multi-level sparse regular grids for finite
element methods, level set methods [Osher and Sethian 1988], or
Eulerian fluid simulation. 
> *multi-level sparse regular grid* 指的是 ? 和这里的 multi-level sparse 有什么区别 ?
> 这一段列举了各种写法，但是其中最关键的没有指出来 ? 我门现在的方法的优势是什么 ?


## 总结
可以在不修改代码的情况下，使用文档

## doc
1. 说好的，根据用户设计的数据结构，然后可以自动编译，体现在什么地方 ?
2. 为什么要使用 kernel 这个概念 ?
3. 真的需要到使用语言的这个地步吗 ?
4. 这个到底解决了什么问题啊 ?
5. simulation 工作到底做的事情是什么 ?
6. 稀疏矩阵到底如何实现的 ?


等等:
1. 当谈到 vector 和 matrix 的区别的时候，那么 tensor 是什么东西 ? 
      1. 这些概念和 dense hash dynamic 的区别是什么 ?

在高级数据结构的哪一个章节中间，其中分析了 dense 和 var/vector/matrix 的关系，
但是，其实其他的各种数据类型根本不能表示为 var/vector/matrix，而是需要采用其他的方法。

> 我们建议从默认的布局规范开始（通过在 ti.var/Vector/Matrix 中指定 shape 来创建张量），
> 如果需要的话，之后可以再使用 ti.root.X 语法迁移到更高级的布局。

2. ti.func 和 ti.kernel 的关系 ? 显然可以存在多个ti.func，但是存在多个 ti.kernel 吗 ?

从 kernle 的生命周期上来分析，显然是可以存在多个kernel的，


解释一下两个词汇 ?
1. 可微编程

3. 大核心
4. 双尺度自动微分

计算发生在 Taichi 的 内核(kernel) 中。

in Taichi, tensor and matrix are two completely different concepts. Matrices can be used as tensor elements, so you can have tensors with each element being a matrix.

place 的含义:
```
x = ti.var(ti.f32)
ti.root.dense(ti.ij, (4, 3)).place(x)
# 等价于：x = ti.var(ti.f32, shape=(4, 3))
```

1. 由标量组成的张量
2. 向量是只有一列的特殊矩阵。实际上，ti.Vector 只是 ti.Matrix 的别名。

> 为什么总是强调，tensor 中间方一个 vector 的概念

向量一共存在两个表述方法，在局部变量中间，直接成为变量，而在全局中间，需要成为tensor的成员

在编写计算部分的代码之后，用户需要设定内部层次数据结构。包括微观和宏观两部分，宏观上设定层级数据结构组件之间的嵌套关系以及表示稀疏性的方式；微观上，描述数据如何分组(例如，SOA 或 AOS)。Taichi 提供了 结构节点 (SNodes) 以满足不同层级数据结构构建时的需求。其结构和语义具体如下所示：
1. dense
2. bitmasked
3. pointer
4. dynamic


> 1. 如何构建层级结构的 ?
> 2. ti.root


**使用组合索引，来实现对维度不依赖的的编程**

ti.static(range(3))

> 1. 直接 for 1 2 3 不香吗 ? 为什么封装的这么麻烦 ?
> 2. 整个内容放到 metaprogramming 中间，我认为是没有定义清楚的!

数组结构体(AoS)，结构体数组(SoA)


sparse computation :

面向数据编程:
**和面向对象存在任何的区别吗 ?**

taichi内核 生命周期，其实不就是编译的过程


https://www.bilibili.com/video/BV13J411h7up
1. structured nodes
2. node decorator
**node decorator** ?

https://en.wikipedia.org/wiki/Z-order_curve
