# system programming project reading
受 [7days-golang](https://github.com/geektutu/7days-golang) 启发。

## 为什么要阅读源码
阅读了一些源码，学习到了很多东西:
- [Linux](https://github.com/Martins3/Martins3.github.io/blob/master/docs/kernel)
- [QEMU](https://github.com/Martins3/Martins3.github.io/blob/master/docs/qemu)

我认为阅读源码是学习水平中不可获取的一环:
- 教科书只是总结了最核心的原理，没有实战，其中的奥妙总是难以体会
- 文档只会分析其中关键结果，只有骨架，没有血肉

代码的阅读可以尽量的广泛。从编译器，数据库，操作系统，CPU / GPU 都是可以涉猎一下，
一个完全不同的领域可以改变你对于计算机的认识。

我自己认为的阅读方法:
- 显然代码跑起来，要读活代码，不要读死代码
- 文档和代码交替进行
  - 文档枯燥无聊，代码有时候晦涩难懂，如果在代码看不下去的时候阅读文档，那么就会感觉文档是雪中送碳，在枯燥的文档阅读之后，看看代码，如沐春风。
  - 首先将问题列举出来，带着问题去阅读，而不是为了阅读而阅读。

当然，最终的目的是将别人的优秀技术运用在自己的项目中，最好是有所改进。

## [ ] 总结一下阅读方法
- uftrace 之类的
- 内核和用户态的分别都需要一个
- 总结两边都可以使用的

## 我的待阅读列表

## 编译
- [qbe](https://github.com/Martins3/Martins3.github.io/blob/master/compiler/qbe.md) : LLVM 对于我来说已经过于庞大了
- [chibicc](https://github.com/rui314/chibicc) : 支持 C11 编译器
- [lua](https://www.lua.org/source/) : 大名鼎鼎的 lua 语言，被广泛的使用，其代码量只有 10000 多行。

## 操作系统
- [zircon](https://github.com/Martins3/Martins3.github.io/blob/master/os/zicron/zicron_overview.md)
- [skift](https://github.com/skiftOS/skift) : 两万行 C++ 构建的操作，直接 image viewer 之类的

## 虚拟化
- [gvisor](https://github.com/Martins3/Martins3.github.io/blob/master/hack/kvm/gvisor.md)
- [firecracker](https://github.com/Martins3/Martins3.github.io/blob/master/hack/kvm/hypervisor/firecracker.md)
- kvmtool

## 数据库
- [leveldb](https://github.com/google/leveldb)
- sqlite

## 图形栈
- [glfw](https://github.com/glfw/glfw) : 只有两三个文件，大约 2 万行，但是支持各种后端，有很多语言的 binding
- [imgui](https://github.com/ocornut/imgui) : 没有太搞清楚这个库的定位

## 分布式
- [toydb](https://github.com/erikgrinaker/toydb)

## 其他
- [eomaia](https://github.com/Martins3/Martins3.github.io/blob/master/net/eomaia.md) : 很小的项目，用于学习 Modern 的 C++ 的书写风格
- [musl](./linux/musl.md) : 大名鼎鼎的 musl 库，写的非常清晰

## 浏览器
- https://limpet.net/mbrubeck/2014/08/08/toy-layout-engine-1.html

## 语言虚拟机
- lua

## 区块链

## Linker
- [mold](https://github.com/rui314/mold) : 配合[教程学习](https://eli.thegreenplace.net/tag/linkers-and-loaders) 应该是不错的

## 神经网络
- pytorch: https://zhuanlan.zhihu.com/p/34629243
  - 似乎，实际上，如何使用还是一个问题

## 搜索引擎
- 似乎完全没有概念

## JVM
- https://github.com/doocs/jvm
