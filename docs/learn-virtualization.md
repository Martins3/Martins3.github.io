# 虚拟化学习的一点经验之谈
我是首先了解了一点内核再去学习虚拟化的，总体来说，虚拟化还是一个很有趣的方向，而且在云计算等产业中也有很强的应用。
因为自身的水平原因，这里的教程主要侧重于 kvm 相关的东西。

## 教材
Hardware and Software Support for Virtualization : 作者[Edouard Bugnion](https://en.wikipedia.org/wiki/Edouard_Bugnion) 是 VMware 的创始人之一，绝对的经典。
(虽然我还没有看)

## 参考书籍
[深度探索 Linux 系统虚拟化](https://book.douban.com/subject/35238691/) 是基于 kvmtool 的。kvmtool 移除掉 QEMU 中对于二进制翻译的支持，也不支持设备的模拟，其定位是 microvm， 所以代码量很小，而且很整洁，
这本书讲解也更加的清晰，我推荐这本书作为开始。

[QEMU/KVM 源码解析与应用](https://book.douban.com/subject/35324337/)
总体来说是作者自己的 [blog](https://terenceli.github.io/) 的整理，是基于 QEMU 的。kvmtool 实际上在工业上的使用并不多，学习使用 QEMU 是必须的，
但是 QEMU 更加复杂，市面上这是唯一一本比较详细的分析 QEMU 的书籍(中文和英文范围内)。但是这本书也有缺点，那就是不过凝练，很多时候都是代码流程的分析。

## minimal kvm example
- [LWN: Using the KVM API](https://lwn.net/Articles/658511/)
- https://github.com/dpw/kvm-hello-world
- [Learning KVM - implement your own kernel](https://david942j.blogspot.com/2018/10/note-learning-kvm-implement-your-own.html)
  - [源码](https://github.com/david942j/kvm-kernel-example)
- [kvmsample](https://github.com/soulxu/kvmsample) : 几百行，guest 就是一个三行的汇编
- [kvmtool](https://github.com/kvmtool/kvmtool) : 基本不怎么维护, 大约 20000 行，其实也不小，但是和 QEMU 比起来还是很 minimal 的。

## minimal hypervisor / virtual machine example
- https://github.com/calinyara/asor
- https://github.com/ionescu007/SimpleVisor : Window Driver 或者 UEFI driver 方式执行的 hypervisor
- [Write your Own Virtual Machine](https://justinmeiners.github.io/lc3-vm)
- [mvisor](https://github.com/78/mvisor) : 一个 QEMU 的 mini 替代，可以运行 Windows 的图形界面

## 一些补充资料
- [cpu internals](https://github.com/LordNoteworthy/cpu-internals) 是 intel SDM 的笔记，其中覆盖了 intel CPU 在硬件上如何支持虚拟化的, 阅读 kvm 的时候可以配合这阅读一下。
- [ASPLOS IOMMU tutorial](http://pages.cs.wisc.edu/~basu/isca_iommu_tutorial/IOMMU_TUTORIAL_ASPLOS_2016.pdf) 介绍了 IOMMU，学习 vt-d 之前可以阅读一下。

## 一些有用的 blog
- [KVM forum](https://www.linux-kvm.org/page/KVM_Forum) :star: :star: :star: kvm forum 每年的会议 slides 都是精华
- [kernelgo](https://kernelgo.org/) :star: :star:
  - [作者整理的 gitbook](https://luohao-brian.gitbooks.io/interrupt-virtualization/content/qemuzhi-network-device-quan-xu-ni-fang-an-4e8c3a-xu-ni-wang-qia.html)
- [terenceli](https://terenceli.github.io/) :star:
  - 值得一个个的翻阅, 在 youtube 上有对应的[录像](https://www.youtube.com/channel/UCRCSQmAOh7yzgheq-emy1xA)
- [Redhat blog](https://www.redhat.com/en/blog) 的 virtualization 板块
  - [All you need to know about KVM userspace](https://www.redhat.com/en/blog/all-you-need-know-about-kvm-userspace)
- [陈伟任的笔记](https://github.com/azru0512/slide/tree/master/QEMU)
- [Stefan Hajnoczi](http://blog.vmsplice.net/2020/08/qemu-internals-event-loops.html)
- [opengers](https://opengers.github.io/) : kvm ceph 之类的
- [mpolednik](https://mpolednik.github.io/) : redhat 工程师，vfio 之类的
- [stefano garzarella](https://stefano-garzarella.github.io/) : redhat 工程师
- [知乎专栏 : 河马虚拟化](https://www.zhihu.com/column/c_1040263672760885248)
- [知乎专栏 : 虚拟化笔记](https://www.zhihu.com/column/huiweics)
- [kraxel](https://www.kraxel.org/blog/) : 主要关于 GPU 之类的
- [空客的 QEMU 源码分析](https://github.com/airbus-seclab/qemu_blog)

## 一些有意思的项目
- [VivienneVMM](https://github.com/changeofpace/VivienneVMM) : 使用 vmx 实现的调试器

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
