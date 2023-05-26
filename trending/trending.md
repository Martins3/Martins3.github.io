# How to kept myself informed ?

- [ ] 李存的找文章的几个网站

## 订阅的邮件列表

- kernel
- kernel newbie
- QEMU

- [ ] 更好的邮件阅读器?
- [ ] 重新搭建一个 zotero，在 mac 上

## 等待分析
- https://news.ycombinator.com/item?id=22902204
- https://github.com/Wenzel/awesome-virtualization#papers

## LPC
- https://www.youtube.com/watch?v=U6HYrd85hQ8

暂时都没有搞清楚这个东西到底是做什么的?

## KVM Forum
- youtube : https://www.youtube.com/channel/UCRCSQmAOh7yzgheq-emy1xA

这是正确的资源入口吗?
- https://events.linuxfoundation.org/archive/2020/kvm-forum/

可以将这种分散的合并起来吗?
- https://people.redhat.com/~aarcange/slides/2019-KVM-monolithic.pdf

## CCF 会议
- https://ccfddl.github.io/

- [ ] Data center 相关的会议也是需要的

### 计算机体系结构/并行与分布计算/存储系统
[计算机体系结构/并行与分布计算/存储系统](https://www.ccf.org.cn/Academic_Evaluation/ARCH_DCP_SS/)

- [ASPLOS](https://dblp.uni-trier.de/db/conf/asplos/index.html)


### 软件工程/系统软件/程序设计语言
[软件工程/系统软件/程序设计语言](https://www.ccf.org.cn/Academic_Evaluation/TCSE_SS_PDL/)

### 计算机网络
- [计算机网络](https://www.ccf.org.cn/Academic_Evaluation/CN/)

## Linux Storage, Filesystem, Memory-Management, and BPF Summit
- [2022](https://lwn.net/Articles/893733/)

## Linux plumber conference
https://www.youtube.com/watch?v=U6HYrd85hQ8&t=1475s

## ebpf submit
- https://ebpf.io/summit-2022/

## gallery
- https://github.com/facundoolano/software-papers : 各个领域的经典论文

## hotchip


## 暂时无法分类
###  ACM SIGARCH Computer Architecture News
> ????

We investigate why uncooperative swapping degrades
performance in practice and find that it is largely because of:
(1) “silent swap writes” that copy unchanged blocks of data
from the guest disk image to the host swap area; (2) “stale
swap reads” triggered when guests perform explicit disk
reads whose destination buffers are pages swapped out by
the host; (3) “false swap reads” triggered when guests overwrite whole pages previously swapped out by the host while
disregarding their old content (e.g., when copying-on-write);
(4) “decayed swap sequentiality” that causes unchanged
guest file blocks to gradually lose their contiguity while being kept in the host swap area and thereby hindering swap
prefetching; and (5) “false page anonymity” that occurs
when mislabeling guest pages backed by files as anonymous
and thereby confusing the page reclamation algorithm. We
characterize and exemplify these problems in Section 3.

- [ ] 这是没有 swap 的出现问题的几种原因

- [ ] 这里介绍了两种方法来解决

- 最好的是将两种方法都来搞一下。
- 没有 balloon 的一个问题:
  - 如果 host 将 guest 中 swap 的内存，guest 重新 swap 出去，这导致 host 需要将这个数据重新读回来。

## Binary Translation
- [Optimizing Memory Translation Emulation in Full System Emulators](https://dl.acm.org/doi/pdf/10.1145/2686034) :star:
- [Acceleration of memory accesses in dynamic binary translation](https://tel.archives-ouvertes.fr/tel-02004524/document) :star:
- [Hardware-Accelerated Cross-Architecture Full-System Virtualization](https://dl.acm.org/doi/10.1145/2996798) :star:
- [A Retargetable System-Level DBT Hypervisor](https://www.usenix.org/conference/atc19/presentation/spink)

Kind of tricky to read without preliminary knowledge about GenSim.

- [Efficient Cross-architecture Hardware Virtualisation](https://era.ed.ac.uk/handle/1842/25377)

- [Reducing World Switches in Virtualized Environment with Flexible Cross-world Calls](https://trustkernel.com/uploads/pubs/CrossOver_ISCA2015.pdf)

## classic papers
- [The UNIX Time-Sharing System](https://chsasank.github.io/classic_papers/unix-time-sharing-system.html)
- https://meltdownattack.com/

## Gallery
- https://www.zhihu.com/column/yxg-paper-intro
- https://www.youtube.com/watch?v=zyuMwIza4Pw&feature=youtu.be
- [论文阅读笔记（分布式，虚拟化，机器学习）](https://github.com/dyweb/papers-notebook/issues)
- [alastairreid's reading list](https://alastairreid.github.io/RelatedWork/papers/)
- [micahlerner](https://www.micahlerner.com/2021/12/28/ghost-fast-and-flexible-user-space-delegation-of-linux-scheduling.html)
- [System/Networking](https://github.com/Romero027/sysnet-reading-list)
- [计算机系统会议论文](https://www.zhihu.com/column/c_1424714267832967169)
- [存储（分布式、存储引擎等）领域论文阅读笔记索引](https://github.com/lichuang/storage-paper-reading-cn)
- [虚拟化，共有云](https://github.com/liujunming/paper_reading_notes/issues)

## Cloud / Virtualization
- [Alita: Comprehensive Performance Isolation through Bias Resource Management](https://mp.weixin.qq.com/s/S0lvODk2fe91AxWyMACgEQ)
- [OSv—Optimizing the Operating System for Virtual Machines](https://www.usenix.org/conference/atc14/technical-sessions/presentation/kivity)
- [A Linux in unikernel clothing](https://dl.acm.org/doi/10.1145/3342195.3387526)

之前的 unikernel 就是重新开发的，而作者通过内核的裁剪和 KML 将 Linux 变为一个 unikernel 了

## How kernel implements
- https://www.usenix.org/legacy/events/osdi10/tech/full_papers/Ben-Yehuda.pdf
- https://kernel.dk/systor13-final18.pdf
- https://www.kernel.org/doc/ols/2007/ols2007v1-pages-225-230.pdf

## os
- https://github.com/theseus-os/Theseus
- https://github.com/demikernel/demikernel
    - Demikernel architecture offers a uniform system call API across kernel-bypass technologies (e.g., RDMA, DPDK) and OS functionality (e.g., a user-level networking stack for DPDK).

## compiler
- https://zhuanlan.zhihu.com/p/336543238

## TODO
- https://www.usenix.org/system/files/osdi18-shan.pdf

## fs
- [使用 Rust 开发文件系统](http://blog.jcix.top/2021-04-10/bentofs/)

## 其他
- [How to circumvent Sci-Hub ISP block](https://fragile-credences.github.io/scihub-proxy/)
- [如何阅读学术论文](https://deardrops.github.io/post/how-to-read-academic-papers/)
- [体系结构，操作系统相关学术资源](https://github.com/rajesh-s/computer-engineering-resources) :star:
- [readpaper](https://readpaper.com/) :star:


## 用上 AI
- https://chatpaper.org/
- https://www.chatpdf.com/

## 读博经验
https://github.com/pengsida/learning_research
