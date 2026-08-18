# GPU Load Generator Demos

这组小 demo 用于在 Windows 任务管理器中演示 GPU 各项资源的占用。

## 监控位置

打开 **任务管理器** -> **性能** -> **GPU**，可以看到：
- **3D** - GPU 3D 引擎使用率
- **Copy** - 数据传输引擎使用率  
- **Video Decode** - 视频解码引擎使用率
- **Dedicated GPU memory** - 专用显存占用
- **Shared GPU memory** - 共享显存占用

## 程序列表

### 1. gpu_3d_load.exe - 3D 引擎负载

渲染大量三角形并执行复杂像素着色器计算。

```cmd
# 默认运行 30 秒，无限制帧率（最大负载）
gpu_3d_load.exe

# 运行 60 秒，限制 30 FPS
gpu_3d_load.exe 60 30
```

**预期效果**: 任务管理器中"3D"项使用率上升。

### 2. gpu_copy_load.exe - Copy 引擎负载

在 CPU 和 GPU 之间进行大量数据传输。

```cmd
# 默认运行 30 秒，上传模式 (CPU -> GPU)
gpu_copy_load.exe

# 运行 60 秒，下载模式 (GPU -> CPU)
gpu_copy_load.exe 60 1

# 运行 60 秒，双向传输
gpu_copy_load.exe 60 2
```

**预期效果**: 任务管理器中"Copy"项使用率上升。

### 3. gpu_decode_load.exe - 视频解码负载

使用视频处理器模拟解码负载。

```cmd
# 默认运行 30 秒，4 个流，1080p
gpu_decode_load.exe

# 运行 60 秒，8 个流
gpu_decode_load.exe 60 8
```

**预期效果**: 任务管理器中"Video Decode"或"Video Processing"项使用率上升。

**注意**: 真正的硬件解码需要视频文件和完整的 Media Foundation Pipeline。
此程序使用 Video Processor 生成类似负载。

### 4. gpu_dedicated_vram.exe - 专用显存占用

分配 GPU 独占的显存（物理显存，如 GDDR6）。

```cmd
# 默认分配 2GB，等待按键退出
gpu_dedicated_vram.exe

# 分配 4GB，持续 30 秒
gpu_dedicated_vram.exe 4096 30
```

**预期效果**: 任务管理器中"Dedicated GPU memory"使用量上升。

### 5. gpu_shared_vram.exe - 共享显存占用

分配 GPU 可访问的系统内存。

```cmd
# 默认分配 1GB，等待按键退出
gpu_shared_vram.exe

# 分配 2GB，持续 30 秒
gpu_shared_vram.exe 2048 30
```

**预期效果**: 任务管理器中"Shared GPU memory"使用量上升。

**原理**: 使用 D3D11_USAGE_STAGING 缓冲区，位于系统内存但可被 GPU 访问。
对于集成显卡，所有 GPU 内存都是"共享"的。

## 构建方法

### 方法 1: 使用提供的批处理脚本

```cmd
cd gpu_demo
build.bat
```

### 方法 2: 使用 Visual Studio 开发者命令提示符

```cmd
# 打开 "x64 Native Tools Command Prompt for VS 2022"
cd gpu_demo

cl /O2 gpu_3d_load.c /link d3d11.lib d3dcompiler.lib user32.lib
cl /O2 gpu_copy_load.c /link d3d11.lib
cl /O2 gpu_decode_load.c /link d3d11.lib mf.lib mfplat.lib evr.lib
cl /O2 gpu_dedicated_vram.c /link d3d11.lib dxgi.lib
cl /O2 gpu_shared_vram.c /link d3d11.lib dxgi.lib
```

## 系统要求

- Windows 10/11
- DirectX 11 兼容 GPU
- Windows SDK
- Visual C++ 编译器 (Visual Studio 2019/2022)

## 技术细节

| Demo | API | 原理 |
|------|-----|------|
| 3D Load | Direct3D 11 | 10000 三角形 + 复杂像素着色器 (1000 次 sin/cos 循环) |
| Copy Load | Direct3D 11 | UpdateSubresource 大缓冲区 (256MB x 4) |
| Decode Load | D3D11 Video | Video Processor Blit 多流 1080p/4K |
| Dedicated VRAM | Direct3D 11 | 创建 RENDER_TARGET 纹理 (独占 GPU 内存) |
| Shared VRAM | Direct3D 11 | 创建 STAGING 缓冲区 (系统内存，GPU 可访问) |

## 注意事项

1. **驱动控制面板**可能限制或重定向某些操作
2. **Optimus 笔记本**（双显卡）可能需要在 NVIDIA 控制面板中设置"高性能 NVIDIA 处理器"
3. **集成显卡**没有专用显存，所有内存都是共享的
4. **视频解码**需要 GPU 支持对应的解码格式（H264, HEVC, AV1 等）

## 截图示例

运行以下命令可以看到明显的资源占用：

```cmd
:: 同时开多个窗口测试不同资源
start gpu_3d_load.exe 60
start gpu_copy_load.exe 60 2
start gpu_dedicated_vram.exe 2048 60
```
