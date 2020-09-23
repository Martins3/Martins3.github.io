# 深入浅出SSD
https://github.com/spdk/spdk : 配合 dpdk 了解一下，还可以分析一下 build your own x 的 tcp stack 的构建过程

https://github.com/linux-rdma/rdma-core
  - https://www.youtube.com/watch?v=QAIAoNheX-8 : RDMA 的想法应该很简单吧!

https://link.springer.com/chapter/10.1007/978-94-017-7512-0_10

## Introduction
·FTL闪存转换层：作为SSD固件的核心部分，FTL实现了例如映射管理、磨损均衡、垃圾回收、坏块管理等诸多功能，本书将一一介绍。
·NAND Flash：NAND Flash作为SSD的存储介质，具有很多与传统磁介质不同的特性，本书将从器件原理、实战指南、闪存特性及数据完整性等方面展开。
·NVMe存储协议：作为专门为SSD开发的软件存储协议，NVMe正在迅速占领SSD市场。本书将从其优势、基础架构、寻址方式、数据安全等方面展开。为了让读者对NVMe命令处理有更加直观的认识，本书结合实际的PCIe trace进行阐述。同时，本书也介绍了NVMe Over Fabric的相关知识，让读者能够对未 […]
·PCIe协议：PCIe作为目前主流的SSD前端总线，与之前的SATA接口相比有着极大的性能优势。本书将从PCIe总线拓扑结构、分层结构、TLP类型与路由、配置和地址空间等方面进行介绍。
·电源管理：本书详述了SSD前端总线（包括SATA和PCIe）的各种节能模式、NVMe协议的电源管理方案及在SSD里常用的整体电源管理架构——Power Domain。
·ECC：本书介绍了ECC的基本概念，重点介绍了LDPC的解码和编码原理，以及在NAND上的应用。

SSD是用固态电子存储芯片阵列制成的硬盘，主要部件为控制器和存储芯片，内部构造十分简单。详细来看，SSD硬件包括几大组成部分：主控、闪存、缓存芯片DRAM（可选，有些SSD上可能只有SRAM，并没有配置DRAM）、PCB（电源芯片、电阻、电容等）、接口（SATA、SAS、PCIe等），其主体就是一块PCB，如图1-3所示。软件角度，SSD内部运行固件（Firmware，FW）负责调度数据从接口端到介质端的读写，还包括嵌入核心的闪存介质寿命和可靠性管理调度算法，以及其他一些SSD内部算法。SSD控制器、闪存和固件是SSD的三大技术核心

SSD通过诸如SATA、SAS和PCIe等接口与主机相连，实现对应的ATA、SCSI和NVMe等协议
> 补充一下表格 1-5

主要有以下几种原因导致：
- 擦写磨损（P/E Cycle）。
- 读取干扰（Read Disturb）。
- 编程干扰（Program Disturb）。
- 数据保持（Data Retention）发生错误。

- UBER：Uncorrectable Bit Error Rate，不可修复的错误比特率。
- RBER：Raw Bit Error Rate，原始错误比特率。
- MTBF：Mean Time Between Failure，平均故障间隔时间。

SSD上电加载后，主机BIOS开始自检，主机中的BIOS作为第一层软件和SSD进行交互：第一步，和SSD发生链接，SATA和PCIe走不同的底层链路链接，协商（negotiate）到正确的速度上（当然，不同接口也会有上下兼容的问题），自此主机端和SSD连接成功；第一步，发出识别盘的命令（如SATA Identify）来读取盘的基本信息，基本信息包括产品part number、FW版本号、产品版本号等，BIOS会验证信息的格式和数据的正确性，然后BIOS会走到第三步去读取盘其他信息，如SMART，直到BIOS找到硬盘上的主引导记录MBR，加载MBR；第四步，MBR开始读取硬盘分区表DPT，找到活 动分区中的分区引导记录PBR，并且把控制权交给PBR……最后，SSD通过数据读写功能来完成最后的OS加载。完成以上所有这些步骤就标志着BIOS和OS在SSD上电加载成功。任何一步发生错误，都会导致SSD交互失败，进而导致系统启动失败，弹出Error window或蓝屏。

## 主控
主控是SSD的大脑，承担着指挥、运算和协调的作用，具体表现在：一是实现标准主机接口与主机通信；二是实现与闪存的通信；三是运行SSD内部FTL算法。


