3. DRAM : 破坏性读，电容 -- 回填/flush
5. DRAM 结构总结 :
    1. 128bit 的bus 访问两个
    2. DIMM
    3. RANK
    4. DRAM chip
    5. BANK

DIMM is a module that contains one or several random access memory (RAM) chips on a small circuit board with pins that connect it to the computer motherboard.

- https://news.ycombinator.com/item?id=41629923
  - https://www.righto.com/2024/09/ramtron-ferroelectric-fram-die.html

https://www.systemverilog.io/ddr4-initialization-and-calibration

## LPDDR5X 也是推理的显存速度

因为 https://www.intel.com/content/www/us/en/products/details/discrete-gpus/data-center-gpu/d-series.html 看到，
似乎 AMD 和 Intel 都是会使用 LPDDR5X 而不是 HBM

LPDDR5X 这个名字是什么意思？
- LP：Low Power，设计重点包含降低工作、空闲和数据传输时的能耗。
- DDR：Double Data Rate，属于双倍数据率的同步 DRAM 家族。
- 5X：LPDDR5 的增强版本，提高数据传输能力等。

它和 DDR5 是不同的接口标准，需要对应的内存控制器，不能理解成“把普通 DDR5 降低电压”。LPDDR5X
常见于手机、轻薄电脑，现在也进入数据中心。其节能设计包括较低的电压、细分供电域和相应的电源管理机制。Micron 技术说明
(https://www.micron.com/about/blog/memory/dram/how-does-voltage-scaling-enable-lpddr5x-to-deliver-efficient-ai-user-experiences)

它与 DDR、GDDR、HBM 的区别，主要在设计取舍。

 类型       主要侧重                                常见组织方式
━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 DDR5       通用系统内存，兼顾容量、扩展性和性能    CPU 连接多个内存通道，常见 DIMM
─────────  ──────────────────────────────────────  ────────────────────────────────────────────────────
 LPDDR5X    数据传输能效、容量与带宽的平衡          通过多个通道连接处理器，既有焊接封装，也有模块形式
─────────  ──────────────────────────────────────  ────────────────────────────────────────────────────
 GDDR6/7    很高的每引脚传输速率                    GPU 周围布置显存颗粒，通过 PCB 连接
─────────  ──────────────────────────────────────  ────────────────────────────────────────────────────
 HBM        极高的总带宽和带宽密度                  堆叠 DRAM，通过极宽接口与处理器近距离连接

## 如何理解通道

13900K 官方规格是最多两个内存通道，
主板也明确采用双通道架构。
Intel CPU 规格
(https://www.intel.com/content/www/us/en/products/sku/230496/intel-core-i913900k-processor-36m-cache-up-to-5-80-ghz/specificatio
.html)、
ASUS 主板规格 (https://www.asus.com/au/motherboards-components/motherboards/tuf-gaming/tuf-gaming-b660-plus-wifi-d4/techspec/)

你的连接关系可以理解为：

CPU 内存控制器
  │
  ├── 通道 0：64 bit 数据总线
  │     ├── 32 GiB DDR4，2 Rank
  │     └── 32 GiB DDR4，2 Rank
  │
  └── 通道 1：64 bit 数据总线
        ├── 32 GiB DDR4，2 Rank
        └── 32 GiB DDR4，2 Rank

这里有几个容易混淆的地方：

• 4 个插槽不等于 4 通道：你这是每通道两条，也叫 2 DPC（DIMMs Per Channel）。
• Rank 不等于通道：Rank 是内存条上能被一起选中的一组 DRAM 芯片。每条有两个 Rank，它们也共享所属通道的数据总线。
• 1 个 NUMA node 不等于单通道：系统报告一个 NUMA node，但这个节点内部有两个内存通道。通道分配由硬件处理，普通程序无需显式选择。

当前值得注意的是速率只有 2400 MT/s。 按 DDR4 每通道 64 bit，即每次传输 8 字节计算：

单通道理论带宽 = 2400 × 10⁶ × 8 = 19.2 GB/s
双通道理论带宽 = 19.2 × 2      = 38.4 GB/s

这是理论上限，实际应用带宽会更低；双通道主要提高吞吐量，不意味着单次内存访问延迟减半。
Intel 带宽计算说明
(https://www.intel.com/content/www/us/en/support/articles/000056722/processors/intel-core-processors.html)

另外，你的四条内存有两种型号：VGM4UX32C18BG-DTACW 和 VGM4UX36C18BG-DTACW，每个通道各一条。
目前能确认 BIOS 报告它们统一运行在 2400 MT/s；
仅凭这些信息，还不能确定是未启用 XMP、手动设置，还是混插与四条满插后的训练结果。

### 通道数量和 CPU 型号的关系

通道数量主要受到 CPU 的限制，显然通道数量

 CPU 型号                                               每颗 CPU 通道数    双路整机通道数
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━
 Xeon E5-2698 v4 / E5-2699 v4                                         4                 8
─────────────────────────────────────────────────────  ─────────────────  ────────────────
 Xeon Silver 41xx、42xx / Gold 52xx、62xx                              6                12
─────────────────────────────────────────────────────  ─────────────────  ────────────────
 海光 7285 / 7380                                                     8                16
─────────────────────────────────────────────────────  ─────────────────  ────────────────
 海光 7490                                                           12                24
─────────────────────────────────────────────────────  ─────────────────  ────────────────
 鲲鹏 920                                                             8                16


### Rank
理解它的关键是：单颗 DRAM 芯片的数据宽度，通常没有整个通道那么宽，需要多颗并联。

假设每颗芯片的数据宽度是 x8，即每次传输 8 bit，那么需要 8 颗一起工作：

一个 Rank，共 64 bit

芯片编号    0     1     2     3     4     5     6     7
提供位数   8bit  8bit  8bit  8bit  8bit  8bit  8bit  8bit
            └─────────────────────────────────────┘
                    同时工作，合计 64 bit

它们收到相应的地址和命令，各自提供这份数据中的一部分。这里的 x8 是位宽，不是容量，也不是 8
颗芯片。例如厂商的 1Rx8 内存规格，就可以由 8 颗 x8 芯片组成一个 Rank。Kingston 实例
(https://www.kingston.com.cn/datasheets/KVR32S22S8_8.pdf)

Dual Rank 就是在一条内存上，放了两组这样的存储资源。 两组通过不同的片选信号选择，但共享通道的数据线：

```txt
                   同一组 64 bit 数据线
内存控制器 ═════════════╦══════════════
                        │
                   ┌────┴─────┐
                   │ 一条DIMM │
                   │          │
 片选 Rank 0 ──────→ Rank 0   │
 片选 Rank 1 ──────→ Rank 1   │
                   └──────────┘
```

这意味着：

- 增加 Rank 可以增加容量，但不会把通道从 64 bit 变成 128 bit。
- 同一通道的数据传输需要排队，不同 Rank 不能同时占用数据线传数据。
- 控制器可以交错调度不同 Rank，例如一个 Rank 刷新时访问另一个，从而提高利用率；带宽不会因此直接翻倍。

 概念       含义
━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 DIMM       一整条物理内存模块
─────────  ────────────────────────────────────────────
 Channel    控制器连接内存的独立通信通道
─────────  ────────────────────────────────────────────
 Rank       一组一起响应、凑齐通道数据宽度的 DRAM 芯片
─────────  ────────────────────────────────────────────
 Bank       每颗 DRAM 芯片内部的存储阵列分区

### Bank

Bank 是一颗 DRAM 芯片内部，可以独立管理“当前打开哪一行”的存储分区。 分成多个 Bank，就能让不同分区的内部操
作有所重叠，提高访问效率。

先从一个 Bank 看起。可以把它理解成一个按行、列排列的存储阵列，以及用于访问当前行的 Row Buffer（行缓冲）：

一个 Bank
┌───────────────────────────┐
│ 存储阵列                  │
│   Row 0：................ │
│   Row 1：................ │
│   Row 2：................ │
│   ...                     │
│           ↓ ACTIVATE      │
│ 行缓冲：当前打开的一行    │
│           ↓ 按列 READ/WRITE│
└───────────────────────────┘

访问数据通常经历这样的过程：

1. ACTIVATE：选中某个 Bank 的某一行，通过感应放大器激活这一行；可以将感应放大器理解为行缓冲。
2. READ / WRITE：指定列，读写已打开行中的一部分数据。
3. PRECHARGE：关闭当前行，让这个 Bank 准备好打开另一行。

如果要访问同一个 Bank 的另一行，必须先关闭旧行，再打开新行。这是 DDR4 命令规定的行为。Micron DDR4 数据手册
(https://www.alliancememory.com/wp-content/uploads/Micron_8gb-Commercial-ddr4-dram-Alliance_MT40A512M16TD-062ER-MT40A1G8AG-062ER_reduced.pdf)

因此，两个地址即使都在内存里，访问代价也可能不同：

 访问情况                                需要的操作
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 目标行已经打开，Row hit                 直接发读写命令
──────────────────────────────────────  ──────────────────────────────────────────
 Bank 当前没有打开的行                   先激活目标行，再读写
──────────────────────────────────────  ──────────────────────────────────────────
 同一 Bank 打开了其他行，Row conflict    先预充电关闭旧行，再激活目标行，最后读写

多个 Bank 的意义就在这里：Bank 0 打开 Row 100 时，Bank 1 可以同时保持 Row 200 打开。控制器还能在等待某个
Bank 内部操作完成时，调度其他 Bank。不过，它们最终仍共享芯片的数据接口和通道总线，不能各自无限制地向 CPU
传输数据。

与前面 Rank 的关系，需要特别注意：

一个 Rank：假设由 8 颗 x8 芯片组成

芯片 0          芯片 1                 芯片 7
├─ Bank 0       ├─ Bank 0              ├─ Bank 0
├─ Bank 1       ├─ Bank 1              ├─ Bank 1
├─ ...          ├─ ...                 ├─ ...
└─ Bank N       └─ Bank N              └─ Bank N
   提供 8 bit      提供 8 bit              提供 8 bit

控制器访问这个 Rank 的 Bank 1、Row X、Column Y 时，各颗芯片响应相同的 Bank、行、列选择，
分别贡献数据位，合起来形成 64 bit。

```txt
DDR4 Memory Channel（64-bit）
│
├── Rank 0
│   ├── DRAM Chip 0 (x8)
│   │   ├── Bank 0
│   │   ├── Bank 1
│   │   └── ... Bank 15
│   ├── DRAM Chip 1 (x8)
│   │   ├── Bank 0
│   │   ├── Bank 1
│   │   └── ... Bank 15
│   └── ... 共 8 颗芯片
│
└── Rank 1
    ├── DRAM Chip 0 (x8)
    ├── DRAM Chip 1 (x8)
    └── ... 共 8 颗芯片
```

所以，虽然物理上每颗芯片都有自己的 Bank，这些芯片的同编号 Bank 在 Rank 层面是协同工作的，
不能简单把“8 颗芯 片 × 每颗 16 Bank”理解为控制器能独立选择 128 个 Bank。

DDR4 又把 Bank 分成 Bank Group。常见 x8 DDR4 芯片是 4 个 Bank Group，每组 4 个 Bank，共 16 个；同组与跨组
访问有不同的时序约束。你当前内存的具体颗粒组织尚未确认，不能仅从 dmidecode 的 Rank: 2 得到 Bank 数量。

另外，你之前输出中的 Bank Locator: BANK 0 是 SMBIOS 的插槽定位字符串，不是这里说的芯片内部 Bank 编号。

## memory interleave


已确认：当前这台 i9-13900K 启用了内存交织。 这次直接读取了控制器寄存器。

两个控制器的 MAD_MC_HASH 都是 0x00000807，解码如下：

 字段             实际值    含义
━━━━━━━━━━━━━━  ━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 HASH_ENABLED          1    两个内存控制器之间的交织已启用
──────────────  ─────────  ──────────────────────────────────────────────
 HASH_LSB              3    交织地址位为 bit 9，对应 512 B
──────────────  ─────────  ──────────────────────────────────────────────
 ZONE1_START     128 GiB    与总容量一致，全部 DRAM 容量属于对称交织区域
──────────────  ─────────  ──────────────────────────────────────────────
 STACKED_MODE          0    未启用堆叠模式

字段定义见 Intel 寄存器文档，第 139–140 页（PDF 第 349–350 页） (https://m.media-amazon.com/images/I/D1YR5I0KShL.pdf#page=349)；Linux 的 地址转换代码
(/home/martins3/data/kernel/linux-full/drivers/edac/igen6_edac.c:528) 也按 HASH_LSB + 6 获取交织地址位。

需要纠正我前面的说法：不能把这台机器描述为“每 64 B 简单交替一次”。 实际读取的交织选择位是 bit 9；完整通道选择还可能涉及地址哈希，不能仅凭这个字段画出严
格的交替顺序。

对于你关心的问题：同一个 4 KiB 页可以跨两个内存通道，因为 bit 9 位于页内偏移中。 精确到某条 cache line 属于哪个通道，还需要完整解码地址哈希。

全程只读访问 PCI 配置和 /dev/mem，未写入寄存器。

### 利用 perf 确认

如果 perf 来观察，可以发现 13900k 的两个 channel 的 io 量基本相同，
所以我们可以做的:
```sh
sudo perf stat -a \
  -e uncore_imc_free_running_0/data_read/ \
  -e uncore_imc_free_running_1/data_read/ \
  -e uncore_imc_free_running_0/data_write/ \
  -e uncore_imc_free_running_1/data_write/ \
  -- sleep 1
```

```txt
 Performance counter stats for 'system wide':

          1,236.98 MiB  uncore_imc_free_running_0/data_read/
          1,237.00 MiB  uncore_imc_free_running_1/data_read/
            385.36 MiB  uncore_imc_free_running_0/data_write/
            386.04 MiB  uncore_imc_free_running_1/data_write/

       1.000631577 seconds time elapsed
```

### interleave 具体是多少

利用 codex 找到的，感觉普通人基本上想不到这个方法。

通过 读取 PCI 配置得到寄存器基址，再通过 /dev/mem 读取 MMIO 寄存器获取的。

第一步，读取 CPU Host Bridge 的 MCHBAR：
```sh
sudo setpci -s 00:00.0 48.l 4c.l
```

实际输出：

fedc0001
00000000

这是一个 64 位寄存器的低、高 32 位。最低位 1 表示 MCHBAR 已启用，得到本机 MMIO 基址：

MCHBAR = 0xFEDC0000

第二步，根据寄存器布局计算地址：

MC0 MAD_MC_HASH = MCHBAR           + 0xD9B8 = 0xFEDCD9B8
MC1 MAD_MC_HASH = MCHBAR + 0x10000 + 0xD9B8 = 0xFEDDD9B8

第三步，用 Python 以只读方式打开 /dev/mem，从这两个物理地址各读取 4 字节，按小端整数解码。下面是等价的复现代码：

sudo python3 - <<'PY'
import os
import struct

fd = os.open("/dev/mem", os.O_RDONLY | os.O_SYNC)
try:
    for mc, addr in [(0, 0xFEDCD9B8), (1, 0xFEDDD9B8)]:
        data = os.pread(fd, 4, addr)
        value = struct.unpack("<I", data)[0]
        print(f"MC{mc}: address={addr:#x}, MAD_MC_HASH={value:#010x}")
finally:
    os.close(fd)
PY

实际读取结果：

MC0: address=0xfedcd9b8, MAD_MC_HASH=0x00000807
MC1: address=0xfeddd9b8, MAD_MC_HASH=0x00000807

最后进行位域提取：

value = 0x00000807

enabled     = value & 1          # 1
hash_lsb    = (value >> 1) & 7    # 3 → 地址 bit 9
zone1_start = (value >> 4) & 511  # 128 GiB
stacked     = (value >> 13) & 1   # 0

这些地址是你这台机器当前的地址，其他机器应重新读取 MCHBAR，并核对对应 CPU 的寄存器布局。整个过程没有写寄存器，也没有加载内核模
块。

字段定义见 Intel 寄存器文档，第 139–140 页（PDF 第 349–350 页）
(https://m.media-amazon.com/images/I/D1YR5I0KShL.pdf#page=349)；Linux 的 地址转换代码
(/home/martins3/data/kernel/linux-full/drivers/edac/igen6_edac.c:528) 也按 HASH_LSB + 6 获取交织地址位。

```txt
 字段             实际值    含义
━━━━━━━━━━━━━━  ━━━━━━━━━  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 HASH_ENABLED          1    两个内存控制器之间的交织已启用
──────────────  ─────────  ──────────────────────────────────────────────
 HASH_LSB              3    交织地址位为 bit 9，对应 512 B
──────────────  ─────────  ──────────────────────────────────────────────
 ZONE1_START     128 GiB    与总容量一致，全部 DRAM 容量属于对称交织区域
──────────────  ─────────  ──────────────────────────────────────────────
 STACKED_MODE          0    未启用堆叠模式
```

### numa 和 memory interleaving

如果只有一个 numa node ，那么就是 channel 越多越好，但是
如果多个 numa node ，参与 memory interleaving 会导致访问内存的时候，
一部分会进入到跨到其他的 numa 中，这是需要考虑的。

### dpc 问题

https://almalinux.org/blog/2026-10-16-pwnescape-cve-2026-99999/

这个在我的 13900k 的机器上也都是的确观察到过了。

## edac

```txt
/sys/devices/system/edac

├── mc
│   ├── power
│   │   ├── autosuspend_delay_ms
│   │   ├── control
│   │   ├── runtime_active_time
│   │   ├── runtime_status
│   │   └── runtime_suspended_time
│   ├── subsystem -> ../../../../bus/edac
│   └── uevent
├── power
│   ├── autosuspend_delay_ms
│   ├── control
│   ├── runtime_active_time
│   ├── runtime_status
│   └── runtime_suspended_time
└── uevent
```

EDAC = Error Detection And Correction（错误检测与纠正），是 Linux 内核中用于监测和报告硬件错误的子系统，最常见的用途是监控 ECC 内存错误。代码主要在
drivers/edac/。

它主要报告两类错误：

• CE（Correctable Error，可纠正错误）：例如 ECC 内存出现单比特翻转，硬件已经纠正。EDAC 负责记录、计数，并尽可能指出哪个内存通道、哪根 DIMM 出错。
• UE（Uncorrectable Error，不可纠正错误）：错误超出硬件纠正能力，可能造成数据损坏、进程被终止或系统崩溃。

实际纠错通常由内存控制器等硬件完成，EDAC 主要负责读取错误状态并向系统报告，不是靠内核软件把坏内存修好。

例如，内核日志中的：

EDAC MC0: 1 CE ... DIMM ...

意思是：内存控制器 MC0 检测到一次已纠正错误。偶发错误可以观察；如果某根 DIMM 的错误持续增加，就需要排查内存条、插槽或相关硬件。

相关统计通常在 /sys/devices/system/edac/ 下，用户态可以通过 rasdaemon 收集和分析。


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
