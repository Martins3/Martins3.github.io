## 如何刷 bios
https://www.reddit.com/r/linux_gaming/comments/1n75ygf/guide_using_nvflash_to_readwrite_nvidia_gpu_bios/

## windows 的
https://docs.nvidia.com/vgpu/13.0/grid-licensing-user-guide/index.html#troubleshooting-grid-licensing

A30 仅仅 15 系驱动，后面的驱动都不支持
A6000 需要切换模式，排查方法，如果机器启动的时候没有 SR-IOV 功能了之后:
```txt
$ lspci -s 0000:86:00.0 -v | grep SR-IOV
        Capabilities: [bcc] Single Root I/O Virtualization (SR-IOV)
```

- 调整方法 : http://192.168.17.20/gaomingye/vgpu/Display_Mode-1.67.0.zip
- 适用的卡 : https://developer.nvidia.com/displaymodeselector
  - The NVIDIA Display Mode Selector Tool is a utility to set the desired display mode for NVIDIA L40S, NVIDIA L40, NVIDIA RTX 6000 Ada, NVIDIA A40, NVIDIA RTX A5000, NVIDIA RTX A5500, and NVIDIA RTX A6000 GPUs.
- 注意点 : 先需要删掉驱动，然后再去更新 GPU

## [ ] 需要将 windows 中 WDDM 模式切换
```txt
GPU instance ID                   : N/A
Compute instance ID               : N/A
Process ID                        : 10588
     Type                          : C+G
     Name                          : C:\Windows\SystemApps\Microsoft.Windows.StartMenuExperienceHost_cw5n1h2txyewy\StartMenuExperienceHost.exe
     Used GPU Memory               : Not available in WDDM driver model
```


## 如何添加 vGPU 的 licence
https://docs.nvidia.com/vgpu/13.0/grid-licensing-user-guide/index.html#troubleshooting-grid-licensing

0. Copy the client configuration token to the %SystemDrive%:\Program Files\NVIDIA Corporation\vGPU Licensing\ClientConfigToken folder.
1. On Windows, licensing events are logged in the plain-text file %SystemDrive%\Users\Public\Documents\NvidiaLogging\Log.NVDisplay.Container.exe.log.

nvidia-smi -q 可以看到如下:
```txt
  vGPU Software Licensed Product Product Name : NVIDIA Virtual Compute Server License Status : Licensed (Expiry: 2021-11-13 18:29:59 GMT)
```

https://docs.nvidia.com/vgpu/17.0/grid-licensing-user-guide/index.html#intro-to-grid-licensing
这里有三个 table 的结果如下:

支持的模型为:
|-------|------------------------------------------------|
| vApps | A serias                                       |
| vPC   | B-series NVIDIA vGPUs                          |
| vWS   | Q-series NVIDIA vGPUs && B-series NVIDIA vGPUs |

是驱动的版本问题吗?


之前报错为:
```txt
Tue Jan 21 19:54:14 2025:<1>:Failed to acquire license from api.cls.licensing.nvidia.com (Info: NVIDIA RTX Virtual Workstation - Error: No pool features found for : NVIDIA RTX Virtual Workstation)
Tue Jan 21 19:54:38 2025:<1>:Failed to acquire license from api.cls.licensing.nvidia.com (Info: NVIDIA RTX Virtual Workstation - Error: No pool features found for : NVIDIA RTX Virtual Workstation)
Tue Jan 21 19:55:06 2025:<1>:Failed to acquire license from api.cls.licensing.nvidia.com (Info: NVIDIA RTX Virtual Workstation - Error: No pool features found for : NVIDIA RTX Virtual Workstation)
Tue Jan 21 19:55:39 2025:<1>:Failed to acquire license from api.cls.licensing.nvidia.com (Info: NVIDIA RTX Virtual Workstation - Error: No pool features found for : NVIDIA RTX Virtual Workstation)
```

```txt
Wed Jan 22 10:42:33 2025:<1>:Failed to acquire license from api.cls.licensing.nvidia.com (Info: NVIDIA Virtual PC - Error: No pool features found for : NVIDIA Virtual PC)
Wed Jan 22 10:42:57 2025:<1>:Failed to acquire license from api.cls.licensing.nvidia.com (Info: NVIDIA Virtual PC - Error: No pool features found for : NVIDIA Virtual PC)
```

在看看这个文档，似乎就是如此:
https://www.nvidia.com/content/dam/en-zz/Solutions/design-visualization/solutions/resources/documents1/nvidia-virtual-compute-server-solution-overview.pdf

注意，licence 不具有兼容性，必须在任何位置使用。

## 常见文档收集
- k8s  : https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/latest/gpu-sharing.html

## vGPU : https://docs.nvidia.com/vgpu/

似乎以前叫 Grid ，后来叫做 vGPU ，