## Flash
一个Block当中的所有这些存储单元都是共用一个衬底的。

一个Wordline对应着一个或若干个Page，具体是多少取决于是SLC、MLC或者TLC。对SLC来说，一个Wordline对应一个Page；MLC则对应2个Page，这两个Page是一对（Lower Page和Upper Page);

- 闪存坏块
- 要注意的是，读干扰影响的是同一个闪存块中的其他闪存页，而非读取的闪存页本身。
- 写干扰影响的不仅是同一个闪存块当中的其他闪存页，自身闪存页也会受到影响。相同的是，都会因不期望的轻微写导致比特翻转，都会产生非永久性损伤，经擦除后，闪存块还能再次使用
- 存储单元间的耦合
- 电荷泄露


MLC 可以包含两个 bit 的信息，对应 lower page 和 uppper page

读干扰会导致浮栅极进入电子。由于有额外的电子进入，会导致晶体管阈值电压右移（Data Retention问题导致阈值电压左移）

## PCIe
PCI使用并口传输数据，而PCIe使用的是串口传输

PCI采用的是总线型拓扑结构，一条PCI总线上挂着若干个PCI终端设备或者PCI桥设备，大家共享该条PCI总线，哪个人想说话，必须获得总线使用权，然后才能发言
> 图5.6 和 5.7 很有，补充一下

Root Complex（RC）是树的根，它为CPU代言，与整个计算机系统其他部分通信，比如CPU通过它访问内存，通过它访问PCIe系统中的设备。 RC的内部实现很复杂，PCIe Spec也没有规定RC该做什么，不该做什么。我们也不需要知道那么多，只需清楚：它一般实现了一条内部PCIe总线（BUS 0），以及通过若干个PCIe bridge，扩展出一些PCIe Port，如图5-8所示。

PCIe定义了下三层：事务层（Transaction Layer）、数据链路层（Data Link Layer）和物理层（Physical Layer，包括逻辑子模块和电气子模块）

事务层的主要职责是创建（发送）或者解析（接收）TLP（Transaction Layer Packet）、流量控制、QoS、事务排序等。
数据链路层的主要职责是创建（发送）或者解析（接收）DLLP（Data Link Layer Packet）、Ack/Nak协议（链路层检错和纠错）、流控、电源管理等。
物理层的主要职责是处理所有的Packet数据物理传输，发送端数据分发到各个Lane传输（Stripe），接收端把各个Lane上的数据汇总起来（De-stripe），每个Lane上加扰（Scramble，目的是让0和1分布均匀，去除信道的电磁干扰EMI）和去扰（De-scramble），以及8/10或者128/130编码解码等。

Switch的主要功能是转发数据，为什么还需要实现事务层？Switch必须实现这三层，因为数据的目的地信息是在TLP中的，
如果不实现这一层，就无法知道目的地址，也就无法实现数据寻址路由。

根据软件层的不同请求，事务层产生四种不同的TLP请求：
- Memory；
- IO；
- Configuration；
- Message : 中断 错误 电源管理


每个PCIe设备都有这样一段空间，主机软件可以通过读取它获得该设备的一些信息，也可以通过它来配置该设备，这段空间就称为PCIe的配置空间。 整个配置空间就是一系列寄存器的集合，由两部分组成：64B的Header和192B的Capability数据结构。

**具体实现就是上电的时候，系统把PCIe设备开放的空间（系统软件可见）映射到内存地址空间，CPU要访问该PCIe设备空间，只需访问对应的内存地址空间。RC检查该内存地址，如果发现该内存空间地址是某个PCIe设备空间的映射，就会触发其产生TLP，去访问对应的PCIe设备，读取或者写入PCIe设备。**


## FTL
- 每个闪存块读的次数是有限的，读得太多了，上面的数据便会出错，造成读干扰（Read Disturb）问题。
- 闪存的数据保持（Data Retention）。
- 对MLC或TLC来说，存在Lower Page corruption的问题 : 即在对Upper Page/ExtraPage（和Lower Page共享存储单元的闪存页）写入时，如果发生异常掉电，也会把之前Lower Page上成功写入的数据破坏掉。好的FTL，应该有机制尽可能避免这个问题；
- MLC或TLC的读写速度都不如SLC，但它们都可以配成SLC模式来使用。

