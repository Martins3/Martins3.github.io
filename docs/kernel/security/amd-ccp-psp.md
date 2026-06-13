## drivers/crypto/ccp/ 源码中包含了什么东西
<!-- b8e7595b-71bb-437e-8a03-aa058b4efa6f -->

这个文件（`ccp_c_files_concatenated.txt`）是 Linux 内核源码中 **AMD Cryptographic Coprocessor (CCP)** 驱动及相关安全模块的 C 语言源代码拼接而成，内容覆盖了从底层硬件驱动到上层安全功能（如 SEV、TEE、DBC、HSTI 等）的完整实现。其主要内容可分为以下几大模块：

### 1. **CCP 加密驱动核心（`drivers/crypto/ccp/`）**
这部分实现了对 AMD CCP 硬件加速器的支持，提供多种加密算法的内核 crypto API 封装。

- **对称加密**：
  - AES（ECB/CBC/CTR/XTS/CMAC/GCM）
  - 3DES（ECB/CBC）
- **哈希算法**：
  - SHA-1、SHA-224/256/384/512（包括 HMAC）
- **非对称加密**：
  - RSA（加解密）
  - ECC（椭圆曲线：点加、点乘、模逆等）
- **其他功能**：
  - Passthrough（数据直通，用于 DMA 或位操作）
  - 硬件随机数发生器（RNG）
  - DMA 引擎集成（`ccp-dmaengine.c`）

> 驱动通过将用户请求转化为 CCP 硬件命令（`ccp_cmd`），利用 DMA、scatterlist、命令队列、中断/任务队列等机制，实现了异步、高性能的硬件加速。

### 2. **Secure Processor (SP) 抽象层**
- `sp-dev.c` / `sp-pci.c` / `sp-platform.c` 提供了对“安全处理器”（SP）设备的抽象。
- SP 是一个包含 CCP（加密）和 PSP（平台安全）的统一设备。
- 支持 PCI 和 platform（ACPI/OF）两种设备发现方式。

### 3. **Platform Security Processor (PSP) 与 SEV（Secure Encrypted Virtualization）**
- **SEV 支持**（`sev-dev.c`）：
  - 提供 `/dev/sev` 字符设备，用户态可通过 ioctl 与 AMD SEV 固件交互。
  - 支持 SEV（传统加密虚拟机）、SEV-ES（加密 + 寄存器状态保护）、SEV-SNP（安全嵌套分页，带完整性保护）。
  - 实现了 INIT、LAUNCH、ACTIVATE、SHUTDOWN、ATTESTATION 等关键命令。
  - 集成 INIT_EX（带非易失配置）、SNP_INIT_EX（带内存范围列表）等高级功能。
  - 支持 panic 时自动 SNP shutdown，确保安全。

- **TEE（可信执行环境）**（`tee-dev.c`）：
  - 通过 ring buffer 与 PSP 上运行的 TEE OS 通信。
  - 提供 `psp_tee_process_cmd()` 供其他内核模块调用。

- **DBC（Dynamic Boost Control）**（`dbc.c`）：
  - 允许授权用户通过 ioctl 动态调整 CPU 频率、功耗上限等（需认证）。

- **HSTI（Hardware Security Test Interface）**（`hsti.c`）：
  - 暴露平台安全属性（如 fuse 状态、debug lock、TSME、anti-rollback 等）到 sysfs。

- **Platform Access**（`platform-access.c`）：
  - 提供通用 mailbox 机制，用于与 PSP 固件通信（如 HSTI 查询、DBC）。

### 4. **底层硬件操作（`ccp-dev-v3.c` / `ccp-dev-v5.c`）**
- 针对不同 CCP 硬件版本（v3 / v5）实现寄存器操作、命令队列管理、中断处理、KSB/LSB（密钥/上下文存储区）分配等。
- 包含 debugfs 支持（`ccp-debugfs.c`），可查看队列状态、操作计数等。

### 5. ccp 是 AMD 独有的
从你提供的 `ccp_c_files_concatenated.txt` 文件内容来看，它完整地实现了 **AMD Cryptographic Coprocessor (CCP)** 及其扩展功能（如 **Secure Processor (SP)**、**Platform Security Processor (PSP)**、**SEV/SEV-ES/SEV-SNP**、**TEE**、**DBC**、**HSTI** 等），这些全部是 **AMD 平台特有的硬件安全与加密加速功能**，具体体现在以下几个方面：

