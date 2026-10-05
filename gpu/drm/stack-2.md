## 关于 graphics stack 的再次回答

2026-10-02 通过这个问题，我清晰了，
也就是 EGL 窗口系统。

可以把它们按“谁负责什么”拆开来看。最核心的一张关系图是：

```text
Application
   │
   ├── OpenGL / OpenGL ES
   │      负责：渲染命令
   │      例如 glDrawArrays(), glBindTexture()
   │
   ├── EGL
   │      负责：创建 GL context、surface，
   │            把 OpenGL 和窗口/显示系统接起来
   │
   ├── Wayland client
   │      负责：窗口、surface、输入、窗口协议
   │
   ▼
Wayland compositor
   │
   ├── GBM
   │      负责：分配 GPU 可用的 buffer
   │
   ├── EGL / OpenGL
   │      负责：compositor 自己合成窗口
   │
   ▼
DRM / KMS
   │
   ├── DRM：GPU/显存/buffer/同步等内核接口
   └── KMS：显示控制，CRTC / plane / connector / mode
   │
   ▼
GPU + Display Controller
   │
   ▼
Monitor
```

但这张图还是稍微简化了。你如果从 Linux kernel/driver 的角度理解，可以分成五层。

### 1. OpenGL：负责“画”

OpenGL 是 rendering API。

比如：

```c
glClear(GL_COLOR_BUFFER_BIT);
glBindTexture(...);
glDrawElements(...);
```

它描述：

- shader
- texture
- vertex buffer
- framebuffer
- blending
- depth test
- draw call

但 OpenGL **不负责窗口**。

它甚至不关心：

> 你最终是在 Wayland 窗口里画，还是离屏画，还是直接显示到屏幕。

因此 OpenGL 本身没有“创建 Linux 窗口”的 API。

---

### 2. EGL：负责“给 OpenGL 准备运行环境”

EGL 处于 OpenGL 和 native platform 之间。

典型流程：

```c
wl_surface *surface = ...;    // Wayland 创建

EGLDisplay dpy = eglGetDisplay(...);

eglInitialize(dpy, ...);

EGLContext ctx =
    eglCreateContext(...);

EGLSurface egl_surface =
    eglCreateWindowSurface(...);

eglMakeCurrent(...);

glDrawArrays(...);

eglSwapBuffers(...);
```

所以 EGL 干的事情主要是：

```text
native display
native window
     │
     ▼
   EGL
     │
     ├── EGLDisplay
     ├── EGLContext
     └── EGLSurface
     │
     ▼
 OpenGL/OpenGL ES
```

可以粗略类比：

```text
Windows:
WGL + OpenGL

X11:
GLX + OpenGL

Wayland:
EGL + OpenGL
```

不过现代 X11 也可以用 EGL。

---

## 3. Wayland：负责“窗口协议”，不是负责渲染

这是最容易误解的地方。

Wayland 本质上是：

> client 和 compositor 之间的 IPC protocol。

比如 application 创建：

```text
wl_surface
```

然后告诉 compositor：

> “我有一个新的 buffer，请把它作为这个 surface 的内容。”

核心操作类似：

```c
wl_surface_attach(surface, buffer, ...);
wl_surface_commit(surface);
```

Wayland 本身基本不规定：

> 这个 buffer 里面的像素是怎么画出来的。

buffer 可以来自：

- CPU software rendering
- OpenGL
- Vulkan
- CUDA
- video decoder
- camera
- anything capable of producing a DMA-BUF

所以：

```text
Wayland != graphics renderer
```

Wayland 更接近：

```text
window/surface protocol
+
buffer passing protocol
```

---

## 4. GBM：负责 GPU buffer allocation

GBM = **Generic Buffer Management**。

它是 Mesa 提供的一套 API，用于直接基于 DRM device 分配 GPU buffer。

典型：