- Virtual GPU Software for Every Workload
  - vWS
  - vPC
  - vApps

## nvidia-smi
<!-- b0edf153-72ec-4d49-bc37-1e47de888398 -->

### 查询
nvidia-smi -q 检查基本消息

### nvidia-smi dmon
```txt
nvidia-smi dmon --select=pucvmet --options=DT --count=10
#Date        Time         gpu    pwr  gtemp  mtemp     sm    mem    enc    dec    jpg    ofa   mclk   pclk  pviol  tviol     fb   bar1   ccpm  sbecc  dbecc    pci  rxpci  txpci
#YYYYMMDD    HH:MM:SS     Idx      W      C      C      %      %      %      %      %      %    MHz    MHz      %   bool     MB     MB     MB   errs   errs   errs   MB/s   MB/s
 20260816    09:22:34       0      6     38      -      0      0      0      0      0      0    405    180      0      0      2      2      0      -      -      0      0      2
 20260816    09:22:35       0      6     38      -      0      0      0      0      0      0    405    180      0      0      2      2      0      -      -      0      0      0
 20260816    09:22:36       0      6     38      -      0      0      0      0      0      0    405    180      0      0      2      2      0      -      -      0      0      2
 20260816    09:22:37       0      6     38      -      0      0      0      0      0      0    405    180      0      0      2      2      0      -      -      0      0 ^C     0
```

这条命令用于连续监控 NVIDIA GPU 的运行状态：

```bash
nvidia-smi dmon --select=pucvmet --options=DT --count=10
```

- `--select=pucvmet`：选择要显示的指标组：
  - `p`：功耗、温度
  - `u`：各计算/编解码单元利用率
  - `c`：显存与核心时钟、限频状态
  - `v`：显存、BAR1 等内存占用
  - `m`：显存相关信息
  - `e`：ECC 与 PCIe 错误
  - `t`：PCIe 接收/发送吞吐量

| 字段    |   单位 | 含义                                                                                         |
| ------- | -----: | -------------------------------------------------------------------------------------------- |
| `gpu`   |   编号 | GPU 索引，从 `0` 开始。多张卡时会出现 `0、1、2…`                                             |
| `pwr`   |      W | GPU 当前功耗                                                                                 |
| `gtemp` |     °C | GPU 核心温度                                                                                 |
| `mtemp` |     °C | 显存温度；`-` 表示硬件或驱动不支持读取                                                       |
| `sm`    |      % | Streaming Multiprocessor 利用率，反映 CUDA/图形计算核心有多忙                                |
| `mem`   |      % | 显存控制器利用率，即采样期间显存读写单元忙碌的时间比例；**不是显存占用百分比**               |
| `enc`   |      % | NVENC 硬件视频编码器利用率                                                                   |
| `dec`   |      % | NVDEC 硬件视频解码器利用率                                                                   |
| `jpg`   |      % | 硬件 JPEG 编解码单元利用率                                                                   |
| `ofa`   |      % | Optical Flow Accelerator（光流加速器）利用率，用于光流估算、帧生成等                         |
| `mclk`  |    MHz | 显存当前时钟频率，即 Memory Clock                                                            |
| `pclk`  |    MHz | GPU 处理器/核心当前时钟频率，即 Processor Clock                                              |
| `pviol` |      % | 因功耗限制而发生降频的时间比例。长期较高表示 GPU 经常撞到功耗上限                            |
| `tviol` | 布尔值 | 是否因温度限制发生降频；`0` 表示没有，`1` 表示发生                                           |
| `fb`    |     MB | 已使用的板载 Frame Buffer 显存，也就是通常所说的显存占用                                     |
| `bar1`  |     MB | 已使用的 BAR1 映射空间。BAR1 将部分 GPU 显存映射给 CPU 或其他 PCIe 设备直接访问              |
| `ccpm`  |     MB | Confidential Compute Protected Memory，机密计算保护显存的占用量；普通显卡/未启用时通常为 `0` |
| `sbecc` |     次 | 单比特 ECC 错误数。一般可以被 ECC 自动纠正                                                   |
| `dbecc` |     次 | 双比特 ECC 错误数。通常不可纠正，需要重点关注                                                |
| `pci`   |     次 | PCIe Replay 错误/重传计数，表示 PCIe 数据包需要重新传输                                      |
| `rxpci` |   MB/s | GPU 通过 PCIe 接收数据的速度，通常是主机到 GPU                                           |
| `txpci` |   MB/s | GPU 通过 PCIe 发送数据的速度，通常是 GPU 到主机                                          |

### nvidia-smi pmon