1. **硬件架构绑定**
	- CCP（Cryptographic Coprocessor）是 AMD CPU（特别是 EPYC 系列）中集成的专用硬件加速引擎，用于执行 AES、SHA、RSA、ECC、DES3、XTS、GCM、CMAC 等密码学操作。
	- 代码大量使用 AMD 特定的寄存器布局（如 `CMD_REQ0`, `Q_MASK_REG`, `IRQ_STATUS_REG` 等）和硬件命令格式。
	- 有明确的版本区分（如 `CCP_VERSION(3, 0)` 和 `CCP_VERSION(5, 0)`），对应不同代的 AMD CPU（如 Zen、Zen 2/3）。

2. **机密计算技术（Confidential Computing）**
	- **SEV（Secure Encrypted Virtualization）**、**SEV-ES**、**SEV-SNP** 是 AMD 独有的虚拟机内存加密与完整性保护技术，用于保护虚拟机免受 hypervisor 或物理攻击。
	- 这些功能依赖 PSP（Platform Security Processor）固件，而 PSP 是 AMD Secure Processor 的一部分，**Intel 和 ARM 没有等效实现**（Intel 有 TDX，ARM 有 CCA，但机制和接口完全不同）。

3. **驱动模块与 PCI ID 绑定**
	- 代码中注册的 PCI 设备 ID（如 `0x1537`, `0x1456`, `0x1486`, `0x15DF` 等）都是 **AMD 官方分配的 Secure Processor / CCP 设备 ID**。
	- 驱动通过 `sp_pci_table` 与这些 AMD 特定的硬件匹配。

4. **与其他厂商对比**
	- **Intel** 使用 **QuickAssist (QAT)** 或 **AES-NI + SGX/TDX** 提供类似功能，但驱动和接口完全不同。
	- **ARM** 平台通常依赖 TrustZone 或 SMMU + 固件实现 TEE，也没有 CCP/PSP 这类模块。
	- 因此，该代码 **无法在非 AMD 平台上运行**，也 **不适用于其他厂商的加密或安全处理器**。

## 什么是 hygon tkm
<!-- c93018ab-bb5d-4124-8f4c-6b194e3c42af -->

tkm，很可能是 Hygon（海光）平台在国产化或 OpenAnolis 生态中引入的特定可信密钥管理模块，属于海光对 AMD PSP/CCP 技术的扩展或定制，
并未合并到主线 Linux 内核的 CCP/PSP 驱动中。

因此：
在标准 AMD 平台的 Linux 内核驱动（即你上传的代码）中，不存在 tkm。
如果你在 Hygon 平台或特定发行版（如 OpenAnolis）中看到 tkm，那属于厂商定制模块，需查阅其专属代码仓库或文档。

 `tkm`，这很可能是 **Hygon 在 Secure Processor 中新增的密钥管理模块**，用于：
- 管理 CSV VM 的加密密钥
- 支持国密算法（SM2/SM3/SM4）
- 与国产 TEE 或 HSM 对接

它 **不在 AMD PSP 标准功能中**，属于 Hygon 对 PSP 的 **安全扩展**。

## 什么是 amd psp
<!-- dbc5b1f2-c885-40fb-8b7c-042b3746c5cb -->

PSP（Platform Security Processor）和 CCP（Cryptographic Coprocessor）是 **AMD 安全处理器（AMD Secure Processor）中两个紧密集成但功能不同的硬件组件**。它们的关系可以从硬件、软件和功能三个层面来理解：

PSP 和 CCP 在一个芯片中:
- **CCP**：用于高速对称/非对称加解密（AES、SHA、RSA、ECC 等）
- **PSP**：用于平台安全功能（安全启动、密钥管理、SEV、TEE 等）

| 组件    | 主要职责               | 典型功能                                                                                                                                            |
|---------|------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------|
| **CCP** | **通用密码加速**       | - AES（ECB/CBC/CTR/XTS/GCM/CMAC）<br> - SHA-1/224/256/384/512<br> - RSA/ECC<br> - 硬件随机数（TRNG）                                                |
| **PSP** | **平台安全与可信计算** | - 安全启动（Secure Boot）<br> - SEV/SEV-ES/SEV-SNP（虚拟机内存加密）<br> - TEE（可信执行环境）<br> - DBC（动态性能控制）<br> - HSTI（安全状态报告） |

PSP 依赖密钥和加密操作，**部分功能（如 SEV、TEE）内部会调用 CCP 引擎执行底层加密**，但对用户透明。

