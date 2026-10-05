# Taichi: A Language for High-Performance Computation on Spatially Sparse Data Structures

## 为什么要发明这个语言
为什么新的语言层出不穷，不是因为发明一个新的语言可以彰显自己的技术实力，而是新的问题需要更加贴切的刻画方法， 之所以发明C语言，因为汇编语言表达力不足，易于出错， C++则是为了解决C语言对于面向对象的支持不力，常用库的缺失， 使用C++容易产生内存管理问题，虽然其在内存管理的设计思想上是完善的，但是没有办法强制程序员遵循，而Rust将这种 规则写入到编译器，让不守规则的代码无法编译。 同样的，大规模的3D模拟，渲染，视觉之类的任务需要处理大量的有规则的稀疏数据，使用简单的调用数据结构的库来并不能保证效率， 在其可能访问数据的时间要远远大于真正计算的时间， 传统的计算机语言没有办法优雅简洁的描述在上述的任务中间的代码，更加没有办法将数据结构和算法解除耦合， 这限制了相关算法的快速的实现和对于使用的数据结构的调整， 如果不使用通用的数据结构库，而是手动的调试，虽然可能获取性能的提升， 但是需要小心的调整，以及对于硬件底层的深刻理解，这如同使用汇编语言， 不仅仅对于算法实现者要求高，而且降低了编程效率，为了解决这些问题， taichi语言产生了。taichi并没有解决具体的问题，它的目的是加速解决问题。

<放个图 Fig3>

## taichi是一个什么样的语言

taichi是为了图形学而生的语言， 从人友好的角度，其应该具有很强的表达力，可以轻松的描述各种图形学的问题， 从机器友好态度，其首先应该具有很高的性能，其次应该使用于各种设备。

为了描述的稀疏数据， taichi数据结构组织成为层次结构，这是taichi的核心设计思想之一， 构成这些层次结构的元素是哈希，数组和变长数组，而它们的组合方法是随意， 为了便于编译器的优化，taichi规定层次结构不可以动态改变。

为了让算法使用的数据结构可以快速迭代，taichi将数据结构和算法解除掉耦合，这样的 话就可以在不修改的算法的前体现，切换使用不同的数据结构，从而比较不同的数据结构 的性能差别。所以，对于抽象的数据结构的索引方式，其总是采用多个下标索引，也即是 说，无论数据结构是如何构建的，从算法的角度，其访问方式都类似是多维数组。

taichi语言可以封装在C++中间，也可以封装到Python中间，其语言采用SPMD(Single Program Multiple Data)范式， 
在此基础上，其增加三个不同点
1. for 循环自动并行化，向量化 循环控制，除了for循环之外，其还提while等循环控制语句

```cpp
// Parallel loop over the sparse tensor "var"
For(Expr var, std::function)
// Loop over [begin, end)
For(Expr begin, Expr end, std::function)
// Access one element in "var" with index (i, ...)
While(Expr cond, std::function)
If(Expr cond)
If::Then(std::function)
If::Else(std::function)
```
2. 多维稀疏数组访问
```
// Access one element in "var" with index (i, ...)
operator[](Expr var, Expr i, ...)
```

3. 为编译器的优化提供显示的标注
```cpp
// For CPU
Parallelize(int num_threads) // Multi-threading
Vectorize(int width) // Loop vectorization
// For GPU
BlockDim(int blockDim) // Specify GPU block size
// For scratchpad optimization
AssumeInRange(Expr base, int lower, int upper)
Cache(Expr)
// Cache data into GPU L1 cache
CacheL1(Expr)
```

上面说明了在算法上提供的基本控制语句，下面讲解taichi是如何构建数据结构。 taichi构建的层次结构的数据结构，其构成的基本元素(称之为 node)为 dense, hash 和 dynamic 分别表示为定长数组，哈希表和不定长数组。并且对于这些基本元素，有相应的描述子， morton，bitmasked 和 pointer。

下一步就是构建这个利用node来构建层次结构， Global定义全局可以访问的节点，root表示层次的根节点，利用place来放置node

<使用Fig c> 说明

## taichi使用了技术来达成目标
将数据结构定义为层次结构的并不能的解决问题，因为多次遍历数据结构和直接使用数据结构的库相同， 还是存在大量的冗余的对于数据结构的多余的访问，而taichi使用编译的方法减少了这种多余的访问，其示意图如下:

<Fig 9> 说明。

taichi将访问分为两个过程，层级的切换，也就是进入到下一个层次，称之为 macro access， 而对于在层次内的访问，称之为 micro access。为了更加容易地翻译为后端，taichi自定义了中间表示层，其形式为 和LLVM相同，采用静态单赋值的方式表示。 taichi 为micor accss 在中间表示层定义语句为: OffsetAndExtractBit, SNodeLookup, GetCh, and IntegerOffset. OffsetAndExtractBit 用于计算在每一个维度的偏移量。 Linearize: 将多个维度的偏移量合并为一个索引。 SNodeLookup : 查询对应位置的节点是否存在。 GetCh: 获取该位置上的节点。 如果在编译的过程中间，发现两个micro指令访问的位置确定的，那么第二个访问就是没有必要使用 macro access， 而是使用 IntegerOffset 访问第二个即可。

除去上面介绍如何减少冗余的数据访问，taichi还使用各种常规的优化，例如公共子表达式消除， 死代码消除等，并且将优化划分为两个部分。

访问数据，希望尽可能在cache中间访问，而不是在内存中间，在taichi中间，可以使用Cache(v) 来将目标数据放到cache中间，但是，编译期间不一定总是可以确定放入cache的数据的大小， taichi提供了估计大小的语句，AssumeInRange。

对于密集数据，只要平均划分数据，就可以让每一个worker的负载程度相同，但是稀疏数据按照平均划分，很有可能 是部分worker处于繁忙，部分是处于无事可做的状态。 对于CPU，可以首先对于数据进行轻量级的遍历，将任务放到队列中间，然后利用OpenMP进行并行化操作。 对于GPU，维护多个任务队列，并且一层层的生成。

taichi适合处理的任务存在大量的内存管理的操作，使用了两个优化。第一个是对于每一个node， 其使用其定制的内存分配器，这样，每一个分配器分配的大小都是确定，从而简化了和加速了内存分配。 此外，taichi在运行的时候，首先会分配充分大的虚拟地址空间，而数据在真正使用需要再加载。

最后，一个技术是taichi在CPU上对于向量操作的支持。

## 效果
原论文中间一共提供了5个案例，涉及到物理模拟，渲染，3D深度学习， 总结来看，taichi可以复现这些算法，其代码量只有参考的十分之一，但是速度缺失参考的2.82倍， 在一些案例中间，作者开始是没有办法获取良好的结果，但是得益于taichi将数据结构和算法的解耦合， 可以快速尝试新的数据结构，从而实现性能的提升

## 总结