安装 process 采样
```txt
nvidia-smi pmon --select=um --options=DT --count=10
#Date        Time         gpu         pid   type     sm    mem    enc    dec    jpg    ofa     fb   ccpm    command
#YYYYMMDD    HH:MM:SS     Idx           #    C/G      %      %      %      %      %      %     MB     MB    name
 20260816    09:22:41       0          -     -      -      -      -      -      -      -      -      -    -
 20260816    09:22:42       0          -     -      -      -      -      -      -      -      -      -    -
 20260816    09:22:43       0          -     -      -      -      -      -      -      -      -      -    -
```


## nvidia GPU 架构
<!-- 51c8965a-8808-4fe4-82f9-e1105a26b380 -->

> [!NOTE]
> `sm_61`、`sm_70` 这类编号不是 CUDA 版本，而是 NVIDIA 的 Compute Capability / SM 编号。
> 下面额外补了 "常见 SM/CC" 和 "首个支持该架构的 CUDA Toolkit" 两列。

| 架构代号（Codename） | 微架构名称                             | 常见 SM / Compute Capability                    | 首个支持该架构的 CUDA Toolkit | 制程工艺                | 首发年份  | 代表产品系列                                                                      |
|----------------------|----------------------------------------|-------------------------------------------------|-------------------------------|-------------------------|-----------|-----------------------------------------------------------------------------------|
| Tesla                | G80 / G92 等                           | `sm_1.x`                                        | CUDA 1.x                      | 65-55 nm                | 2006-2008 | GeForce 8 / 9 / 200 系列（如 GTX 280）                                            |
| Fermi                | GF100 / GF110                          | `sm_20`                                         | CUDA 3.0                      | 40 nm                   | 2010      | GeForce 400 / 500 系列（如 GTX 580）                                              |
| Kepler               | GK104 / GK110                          | `sm_30` / `sm_32` / `sm_35` / `sm_37`           | CUDA 5.0                      | 28 nm                   | 2012      | GeForce 600 / 700 系列（如 GTX 780 / Titan）                                      |
| Maxwell              | GM107 / GM204                          | `sm_50` / `sm_52` / `sm_53`                     | CUDA 6.0                      | 28 nm                   | 2014      | GeForce 900 系列（如 GTX 980 / GTX 970）                                          |
| Pascal               | GP100 / GP104 / GP102                  | `sm_60` / `sm_61` / `sm_62`                     | CUDA 8.0                      | 16 nm                   | 2016      | GeForce 10 系列（如 GTX 1080 Ti / Titan Xp）                                      |
| Volta                | GV100                                  | `sm_70` / `sm_72`                               | CUDA 9.0                      | 12 nm                   | 2017      | 仅专业/计算卡（如 Tesla V100 / Titan V）                                          |
| Turing               | TU102 / TU104 / TU106                  | `sm_75`                                         | CUDA 10.0                     | 12 nm                   | 2018      | GeForce RTX 20 系列（如 RTX 2080 / RTX 2060）                                     |
| Ampere               | GA100 / GA102 / GA104 / GA106          | `sm_80` / `sm_86` / `sm_87`                     | CUDA 11.0                     | 8 nm（三星）            | 2020      | GeForce RTX 30 系列、A100 / A30 等                                                |
| Ada Lovelace         | AD102 / AD103 / AD104                  | `sm_89`                                         | CUDA 11.8                     | 4N（定制 5 nm，台积电） | 2022      | GeForce RTX 40 系列（如 RTX 4090 / RTX 4060）                                     |
| Hopper               | GH100 / GH200                          | `sm_90`                                         | CUDA 11.8                     | 4N（台积电）            | 2022-2023 | H100 / H200 / GH200                                                               |
| Blackwell            | GB200 / B200，GB202 / GB203 / GB205 等 | 数据中心：`sm_100` / `sm_103`；消费级：`sm_120` | CUDA 12.8                     | 4NP（台积电）           | 2024-2025 | 数据中心/HPC：B200 / GB200；消费级：GeForce RTX 50 系列（如 RTX 5090 / RTX 5080） |

补充说明：

- `CUDA Toolkit 版本` 和 `SM/CC` 是两套编号体系。比如 Pascal 常见的是 `sm_61`，但它最早对应的工具链是 CUDA 8.0。
- 同一代架构里可能有多个 CC/SM 变体，例如 Pascal 有 `sm_60`、`sm_61`、`sm_62`，Ampere 有 `sm_80`、`sm_86`、`sm_87`。
- “首个支持该架构的 CUDA Toolkit” 这一列按该架构进入原生编译支持的最早工具链代际来写；具体到某个细分 `sm_xx`，可能是在后续小版本里补充支持。
- Blackwell 这一代分化更明显：B200 / GB200 属于数据中心线，GeForce RTX 50 系列属于消费级线，当前官方表里已经分成不同的 Compute Capability。

参考：