```c
int fd = open("/dev/dri/card0", ...);

struct gbm_device *dev =
    gbm_create_device(fd);

struct gbm_bo *bo =
    gbm_bo_create(dev,
                  width,
                  height,
                  format,
                  flags);
```

其中：

```text
gbm_bo
```

就是一个 graphics buffer object。

它最终通常对应：

```text
DRM GEM object
```

或者某种 driver-specific GPU memory object。

所以关系大致：

```text
GBM
 ↓
libdrm
 ↓
DRM ioctl
 ↓
kernel drm driver
 ↓
GPU memory
```

GBM 本质上解决：

> 我没有 X Server，怎么直接通过 DRM 创建一个 GPU 可以用的 scanout/render buffer？

这在：

- Wayland compositor
- embedded Linux
- kiosk
- headless rendering

特别重要。

---

# 5. DRM：kernel graphics subsystem

DRM = **Direct Rendering Manager**。

这是 Linux kernel graphics subsystem。

用户态最终通过：

```text
/dev/dri/card0
/dev/dri/renderD128
```

和 kernel GPU driver 通信。

例如：

```text
Mesa
 ↓
libdrm
 ↓
ioctl(fd, DRM_IOCTL_...)
 ↓
kernel
 ↓
amdgpu / i915 / nouveau / ...
```

DRM 的职责比名字大得多，现在包括：

- GPU buffer management
- GEM
- PRIME
- dma-buf
- synchronization
- GPU command submission
- display management
- mode setting

所以：

```text
DRM = 整个 Linux kernel graphics framework
```

---

# 6. KMS：DRM 中负责显示扫描输出的部分

KMS = **Kernel Mode Setting**。

KMS 属于 DRM subsystem。

你可以认为：

```text
DRM
├── GPU execution / memory
├── GEM
├── PRIME / dma-buf
└── KMS
```

KMS 管：

```text
connector
encoder
CRTC
plane
framebuffer
mode
```

典型链路：

```text
Framebuffer
   ↓
Plane
   ↓
CRTC
   ↓
Encoder
   ↓
Connector
   ↓
HDMI / DP
   ↓
Monitor
```

比如 compositor 最终会干类似：

```c
drmModeAtomicCommit(...)
```

告诉 kernel：

> 把这个 framebuffer 放到这个 plane 上，
> 让这个 CRTC 输出 2560x1440@144Hz，
> 经这个 DP connector 输出。

所以 KMS 是：

> 最接近物理显示器的那一层。

---

# 把完整路径串起来

假设你运行：

```text
weston
```

然后启动一个 OpenGL Wayland app。

应用渲染流程可能是：

```text
Application
     │
     │ Wayland protocol
     ▼
wl_surface
     │
     │
     ├─────────────── EGL
     │                 │
     │                 ▼
     │              OpenGL
     │                 │
     │                 ▼
     │            Mesa driver
     │                 │
     │                 ▼
     │              GPU render
     │
     ▼
wl_buffer
```

这里产生的 buffer 最后可能是：

```text
dma-buf
```

application 把 buffer 提交给 compositor：

```text
client
   │
   │ wl_surface.attach(buffer)
   ▼
Wayland compositor
```

然后 compositor 收到很多窗口：

```text
terminal buffer
browser buffer
video buffer
editor buffer
```

把它们合成：

```text
        OpenGL/Vulkan
             │
             ▼
      compositor output
             │
             ▼
         GBM buffer
             │
             ▼
        DRM framebuffer
             │
             ▼
            KMS
             │
             ▼
           monitor
```

因此完整路径可以写成：

