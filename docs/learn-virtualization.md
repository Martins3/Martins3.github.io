# 虚拟化学习的一点经验之谈
我是首先了解了一点内核再去学习虚拟化的，总体来说，虚拟化还是一个很有趣的方向，而且在云计算等产业中也有很强的应用。

## 教材
Hardware and Software Support for Virtualization : 作者[Edouard Bugnion](https://en.wikipedia.org/wiki/Edouard_Bugnion) 是 VMware 的创始人之一，绝对的经典。
(虽然我还没有看)


## 参考书籍
[深度探索 Linux 系统虚拟化](https://book.douban.com/subject/35238691/) 是基于 kvmtool 的。kvmtool 移除掉 QEMU 中对于二进制翻译的支持，也不支持设备的模拟，其定位是 microvm， 所以代码量很小，而且很整洁，
这本书讲解也更加的清晰，我推荐这本书作为开始。

[QEMU/KVM源码解析与应用](https://book.douban.com/subject/35324337/)
总体来说是作者自己的 [blog](https://terenceli.github.io/) 的整理，是基于 QEMU 的。kvmtool 实际上在工业上的使用并不多，学习使用 QEMU 是必须的，
但是 QEMU 更加复杂，市面上这是唯一一本比较详细的分析 QEMU 的书籍(中文和英文范围内)。但是这本书也有缺点，那就是不过凝练，很多时候都是代码流程的分析。

## 一些补充资料
- [cpu internals](https://github.com/LordNoteworthy/cpu-internals) 是 intel SDM 的笔记，其中覆盖了 intel CPU 在硬件上如何支持虚拟化的, 阅读内核的 kvm 最好阅读一下。
- [ASPLOS IOMMU tutorial](http://pages.cs.wisc.edu/~basu/isca_iommu_tutorial/IOMMU_TUTORIAL_ASPLOS_2016.pdf) 介绍了 IOMMU，学习 vt-d 之前可以阅读一下。

## 一些有用的 blog
- https://kernelgo.org/
- https://terenceli.github.io/
- https://www.linux-kvm.org/page/KVM_Forum : kvm forum 每年的会议 slides 都是精华，值得一个个的翻阅