- **同一个 PCIe/ACPI 设备**会被 `sp-dev.c` 识别为 `sp_device`（Secure Processor device）。
- 然后根据设备能力，**分别初始化 CCP 和 PSP 子系统**：
  - `ccp_dev_init()` → 加载 `ccp` 驱动，注册 crypto API（如 `aes-ccp`、`sha256-ccp`）
  - `psp_dev_init()` → 加载 `psp` 驱动，暴露 `/dev/sev`、TEE、DBC、HSTI 等接口

> 虽然代码分开（`ccp-*.c` 与 `psp/sev/tee/hsti.c`），但它们共享：
> - 同一 I/O memory 映射（`sp->io_map`）
> - 同一中断线（通过 `sp_irq_handler` 分发）
> - 同一设备上下文（`struct sp_device`）


PSP 和 CCP 共享密钥存储和命令队列

- **KSB/LSB（Key/Local Storage Block）**：CCP 和 PSP 都使用同一组安全存储单元来暂存密钥或上下文。
- **命令队列**：CCP 有自己的命令队列（`cmd_q`），PSP 子功能（如 SEV）通过 mailbox 与 PSP 固件通信，但底层可能复用部分硬件调度逻辑。


| 用户场景        | 暴露接口                                    | 依赖组件                           |
|-----------------|---------------------------------------------|------------------------------------|
| 使用 AES 加速   | `openssl`、`AF_ALG`、`/proc/crypto`         | **CCP**                            |
| 启动 SEV 虚拟机 | `QEMU` + `/dev/sev`                         | **PSP**（内部可能用 CCP 加密密钥） |
| 查询安全状态    | `sysfs`（如 `/sys/devices/.../fused_part`） | **PSP + HSTI**                     |
| 使用硬件 RNG    | `/dev/hwrng`                                | **CCP**（TRNG 引擎）               |

总结:

**PSP 和 CCP 是 AMD Secure Processor 的两个功能模块**：
- **CCP = 密码加速器**（面向通用加密）
- **PSP = 安全控制器**（面向平台信任与机密计算）
- **硬件一体，软件解耦，功能互补**

## CSV
<!-- fba8b60a-7111-42b7-ad88-2ee9efa5abc2 -->

CSV（China Secure Virtualization）是 **Hygon（海光）** 在其国产 x86 处理器上实现的 **安全虚拟化技术**，它与你之前讨论的 **AMD PSP/CCP/SEV 技术栈** 有 **高度的继承与定制关系**，但也有 **关键的差异化设计**。可以将其理解为 **Hygon 基于 AMD SEV 技术的自主演进版本**。

下面从多个维度说明 CSV 与 AMD PSP/CCP/SEV 的关系：

### 一、**架构继承关系：CSV 基于 AMD SEV**

Hygon 的处理器（如 Dhyana 系列）是基于 **AMD Zen 微架构授权**开发的，因此其安全模块（Secure Processor）在硬件和固件层面 **最初完全兼容 AMD PSP/SEV**。

- **CSV（第 1 代） ≈ AMD SEV**
  提供 **VM 内存加密**，每个 VM 有独立密钥，由 Secure Processor 管理。
  - 对应 AMD SEV（Secure Encrypted Virtualization）

- **CSV2（第 2 代） ≈ AMD SEV-ES**
  在内存加密基础上，**保护 vCPU 寄存器状态**，防止 Host 恶意读取/篡改。
  - 对应 AMD SEV-ES（SEV with Encrypted State）

- **CSV3（第 3 代） ≈ AMD SEV-SNP + 自主增强**
  引入 **内存隔离（Memory Isolation）** 和 **硬件级访问控制**，类似 SEV-SNP 的 **RMP（Restricted Memory Protection）**，但实现方式不同：
  - CSV3 使用 **专用隔离硬件** 和 **CMA（Contiguous Memory Allocator）** 管理安全内存；
  - AMD SEV-SNP 使用 **RMP Table + SNP Page State**，由 CPU MMU 硬件强制执行。

> ✅ **结论**：CSV 是 Hygon 在 AMD SEV 技术基础上，结合中国安全需求和自主可控目标，进行深度定制和增强的产物。

### 二、**硬件实现差异**

| 功能         | AMD SEV/SEV-SNP                                      | Hygon CSV/CSV3                                        |
|--------------|------------------------------------------------------|-------------------------------------------------------|
| 安全处理器   | PSP（Platform Security Processor）                   | Hygon Secure Processor（兼容 PSP 接口）               |
| 内存加密引擎 | 集成在 Memory Controller / IF                        | 类似，但可能微调                                      |
| 内存隔离机制 | RMP（Restricted Memory Protection） + SNP Page State | **专用隔离硬件 + CMA 内存池**                         |
| 安全内存分配 | 由 Hypervisor 分配，Firmware 标记页状态              | 必须通过 **CMA 预留**（`csv_mem_size`）               |
| 寄存器保护   | SEV-ES：VMSA 加密                                    | CSV2：类似，寄存器状态加密                            |
| 共享内存机制 | SNP：通过 `PAGE_SHARED` 位和 `VMGEXIT`               | CSV3：通过 **专用 fault page + secure call** 触发 NPF |

