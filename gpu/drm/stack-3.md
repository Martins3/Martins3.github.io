# EGL 的实现在哪里?
对 Linux 桌面这条栈来说，**EGL 最主要的开源实现就在 Mesa 里**。更准确地说，现代 Linux 上通常还会在它前面多一层 **libglvnd** 做 vendor dispatch。

你可以先记这张图：

```text
Application
    |
    | eglGetDisplay()
    | eglCreateContext()
    | eglCreateWindowSurface()
    | eglSwapBuffers()
    v
libEGL.so
    |
    +-- libglvnd              <- 很多发行版上的 libEGL.so 入口/dispatch
    |       |
    |       +---- Mesa EGL vendor
    |       |
    |       +---- NVIDIA EGL vendor
    |
    v
Mesa
src/egl/
    |
    +-- main/                 <- EGL 公共逻辑 / objects / dispatch
    |
    +-- drivers/dri2/         <- Linux 上最关键
            |
            +-- platform_wayland.c
            +-- platform_x11.c
            +-- platform_drm.c
            +-- platform_surfaceless.c
            +-- platform_device.c
            ...
```

Mesa 官方文档直接说明，EGL 的主要源码位于 **`src/egl/`**，其中 Mesa 的 `libEGL` 提供 EGL API 入口和公共对象模型，而实际 platform-specific 工作主要由 driver 完成。Linux 上关键的 driver 就是 `egl_dri2`，它支持 Wayland、X11、DRM、surfaceless、device 等 platform。[Mesa 3D Documentation](https://docs.mesa3d.org/egl.html?utm_source=chatgpt.com)

### 你如果想直接读代码，最值得看的就是这几个位置

在 Mesa 源码树：

```text
mesa/
└── src/
    └── egl/
        ├── main/
        │   ├── eglapi.c
        │   ├── eglcontext.c
        │   ├── eglsurface.c
        │   ├── egldisplay.c
        │   ├── eglconfig.c
        │   └── egldriver.h
        │
        └── drivers/
            └── dri2/
                ├── egl_dri2.c
                ├── platform_wayland.c
                ├── platform_x11.c
                ├── platform_drm.c
                ├── platform_surfaceless.c
                └── ...
```

如果你现在主要研究 **Wayland + EGL**，我建议你首先读：

```text
src/egl/drivers/dri2/platform_wayland.c
```

它基本就是：

> “Mesa 的 EGL 怎么把 Wayland native objects 接到 Mesa/DRI/GPU buffer 上。”

然后再沿着：

```text
eglGetPlatformDisplay()
        ↓
eglInitialize()
        ↓
dri2_initialize_wayland()
```

这样的路径追。

Mesa 文档也明确说 `egl_dri2` 本质上是一个 **DRI driver loader**，并且其 Wayland platform 实现就在这里。[Mesa 3D Documentation](https://docs.mesa3d.org/egl.html?utm_source=chatgpt.com)

---

### 一个重要区别：你看到的 `libEGL.so` 不一定就是 Mesa 本体

现代 Linux 系统，例如 Fedora、Ubuntu、Arch，往往采用：

```text
app
 ↓
libEGL.so
 ↓
libglvnd
 ↓
vendor EGL
```

`libglvnd` 的作用类似 Vulkan loader：

```text
libEGL.so
   |
   +-- Mesa
   |
   +-- NVIDIA
   |
   +-- 其他 vendor
```

它通过 vendor JSON 来发现 EGL 实现。libglvnd 官方文档说明，EGL 会枚举 vendor library，然后 `eglGetPlatformDisplay()` 会尝试不同 vendor，成功返回 display 的 vendor 就拥有这个 `EGLDisplay`。[GitHub](https://github.com/NVIDIA/libglvnd/blob/master/README.md?utm_source=chatgpt.com)

例如你机器上经常可以看到：

```bash
/usr/share/glvnd/egl_vendor.d/
```

类似：

```text
50_mesa.json
10_nvidia.json
```

然后：

```text
libEGL.so
    ↓
libEGL_mesa.so.0
```

或者：

```text
libEGL_nvidia.so.*
```

所以要区分两个东西：

```text
libglvnd libEGL
       =
EGL loader / dispatcher

Mesa libEGL_mesa
       =
真正的 Mesa EGL implementation
```

这点非常像 Vulkan：

```text
Vulkan
libvulkan.so
    ↓
ICD
    ↓
RADV / ANV / NVIDIA
```

对应 EGL：

```text
EGL
libEGL.so
    ↓
GLVND
    ↓
Mesa / NVIDIA EGL vendor
```

---

### 那 Wayland EGL 又在哪？

这里又有一个很容易混淆的名字：

```text
wayland-egl
```

你可能会看到：

```c
struct wl_egl_window *
wl_egl_window_create(struct wl_surface *, int width, int height);
```

这个东西 **不是 EGL 的主要实现**。

它只是 Wayland 和 EGL 之间的一个很薄的 native-window glue。

大致：

```text
Wayland
wl_surface
    |
    v
wl_egl_window
    |
    v
EGLNativeWindowType
    |
    v
eglCreateWindowSurface()
    |
    v
Mesa platform_wayland.c
```

所以可以理解为：

```text
wayland-egl
=
把 wl_surface 包装成 EGL 能认识的 native window
```

真正的：

```text
eglCreateContext
eglMakeCurrent
eglSwapBuffers
eglCreateImage
...
```

实现还是主要在 Mesa EGL / vendor EGL 中。

---

## 从你熟悉的 kernel 视角理解最好

EGL 本身几乎完全是 **userspace** 的。

例如：

```text
Application
        |
        | eglCreateWindowSurface()
        v
Mesa EGL
        |
        | platform_wayland.c
        v
Wayland / DRI
        |
        | allocate/import buffer
        v
GBM / DRI
        |
        | DRM ioctl
        v
Linux DRM
        |
        v
GPU driver
```

所以 EGL 并不存在类似：

```text
drivers/gpu/drm/egl/
```

这样的 kernel subsystem。

它是纯粹的 user-space API/interface layer。

真正进入 kernel 的通常已经变成：

```text
DRM ioctl
dma-buf
syncobj
GEM
execbuf / command submission
```

---

## `eglSwapBuffers()` 是一个特别好的入口

如果你想真正理解 EGL，我推荐直接追：

```c
eglSwapBuffers(display, surface)
```

比如 Wayland 下概念上会变成：

```text
eglSwapBuffers()
      |
      v
Mesa EGL
      |
      v
DRI2 EGL Wayland backend
      |
      v
获取 render buffer
      |
GPU rendering complete
      |
      v
生成/使用 wl_buffer
      |
      v
wl_surface_attach()
      |
wl_surface_damage()
      |
wl_surface_commit()
      |
      v
Wayland compositor
```

注意：

```text
eglSwapBuffers
```

在 Wayland 下并不意味着传统意义上的：

```text
front buffer <-> back buffer
```

简单交换指针。

更准确地说：

> 把下一帧 rendering buffer 提交给 native window system。

所以 Wayland 下：

```text
eglSwapBuffers()
```

最终最重要的动作之一就是把对应 buffer 提交给：

```text
wl_surface
```

让 compositor 接管。

---

## 如果把 Mesa EGL 和 Mesa OpenGL 放到源码层面

你会发现它们其实是两条不同的入口：

```text
                 Mesa
                   |
         +---------+---------+
         |                   |
       EGL                OpenGL
         |                   |
src/egl/                  Mesa GL API
         |                   |
platform_wayland              |
         |                    |
         +------ context -----+
                   |
                   v
               Gallium
                   |
                   v
             radeonsi / iris
                   |
                   v
                  DRM
```

所以：

```text
EGL
```

不是 OpenGL implementation。

Mesa 同时实现了：

```text
OpenGL
OpenGL ES
EGL
Vulkan
```

它们只是在较下面的 driver/backend 层产生交集。

---

### 如果你现在准备读源码，我建议阅读顺序

以 Wayland 为目标，不要一上来从整个 Mesa 开始。

```text
1. wayland-egl
   wl_egl_window_create()

           ↓

2. Mesa EGL public API
   src/egl/main/eglapi.c

           ↓

3. EGL display/context/surface
   egldisplay.c
   eglcontext.c
   eglsurface.c

           ↓

4. DRI2 EGL driver
   src/egl/drivers/dri2/egl_dri2.c

           ↓

5. Wayland platform
   src/egl/drivers/dri2/platform_wayland.c

           ↓

6. DRI / GBM / dma-buf

           ↓

7. Gallium driver / DRM
```

尤其值得自己 trace 一遍：

```text
eglGetPlatformDisplay(EGL_PLATFORM_WAYLAND_KHR, ...)
        ↓
eglInitialize()
        ↓
eglChooseConfig()
        ↓
eglCreateContext()
        ↓
eglCreateWindowSurface()
        ↓
eglMakeCurrent()
        ↓
glDraw*
        ↓
eglSwapBuffers()
```

你把这一条调用链读通以后，**EGL → Wayland → dma-buf → DRM 的边界基本就彻底清楚了**。Mesa 的 EGL 文档也把 `src/egl/`、`egl_dri2` 以及 Wayland/DRM 等平台支持列为实现核心。[Mesa 3D Documentation](https://docs.mesa3d.org/egl.html?utm_source=chatgpt.com)
