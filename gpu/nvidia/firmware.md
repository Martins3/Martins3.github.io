## nvidia 的 GSP 固件，是从文件系统中加载的，还是板载的
<!-- 97bd17e6-ca0b-4736-9190-85fa7f880f4a -->

文件系统中加载的，没仔细的定位，但是从这个版本就可以知道了:
```txt
🧀  modinfo nvidia
filename:       /lib/modules/7.0.8-200.fc44.x86_64/kernel/drivers/video/nvidia.ko
import_ns:      DMA_BUF
alias:          char-major-195-*
description:    NVIDIA core GPU kernel module
version:        595.71.05
supported:      external
license:        Dual MIT/GPL
firmware:       nvidia/595.71.05/gsp_tu10x.bin
firmware:       nvidia/595.71.05/gsp_ga10x.bin

nvidia-smi -q | grep -i firmware
    GSP Firmware Version                               : 595.71.05
            Firmware                                   : N/A

ls -la /usr/lib/firmware/nvidia/595.71.05/gsp_tu10x.bin
.r--r--r-- 30M root 24 Apr 13:42 /usr/lib/firmware/nvidia/595.71.05/gsp_tu10x.bin
```
这里，我其实有点怀疑 modinfo nvidia 的结果根据 /usr/lib/firmware 中自动计算的。

板载的固件为:
```txt
nvidia-smi -q | grep -i vbios
```

