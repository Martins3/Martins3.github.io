# aarch64 ESR.ISS
<!-- f586b262-bafe-41c3-970e-ff491bc99562 -->

(2026-06-15 kimi 2.7 ，写看完了，感觉没有任何问题)

`ESR.ISS` 是 ARM64（aarch64）异常处理中一个非常关键但也很容易混淆的字段。

## 1. ESR 寄存器是什么

在 aarch64 中，有多个 ESR 寄存器：

- `ESR_EL1`：EL1（内核态）捕获异常时使用
- `ESR_EL2`：虚拟化 Hypervisor 使用
- `ESR_EL3`：Secure Monitor 使用

当一个异常进入某个 Exception Level 时，硬件会自动把**异常原因**写入对应 EL 的 ESR 寄存器。软件的异常处理函数（比如内核的 `do_mem_abort`、`do_el0_sync`）会读取 ESR 来判断发生了什么。

## 2. ESR 的位域结构

`ESR_ELx` 是 64 位寄存器，但有效位只有低 32 位（高 32 位保留为 0）。结构如下：

```
31  26 25 24          0
+------+---+------------+
|  EC  | IL |    ISS    |
+------+---+------------+
```

- **EC（Exception Class）**：bits[31:26]，共 6 位，表示异常大类
  - `0x24`：Data Abort from a lower EL
  - `0x25`：Data Abort taken without a change in Exception Level
  - `0x20`：Instruction Abort
  - `0x00`：Unknown/Uncategorized
  - 等等

- **IL（Instruction Length）**：bit[25]，异常指令长度
  - `1`：32 位指令（A64 模式）
  - `0`：16 位指令（A32/T32 模式）

- **ISS（Instruction Specific Syndrome）**：bits[24:0]，共 25 位，**这个字段的含义依赖于 EC**

## 3. ISS 的关键点：它是“EC-specific”的

**ISS 不是固定格式的。** 同一个 25 位字段，在 EC=0x24（Data Abort）和 EC=0x20（Instruction Abort）时含义完全不同。你需要先读 EC，再按对应的格式解析 ISS。

### 3.1 EC = 0x24 / 0x25（Data Abort）时 ISS 格式

```
24    24 23 22 21 20 19 16 15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
+--------+---+---+---+------+---+---+---+---+---+---+---+---+-----+
|  ISV   | 0 | 0 | 0 |  SAS  | SSE |  SRT  | SF  |  AR | 0 |  FN   |
+--------+---+---+---+------+---+---+---+---+---+---+---+---+-----+
|                        IFSC / DFSC                              |
```

几个重要位：

- **DFSC（Data Fault Status Code）**：bits[5:0]，MMU/异常的具体原因
  - `0x04`：Translation fault, level 0
  - `0x05`：Translation fault, level 1
  - `0x06`：Translation fault, level 2
  - `0x07`：Translation fault, level 3
  - `0x0c`：Permission fault, level 1
  - `0x0d`：Permission fault, level 2
  - `0x0f`：Permission fault, level 3
  - `0x21`：Alignment fault
  - `0x35`：TLB conflict abort

- **WnR（Write not Read）**：bit[6]，访问方向
  - `1`：写操作导致异常
  - `0`：读操作导致异常

- **S1PTW / CM / EA / FnV** 等也有各自含义。

### 3.2 EC = 0x20 / 0x21（Instruction Abort）时 ISS 格式

主要是 IFSC：

```
24 13 12 11 10 9 8 7 6 5 4 3 2 1 0
0  0  0  0  0  0  0 0 0 IFSC
```

### 3.3 EC = 0x00（Unknown reason）时

ISS 通常全为 0，或者提供很少信息。

## 4. 举个例子

假设你读取 `ESR_EL1` 得到 `0x96000007`：

```
31 26 25 24           0
1001 01 1 00000000000000000000111
```

- EC = `100101` = 0x25 = Data Abort taken without a change in EL（内核自己访问内存出错）
- IL = 1
- ISS = 0x0000007
  - DFSC = 0x07 = Translation fault, level 3
  - WnR = 0 = 读操作

所以这是一个**内核态的读操作触发的三级页表转换错误**。

## 5. 在 Linux 内核中的使用

Linux 内核代码里随处可见对 ESR 的解析，关键宏在：

```c
// arch/arm64/include/asm/esr.h

#define ESR_ELx_EC_SHIFT    (26)
#define ESR_ELx_EC_MASK     (UL(0x3F) << ESR_ELx_EC_SHIFT)
#define ESR_ELx_EC(esr)     (((esr) & ESR_ELx_EC_MASK) >> ESR_ELx_EC_SHIFT)

#define ESR_ELx_IL          (UL(1) << 25)
#define ESR_ELx_ISS_MASK    (ESR_ELx_IL - 1)

#define ESR_ELx_FSC_TYPE    (0x3C)
#define ESR_ELx_FSC_LEVEL   (0x03)
#define ESR_ELx_FSC_FAULT(type, level)  ((type) | (level))
#define ESR_ELx_FSC_TF_L0   ESR_ELx_FSC_FAULT(0x04, 0)
...
```

典型用法：

```c
u64 esr = read_sysreg(esr_el1);
u32 ec = ESR_ELx_EC(esr);
u32 iss = esr & ESR_ELx_ISS_MASK;

switch (ec) {
case ESR_ELx_EC_DABT_LOW:
    // 用户态数据异常
    break;
case ESR_ELx_EC_DABT_CUR:
    // 内核态数据异常
    break;
}
```

## 6. 总结

| 概念 | 含义 |
|------|------|
| ESR | 异常综合征寄存器，记录异常原因 |
| EC | 异常大类，决定 ISS 的解析方式 |
| IL | 异常指令长度 |
| ISS | 指令特定综合征，25 位，格式随 EC 变化 |
| DFSC/IFSC | 数据/指令页表错误状态码 |

理解 ESR.ISS 的关键就是：**先看 EC，再按 EC 对应的格式解析 ISS**。不要试图用一个固定格式解释所有 ISS。

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
