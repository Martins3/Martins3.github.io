# `nvme id-ns` 输出梳理

`nvme id-ns` 发送 Identify Namespace 命令，查看的是 namespace 的容量、LBA
格式和可选能力，并不是控制器的完整信息。当前系统有三块 NVMe 盘，先展示
Fanxiang S790 对应的 `/dev/nvme1n1` 原始输出，再统一对比三块盘。

## 原始输出

```console
$ sudo nvme id-ns /dev/nvme1n1
NVME Identify Namespace 1:
nsze    : 0x1d1c0beb0
ncap    : 0x1d1c0beb0
nuse    : 0x1d1c0beb0
nsfeat  : 0
nlbaf   : 0
flbas   : 0
mc      : 0
dpc     : 0
dps     : 0
nmic    : 0
rescap  : 0
fpi     : 0
dlfeat  : 0
nawun   : 0
nawupf  : 0
nacwu   : 0
nabsn   : 0
nabo    : 0
nabspf  : 0
noiob   : 0
nvmcap  : 0
mssrl   : 0
mcl     : 0
msrc    : 0
kpios   : 0
nulbaf  : 0
kpiodaag: 0
anagrpid: 0
nsattr  : 0
nvmsetid: 0
endgid  : 0
nguid   : 0000000000000000d0d0d0d0d0d0d0d0
eui64   : d0d0d0d0d0d0d0d0
lbaf  0 : ms:0   lbads:9  rp:0 (in use)
```

## 盘的统一对比

```txt
02:00.0 Non-Volatile memory controller [0108]: Yangtze Memory Technologies Co.,Ltd ZHITAI TiPro7000 [1e49:0041] (rev 01)
03:00.0 Non-Volatile memory controller [0108]: Yangtze Memory Technologies Co.,Ltd ZHITAI TiPlus7100 [1e49:0071] (rev 01)
07:00.0 Non-Volatile memory controller [0108]: MAXIO Technology (Hangzhou) Ltd. NVMe SSD Controller MAP1602 (DRAM-less) [1e4b:1602] (rev 01)
```

| 字段              | `/dev/nvme0n1`                     | `/dev/nvme1n1`                     | `/dev/nvme2n1`                     |
| ---               | ---                                | ---                                | ---                                |
| `nsze`            | `0x773bd2b0`                       | `0x1d1c0beb0`                      | `0x773bd2b0`                       |
| `ncap`            | `0x773bd2b0`                       | `0x1d1c0beb0`                      | `0x773bd2b0`                       |
| `nuse`            | `0x773bd2b0`                       | `0x1d1c0beb0`                      | `0x773bd2b0`                       |
| `nsfeat`          | `0`                                | `0`                                | `0`                                |
| `nlbaf`           | `0`                                | `0`                                | `0`                                |
| `flbas`           | `0x10`                             | `0`                                | `0`                                |
| `mc`              | `0`                                | `0`                                | `0`                                |
| `dpc`             | `0`                                | `0`                                | `0`                                |
| `dps`             | `0`                                | `0`                                | `0`                                |
| `nmic`            | `0`                                | `0`                                | `0`                                |
| `rescap`          | `0`                                | `0`                                | `0`                                |
| `fpi`             | `0`                                | `0`                                | `0`                                |
| `dlfeat`          | `0`                                | `0`                                | `0`                                |
| `nawun`           | `0`                                | `0`                                | `0`                                |
| `nawupf`          | `0`                                | `0`                                | `0`                                |
| `nacwu`           | `0`                                | `0`                                | `0`                                |
| `nabsn`           | `0`                                | `0`                                | `0`                                |
| `nabo`            | `0`                                | `0`                                | `0`                                |
| `nabspf`          | `0`                                | `0`                                | `0`                                |
| `noiob`           | `0`                                | `0`                                | `0`                                |
| `nvmcap`          | `0`                                | `0`                                | `0`                                |
| `mssrl`           | `0`                                | `0`                                | `0`                                |
| `mcl`             | `0`                                | `0`                                | `0`                                |
| `msrc`            | `0`                                | `0`                                | `0`                                |
| `kpios`           | `0`                                | `0`                                | `0`                                |
| `nulbaf`          | `0`                                | `0`                                | `0`                                |
| `kpiodaag`        | `0`                                | `0`                                | `0`                                |
| `anagrpid`        | `0`                                | `0`                                | `0`                                |
| `nsattr`          | `0`                                | `0`                                | `0`                                |
| `nvmsetid`        | `0`                                | `0`                                | `0`                                |
| `endgid`          | `0`                                | `0`                                | `0`                                |
| `nguid`           | `0000000000000000a428b75644cedacc` | `0000000000000000d0d0d0d0d0d0d0d0` | `0000000000000000a428b70020d600c2` |
| `eui64`           | `a428b701f0d70084`                 | `d0d0d0d0d0d0d0d0`                 | `a428b70020d600c2`                 |
| `lbaf 0`          | `ms:0 lbads:9 rp:0 (in use)`       | `ms:0 lbads:9 rp:0 (in use)`       | `ms:0 lbads:9 rp:0 (in use)`       |

下面的逐字段讲解只以 Fanxiang S790 4TB（`/dev/nvme1n1`）为例。

