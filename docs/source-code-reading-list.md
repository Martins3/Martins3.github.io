# system programming project reading
受 [7days-golang](https://github.com/geektutu/7days-golang) 启发。

## 为什么要阅读源码
之前简单地阅读了 Linux 和 QEMU ，学习到了很多东西:
- [Linux](https://github.com/Martins3/Martins3.github.io/blob/master/docs/kernel)
- [QEMU](https://github.com/Martins3/Martins3.github.io/blob/master/docs/qemu)

我认为阅读源码是学习水平中不可获取的一环:
- 教科书只是总结了最核心的原理，没有实战，其中的奥妙总是难以体会
- 文档只会分析其中关键结果，只有骨架，没有血肉

我自己认为的阅读方法:
- 显然代码跑起来，要读活代码，不要读死代码
- 文档和代码交替进行
  - 文档枯燥无聊，代码有时候晦涩难懂，如果在代码看不下去的时候阅读文档，那么就会感觉文档是雪中送碳，在枯燥的文档阅读之后，看看代码，如沐春风。
  - 首先将问题列举出来，带着问题去阅读，而不是为了阅读而阅读。

当然，最终的目的是将别人的优秀技术运用在自己的项目中，最好是有所改进。

## 我的阅读列表

## 编译
- [qbe](https://github.com/Martins3/Martins3.github.io/blob/master/compiler/qbe.md) : LLVM 对于我来说已经过于庞大了
- [wren](https://github.com/Martins3/Martins3.github.io/blob/master/compiler/wren.md) : 一直想要搞一个 vm, 每次听到别人谈论 v8, JVM 虚拟机的时候都非常的向往。
- [mold](https://github.com/rui314/mold) : 代码量非常的小，配合[教程学习](https://eli.thegreenplace.net/tag/linkers-and-loaders) 应该是不错的
- [chibicc](https://github.com/rui314/chibicc) : 支持 C11 编译器
- [lua](https://www.lua.org/source/) : 大名鼎鼎的 lua 语言，被广泛的使用，其代码量只有 10000 多行。

## 操作系统
- [zircon](https://github.com/Martins3/Martins3.github.io/blob/master/os/zicron/zicron_overview.md)
- [skift](https://github.com/skiftOS/skift) : 两万行 C++ 构建的操作，直接 image viewer 之类的

## 虚拟化
- [gvisor](https://github.com/Martins3/Martins3.github.io/blob/master/hack/kvm/gvisor.md)
- [firecracker](https://github.com/Martins3/Martins3.github.io/blob/master/hack/kvm/hypervisor/firecracker.md)
- kvmtool

## 分布式
暂无

## 其他
- [eomaia](https://github.com/Martins3/Martins3.github.io/blob/master/net/eomaia.md) : 很小的项目，用于学习 Modern 的 C++ 的书写风格
- [musl](./linux/musl.md) : 大名鼎鼎的 musl 库，写的非常清晰
