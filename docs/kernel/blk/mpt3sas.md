## https://access.redhat.com/solutions/4954451

kimi 2.5 分析的，感觉差不多是这个意思了:

## 3. 根本原因分析

### 3.1 双重复合问题

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          问题根源：两个叠加的缺陷                            │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  问题 1: NVDATA EEDPTagMode 设置错误                                          │
│  ─────────────────────────────────────                                       │
│  • 出厂制造页 (Manufacturing Page 11) 中的 EEDPTagMode 标志位设置不正确       │
│  • 这决定了控制器如何处理磁盘上的 T10 PI/DIF 数据校验                         │
│  • 部分卡有此问题，部分卡没有（取决于 NVDATA 内容）                           │
│                                                                              │
│  mpt3sas 驱动的修正代码：                                                     │
│  ┌─────────────────────────────────────────────────────────────────────┐   │
│  │ /*                                                                  │   │
│  │  * Ensure correct T10 PI operation if vendor left EEDPTagMode    │   │
│  │  * flag unset in NVDATA.                                         │   │
│  │  */                                                              │   │
│  │ mpt3sas_config_get_manufacturing_pg11(ioc, &mpi_reply,           │   │
│  │                                       &ioc->manu_pg11);          │   │
│  │ if (!ioc->is_gen35_ioc && ioc->manu_pg11.EEDPTagMode == 0) {     │   │
│  │     pr_err("%s: overriding NVDATA EEDPTagMode setting\n",         │   │
│  │             ioc->name);                                          │   │
│  │     ioc->manu_pg11.EEDPTagMode &= ~0x3;                          │   │
│  │     ioc->manu_pg11.EEDPTagMode |= 0x1;  // 设置为 01b            │   │
│  │     mpt3sas_config_set_manufacturing_pg11(ioc, &mpi_reply,       │   │
│  │                                           &ioc->manu_pg11);      │   │
│  │ }                                                                │   │
│  └─────────────────────────────────────────────────────────────────────┘   │
│                                                                              │
│  注意：此修复只应用于 "旧" 型号的卡（非 SAS3.5 型号），LSI 3008 属于旧型号  │
│                                                                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  问题 2: 固件 Bug 忽略 Page 11 的更改                                         │
│  ─────────────────────────────────────────                                   │
│  • 控制器固件忽略了当前 Page 11 中更新的操作控制设置                          │
│  • 驱动写入的新设置没有被固件采纳                                             │
│  • 结果：HBA 硬件中的错误操作控制没有被纠正                                   │
│                                                                              │
│  后果：控制器对所有 T10 PI 磁盘数据进行校验，                                  │
│        而不是检测并忽略未初始化的 PI/DIF 元数据                               │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 3.2 T10 PI (Protection Information) 技术背景

#### 磁盘扇区结构（带数据保护）

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    T10 PI 磁盘扇区结构（512+8 或 4096+8）                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  字节     7      6       5       4       3       2       1       0           │
│       ┌────────┬───────┬───────┬───────┬───────┬───────┬───────┬────────┐  │
│   0   │                                                               │  │
│       ├───────┬                                                       ├  │
│       :                        用户数据区域 (512 或 4096 字节)         :  │
│       ├───────┴                                                       ├  │
│ n-1   │                                                               │  │
│       ├────────┬───────┬───────┬───────┬───────┬───────┬───────┬────────┤  │
│       │                    8 字节保护元数据 (DIF)                       │  │
│       ├────────┬───────┬───────┬───────┬───────┬───────┬───────┬────────┤  │
│   n   │ (MSB) │                                                        │  │
│       ├───────┤              Logical Block Guard (CRC16)               ├──┤
│ n+1   │       │                                                        │  │
│       ├────────┼───────┬───────┬───────┬───────┬───────┬───────┬────────┤  │
│ n+2   │ (MSB) │                                                │ (LSB) │  │
│       ├───────┤        Logical Block Application Tag           ├───────┤  │
│ n+3   │       │                                                │       │  │
│       ├────────┼───────┬───────┬───────┬───────┬───────┬───────┬────────┤  │
│ n+4   │                                                               │  │
│       ├───────┤                                                       ├──┤
│       :                    Logical Block Reference Tag                :  │
│       ├───────┤                                                       ├──┤
│ n+7   │                                                               │  │
│       └────────┴───────┴───────┴───────┴───────┴───────┴───────┴────────┘  │
│                                                                              │
│  说明：                                                                       │
│  • n = 512 或 4096（标准扇区大小）                                           │
│  • DIF = Data Integrity Field（数据完整性字段）                               │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

#### 三种元数据的作用

| 字段 | 大小 | 用途 | 计算/提供方 |
|------|------|------|-------------|
| **Logical Block Guard** | 2 字节 | CRC16 校验，检测磁盘数据错误 | 磁盘/控制器计算 |
| **Logical Block Application Tag** | 2 字节 | 应用标签，0xFFFF 表示"未初始化" | 应用层提供 |
| **Logical Block Reference Tag** | 4 字节 | LBA 参考标签，检测误读/误写 | 主机提供（LBA 低32位）|

