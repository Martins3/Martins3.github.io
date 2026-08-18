# Alacritty GPU 加速与 Wayland 适配调研
<!-- 962c53e7-5b5d-4237-b43c-02fac80029ff -->

原来只要是写图形界面，就是需要考虑 wayland 的问题。

## 一、GPU 加速实现

Alacritty 的 "GPU 加速" 本质上是**将整个终端界面的绘制全部交给 OpenGL 完成**，避免 CPU 软件渲染。

### 1. 核心渲染 API：OpenGL

- **最低要求**：OpenGL ES 2.0（兼容老旧/嵌入式 GPU）
- **首选路径**：OpenGL 3.3+（GLSL 3.3 渲染器）
- **上下文管理**：使用 [`glutin`](https://github.com/rust-windowing/glutin) 创建和管理 OpenGL 上下文

### 2. 两种渲染后端

代码中根据 GPU 能力自动选择：

| 后端 | 条件 | 特点 |
|------|------|------|
| **GLSL3** | Shader ≥ 3.3 且非 GLES 上下文 | 功能完整，支持 dual source blending |
| **GLES2** | 老旧 GPU 或强制配置 | 兼容性更强，部分功能受限 |

### 3. 文本渲染：纹理图集 + 实例化绘制

**a) 字形缓存（Texture Atlas）**

- 使用 `crossfont` 库在 **CPU 端将字体光栅化**成位图
- 将位图上传到 GPU 的 **OpenGL 2D 纹理图集**（每个 atlas 尺寸为 `1024×1024`）
- 后续相同字符直接从 GPU 纹理采样，无需重复光栅化

**b) 实例化渲染（Instanced Rendering）**

- 每个终端单元格（字符）作为一条**实例数据**（`InstanceData`），包含：
  - 网格坐标（列/行）
  - 字形在 atlas 中的 UV 坐标
  - 前景色/背景色
  - 单元格标志（宽字符、彩色 emoji 等）
- 每帧将大量单元格数据批量写入 VBO，调用一次 **`glDrawElementsInstanced`** 绘制整个屏幕
- 批次上限：`0x10000`（65536）个实例

**c) 两 Pass 着色器**

顶点着色器 (`text.v.glsl`) 和片段着色器 (`text.f.glsl`) 分两次绘制：
1. **Pass 0（背景）**：绘制所有单元格的背景色矩形
2. **Pass 1（文字）**：从 atlas 纹理采样字形，应用前景色

支持彩色字形（如 Emoji）和普通文本的抗锯齿渲染。

### 4. 矩形渲染：光标/下划线/装饰线

- 下划线、删除线、undercurl、光标块、搜索栏、消息栏等**非文本元素**使用独立的 `RectRenderer`
- 同样使用 VAO/VBO + 自定义 shader (`rect.v.glsl` / `rect.f.glsl`)
- 支持多种样式：普通实线、undercurl（波浪线）、点划线、虚线

### 5. 关键性能优化

| 优化点 | 说明 |
|--------|------|
| **批量提交** | 大量单元格合并为一次 draw call，减少 CPU-GPU 通信开销 |
| **纹理图集** | 字形位图常驻 GPU 显存，避免每帧重复上传 |
| **Damage Tracking** | Wayland 下使用 `EGL_KHR_swap_buffers_with_damage` 只提交变化区域 |
| **VBO 流式更新** | 实例数据使用 `GL_STREAM_DRAW` 模式每帧更新 |

### 总结

Alacritty 的 GPU 加速**并不是用 GPU 来计算终端逻辑或字体光栅化**，而是：

1. **CPU 负责**：解析终端转义序列、布局文本、字体光栅化（生成位图）
2. **GPU 负责**：将所有可见元素（文字背景、文字字形、光标、下划线等）通过 OpenGL 纹理和实例化渲染批量绘制到屏幕

这种架构的核心优势是：避免了传统终端模拟器在 CPU 上逐个像素绘制的方式，利用 GPU 的高吞吐量并行渲染能力，尤其在大窗口、快速滚动时帧率更高。

---

## 二、Wayland 适配

Alacritty 能在 Wayland 下运行，核心依赖是 **`winit` + `glutin`** 这两个 Rust 库对 Wayland 的抽象，但 Alacritty 自身也在多个关键点做了显式适配。

### 1. 编译时特性开关（Cargo 层）

`Cargo.toml` 定义了 `wayland` feature，默认开启：

```toml
wayland = [
    "copypasta/wayland",
    "glutin/wayland",
    "winit/wayland",
    "winit/wayland-dlopen",
    "winit/wayland-csd-adwaita-crossfont",
]
```

- `glutin/wayland`：EGL on Wayland 的 GL 上下文支持
- `winit/wayland`：窗口创建、事件循环、输入处理
- `copypasta/wayland`：Wayland 原生剪贴板协议
- `wayland-csd-adwaita-crossfont`：Wayland 客户端装饰（CSD）

> 也可 `--no-default-features --features=wayland` 仅编译 Wayland 版本。

### 2. 运行时平台检测

代码中大量通过 `raw_window_handle` 判断当前是 Wayland 还是 X11：

```rust
let is_wayland = matches!(raw_window_handle, RawWindowHandle::Wayland(_));
```

以及检测 display handle：

```rust
if matches!(display_handle.as_raw(), RawDisplayHandle::Wayland(_)) {
    info!("Running on Wayland");
}
```

### 3. OpenGL 上下文：EGL on Wayland

在 `renderer/platform.rs` 中，创建 GL display 时：

```rust
#[cfg(all(not(feature = "x11"), not(any(target_os = "macos", windows))))]
let preference = DisplayApiPreference::Egl;
```

