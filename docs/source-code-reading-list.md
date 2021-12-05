# 7 days system programming project
inspired by https://github.com/geektutu/7days-golang

最近(研一/研二)因为工作需要一直在阅读 Linux 和 Qemu 的源码，但是在学习过程中间，也是找到了一些其他的非常有意思的代码,下面收集了一些我感觉值得阅读的一些代码。
这些代码主要都是一些短小精悍，但是强于 toy 级别的，大约一周的时间可以上手的。

## 编译
- [qbe](https://github.com/Martins3/Martins3.github.io/blob/master/compiler/qbe.md) : LLVM 对于我来说已经过于庞大了
- [wren](https://github.com/Martins3/Martins3.github.io/blob/master/compiler/wren.md) : 一直想要搞一个 vm, 每次听到别人谈论 v8, JVM 虚拟机的时候都非常的向往。
- [mold](https://github.com/rui314/mold) : 代码量非常的小，配合[教程学习](https://eli.thegreenplace.net/tag/linkers-and-loaders) 应该是不错的
- [chibicc](https://github.com/rui314/chibicc) : C11 编译器
- [lua](https://www.lua.org/source/) : 大名鼎鼎的 lua 语言，被广泛的使用，其代码量只有 10000 多行。

## 操作系统
- [zircon](https://github.com/Martins3/Martins3.github.io/blob/master/os/zicron/zicron_overview.md)

## 虚拟化
- [gvisor](https://github.com/Martins3/Martins3.github.io/blob/master/hack/kvm/gvisor.md)
- [firecracker](https://github.com/Martins3/Martins3.github.io/blob/master/hack/kvm/hypervisor/firecracker.md)
- kvmtool

## 分布式
暂无

## 数据库
- [leveldb](https://www.qtmuniao.com/2020/07/03/leveldb-data-structures-skip-list/) :  C++ 数据库 Jeff Dean 1000 行 :star:
    - https://medium.com/databasss/on-disk-io-part-1-flavours-of-io-8e1ace1de017 : 数据库教程

## 其他
- [eomaia](https://github.com/Martins3/Martins3.github.io/blob/master/net/eomaia.md) : 很小的项目，用于学习 Modern 的 C++ 的书写风格
- [musl](https://github.com/Martins3/Martins3.github.io/blob/master/unix/musl.md) : 大名鼎鼎的 musl 库，写的非常清晰
