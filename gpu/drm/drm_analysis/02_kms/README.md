# KMS (Kernel Mode Setting)

## 什么是 KMS？

KMS 是 Linux 内核中负责显示控制的子系统。它让内核（而不是用户空间 X server）来控制显示硬件。

### 历史背景

**传统方式 (UMS - User Mode Setting)**:
```
X Server (用户空间)
    ↓ 直接写显卡寄存器
设置显示模式 (分辨率、刷新率)
问题: 需要 root 权限，切换时容易崩溃
```

**KMS 方式**:
```
Kernel (KMS 子系统)
    ↓ 安全的硬件抽象
管理所有显示设置
好处: 更稳定，支持无缝切换
```

### KMS 核心组件

```
┌─────────────────────────────────────────────────────────────┐
│                     KMS 显示架构                             │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│   ┌──────────┐     ┌──────────┐     ┌──────────┐          │
│   │  CRTC    │────→│  Encoder │────→│ Connector│────→ 显示器│
│   │(显示控制)│     │ (编码器) │     │ (连接器) │          │
│   └────┬─────┘     └──────────┘     └──────────┘          │
│        │                                                    │
│        │ 扫描                                               │
│        ↓                                                    │
│   ┌──────────┐                                             │
│   │Framebuffer│ ← GPU 显存 (GEM 对象)                       │
│   │ (帧缓冲区)│                                             │
│   └──────────┘                                             │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 核心数据结构

```c
// CRTC - Cathode Ray Tube Controller (显示控制器)
struct drm_crtc {
    struct drm_device *dev;

    // 当前显示模式
    struct drm_display_mode mode;

    // 当前使用的 framebuffer
    struct drm_framebuffer *fb;

    // 位置（多显示器时）
    int x, y;

    // 是否启用
    bool enabled;
};

// Connector - 物理连接器 (HDMI/DP/VGA)
struct drm_connector {
    struct drm_device *dev;

    // 连接器类型
    int connector_type;  // DRM_MODE_CONNECTOR_HDMIA, etc.

    // 连接状态
    enum drm_connector_status status;  // 已连接/断开

    // 支持的显示模式
    struct list_head modes;

    // 当前编码器
    struct drm_encoder *encoder;
};

// Encoder - 信号编码器
struct drm_encoder {
    struct drm_device *dev;

    // 编码器类型
    int encoder_type;  // TMDS/LVDS/TVDAC

    // 可能的 CRTC
    uint32_t possible_crtcs;
};

// Framebuffer - 显示缓冲区
struct drm_framebuffer {
    struct drm_device *dev;

    // 尺寸
    uint32_t width, height;

    // 像素格式
    uint32_t pixel_format;  // DRM_FORMAT_XRGB8888, etc.

    // 关联的 GEM 对象
    struct drm_gem_object *obj[4];
};
```

### 工作流程

```
1. 初始化 KMS
   - 枚举所有 connectors
   - 检测哪些连接了显示器
   - 读取显示器支持的显示模式

2. 设置显示模式
   - 创建 framebuffer (GEM 对象)
   - 配置 CRTC (分辨率、刷新率)
   - 配置 Encoder (信号类型)
   - 启用 Connector (输出信号)

3. 显示内容
   - 应用程序绘制到 framebuffer
   - CRTC 自动扫描 framebuffer
   - 信号经过 Encoder 输出到 Connector
   - 显示器显示画面
```

## 实验 1: 查询显示信息

```bash
make kms_info.out && ./kms_info.out
```

显示当前系统的所有 KMS 资源。

## 实验 2: 设置显示模式

```bash
make kms_setmode.out && sudo ./kms_setmode.out
```

尝试设置显示模式（需要 root 权限）。

## 源码参考

- `drivers/gpu/drm/drm_crtc.c` - KMS 核心实现
- `drivers/gpu/drm/drm_framebuffer.c` - Framebuffer 管理