## 容量

`nsze`、`ncap` 和 `nuse` 的单位都是逻辑块，而不是 byte：

| 字段 | 含义 | 当前值 |
| --- | --- | --- |
| `nsze` | Namespace Size，可寻址的逻辑块总数 | `0x1d1c0beb0` = 7,814,037,168 |
| `ncap` | Namespace Capacity，最多可分配的逻辑块数 | 7,814,037,168 |
| `nuse` | Namespace Utilization，当前已分配的逻辑块数 | 7,814,037,168 |

当前 LBA 大小为 512 B，因此容量为：

```text
7,814,037,168 LBA * 512 B = 4,000,787,030,016 B
                                  = 4.000787030 TB
                                  = 3.638694607 TiB
```

三个字段相等，表示整个 namespace 都具有可用的 LBA 地址。它不表示文件系统已经
写满，也不能据此判断 SSD 控制器内部是否保留了 over-provisioning 空间。文件系统
使用量应通过 `df` 等工具观察。

`nvmcap` 为 0 表示设备没有在这个字段中报告 namespace 容量，不能将它解释为盘的
容量为 0；这块盘的容量应由 `nsze * LBA 大小` 得到，也与 `lsblk` 报告一致。

## LBA 格式

```text
nlbaf   : 0
flbas   : 0
lbaf  0 : ms:0   lbads:9  rp:0 (in use)
```

- `nlbaf` 是“支持的 LBA 格式数量减 1”。值为 0，表示只提供 `lbaf 0` 一种格式。
- `flbas` 的格式索引为 0，所以当前正在使用 `lbaf 0`。
- `lbads:9` 表示数据长度为 `2^9 = 512 B`。
- `ms:0` 表示每个 LBA 没有 metadata。
- `rp:0` 表示该格式的相对性能等级为 Best。

因此，这个 namespace 只有 **512 B 数据 + 0 B metadata** 的格式。Identify
Namespace 数据没有列出 4 KiB LBA 格式，不能通过 `nvme format` 将它切换为 4Kn。

## namespace 特性

下列字段是位图，值为 0 表示对应能力位都没有置位：

| 字段 | 当前含义 |
| --- | --- |
| `nsfeat: 0` | 不支持 thin provisioning 和 Deallocated/Unwritten Logical Block Error；原子写参数使用控制器级的 `AWUN/AWUPF/ACWU` |
| `mc: 0` | 不支持独立 metadata buffer，也不支持 extended LBA 中的 metadata |
| `dpc: 0`、`dps: 0` | 不支持且未启用端到端 Protection Information（PI） |
| `nmic: 0` | namespace 不具备 NVMe multipath/shared namespace 能力 |
| `rescap: 0` | 不支持 NVMe Reservation |
| `fpi: 0` | 不支持 Format Progress Indicator |
| `dlfeat: 0` | 未报告释放后 LBA 的读取值；不支持 Write Zeroes 命令的 Deallocate 位 |
| `nsattr: 0` | namespace 当前没有写保护 |
| `kpios: 0` | 未启用，也不支持 Key Per I/O |

`dlfeat: 0` 只描述这里列出的 deallocation 行为，不足以单独判断普通 Dataset
Management/TRIM 是否可用。

## 原子写、边界和 Copy 限制

```text
nawun   : 0
nawupf  : 0
nacwu   : 0
nabsn   : 0
nabo    : 0
nabspf  : 0
noiob   : 0
mssrl   : 0
mcl     : 0
msrc    : 0
```

由于 `nsfeat[1]` 为 0，`nawun`、`nawupf` 和 `nacwu` 不是当前 namespace 的有效
覆盖值，实际原子写能力要继续查看 `nvme id-ctrl /dev/nvme1` 中的控制器级字段。
同理，不能笼统地把所有数值字段中的 0 都解释成“单个 LBA 写是原子的”。

`nabsn`、`nabo`、`nabspf` 没有给出 namespace 专用原子边界；`noiob: 0` 表示没有
报告最优 I/O 边界。`mssrl`、`mcl`、`msrc` 也没有在 Identify Namespace 数据中
给出 Copy 命令的 namespace 限制。

## 分组与标识符

```text
anagrpid: 0
nvmsetid: 0
endgid  : 0
nguid   : 0000000000000000d0d0d0d0d0d0d0d0
eui64   : d0d0d0d0d0d0d0d0
```

- `anagrpid`、`nvmsetid` 和 `endgid` 都为 0，没有报告 ANA group、NVM Set 或
  Endurance Group 关联；当前 namespace 本身也不是 multipath capable。
- `nguid` 和 `eui64` 虽然非零，但内容是重复的 `d0` 模式，不宜据此推导型号、
  序列号或路径关系。

## 结论

当前 `/dev/nvme1n1` 是一个容量约 4 TB 的 namespace 1。它只提供 512 B LBA，
没有 metadata 和 PI，不支持 thin provisioning、namespace multipath、NVMe
Reservation 或写保护。大量为 0 的字段反映的是这块消费级盘没有暴露相应的
namespace 高级能力，不能脱离字段类型一概解释成数值 0 或 1 LBA。

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
