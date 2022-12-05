## Notes
- https://www.bilibili.com/video/BV1Za41177L2
- https://ipads.se.sjtu.edu.cn/mospi/

操作系统研究受到上层应用和底层硬件双重驱动:
- 互联网、网络搜索、大数据、人工智能、智能驾驶、云计算等
- 持久性内存、GPU、智能网卡、Al芯片、硬件Enclave等

**如何向上更好的抽象硬件给上层接口**。

操作系统 8 个前沿领域:
- 异构操作系统；
- 新的应用接口；
- 同步原语；
- 持久性内存；
- 智能网卡；
- 系统安全；
- 操作系统测试；
- 形式化验证；

GPU 访问文件系统，现有的模型下是需要 CPU 的辅助的；

vhost 是一个例子，如何将 control plain 和 data plain 分离开.
用类似的方法，让 GPU 访问文件系统。

挑战:OS如何使用智能网卡优化网络应用?
- 方向一:卸载部分应用逻辑 KV-Direct (SOSP'17),NetCache (SOSP'17) NICA(ATC'19),E3(ATC'19), iPipe (SIGCOMM'19)
- 方向二:卸载操作系统功能 卸载网络虚拟化功能(NSDI'18) 卸载部分网络协议栈功能: AccelTCP (NSDI20)

形式化证明的案例
* 操作系统
  - seL4，首个形式化验证的微内核，SOSP'09最佳论文-
  - CertikOS，耶鲁大学开发的支持并发的内核[OSDI'16]
* 文件系统
  - FScQ，具有崩溃安全性的串行文件系统[SOSP'15]-
  - AtomFS，接口具有原子性的并发文件系统[SOSP'19]。
* 分布式协议
  * Amazon，广泛应用TLA+用于保障软件协议设计的正确性
  * 证明分布式共识协议Paxos与Raft联系与优化迁移[PODC'19]

[^1]: https://github.com/GooTal/Notes
