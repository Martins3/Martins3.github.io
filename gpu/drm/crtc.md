## DRM 中的 CRTC 做什么的
<!-- a48989ae-8714-4bb9-97f5-de7be4038743 -->

在 Linux 内核的 DRM（Direct Rendering Manager）框架中。CRTC 全称为 **Cathode Ray Tube Controller**（阴极射线管控制器），是显示流水线的核心抽象，虽名称源于历史，但在现代显示系统中依然是关键组件。

### 核心定位与功能
CRTC 是显示控制器的软件抽象，负责**时序生成**与**图层合成**，如同流水线的“指挥家”。它不直接存储像素，而是统筹以下工作：
- **时序生成**：根据显示模式（分辨率、刷新率等）生成 HSync/VSync 等精确电气时序，驱动显示器刷新。
- **图层合成**：将主平面（Primary Plane）、叠加层（Overlay Plane）、光标平面（Cursor Plane）按 Z 序混合为完整画面。
- **模式设置**：配置分辨率、刷新率、Gamma 校正等显示参数，是 KMS（Kernel Mode Setting）的核心操作对象。

### 关键关联对象
CRTC 在显示管线中承上启下，与以下对象协作：
| 对象 | 作用 | 与 CRTC 的关系 |
| :--- | :--- | :--- |
| **Framebuffer** | 内存中的像素数据缓冲区 | 为 CRTC 提供原始画面源 |
| **Plane** | 图层抽象，负责像素混合 | CRTC 接收各 Plane 的输入并合成 |
| **Encoder** | 信号编码器（如 HDMI/DP） | 接收 CRTC 合成后的像素并转换为信号格式 |
| **Connector** | 物理接口（如 DP/Type-C） | 连接 Encoder 与显示器，感知链路状态 |

### 内核抽象与操作
内核中以 `struct drm_crtc` 描述 CRTC 实例，包含核心属性与操作接口：
```c
struct drm_crtc {
    struct drm_device *dev;          // 所属 DRM 设备
    struct drm_plane *primary;      // 主平面
    struct drm_plane *cursor;       // 光标平面
    struct drm_display_mode mode;   // 当前显示模式
    const struct drm_crtc_funcs *funcs; // 核心操作函数集
};
```
常用操作包括模式设置（`drmModeSetCrtc`）、页面翻转（Page Flip）、垂直同步（VBlank）事件等。

### 现代意义与数量
- **名称历史**：CRTC 命名源于早期 CRT 显示器的控制器，现代 LCD/LED 显示器仍沿用此抽象以统一接口。
- **多屏支持**：系统中 CRTC 的数量决定了可独立驱动的显示器数量，每个 CRTC 可对应一个独立显示输出。

### 总结
CRTC 是 DRM 显示系统的核心调度者，统筹时序生成与图层合成，连接 Framebuffer、Plane、Encoder 与 Connector，共同完成从像素到屏幕的显示流程。