- <https://developer.nvidia.com/cuda/gpus>
- <https://developer.nvidia.com/cuda-legacy-gpus>
- <https://docs.nvidia.com/datacenter/tesla/drivers/cuda-toolkit-driver-and-architecture-matrix.html>


RTX 5090 和 B200 虽然同属 Blackwell 架构，但本质上是面向完全不同场景的产品，核心区别总结
 维度         RTX 5090              B200
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 产品定位     消费级游戏/创作显卡   数据中心 AI 计算卡
 核心代号     GB202（单芯片）       GB200（双芯片封装）
 晶体管数量   920 亿                2080 亿（2×1040亿）
 显存         32GB GDDR7            192GB HBM3e
 显存带宽     ~1 TB/s               ~8 TB/s
 TDP/TBP      575W                  1000W+
 价格         $1,999                $3-4 万美元（估计）
本质差异
1. 芯片设计
• 5090：单芯片设计，针对图形渲染、游戏帧率优化
• B200：双芯片（CoWoS 封装）设计，两颗 GPU 通过高速互联集成，专为大规模并行计算
2. 显存选择
• 5090：GDDR7 - 成本较低，延迟相对高，适合图形纹理访问模式
• B200：HBM3e - 极高带宽、低功耗，适合 AI 大模型参数读取
3. 互联能力
• 5090：支持 NVLink 桥接（双卡互联），主要用于 SLI/多卡渲染
• B200：第五代 NVLink，支持 72 GPU 全互联，构成超大规模计算集群
4. 软件生态
• 5090：CUDA、DirectX、Vulkan、DLSS 4 - 面向游戏引擎和创作软件
• B200：TensorRT、Triton 推理服务器、vGPU、机密计算 - 面向企业 AI 训练和推理
5. 精度支持
• 5090：消费级 FP4/FP8 支持（主要加速 AI 图像生成如 FLUX）
• B200：完整的 AI 计算精度（FP8、FP4、INT8 等），支持 Transformer Engine
类比理解
可以将它们看作同一架构的两种形态：
• 5090 = 高性能跑车（追求单用户响应速度、图形效果）
• B200 = 货运列车（追求吞吐量和数据搬运能力，组成"火车队"进行大规模计算）

B200 实际上是两块 Blackwell GPU 芯片封装在一起，通过 NVLink-HBI（高带宽互联）连接，配合 HBM3e 显存，专为 ChatGPT 这类大模型训练和推理设计；而 5090 是单芯片，用 GDDR7 平衡成本和游戏性能。


这是一个重要的澄清：RTX 5090 和 B200 并非"相同芯片"。
它们属于同一 Blackwell 架构代际，但使用的是完全不同的芯片设计：
            RTX 5090     B200
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 核心代号   GB202        GB200 (双芯片封装)
 芯片定位   消费级旗舰   数据中心级
 芯片数量   单芯片       双芯片 (CoWoS 封装)
 晶体管     920 亿       2080 亿 (2×1040亿)
为什么会有精度支持差异
1. Tensor Core 代际不同
• GB202 (5090)：第 5 代 Tensor Core，支持 FP4/FP8 推理加速
• GB200 (B200)：第 5 代 Tensor Core + Transformer Engine 2.0
2. Transformer Engine 是独立硬件单元
Transformer Engine 不只是软件功能，它包含：
• 动态范围管理硬件：自动在 FP16、FP8、FP4 间切换
• 细粒度量化逻辑：针对注意力机制的专用优化
• 更高的张量计算吞吐：每时钟周期处理更多矩阵运算
GB202 为降低成本和功耗，裁剪了部分 Transformer Engine 硬件，仅保留 FP4/FP8 推理路径。
3. 内存带宽决定应用场景
        5090         B200
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 显存   32GB GDDR7   192GB HBM3e
 带宽   ~1 TB/s      ~8 TB/s
• 5090 的 32GB：只能跑 70B 模型 INT4 量化或 7B-13B 全精度 → 所以只需 FP4/FP8 推理
• B200 的 192GB：能跑完整 70B+ 模型训练 → 需要完整精度支持和 Transformer Engine 加速
4. 商业定位差异
• 5090：面向游戏玩家、创作者、AI 爱好者 → 优化单用户 AI 图像生成（FLUX、Stable Diffusion）
• B200：面向数据中心 LLM 训练 → 需要支持完整混合精度训练循环（FP32 主权重 + FP8 梯度 + FP4 推理）
类比理解
类似 Intel i9-14900K 和 Xeon Platinum 的区别：
• 同代架构（Raptor Lake / Blackwell）
• 但 Xeon 支持 AVX-512、更多 PCIe 通道、多路互联
• i9 针对单用户性能优化，裁剪了企业级特性

5090 和 B200 的关系也是如此：同源架构，但针对不同场景做了完全不同的芯片实现。

## 这个是做什么的?
https://developer.nvidia.com/nsight-compute

