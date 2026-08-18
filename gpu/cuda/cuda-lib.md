## 安装的 cuda runtime 包有 20w 行，主要都是做什么的?
<!-- dd88ed83-d2ee-406b-9494-fe0c56eeb11b -->

/usr/local/cuda-12.8/targets/x86_64-linux/

让 kimi 分析了一下:

• 这里是 CUDA 12.8 SDK 的 targets/x86_64-linux/ 目录，主要分为 include/（头文件）和 lib/（库文件）两大块。
  ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  📊 代码统计（tokei 汇总）
   语言         文件数   总行数    代码行    注释行    空行
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   C Header     1,382    535,088   221,770   265,916   47,402
   C++ Header   179      62,725    44,613    10,890    7,222
   CMake        12       1,655     1,051     410       194
  ▌ 注释行比代码行还多，说明这是典型的 SDK 头文件风格（大量 API 文档注释）。
  ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  📁 include/ 目录结构
  按子目录统计：
   目录                  文件数   行数      说明
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   cuda/                 510      73,439    CUDA 核心头文件（运行时 API、标准库、PTX 内联等）
   thrust/               797      121,738   Thrust 并行算法库（C++ 模板，类似 STL 的 GPU 版本）
   CL/                   9        20,080    OpenCL 头文件（cl.h、cl_ext.h 等）
   crt/                  24       21,360    CUDA Runtime 头文件（数学函数、设备函数、MMA 等）
   cooperative_groups/   16       3,768     协作组 API（线程块内协作原语）
   nvtx3/                16       5,154     NVTX 性能标注工具
   cub/                  1        2,953     CUB 并行原语库（device/block/warp 级别算法）
   Openmp/               2        1,183     OpenMP 工具接口
   Openacc/              1        98        OpenACC CUPTI 接口
   nv/                   0        0         空目录
  顶层关键头文件（直接放在 include/ 下）
  主要是各 CUDA 库的 API 入口：
  • cuda.h、cuda_runtime.h — CUDA 驱动/运行时
  • cublas.h、cublasLt.h — 线性代数（BLAS）
  • cusparse.h、cusolverDn.h — 稀疏矩阵、求解器
  • cufft.h — 快速傅里叶变换
  • curand.h — 随机数生成
  • npp.h — NVIDIA 性能基元（图像/信号处理）
  • nvrtc.h — 运行时编译
  • nvml.h — GPU 管理库
  • nvjpeg.h — JPEG 编解码
  • cupti.h — CUDA 性能工具接口
  • 各种 sm_*_intrinsics.h — 不同架构的指令内联
  ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  📦 lib/ 目录结构
  主要是 .so 动态库 和 .a 静态库，总大小约 6.2 GB。
  最大的动态库（按体积）
   库文件                       大小     功能
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   libcublasLt.so.12.8.5.5      717 MB   BLAS Lt（轻量线性代数，含大量 kernel）
   libcusparse.so.12.5.8.93     369 MB   稀疏矩阵运算
   libcufft.so.11.3.3.83        266 MB   FFT
   libcusolver.so.11.7.3.90     231 MB   稠密求解器
   libcusolverMg.so.11.7.3.90   154 MB   多 GPU 求解器
   libcurand.so.10.3.9.90       131 MB   随机数
   libnppif.so.12.3.3.100       123 MB   NPP 图像滤波
   libnvrtc.so.12.8.93          100 MB   运行时编译器
   libnvrtc.alt.so.12.8.93      101 MB   RTC 备用版本
   libnvJitLink.so.12.8.93      90 MB    JIT 链接器
  其他重要库
  • libcudart.so.12.8.90 (728 KB) — CUDA Runtime（核心，较小）
  • libcupti.so.2025.1.1 (7.5 MB) — CUPTI 性能分析
  • libnvToolsExt.so.1.0.0 (36 KB) — NVTX 扩展
  • libOpenCL.so.1.0.0 (31 KB) — OpenCL ICD
  • stubs/ — 包含一系列 .so stub 文件（用于链接时解析）
  ────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  🎯 总结
  这里不是普通项目源代码，而是 CUDA 12.8 的发布版 SDK：
  • include/ 提供 `~1,600` 个头文件，覆盖 GPU 编程全栈（CUDA Runtime、Thrust、CUB、数学库、图像处理、性能工具等）
  • lib/ 提供 ~50+ 个共享/静态库，总体积超过 6GB，其中 cublasLt、cusparse、cufft 最大它们预编译了大量 GPU kernel）