非 X11 的 Linux/BSD 下**直接走 EGL**，这是 Wayland 上 OpenGL 的标准接口。`glutin` 会自动基于 Wayland 的 `wl_display` 创建 EGL display 和 surface。

### 4. Wayland 帧回调（Frame Callback）机制

这是 Wayland 与 X11 最本质的差异。Wayland 使用 `wl_surface.frame` 回调来同步渲染节奏，Alacritty 做了专门适配：

**a) `has_frame` 标志**

```rust
pub struct Window {
    pub has_frame: bool,  // 是否有可用的帧回调
    ...
}
```

**b) 请求帧的逻辑差异**

在 `Display::draw()` 末尾：

```rust
// X11 主动请求下一帧
if !matches!(self.raw_window_handle, RawWindowHandle::Wayland(_)) {
    self.request_frame(scheduler);
}
```

Wayland 下**不主动轮询**，而是等待 compositor 通过 winit 发送 `RedrawRequested`。

**c) 收到帧回调后的处理**

winit 收到 Wayland 的 frame callback 后，会触发窗口的 `RedrawRequested`。Alacritty 将其转换为内部 `EventType::Frame` 事件：

```rust
(EventType::Frame, Some(window_id)) => {
    window_context.display.window.has_frame = true;
    if window_context.dirty {
        window_context.display.window.request_redraw();
    }
}
```

只有在 `has_frame == true` 时，Alacritty 才会真正执行绘制。这避免了在 Wayland 下无意义地渲染，浪费 CPU/GPU。

### 5. Damage Tracking / 局部更新

Wayland 支持 `EGL_KHR_swap_buffers_with_damage`，Alacritty 充分利用了这一点：

```rust
fn swap_buffers(&self) {
    match (self.surface.deref(), &self.context.deref()) {
        (Surface::Egl(surface), PossiblyCurrentContext::Egl(context))
            if matches!(self.raw_window_handle, RawWindowHandle::Wayland(_))
                && !self.damage_tracker.debug =>
        {
            let damage = self.damage_tracker.shape_frame_damage(self.size_info.into());
            surface.swap_buffers_with_damage(context, &damage)
        },
        (surface, context) => surface.swap_buffers(context),
    };
}
```

`DamageTracker` 会精确追踪：
- 终端内容变化的行（`LineDamageBounds`）
- 光标、选择区域、搜索栏等矩形区域

最终只把**脏区域**提交给 compositor，大幅降低带宽和功耗。

### 6. 剪贴板：Wayland 原生协议

`clipboard.rs` 中针对 Wayland 做了独立分支：

```rust
match display {
    RawDisplayHandle::Wayland(display) => {
        let (selection, clipboard) = unsafe {
            wayland_clipboard::create_clipboards_from_external(display.display.as_ptr())
        };
        Self { clipboard: Box::new(clipboard), selection: Some(Box::new(selection)) }
    },
    _ => Self::default(),
}
```

Wayland 下同时支持：
- **Clipboard**（Ctrl+C/Ctrl+V）
- **Primary Selection**（鼠标中键粘贴）

### 7. 启动通知（Startup Notification）

支持 Wayland 的 `XDG_ACTIVATION_TOKEN`（以及兼容的 `DESKTOP_STARTUP_ID`）：

```rust
// main.rs
window_options.activation_token =
    env::var("XDG_ACTIVATION_TOKEN").or_else(|_| env::var("DESKTOP_STARTUP_ID")).ok();

// window.rs
window_attributes = window_attributes.with_activation_token(token);
startup_notify::reset_activation_token_env();
```

这让 Alacritty 在多窗口启动时能被 compositor 正确聚焦。

### 8. 窗口初始化的 Wayland 特殊处理

在 `Display::new()` 中：

```rust
// On Wayland we can safely ignore this call, since the window isn't visible
// until you actually draw something into it and commit those changes.
if !is_wayland {
    surface.swap_buffers(&context).expect("failed to swap buffers.");
    renderer.finish();
}
```

Wayland 的窗口**在首次提交前默认不可见**，所以不需要像 X11 那样初始化时 swap 一次 buffers。

另外，`StartupMode::Maximized` 在 Wayland 下被跳过（某些 compositor 的行为问题）：

```rust
StartupMode::Maximized if !is_wayland => window.set_maximized(true),
```

### 9. IME 输入法的差异处理

`update_ime_position()` 中区分了 X11 和 Wayland：

```rust
let offset = if self.is_x11 { 1 } else { 0 };
```

X11 不支持光标区域（cursor area），需要手动偏移避免遮挡文本；Wayland 则可以直接设置准确的 IME 弹出位置。

### 10. `pre_present_notify` 同步

在每次 `swap_buffers` 前调用：

```rust
self.window.pre_present_notify();
```

这是 winit 为 Wayland 等现代后端提供的显式同步 API，告诉窗口系统"即将呈现"，有助于 compositor 优化合成时序。

---

## 三、架构总结

Alacritty 本身**没有直接调用 libwayland 客户端 API**，而是通过分层抽象：

| 层级 | 库/模块 | Wayland 职责 |
|------|---------|-------------|
| 窗口+事件 | `winit` | `wl_surface`, `wl_seat`, 帧回调, CSD |
| OpenGL 上下文 | `glutin` | EGL display/surface/context |
| 剪贴板 | `copypasta` | `wl_data_device`, `zwp_primary_selection` |
| 终端适配 | Alacritty 自身 | 帧节奏控制、damage tracking、平台差异处理 |

Alacritty 的核心 Wayland 适配工作在于：**尊重 Wayland 的按需渲染模型（frame callback）、利用 damage 减少传输、以及处理 Wayland 与 X11 在窗口生命周期和输入上的行为差异**。
