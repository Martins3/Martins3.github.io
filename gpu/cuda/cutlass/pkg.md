# 为什么 libcublas / libcublasLt / libnvblas 体积如此巨大

## 概述

这几个库的实际大小差异很大，真正巨大的是
**libcublasLt.so.12.8.3.14（745M）**，**libcublas.so.12.8.3.14（111M）**
次之，而 **libnvblas.so.12.8.3.14** 其实只有 **737K**，非常小。

它们体积巨大的核心原因是：**CUDA 数学库采用 "fatbinary" 机制，在一个 `.so`
文件里打包了面向近 10 代不同 GPU 架构（从 Maxwell 到 Blackwell）的数千个预编译
GPU Kernel 机器码，以及海量的调度配置元数据。**

## 具体空间占用分析

通过 `readelf` 拆解这几个 ELF 文件，各 section 的大小如下：

### libcublasLt.so.12.8.3.14（745M）的主要 section

| Section            | 大小       | 说明                                                                                                       |
| ------------------ | ---------- | ---------------------------------------------------------------------------------------------------------- |
| `.nv_fatbin`       | **273.8M** | CUDA Fat Binary，包含所有预编译的 GPU Kernel（SASS/PTX）                                                   |
| `.cask_resource`   | **164.3M** | cuBLASLt 的 CASK（Compute Automation and Scheduling Kernel）资源，存储海量的 GEMM 调优配置、启发式算法模板 |
| `.text`            | **128.9M** | CPU 端 host 代码（调度器、API 封装、算法选择逻辑）                                                         |
| `.ldata` / `.lbss` | **~147M**  | 大量全局静态数据结构                                                                                       |
| `.rodata`          | **42.8M**  | 只读数据                                                                                                   |

### libcublas.so.12.8.3.14（111M）的主要 section

| Section      | 大小       | 说明              |
| ------------ | ---------- | ----------------- |
| `.nv_fatbin` | **103.9M** | GPU Kernel 机器码 |
| `.text`      | **5.8M**   | CPU 端代码        |
| 其他         | **~1M**    | 数据段等          |

---

## 为什么 `.nv_fatbin` 会占这么多空间？

通过 `cuobjdump` 列出的 GPU 机器码（cubin）数量：

- **`libcublasLt.so`** 里打包了约 **5986** 个独立 cubin 文件
- **`libcublas.so`** 里打包了约 **1610** 个独立 cubin 文件

这些 cubin 覆盖了从旧架构到新架构的多种 SM 版本：

```
# libcublasLt.so 中的架构分布示例
1668  sm_100   (Blackwell)
1591  sm_90    (Hopper)
1380  sm_120   (下代架构)
 477  sm_80    (Ampere)
 247  sm_89    (Ada Lovelace)
 144  sm_70    (Turing)
 116  sm_75
 100  sm_86
  91  sm_60
  90  sm_50
  82  sm_61
```

**每一个 `sm_XX` 都代表一套完全不同的原生 GPU
机器指令集（SASS）**。同一份矩阵乘（GEMM）逻辑，为了在不同代 GPU
上达到峰值性能，需要编译成各自独立的机器码。此外，对于每一种架构，库里面还包含了针对不同：

- **数据类型**（FP64、FP32、FP16、BF16、TF32、INT8、FP8）
- **矩阵布局**（NN、NT、TN、TT）
- **Tile 大小与分块策略**
- **Split-K、Swizzle、Stream-K 等算法变体**

的 Kernel 实例。cuBLASLt 为了支持 "任意尺寸矩阵都能自动选出最优 Kernel"
的能力，预编译了数量极其庞大的 Kernel 变体。

## `.cask_resource` 是什么？（cuBLASLt 特有）

cuBLASLt 引入了 **CASK（Compute Automation and Scheduling Kernel）**
机制。与标准 cuBLAS 不同，cuBLASLt 提供了 `cublasLtMatmulAlgoGetHeuristic`
这样的 API，让用户在运行时查询当前矩阵尺寸下最快的算法。

为了这个查询能**瞬时完成**（而不是像某些方案那样在线编译或实际跑
benchmark），NVIDIA 把大量预计算的调度策略、性能模型、Kernel
配置模板直接打包进了 `.cask_resource` section。这个 section 一个就占了
**164M**。


## 与 Linux Kernel vmlinux 的对比

带 debuginfo 的 `vmlinux` 约 800M，但那主要是 **DWARF
调试符号**撑起来的。如果不算 debuginfo，压缩后的内核镜像（bzImage）通常只有
10~30M。

而 `libcublasLt.so` 的 745M 是**实打实的代码和数据**：

- 没有 debuginfo（`file` 显示 `stripped`）
- 没有冗余符号表
- 全是运行时必须加载的 GPU 机器码和调度元数据

Linux 内核虽然代码量大，但它是**单份 CPU 代码**（不同架构是分开编译的，x86
的内核不会包含 ARM 的代码）。而 cuBLASLt 是一个 **"单二进制支持所有 GPU"**
的库，它必须在一个 `.so` 里塞进面向近 10 代 GPU 的数千个
Kernel，以保证用户在任何支持的 GPU 上都能即开即用、达到峰值性能。

## 总结

| 库               | 大体积的根本原因                                                         |
| ---------------- | ------------------------------------------------------------------------ |
| `libcublas.so`   | 约 1610 个预编译 GPU Kernel，覆盖 sm_50 ~ sm_120                         |
| `libcublasLt.so` | 约 5986 个 GPU Kernel + 164M CASK 自动调优配置数据 + 复杂的 CPU 调度逻辑 |
| `libnvblas.so`   | 很小（737K），它只是 CPU 端的 BLAS 接口包装层                            |

如果追求更小的部署体积，NVIDIA 在数据中心场景下通常提供 **"slim" 版本**
或可以通过 `nvprune` 工具剔除不需要的 GPU 架构 cubin，只保留目标架构（如只留
`sm_90`）来大幅缩减体积。
