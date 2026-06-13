# Linux 内核如何随着 AI 来演进

## 目前看，是削弱的
我还是认为，内核中最复杂的事情就是关于内存和多核(perfbook)
所以，目前看，当中心转向到了 GPU ，内核相对来说，是被削弱的

### 计算

#### 虚拟化
那是真的没有关系，笑死!

如果你去看看 intel 手册，会发现 system programing 有很大的
一部分都是关于虚拟化的。

但是到了 GPU 时代，尤其是大模型时代，这些东西不在重要，
甚至都是累赘，例如 iommu 会干扰 gpu direct memory 。

相比之下， vdi 的市场也是非常小的

#### DRM
Linux 内核与 AI infra 的关联，总体而言其实不深。DRM 子系统虽在内核中沉淀了大量图形相关代码，但传统图形显示与 GPU 通用计算实际上是清晰切分的——前者关注像素如何在屏幕上呈现，后者关注如何利用并行能力加速数值计算，二者只是恰好共用 GPU 硬件。驱动本身也不是操作系统的竞争重点，NVIDIA 已将 GPU 驱动开源，整个技术栈的核心在于如何用好 GPU，而非驱动层面的实现。

#### 面向 CPU 的通用设计
更深层次的原因在于工作负载的结构性变化。传统 CPU 和操作系统面向极度多样化的场景：编译器、大数据、容器、虚拟化，甚至虚拟机里再嵌套容器。而大模型时代，所有人的底层计算需求高度收敛到了一件事：矩阵乘法。为了将其做到极致，CPU 已经变成了 GPU 的模样，操作系统也完全可能演化为一种围绕 GPU 设计的专用系统，而非今天这种通用资源管理者。

### 存储

GPU 统一内存、异构内存管理等功能确实需要在内核中实现。但客观来说，这些大多属于数据路径中的低速路径，真正的高速路径——矩阵乘法的执行、张量核心的调度——完全由 GPU 上的运行时直接处理，内核几乎感知不到。

此外，P2P 内存访问、GPUDirect Storage（CUDA FS）等技术与内核的耦合是真实且紧密的，是构建大规模高性能 AI 集群的基石，这一点无法否认。

类似 pnfs 来做分布式存储

### 网络

RDMA 是全村的希望了，网络其他的部分是真用不上了。

## 内核是不断演化的
内核是不断演化的，只是之前 kernel 的中心在云计算和云原生上，
所以大家就发现 kvm fuse iouring cgroup bpf 之类的发展的很快。

## 对人的要求是一样的

抛开技术栈的变迁，对人的核心要求从未改变。做内核需要深入问题本质，写代码时充分考虑边界条件、并发安全和性能，把事情做到极致。做 AI infra 同样需要这种严谨和钻研。

内核背景在理解 AI infra 时意外地有价值。借助 AI 辅助阅读 NVIDIA 技术文档时，过往在驱动异步模型、Firmware 交互、事件通知等方面的经验，能让人迅速理清其设计意图。GPU 编程中的流、事件、异步拷贝等概念，本质上与操作系统中的并发控制和 I/O 调度并无二致。

因此，一个人应当先成为优秀的工程师，再成为优秀的内核工程师，而不是反过来。内核经验的价值不在于头衔，而在于它塑造的解决问题的方式。无论是优化一个内核调度器，还是优化分布式训练中的通信，背后都是同一种追求极致的工程态度。

## 待补充内容
Swarm : https://www.99csw.com/book/10133/364677.htm

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
