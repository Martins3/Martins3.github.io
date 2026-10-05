- 图形渲染管线：顶点处理→光栅化→片段处理的完整流程 (这个到底是什么意思?)
- 显存管理：VRAM/UMA 架构
- GBM 是什么?
- EGL 是什么东西

sudo yum install glx-utils vulkan-tools
```txt
# 查看OpenGL信息
glxinfo | grep renderer
# 查看Vulkan信息
vulkaninfo | grep deviceName
# 查看DRM设备信息
drm_info
```

drm_mm ：DRM 子系统中一种通用内存管理器，用于管理设备内存（如 GPU 显存）的动态分配和释放。
Documentation/gpu/drm-mm.rst
drm_buddy ：基于 伙伴内存分配算法(Buddy Allocator) 的内存管理器实现，用于 DRM 子系统中管理 GPU 内存。
drivers/gpu/drm/drm_buddy.c

# 渲染与显示完整流程
Linux 图形栈中一次完整的“绘制并显示”交互，可分为两大阶段：**渲染阶段** 和 **显示阶段**。二者通过同步对象（Fence）衔接，确保在画面渲染完成后才将其送去显示。

以 Wayland 场景为例，一个 3D 应用画面的完整显示流程如下：

## 阶段一：渲染阶段（生成画面）
目标：让 GPU 执行绘图指令，生成一帧新画面。

1. **初始化** 应用程序启动，通过 EGL API 连接到合成器（Compositor）。Compositor
   通过 GBM 为应用创建可渲染的共享缓冲区（gbm_bo）。Mesa 的 EGL 实现内部通过 DRM
   的 ioctl 调用在显存中分配该缓冲区，并将 dma-buf fd 句柄返回给 Compositor。

2. **命令构建**
    应用或合成器调用 OpenGL/Vulkan API（如 `glDrawArrays()`）进行场景绘制。Mesa 3D 接收调用，对应 GPU 驱动（如 Iris）将 API 调用翻译为 GPU 硬件指令，写入命令缓冲区（Command Buffer）。

3. **提交内核**
    Mesa 通过 libdrm 调用 ioctl 接口，将命令缓冲及纹理、顶点数据等图形资源引用提交给内核 DRM 驱动。

4. **硬件执行**
    内核驱动验证命令后，通过 GEM/TTM 管理器将命令缓冲和资源映射到 GPU 可访问地址空间，驱动 GPU 执行渲染任务。

5. **渲染完成通知**
    GPU 渲染完成并将结果写入共享缓冲区，产生硬件中断。内核驱动捕获中断并触发**渲染围栏（Render Fence）**，标记画面在内存中就绪。

---

## 阶段二：显示阶段（送上屏幕）
目标：将已渲染完成的画面缓冲区（Framebuffer）提交给显示控制器并输出到屏幕。

1. **提交与合成**
    应用渲染完成后通知合成器，合成器将所有可见窗口与自身 UI 元素（面板、鼠标指针等）合成为最终帧。
    有 DPU 时优先由 DPU 完成合成以提升能效，无专用 DPU 则由 GPU 完成，合成以 2D 图层混合为主。

2. **请求页面翻转**
    合成器确认渲染围栏触发后，向 DRM 驱动发起 Page Flip 的 ioctl 请求，告知 KMS 将 CRTC 的 Plane 指向新合成的帧缓冲区，并等待 V-Sync 信号执行。

3. **调度显示任务**
    DRM/KMS 驱动不立即更新画面，而是对页面翻转任务进行排程，等待显示器 V-Sync 信号。

4. **执行页面翻转**
    V-Sync 信号到来时，CRTC 硬件原子切换至新缓冲区地址，逐行扫描新画面，经 Encoder、Connector 发送至显示器，避免画面撕裂。

5. **屏幕显示完成**
    页面翻转完成后，内核触发**显示围栏（Display Fence / VBlank Event）**，通知用户态本次显示更新完成。

至此，新一帧画面完整显示在屏幕上，全过程在用户态与内核态间高效流转，充分利用硬件加速能力。

---

图形数据的流转路径 : 一个像素从代码定义到屏幕显示，完整数据流如下：

1. **应用 -> 合成器**
    应用将待显示的窗口缓冲区内容提交给 Wayland Compositor 等显示服务器。

2. **合成器 -> 图形库**
    合成器对多窗口及 UI 元素进行合成，该过程通过 OpenGL/Vulkan API 交由 Mesa 等图形库实现。

3. **图形库 -> 驱动**
    Mesa 将合成任务转为硬件指令，通过 ioctl 提交给 DRM 内核驱动。

4. **驱动 -> GPU**
    DRM 驱动控制 GPU 执行渲染，最终合成画面写入显存中的帧缓冲（Framebuffer）。

5. **显存 -> 显示控制器**
    显示控制器依据 KMS 配置的分辨率、刷新率，周期性读取帧缓冲数据，可进行硬件光标、Overlay 混合、色彩校正等处理。

6. **显示控制器 -> 编码器**
    显示控制器将像素数据流发送至 Encoder。

7. **编码器 -> 显示器**
    Encoder 将像素数据转为 HDMI/DisplayPort/MIPI-DSI 等协议电信号，驱动面板显示。