> CSV3 的 **secure call + nested page fault** 机制，是 Hygon 独有的设计，不同于 AMD 的 `VMGEXIT`。

### 三、**软件接口兼容性**

- **早期 CSV（CSV1/CSV2）**：完全兼容 `/dev/sev` ioctl 接口，可直接使用 AMD `sev-tool`。
- **CSV3**：虽然保留了部分 SEV 接口，但引入了 **新的管理范式**（如 CMA 预留、secure call），需要：
  - 内核配置 `CONFIG_HYGON_CSV=y`
  - 使用 Hygon 特定的 QEMU/KVM 补丁（来自 OpenAnolis / Hygon 官方）
  - 可能需要定制 OVMF（如 `HygonOvmf`）

> 🔒 CSV3 的 **memory isolation** 是其核心安全增强，但这也导致它 **无法完全兼容标准 SEV-SNP 软件栈**。

### 四、**检测与激活方式对比**

| 检测项 | AMD SEV | Hygon CSV |
|-------|--------|----------|
| CPUID | `0x8000001f[EAX].bit0`（SEV） | `0x8000001f[EAX].bit1`（CSV）<br>`bit3`（CSV2）<br>`bit30`（CSV3） |
| MSR 状态 | `0xc0010131`（SEV/SEV-ES/SNP） | `0xc0010131`（CSV/CSV2/CSV3），**复用相同 MSR** |
| 内核配置 | `CONFIG_AMD_MEM_ENCRYPT`, `CONFIG_KVM_AMD_SEV` | `CONFIG_HYGON_CSV`, `CONFIG_CMA` |
| 内存预留 | `sev=on`（由 Host 动态分配） | **必须静态预留 CMA**（`csv_mem_size`） |

> ✅ Hygon **复用了 AMD 的 MSR 和部分 CPUID 布局**，但重新定义了位含义。

### 五、**总结：CSV 与 PSP/CCP/SEV 的关系**

| 层面 | 关系 |
|------|------|
| **血缘** | CSV 是 AMD SEV/SEV-ES/SEV-SNP 的 **Hygon 定制分支** |
| **硬件** | 基于 AMD PSP 架构，但增加了 **专用内存隔离硬件** |
| **固件** | Secure Processor 固件由 Hygon 自主开发，**兼容 PSP mailbox 协议**，但命令集有扩展（如 `tkm`、secure call） |
| **驱动** | Linux 内核中，CSV 通常作为 **AMD SEV 的扩展或替代实现**（如 `drivers/virt/coco/hygon/`） |
| **应用场景** | 面向中国信创/等保/机密计算市场，强调 **自主可控 + 强隔离** |


### 最终结论

> **CSV 是 Hygon 基于 AMD PSP/SEV 技术栈，深度定制并增强的安全虚拟化方案**。
> - CSV1 ≈ SEV
> - CSV2 ≈ SEV-ES
> - CSV3 ≈ SEV-SNP + 独有的内存隔离硬件 + CMA 管理模型
>
> 它保留了 AMD 的基础接口（如 MSR、部分 CPUID、mailbox），但在安全机制、内存管理和软件栈上有显著差异化设计，以满足中国市场的合规与安全需求。

## SEV SEV-ES 和 SEV-SNP 是什么关系?
<!-- 1d7c6321-3e62-4fa3-b170-9d2a07f2301b -->

SEV、SEV-ES 和 SEV-SNP 是 **AMD Secure Encrypted Virtualization（SEV）技术家族中的三个演进层级**，它们在功能上层层递进，提供越来越强的机密计算（Confidential Computing）安全保障。三者的关系可以用 **“基础 → 增强 → 完整可信”** 来概括。

### 一、SEV（Secure Encrypted Virtualization）
- **目标**：保护虚拟机（VM）**内存内容**不被 Hypervisor 或其他 VM 窃取。
- **核心机制**：
  - 每个 VM 使用**唯一的内存加密密钥**（由硬件 Secure Processor 管理）。
  - 内存读写时自动加解密（AES-128-XTS），对 Guest 透明。
  - Hypervisor 无法读取 Guest 内存明文（即使物理访问内存）。
