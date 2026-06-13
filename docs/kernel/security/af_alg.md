## libkcapi : 为什么需要从内核中获取加密功能
<!-- a834610e-76d5-4f22-8573-044fe618968b -->

在用户态不应该是直接调用加速指令不就完了?

所以，我猜测本质的原因是有的加密功能太复杂了，不能是简单的指令。

> [!NOTE]
> 参考 Deepseeek ，有待验证

`libkcapi` 是一个用于 Linux 系统的用户空间（userspace）C 语言库，它为内核提供的 **加密 API（Kernel Crypto API）** 提供了方便的封装接口。其主要目的是让应用程序能够高效、安全地使用 Linux 内核中实现的加密算法，而无需直接处理复杂的内核接口或依赖用户空间的加密库（如 OpenSSL）。

主要功能和用途：

1. **访问内核加密服务**：
   - Linux 内核从 2.6 版本开始就内置了加密框架（Crypto API），支持多种对称加密（如 AES）、哈希（如 SHA-256）、消息认证码（如 HMAC）等。
   - `libkcapi` 提供了一组简洁的 C API，让应用程序可以直接调用这些内核级的加密原语。

2. **高性能与低开销**：
   - 由于加密操作在内核中执行，对于某些硬件加速（如 Intel AES-NI、ARM Crypto Extensions）支持良好的系统，`libkcapi` 可以比纯用户空间实现（如 OpenSSL）更高效，尤其在大量小数据加密/解密场景下。
   - 避免了用户空间与内核空间之间不必要的数据拷贝（通过 AF_ALG socket 接口）。

3. **基于 AF_ALG 接口**：
   - `libkcapi` 底层使用 Linux 的 **AF_ALG** 地址族（Address Family for ALGorithms）套接字接口，这是内核为用户空间提供加密服务的标准方式。
   - 应用程序通过 `socket(AF_ALG, ...)` 与内核加密模块通信，`libkcapi` 对此进行了封装，简化了开发。

4. **支持的算法**：
   - 对称加密：AES、DES、3DES 等（支持 ECB、CBC、CTR、GCM 等模式）
   - 哈希函数：MD5、SHA-1、SHA-224/256/384/512 等
   - 消息认证码：HMAC、CMAC
   - 随机数生成（通过 `/dev/random` 或内核 RNG）
   - 密钥派生函数（如 PBKDF2）

5. **典型应用场景**：
   - 安全启动（Secure Boot）相关工具
   - 加密文件系统或磁盘加密工具
   - 需要与内核加密子系统紧密集成的安全应用
   - 嵌入式系统中资源受限但需要硬件加速加密的场景

示例代码（使用 libkcapi 进行 SHA-256 哈希）：

```c
#include <kcapi.h>
#include <stdio.h>

int main() {
    struct kcapi_handle *handle;
    const char *data = "Hello, libkcapi!";
    unsigned char digest[32];
    int ret;

    ret = kcapi_md_init(&handle, "sha256", 0);
    if (ret)
        return -1;

    kcapi_md_update(handle, (const uint8_t *)data, strlen(data));
    kcapi_md_final(handle, digest, sizeof(digest));

    for (int i = 0; i < 32; i++)
        printf("%02x", digest[i]);
    printf("\n");

    kcapi_md_destroy(handle);
    return 0;
}
```

与 OpenSSL 的区别：

| 特性             | libkcapi                     | OpenSSL                     |
|------------------|------------------------------|-----------------------------|
| 执行位置         | 内核空间（通过 AF_ALG）      | 用户空间                    |
| 硬件加速利用     | 直接使用内核支持的硬件加速   | 依赖 OpenSSL 的引擎支持     |
| 性能（小数据）   | 通常更高（减少上下文切换）   | 可能有额外开销              |
| 功能丰富度       | 有限（仅内核支持的算法）     | 非常丰富（TLS、X.509 等）   |
| 适用场景         | 系统级、嵌入式、安全关键应用 | 通用加密、网络通信等        |

`libkcapi` 是一个轻量级、高性能的库，用于在 Linux 用户空间程序中安全高效地调用内核提供的加密服务。它特别适合需要与内核加密子系统集成、追求低延迟或利用硬件加速的场景，但不适用于需要复杂协议（如 TLS）或广泛算法支持的应用。


## 观察一下 docs/kernel/security/proc-crypto.txt

## 用户态如何使用内核的加密机制
AMD Cryptographic Coprocessor (CCP) 是内核级的硬件加速模块，主要用于提供对称/非对称加密、哈希、机密计算（如 SEV）等能力。它本身并不直接暴露用户态 API，而是通过 Linux 内核的 **crypto API**（`/proc/crypto`、`AF_ALG`）或 **字符设备**（如 `/dev/sev`）供上层使用。

因此，CCP 配套的用户态工具主要分为以下几类：

### 1. **通用加密工具（通过内核 crypto API）**

这些工具通过 `AF_ALG`（Netlink-based 用户态加密接口）调用内核注册的 CCP 算法（如 `aes-ccp`、`sha256-ccp`）：

- **`openssl`（需启用 AF_ALG 引擎）**
  OpenSSL 可通过 `afalg` 引擎利用 CCP 加速。
  示例（需编译时启用）：
  ```bash
  openssl speed -engine afalg -evp aes-128-cbc
  ```
  或在配置中启用：
  ```ini
  [default]
  afalg = AFALG
  ```

- **`cryptsetup`（LUKS）**
  若内核 crypto 子系统将 CCP 注册为默认 AES 实现，`cryptsetup` 会自动使用 CCP 加速全盘加密。

- **`libkcapi` + `kcapi-speed`**
  `libkcapi` 是用户态对 `AF_ALG` 的封装库，配套工具 `kcapi-speed` 可测试硬件加速性能：
  ```bash
  kcapi-speed --cipher aes-cbc --keysize 256
  ```

### 2. **SEV/机密计算专用工具（通过 `/dev/sev`）**

CCP 驱动栈中的 **PSP/SEV** 部分会创建 `/dev/sev` 字符设备，配套工具包括：

- **`sev-tool`（官方参考实现）**
  AMD 提供的 [sev-tool](https://github.com/AMDESE/sev-tool) 可执行：
  - 平台初始化（`platform init`）
  - 生成 CSR（`pek csr`）
  - 导入证书（`pek cert import`）
  - 启动/测量 SEV 虚拟机
  - 生成认证报告（`attestation report`）

- **`QEMU`（SEV/SEV-ES/SEV-SNP 支持）**
  QEMU 通过 `ioctl` 与 `/dev/sev` 交互，启动加密虚拟机：
  ```bash
  qemu-system-x86_64 \
    -machine q35,confidential-guest-support=sev0 \
    -object sev-guest,id=sev0,cbitpos=47,reduced-phys-bits=1
  ```
**提示**：多数场景下，用户无需直接操作 CCP，系统会自动选择最优加速路径（如通过 `cryptd` + `ccp`）。
只有在机密计算（SEV）场景才需要主动使用 `sev-tool` 或 QEMU。


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