#### 数据保护类型

```
Type 0: 无保护（传统磁盘）
Type 1: 8 字节 PI 包含 Guard + Application Tag + Reference Tag
Type 2: 8 字节 PI + 额外元数据空间
Type 3: 8 字节 PI，但 Reference Tag 被忽略
```

### 3.3 错误发生流程

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          错误发生流程详解                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│  1. 磁盘格式化时                                                             │
│     └── 8 字节 PI 元数据被初始化为 0xFFFFFFFFFFFFFFFF                        │
│         └── Application Tag = 0xFFFF（表示"未初始化"）                        │
│                                                                              │
│  2. 主机发起 READ(32) 命令                                                   │
│     └── CDB 中包含 Expected Logical Block Reference Tag（通常是 LBA）         │
│         └── 例如：LBA 的低 32 位                                             │
│                                                                              │
│  3. 磁盘检查 PI 元数据                                                       │
│     └── 如果 Application Tag == 0xFFFF                                       │
│         └── 正确行为：跳过 Guard 和 Reference Tag 检查                       │
│         └── 错误行为：继续执行所有检查（由于固件 bug）                        │
│                                                                              │
│  4. 固件 bug 导致的问题                                                       │
│     └── EEDPTagMode 未正确设置                                               │
│     └── 控制器忽略 Application Tag = 0xFFFF 的"未初始化"标识                 │
│     └── 强制比较 Reference Tag                                               │
│                                                                              │
│  5. 检查失败                                                                  │
│     └── 磁盘上的 Reference Tag = 0xFFFFFFFF（初始化值）                       │
│     └── CDB 中的 Expected Reference Tag = LBA（例如 0x12345678）              │
│     └── 不匹配！0xFFFFFFFF != 0x12345678                                     │
│                                                                              │
│  6. 错误返回                                                                  │
│     └── 磁盘返回 CHECK CONDITION                                             │
│     └── Sense Key: Illegal Request (0x05)                                    │
│     └── Additional Sense:                                                    │
│         └── Logical block reference tag check failed (参考标签不匹配)         │
│         └── 或 Logical block guard check failed (CRC 校验失败)                │
│                                                                              │
│  7. 内核处理                                                                  │
│     └── SCSI 层将错误传递给块层                                              │
│     └── blk_update_request: I/O error                                        │
│     └── Buffer I/O error on dev sdd                                          │
│                                                                              │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 4. 技术细节：EEDPTagMode

### EEDPTagMode 位定义

```
EEDPTagMode (2 bits):
  00b = 保留
  01b = 检查 Application Tag，如果为 0xFFFF 则跳过 Guard 和 Reference Tag 检查
  10b = 保留
  11b = 保留

当 EEDPTagMode = 00b（或错误设置）时：
  └── 固件总是执行完整的 PI 检查
  └── 无法识别"未初始化"的扇区
```

### 驱动修复逻辑

```c
if (!ioc->is_gen35_ioc && ioc->manu_pg11.EEDPTagMode == 0) {
    // 1. 记录覆盖操作
    pr_err("%s: overriding NVDATA EEDPTagMode setting\n", ioc->name);

    // 2. 清除低 2 位
    ioc->manu_pg11.EEDPTagMode &= ~0x3;

    // 3. 设置为 01b（正确的模式）
    ioc->manu_pg11.EEDPTagMode |= 0x1;

    // 4. 写入控制器
    mpt3sas_config_set_manufacturing_pg11(ioc, &mpi_reply, &ioc->manu_pg11);
}
```



```c
// mpt3sas_config.c
int mpt3sas_config_get_manufacturing_pg11(struct MPT3SAS_ADAPTER *ioc,
                                          Mpi2ConfigReply_t *mpi_reply,
                                          Mpi2ManufacturingPage11_t *config_page);

int mpt3sas_config_set_manufacturing_pg11(struct MPT3SAS_ADAPTER *ioc,
                                          Mpi2ConfigReply_t *mpi_reply,
                                          Mpi2ManufacturingPage11_t *config_page);
```

最后的修复方案:
1. **最佳方案**: 升级固件到 16.00.11.00+
   - 完全修复问题
   - 保留 T10 PI 数据保护功能

2. **临时方案**: `mpt3sas.prot_mask=128`
   - 禁用 DIF/DIX
   - 失去端到端数据保护
   - 适用于无法立即升级固件的场景

## 参考资源

- [T10 PI Specification (SBC-3)](https://www.t10.org/)
- [SCSI DIF/DIX](https://docs.oracle.com/cd/E57471_01/E57475/html/ol7-scsi-prot.html)
- [Linux SCSI Data Integrity](https://www.kernel.org/doc/html/latest/scsi/dix.html)
- Broadcom/LSI SAS3008 产品文档

## 如何检查 mpt3sas 卡驱动

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