## falcon
[Documentation/gpu/nova/core/falcon.rst][https://www.kernel.org/doc/html/latest/gpu/nova/core/falcon.html)

GSP 本质上是一个高性能的 Falcon 处理器（通常是一颗基于 RISC-V 的高性能核心）。

## 2026-06-13 kimi 分析 nova-core 源码

drivers/gpu/nova-core/vbios.rs：VBIOS 提取、BIT 表解析、FwSec 微码定位。
drivers/gpu/nova-core/gsp/boot.rs:140：在 GSP 启动时构造 Vbios。
drivers/gpu/nova-core/firmware/fwsec.rs:264：从 Vbios 构建 FWSEC 固件对象并 patch 命令/签名。
drivers/gpu/nova-core/firmware.rs:35：GSP 固件版本号 "570.144"，VBIOS 提取与 GSP 固件是独立的两套固件来源。

总结：Nova 把 VBIOS 当作 “只读 ROM 仓库” 来处理，核心目标是从中安全地挖出 FwSec 微码及其签名，然后交给 GSP Falcon 运行 FWSEC-FRTS，
从而建立受保护的 WPR2 区域并完成 GPU 的初始化起点。

## 2026-06-13 kimi 分析 open-gpu-kernel-modules 源码，得到结论 VBIOS 和 GSP 如何工作

1. VBIOS 的处理

1.1 VBIOS 从哪来

这个开源驱动不自带 VBIOS 文件，VBIOS 是从 GPU 的 PROM / Expansion ROM（PCI Option ROM） 里读出来的。

主要入口：

```c
  // src/nvidia/src/kernel/gpu/gsp/arch/turing/kernel_gsp_vbios_tu102.c:442
  NV_STATUS kgspExtractVbiosFromRom_TU102(...)
```

它通过 NV_PROM_DATA(offset) 直接读 PROM 寄存器，把 VBIOS 整个镜像复制到内核非分页内存里。

1.2 VBIOS 的用途

在现在的 GSP-RM 架构下，VBIOS 主要被用来做一件事：解析 FWSEC 微码，用于 FRTS（安全启动相关）。

```c
  // src/nvidia/src/kernel/gpu/gsp/kernel_gsp.c:4826
  if (pKernelGsp->pFwsecUcode == NULL)
  {
      status = kgspExtractVbiosFromRom_HAL(...);
      status = kgspParseFwsecUcodeFromVbiosImg(...);  // 解析 BIT/FWSEC
      kgspFreeVbiosImg(pVbiosImg);
  }
```

也就是说：
• 驱动从 PROM 把 VBIOS 读出来；
• 解析 BIT（BIOS Information Table）找到 FWSEC 描述符；
• 把 FWSEC ucode 交给 SEC2 运行，为 GSP-RM 的 WPR2 安全环境做准备；
• 读完解析完就释放 VBIOS 镜像，只在 pKernelGsp->vbiosVersionStr 里保留版本号。

1.3 关键文件

┌────────────────────────────────────────────────────────────────────┬──────────────────────────────────────────────┐
│ 文件                                                               │ 作用                                         │
├────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────┤
│ src/nvidia/src/kernel/gpu/gsp/arch/turing/kernel_gsp_vbios_tu102.c │ 从 PROM 读 VBIOS 镜像                        │
├────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────┤
│ src/nvidia/src/kernel/gpu/gsp/kernel_gsp_fwsec.c                   │ 解析 BIT、FWSEC ucode                        │
├────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────┤
│ src/nvidia/src/kernel/gpu/gsp/kernel_gsp.c                         │ kgspInitRm_IMPL，GSP-RM 初始化入口           │
├────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────┤
│ src/nvidia/arch/nvalloc/unix/src/osinit.c                          │ RmInitAdapter、RmGetVbiosVersion             │
├────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────┤
│ src/nvidia/arch/nvalloc/unix/src/vbioscall.c                       │ 遗留的 INT10h/x86emu VGA 控制台路径          │
├────────────────────────────────────────────────────────────────────┼──────────────────────────────────────────────┤
│ src/nvidia/generated/rmconfig.h:690                                │ RMCFG_MODULE_VBIOS 0，legacy OBJVBIOS 已关闭 │
└────────────────────────────────────────────────────────────────────┴──────────────────────────────────────────────┘

1.4 版本号怎么暴露给用户

初始化完成后，驱动通过 RMAPI 控制命令 NV2080_CTRL_CMD_BIOS_GET_INFO_V2 获取版本号，写入 cached_gpu_info.vbios_version，最终在
/proc/driver/nvidia/gpus/*/information 里显示：

```c
  // kernel-open/nvidia/nv-procfs.c:143
  "Video BIOS: \t %s\n"
```

1.5 遗留路径

在 x86_64 主显卡上，驱动还会用内部 x86 模拟器执行 VBIOS 的 INT10h 调用来做控制台状态保存/恢复，文件是 src/nvidia/arch/nvalloc/unix/src/vbioscall.c 和
x86emu/sys.c。这个和 GSP-RM 启动路径是并行的。

────────────────────────────────────────────────────────────────────────────────

2. GSP 固件的处理

2.1 GSP 固件从哪来

GSP 固件是随驱动分发的二进制文件，放在 /lib/firmware/nvidia/<驱动版本>/ 下：

```c
  // kernel-open/nvidia/nv.c:27
  #define NV_FIRMWARE_FOR_NAME(name)  "nvidia/" NV_VERSION_STRING "/" name ".bin"

  // 例如：
  // /lib/firmware/nvidia/570.86.15/gsp_ga10x.bin
  // /lib/firmware/nvidia/570.86.15/gsp_tu10x.bin
  // /lib/firmware/nvidia/570.86.15/gsp_log_ga10x.bin
```

芯片家族映射在 kernel-open/common/inc/nv-firmware.h:32-145：

┌──────────────────────────────────────────┬────────────┬───────────────┐
│ 芯片家族                                 │ GSP 固件名 │ 日志固件名    │
├──────────────────────────────────────────┼────────────┼───────────────┤
│ GA10X / AD10X / GH100 / GB10X / GB20X 等 │ gsp_ga10x  │ gsp_log_ga10x │
├──────────────────────────────────────────┼────────────┼───────────────┤
│ GA100 / TU10X / TU11X                    │ gsp_tu10x  │ gsp_log_tu10x │
└──────────────────────────────────────────┴────────────┴───────────────┘

2.2 加载接口

Linux 内核侧用标准的 request_firmware()：

```c
  // kernel-open/nvidia/nv.c:4150
  const void* nv_get_firmware(...)
  {
      request_firmware(&fw,
                       nv_firmware_for_chip_family(fw_type, fw_chip_family),
                       nvl->dev);
      ...
  }
```

固件通过 MODULE_FIRMWARE() 在模块里声明（kernel-open/nvidia/nv.c:28-30）。

2.3 固件结构

GSP 固件是一个 ELF 容器，主要包含这些 section：

```c
  // src/nvidia/generated/g_kernel_gsp_nvoc.h:250-260
  typedef struct GSP_FIRMWARE {
      const void *pBuf;           // 完整 ELF
      NvU32       size;
      const void *pImageData;     // .fwimage
      NvU64       imageSize;
      const void *pSignatureData; // .fwsignature_*
      NvU64       signatureSize;
      const void *pLogElf;        // gsp_log_*.bin
      NvU32       logElfSize;
  } GSP_FIRMWARE;
```

关键 section：
• .fwversion：必须匹配 NV_VERSION_STRING
• .fwimage：真正的 GSP-RM payload
• .fwsignature_<chipfamily>：签名
• .fwlogging：日志二进制里的

2.4 初始化/启动流程

从模块加载到 GSP 跑起来的大致路径：

```
  nvidia_init_module()
    └── nv_module_init()
          └── rm_init_rm() / osRmInitRm()
                └── [每个 GPU] nv_pci_probe()
                      └── rm_init_adapter()
                            └── RmInitAdapter()          // src/nvidia/arch/nvalloc/unix/src/osinit.c:2021
                                  ├── RmFetchGspRmImages()   // 调用 nv_get_firmware()
                                  ├── GPU 创建，pGpu->isGspClient = NV_TRUE
                                  └── kgspInitRm()           // src/nvidia/src/kernel/gpu/gsp/kernel_gsp.c:4790
                                        ├── kgspExtractVbiosFromRom_HAL()   // 读 VBIOS
                                        ├── kgspParseFwsecUcodeFromVbiosImg() // 解析 FWSEC
                                        ├── kgspPrepareBootBinaryImage()      // Booter/BL
                                        ├── _kgspPrepareGspRmBinaryImage()    // 解析 ELF，建 radix3 描述符
                                        ├── _kgspInitRpcInfrastructure()      // 分配命令/状态队列
                                        └── _kgspBootGspRm()                  // 启动 GSP-RM
                                              ├── kgspBootstrap_TU102() / kgspBootstrap_GH100()
                                              ├── 运行 FWSEC（如果需要）
                                              ├── 执行 Booter Load
                                              ├── kgspSendInitRpcs()          // GSP_SET_SYSTEM_INFO / SET_REGISTRY
                                              └── 等待 GSP_INIT_DONE
```

架构相关启动代码：
• Turing/Ampere：src/nvidia/src/kernel/gpu/gsp/arch/turing/kernel_gsp_tu102.c:477
• Hopper/Blackwell：src/nvidia/src/kernel/gpu/gsp/arch/hopper/kernel_gsp_gh100.c:935（通过 FSP/SEC2 → GSP-FMC）

2.5 CPU-RM ↔ GSP-RM 通信

驱动和 GSP 之间通过共享内存消息队列 + mailbox 通信：

• 命令队列：CPU → GSP
• 状态队列：GSP → CPU

队列在 src/nvidia/src/kernel/gpu/gsp/message_queue_cpu.c:191 分配，默认各 256 KB，pre-silicon 下命令队列会放大到 1.5 MB 以便发送 VBIOS 镜像。

RPC 收发：

```c
  _kgspRpcSendMessage()   // kernel_gsp.c:400
  _kgspRpcRecvPoll()      // kernel_gsp.c:2680
  kgspRpcRecvEvents_IMPL() // 中断底半部
```

RPC 命令定义在 src/nvidia/inc/kernel/vgpu/rpc_global_enums.h，比如：
• GSP_RM_CONTROL
• GSP_RM_ALLOC
• GSP_SET_SYSTEM_INFO
• GSP_INIT_DONE（事件）

心跳通过 NV_PGSP_MAILBOX_REGISTER_GSPRM_HEARTBEAT / LIBOS_HEARTBEAT 来检测。

2.6 关键结构

┌─────────────────────────────────────┬──────────────────────────────────────────────────────────┐
│ 结构/文件                           │ 说明                                                     │
├─────────────────────────────────────┼──────────────────────────────────────────────────────────┤
│ struct KernelGsp                    │ src/nvidia/generated/g_kernel_gsp_nvoc.h:409             │
├─────────────────────────────────────┼──────────────────────────────────────────────────────────┤
│ GSP_FIRMWARE                        │ 固件 ELF 容器                                            │
├─────────────────────────────────────┼──────────────────────────────────────────────────────────┤
│ GspFwWprMeta                        │ src/nvidia/arch/nvalloc/common/inc/gsp/gsp_fw_wpr_meta.h │
├─────────────────────────────────────┼──────────────────────────────────────────────────────────┤
│ GspStaticConfigInfo / GspSystemInfo │ src/nvidia/inc/kernel/gpu/gsp/gsp_static_config.h        │
├─────────────────────────────────────┼──────────────────────────────────────────────────────────┤
│ MESSAGE_QUEUE_COLLECTION            │ src/nvidia/inc/kernel/gpu/gsp/message_queue_priv.h       │
└─────────────────────────────────────┴──────────────────────────────────────────────────────────┘

2.7 GSP 模式

• PF_KERNEL_ONLY 即 GSP-RM client 模式，IS_GSP_CLIENT(pGpu) 判断。
• pGpu->isGspClient 在 GPU 构造时根据 RmVariantHal 设置。
• 还有 SR-IOV / vGPU GSP offload：IS_VGPU_GSP_PLUGIN_OFFLOAD_ENABLED(pGpu)。

────────────────────────────────────────────────────────────────────────────────

3. 一句话总结

• VBIOS：不随驱动分发，启动时从 GPU PROM 读出来，主要给 GSP-RM 提供 FWSEC 微码；旧显卡上还走 x86emu INT10h 控制台路径。
• GSP 固件：随驱动作为 /lib/firmware/nvidia/<ver>/gsp_*.bin 分发，由 Linux request_firmware() 加载，解析 ELF 后通过 Booter ucode 加载到 WPR2，再通过共享内存 RPC
  队列与 GSP-RM 通信。
