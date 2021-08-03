## Binary Translation
- [Optimizing Memory Translation Emulation in Full System Emulators](https://dl.acm.org/doi/pdf/10.1145/2686034) :star:
- [Acceleration of memory accesses in dynamic binary translation](https://tel.archives-ouvertes.fr/tel-02004524/document) :star:
- [Hardware-Accelerated Cross-Architecture Full-System Virtualization](https://dl.acm.org/doi/10.1145/2996798) :star:
- [A Retargetable System-Level DBT Hypervisor](https://www.usenix.org/conference/atc19/presentation/spink)

Kind of tricky to read without preliminary knowledge about GenSim.

- [Efficient Cross-architecture Hardware Virtualisation](https://era.ed.ac.uk/handle/1842/25377)
This is Tom Spink's doctor thesis.

- [Acceleration of memory accesses in dynamic binary translation](http://tima.univ-grenoble-alpes.fr/publications/files/th/2018/2018_10_22_FARAVELON_Antoine_0476.pdf)

- [Efficient memory virtualization for cross-isa system mode emulation](http://vee2014.cs.technion.ac.il/docs/VEE14-present25.pdf)
- [Hspt: Practical implementation and efficient management of embedded shadow page tables for cross-isa system virtual machines](https://zhexwang.github.io/papers/hspt.pdf)

- [Reducing World Switches in Virtualized Environment with Flexible Cross-world Calls](https://trustkernel.com/uploads/pubs/CrossOver_ISCA2015.pdf) 
- [HQEMU](http://csl.iis.sinica.edu.tw/hqemu/)

- MagiXen: Combining Binary Translation and Virtualization

## classic papers
- [The UNIX Time-Sharing System](https://chsasank.github.io/classic_papers/unix-time-sharing-system.html)
- https://meltdownattack.com/

## Gallery
- https://www.zhihu.com/column/yxg-paper-intro
- https://www.youtube.com/watch?v=zyuMwIza4Pw&feature=youtu.be
- [论文阅读笔记（分布式，虚拟化，机器学习）](https://github.com/dyweb/papers-notebook/issues)


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