根据**映射粒度**的不同，FTL映射有基于块的映射，有基于页的映射，还有混合映射（Hybrid Mapping）。

FTL都是基于页映射的，因为现在SSD基本都是采用这种映射方式。

对于绝大多数SSD，我们可以看到上面都有板载DRAM，其主要作用就是存储这张映射表

映射表在SSD掉电前，是需要把它写入到闪存中去的。下次上电初始化时，需要把它从闪存中部分或全部加载到SSD的缓存（DRAM或者SRAM）中。随着SSD的写入，缓存中的映射表不断增加新的映射关系，为防止异常掉电导致这些新的映射关系丢失，SSD的固件不仅仅只在正常掉电前把这些映射关系刷新到闪存中去，而是在SSD运行过程中，按照一定策略把映射表写进闪存。这样，即使发生异常掉电，丢失的也只是一小部分映射关系，上电时可以较快地重建这些映射关系

如果用户顺序写的话，垃圾比较集中，利于SSD做垃圾回收；如果用户是随机写的话，垃圾产生比较分散，SSD做垃圾回收相对来说就更慢

Trim 对于写放大影响很大.
> 文件系统的删除，将特定的区域标记为 free，当文件系统这些地方分配出去，重新写的时候，
> 那么，这些地方被标记为无效, 如果对于 free 的写，不是连续，那么其实是 free 的空间就会被
> 搬运一遍。

> f2fs 的连续写可以防范 ssd 的碎片化

FTL映射表记录每个LBA对应的物理页位置。Valid Page Bit Map（VPBM）记录每个物理块上哪个页有有效数据，Valid Page Count（VPC）则记录每个物理块上的有效页个数。通常GC会使用VPC进行排序来回收最少有效页的闪存块；VPBM则是为了在GC时只读有用的数据，也有部分FTL会省略这个表。

动态磨损平衡算法的基本思想是把热数据写到年轻的块上，即在拿一个新的闪存块用来写的时候，挑选擦写次数小的；静态磨损平衡算法基本思想是把冷数据写到年老的块上，即把冷数据搬到擦写次数比较多的闪存块上。

# 问题
1. erase 和 write 的 granularity 的为什么是 block 和 page
    1. 为什么划分为 block 和 page 的级别 ?
2. nvme 和 PCIe 的关系是相互替代 ？
    1. 所在的层次是什么 ?
    2. 能不能对其编程

3. github 上 nvme 的工具的实现说明这些 SSD 都是存在相同的接口的，那么能不能写一个程序，类似的
瞎几把操作 ssd
    1. 系统中间的

4. FTL的映射表:
    1. FTL 的映射表放在那里啊! 如果也是使用 nand 进行存储，那么，

5. SSD 的缓存
    1. 冷热数据的区分为什么可以更好的处理 flash 特性

6. nand : 写的粒度和 erase 的粒度 ？

7. 操作写放到的因素是什么 ？

8. M.2 的关系是什么 ?
9. PCIe 和 USB 的关系 ?

10. PCIe 的 RC 可以自动将读写设备的空间转化为 设备的访问，那么 IOMMU 做啥的 ?
    1. PCIe 的空间相互冲突，怎么办 ? 那些驱动似乎是将 PCIe 映射的空间在软件重点写死的
    2. PCIe 是如何和 DMA 相互交互的, 存在 PCIe 控制器这种东西吗 ？ 那么 DMA 控制器放在那里了 ？


# 补充资料
https://en.wikipedia.org/wiki/NVM_Express

https://nvmexpress.org/education/drivers/linux-driver-information/

[^1]:SATA uses the Advanced Host Controller Interface (AHCI) to access data. One of the most significant features brought by AHCI is Native Command Queuing (NCQ), explicitly designed to speed up mechanical hard drives and enable hot-swapping.

M.2 is a physical standard that defines the shape, dimensions, and the physical connector itself.

nvme-cli 使用的教程:
https://www.nvmedeveloperdays.com/English/Collaterals/Proceedings/2018/20181204_PRECON2_Hands.pdf

https://en.wikipedia.org/wiki/PCI_Express#Hardware_protocol_summary : 讲解 PCIe 的原理



[^1]: https://phoenixnap.com/kb/nvme-vs-sata-vs-m-2-comparison
