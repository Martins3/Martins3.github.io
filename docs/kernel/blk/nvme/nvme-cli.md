# nvme-cli

nvme id-ns /dev/nvme11n1

### 消费级的盘的观察
```txt
06:00.0 Non-Volatile memory controller: MAXIO Technology (Hangzhou) Ltd. NVMe SSD Controller MAP1602 (DRAM-less) (rev 01)
```

```txt
🤒  sudo nvme id-ns /dev/nvme1n1
[sudo] password for martins3:
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

### 企业盘观察


这是 `nvme id-ns /dev/nvme11n1`（Identify Namespace）输出，描述的是 **NVMe 命名空间 1 的能力、容量和 LBA 格式**。我按“你最可能关心的点”来解读。

## 一、容量相关（最重要）

```
nsze : 0x37e3e92b0
ncap : 0x37e3e92b0
nuse : 0x37e3e92b0
```

* **nsze (Namespace Size)**
  命名空间的**总逻辑块数（LBA 数）**

* **ncap (Namespace Capacity)**
  可用容量（通常 ≤ nsze）

* **nuse (Namespace Utilization)**
  已使用的 LBA 数

👉 三个值相等，说明：

* 没有 over-provision
* 整个 namespace 已经被“分配/使用”（即使文件系统里可能是空的）

### 实际容量是多少？

当前使用的 LBA 格式是：

```
flbas : 0x1
```

`flbas[3:0] = 1` → 使用 **lbaf 1**

```
lbaf 1 : ms:8  lbads:9  rp:0x1 (in use)
```

* `lbads:9` → LBA size = 2⁹ = **512 bytes**
* `nsze = 0x37e3e92b0 = 149,997,707,376 LBA`

计算容量：

```
149,997,707,376 × 512
≈ 76,798,902,220,800 bytes
≈ 76.8 TB (十进制)
≈ 69.9 TiB (二进制)
```

✔ 和下面的字段一致：

```
nvmcap : 7681501126656
```

> 注意：`nvmcap` 单位是 **bytes / allocation granularity**，厂商实现略有差异，但量级是对的。

---

## 二、LBA 格式（是否支持 4K / metadata）

```
nlbaf : 4
```

表示 **支持 5 种 LBA 格式（0~4）**

### 各 LBA 格式解释

| lbaf | lbads | LBA 大小 | ms | metadata | 说明              |
| ---- | ----- | ------ | -- | -------- | --------------- |
| 0    | 9     | 512B   | 0  | 无        | 传统 512B         |
| 1    | 9     | 512B   | 8  | 有        | **当前使用**        |
| 2    | 12    | 4KB    | 64 | 有        | 4K + 大 metadata |
| 3    | 12    | 4KB    | 0  | 无        | 纯 4K            |
| 4    | 12    | 4KB    | 8  | 有        | 4K + 小 metadata |

👉 你现在是：

* **512B sector**
* **带 8B metadata**
* metadata **与数据分离**（不是 inline）

如果你做数据库 / 对齐 IO / O_DIRECT / SPDK，这个点非常关键。

---

## 三、数据保护 / 原子性能力

```
dpc : 0x1f
dps : 0x3
```

### DPC（Data Protection Capabilities）

`0x1f` = 所有位都支持：

* Type 1 / 2 / 3 protection information
* First / Last / Separate metadata

👉 说明这是 **企业级盘**，不是消费级 NVMe

### DPS（当前使用的保护方式）

`0x3`：

* 启用 protection
* Type 1
* metadata **separate**

---

## 四、写入 / 原子性限制（对文件系统、DB 很重要）

```
nawun  : 0
nawupf : 0
nacwu  : 0
```

这些是 **写原子性限制**：

* `0` 表示：
  **单个 LBA 写入是原子的**
* 没有对“未对齐写 / partial write”的特殊限制

👉 对 ext4 / xfs / dm / md 都是友好的

---

## 五、特性支持概览

```
mc     : 0x3
```

* 支持 **metadata pointer**
* 支持 **metadata inline**

```
fpi : 0x80
```

* 支持 **Format Progress Indicator**
* `nvme format` 可查询进度（企业盘常见）

```
rescap : 0
```

* **不支持 namespace reservation**
* 说明不是为 SCSI-3 PR / 集群仲裁设计的 namespace

---

## 六、标识符（用于多路径 / 存储系统）

```
nguid : 01000000000000005cdfb830761a0150
eui64 : 0000000000000000
```

* **NGUID**：全局唯一 namespace ID（多路径非常关键）
* **EUI64**：未使用

👉 multipath / NVMe-oF / SAN 环境主要靠 `nguid`

---

## 七、一句话总结

这是一个：

* **≈ 76.8 TB 企业级 NVMe namespace**
* 当前使用 **512B sector + 8B metadata（separate）**
* 支持 **4K LBA**
* 支持 **完整数据保护（DIF/DIX）**
* 原子写限制非常宽松
* 非 reservation / 非集群仲裁型 namespace

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