```text
                  User Space
┌─────────────────────────────────────────┐
│                                         │
│ Application                             │
│                                         │
│ OpenGL                                  │
│   │                                     │
│ EGL                                     │
│   │                                     │
│ Wayland client                          │
│   │                                     │
│   ▼                                     │
│ Wayland compositor                      │
│   │                                     │
│   ├── OpenGL/Vulkan    ← composition    │
│   │                                     │
│   ├── EGL                              │
│   │                                     │
│   └── GBM         ← buffer allocation   │
│        │                                │
│        ▼                                │
│      libdrm                             │
└────────┼────────────────────────────────┘
         │ ioctl
─────────┼──────────────────────── syscall ──────
         ▼
                 Kernel
┌─────────────────────────────────────────┐
│ DRM                                     │
│                                         │
│ ├── GEM / buffer management             │
│ ├── dma-buf / PRIME                     │
│ ├── GPU driver                          │
│ │     └── GPU command execution         │
│ │                                       │
│ └── KMS                                 │
│       ├── plane                         │
│       ├── CRTC                          │
│       ├── encoder                       │
│       └── connector                     │
└─────────────────────────────────────────┘
              │
              ▼
          GPU / Display
              │
              ▼
           Monitor
```

---

# 有一个特别重要的细节：GBM 和 Wayland 没有直接从属关系

很多人容易画成：

```text
Wayland
  ↓
GBM
```

严格来说不准确。

Wayland client **不一定知道 GBM 存在**。

一个正常 Wayland app 一般只看到：

```text
Wayland
EGL
OpenGL
```

例如：

```text
Firefox
Chrome
GTK
Qt
```

通常不会自己直接操作：

```text
gbm_device
```

GBM 更多是：

> compositor / display server / EGL platform implementation 和 DRM 之间的一层。

例如：

```text
Weston
Sway
Mutter
KWin
```

内部很可能涉及：

```text
GBM
DRM
KMS
```

所以一个更加准确的分层是：

```text
       application
       /         \
 OpenGL/EGL     Wayland
      \           /
       \         /
       compositor
       /        \
 OpenGL/Vulkan   GBM
                  │
                  DRM
                  │
                  KMS
```

---

# dma-buf 是把这些东西串起来的关键

如果你开始真正看 Linux graphics stack，会发现：

**dma-buf 比 GBM 还关键。**

比如 app GPU render 出来的 buffer：

```text
GPU BO
 ↓
dma-buf fd
 ↓
Wayland protocol
 ↓
compositor
```

compositor 不需要复制：

```text
client VRAM
       ↓
     memcpy
       ↓
compositor VRAM
```

而是可以：

```text
same dma-buf
     │
     ├── client GPU render
     │
     ├── compositor sample
     │
     └── KMS scanout
```

因此理想路径是 **zero-copy**：

```text
OpenGL render
     ↓
DMA-BUF
     ↓
Wayland compositor
     ↓
KMS plane
     ↓
display
```

甚至 compositor 可以发现：

> 这个窗口全屏，而且格式/尺寸/modifier 都满足 scanout 条件。

于是直接：

```text
client dma-buf
      │
      ▼
   KMS plane
```

跳过 composition：

```text
OpenGL compositor
```

这就是常说的：

**direct scanout**。

---

## 用一句话给五个组件定位

| 组件 | 核心职责 |
|---|---|
| **OpenGL** | 怎么画 |
| **EGL** | 在哪里画、创建 GL context/surface |
| **Wayland** | 谁的窗口在哪里、窗口提交哪个 buffer |
| **GBM** | 创建 GPU/display 能用的 buffer |
| **DRM** | Linux kernel GPU graphics framework |
| **KMS** | 把 framebuffer 真正送到显示器 |

如果用 kernel 工程师的视角，可以记成：

```text
OpenGL = rendering API
EGL    = userspace context/surface glue
Wayland = window/buffer IPC protocol
GBM    = userspace BO allocator
DRM    = kernel GPU interface
KMS    = kernel display pipeline
```

而真正把现代 Linux 图形栈串起来的“货币”基本就是：

```text
DMA-BUF FD
+
fence / sync
+
DRM format
+
modifier
```

理解了这几个东西之后，再看 `wl_buffer`、`linux-dmabuf`、GBM BO、DRM framebuffer，整个 Wayland/Mesa/DRM 栈会突然变得非常清楚。