- **局限性**：
  - **不保护 CPU 寄存器状态**（如 VMSA，Virtual Machine Save Area）。
  - Hypervisor 仍可篡改或伪造 VM 的 CPU 上下文（例如修改 RIP、RSP）。

**SEV = 内存加密**

### 二、SEV-ES（SEV with Encrypted State）
- **在 SEV 基础上增强**：**加密并保护 VM 的 CPU 寄存器状态**。
- **核心机制**：
  - VM 的 VMSA（包含通用寄存器、段寄存器、控制寄存器等）也由 Secure Processor 加密存储。
  - VM 退出/进入时，状态通过加密通道在 CPU 和 Secure Processor 间交换。
  - Hypervisor 无法读取或篡改 VM 的 CPU 上下文。
- **安全提升**：
  - 防止 Hypervisor 通过修改寄存器（如注入恶意代码、劫持控制流）攻击 Guest。
- **局限性**：
  - **不提供内存完整性保护**：Hypervisor 仍可重放、丢弃或伪造内存页（例如回滚到旧状态）。
  - **无硬件级隔离**：无法阻止某些侧信道或 ROP 攻击。

**SEV-ES = SEV + 寄存器状态加密**

### 三、SEV-SNP（SEV with Secure Nested Paging）
- **在 SEV-ES 基础上进一步增强**：**增加内存完整性、防重放、防篡改、防伪造能力**。
- **核心机制**：
  - 引入 **RMP（Restricted Mode Page）表**：由硬件维护，记录每一页内存的**所有者**（Host / 特定 VM）、**权限**（Private / Shared）、**状态**（Assigned / Unassigned）。
  - 所有内存访问（包括 Host）都受 RMP 检查，违反策略触发 #VMEXIT 或 #NPF。
  - 支持 **内存页完整性校验**：防止页被篡改或回滚。
  - 引入 **反重放计数器（TCB 版本）** 和 **安全启动度量**。
  - 提供 **Guest 证明（Attestation）** 能力，可向第三方证明 VM 完整性。
- **安全目标**：
  - 防止 Hypervisor：
    - 重放（Replay）内存页
    - 伪造（Fabricate）新页
    - 丢弃（Suppress）页
    - 修改页权限或内容
  - 实现 **硬件强制的内存隔离**。

**SEV-SNP = SEV-ES + 内存完整性 + 硬件强制隔离 + 证明**

### 四、三者关系总结

| 功能                   | SEV        | SEV-ES     | SEV-SNP      |
|------------------------|------------|------------|--------------|
| 内存加密               | ✅         | ✅         | ✅           |
| 寄存器状态加密         | ❌         | ✅         | ✅           |
| 内存完整性保护         | ❌         | ❌         | ✅           |
| 防 Hypervisor 篡改内存 | ❌         | ❌         | ✅           |
| 硬件强制隔离（RMP）    | ❌         | ❌         | ✅           |
| 安全启动与证明         | ❌         | ❌         | ✅           |
| 安全级别               | 基础机密性 | 增强机密性 | 完整机密计算 |

> 🔒 **SEV-SNP 是目前 AMD 最高级别的机密虚拟化技术**，对标 Intel TDX 和 ARM CCA。

---

### 五、兼容性与启用关系
- **SEV-SNP 启用时，自动包含 SEV 和 SEV-ES 的所有功能**。
- **不能单独启用 SEV-SNP 而不启用 SEV-ES**。
- 在 Linux 中：
  - 通过 `/dev/sev` ioctl 接口管理。
  - QEMU 使用 `-object sev-guest,...,policy=...` 指定启用级别。
  - 需要 CPU 支持（EPYC Milan 及以后）、固件支持、内核和 OVMF 支持。

---

### 六、Hygon CSV 对应关系（补充）
- **CSV ≈ SEV**
- **CSV2 ≈ SEV-ES**
- **CSV3 ≈ SEV-SNP**（但使用 CMA + 专用隔离硬件，而非 RMP）


**SEV 提供内存加密，SEV-ES 加密 CPU 状态，SEV-SNP 在此基础上增加硬件强制的内存完整性与隔离，构成完整机密虚拟化方案。**

## 总结 hct tkm 等各种功能
<!-- 2fb3eb52-7150-46c9-92e4-b419359ca9a7 -->

1. ccp : 加密加速
2. sev : 可信计算
3. hct (hygon) : ccp 的 mdev 管理机制
4. tkm (hygon) : 密钥管理，无需 qemu 参与
5. csv

## 然后继续这里的东西
https://openanolis.cn/sig/Hygon-Arch/doc/1110520213566721211?lang=zh

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
