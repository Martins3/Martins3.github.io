- https://github.com/Overv/VulkanTutorial (自己都说自己太老了)

https://github.com/KhronosGroup/Vulkan-Guide
https://github.com/KhronosGroup/Vulkan-Tutorial

  这两个项目怎么选

  - Vulkan-Tutorial
    更适合入门和动手。它是一步一步把程序搭起来的“课程式”教程。Khronos 版 README 明确说它已经按现代 Vulkan 更新了，基线是 Vulkan 1.4，并且使用了 dynamic
    rendering、timeline semaphores、Slang、C++20、Vulkan-Hpp RAII。这比老的 vulkan-tutorial.com 更现代。
    仓库：https://github.com/KhronosGroup/Vulkan-Tutorial
  - Vulkan-Guide
    更像“知识地图 + 最佳实践索引”，不是从 0 到 1 手把手带你写完整程序。它覆盖范围很广，比如 loader、layers、queues、WSI、synchronization、memory
    allocation、dynamic state、descriptor buffers 等，适合在你写代码时回头查“这一块现在推荐怎么做”。
    仓库：https://github.com/KhronosGroup/Vulkan-Guide

继续从 evn-setup.md 开始看吧

## 先看看 Vulkan-Tutorial 中的结果

en/02_Development_environment.adoc

## glfw 的功能
http://www.glfw.org/

```txt
• GLFW 和 Mesa 是两个独立的项目，但在 Linux 图形栈中密切配合：
   项目   角色
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   GLFW   窗口和输入管理库，负责创建窗口、读取键盘鼠标、处理事件
   Mesa   开源图形驱动项目，实现了 OpenGL、Vulkan、EGL、GLX 等渲染 API
  它们的关系：
  1. GLFW 调用 Mesa 实现的 API
     GLFW 本身不实现 OpenGL/Vulkan，它通过系统上的 EGL、GLX 或 Vulkan Loader 创建上下文。在大多数 Linux 发行版上，这些接口正是由 Mesa
     供的。
  2. Mesa 提供驱动支持
     无论是 Intel、AMD 的开源驱动，还是 NVIDIA 的 Nouveau，都基于 Mesa。GLFW 创建的窗口要正常渲染，底层通常依赖 Mesa 驱动。
  3. 特定功能协作
     从该项目的更新日志可以看到，GLFW 的 Null（无窗口）平台后端支持通过 Mesa 的扩展 EGL_MESA_platform_surfaceless 创建无表面的 EGL 上
     文，用于离屏渲染或自动化测试场景。

  简单来说：GLFW 负责"窗口和输入"，Mesa 负责"渲染和驱动"，GLFW 在 Linux 上需要依赖 Mesa（或其他厂商驱动）来实际执行图形渲染。
```


```txt
• GLFW 和 Wayland 是平台支持的关系：Wayland 是 GLFW 在 Linux 上支持的显示服务器后端之一（另一个是 X11）。
  具体关系如下：
  1. GLFW 实现了 Wayland 平台后端
  在 src/ 目录中，有专门的 Wayland 平台源码文件：
  • wl_init.c — Wayland 初始化
  • wl_window.c — Wayland 窗口管理
  • wl_monitor.c — Wayland 显示器/输出管理
  • wl_platform.h — Wayland 平台头文件
  这些代码让 GLFW 能够直接在 Wayland 合成器（如 GNOME Mutter、KWin、Sway 等）上创建窗口、处理输入，而不依赖 X11 或 XWayland。
  2. GLFW 自带 Wayland 协议定义
  deps/wayland/ 目录下包含多个 Wayland 协议 XML 文件：
  • xdg-shell.xml — 标准桌面窗口协议
  • fractional-scale-v1.xml — 分数缩放支持
  • pointer-constraints-unstable-v1.xml — 指针约束（如游戏鼠标锁定）
  • relative-pointer-unstable-v1.xml — 相对指针运动
  • 等等...
  GLFW 在构建时会从这些 XML 生成 C 代码，作为 Wayland 客户端协议绑定来与合成器通信。
  3. 实际使用时的关系
  • 如果系统运行的是 Wayland 会话，GLFW 可以原生使用 Wayland 后端
  • 如果运行的是 X11 会话，GLFW 会使用 X11 后端
  • 用户也可以在编译时或运行时通过环境变量（如 GLFW_BACKEND）选择使用哪个后端

  总结：GLFW 是一个跨平台的窗口库，Wayland 是它原生支持的 Linux 显示后端之一。GLFW 直接实现了 Wayland 客户端协议，可以在纯 Wayland 环境下独立运行。
```

```txt
你的应用程序
    │
    ▼
┌─────────────┐
│ glfwInit()  │ ──► _glfwSelectPlatform() ──► _glfwConnectWayland()
└─────────────┘         │                          │
                        │                          ▼
                        │              加载 libwayland-client.so.0
                        │              wl_display_connect()
                        ▼
               _glfw.platform 函数表
                        │
    ┌───────────────────┘
    ▼
┌──────────────────┐
│ glfwCreateWindow()│ ──► _glfwCreateWindowWayland()
└──────────────────┘         │
                             ├── createNativeSurface()
                             │       └── wl_compositor_create_surface()  [Wayland 协议]
                             │
                             ├── wl_egl_window_create()                  [Wayland EGL 桥梁]
                             │
                             ├── _glfwInitEGL()
                             │       └── 加载 libEGL.so.1 (Mesa 提供)
                             │       └── 获取 eglCreateContext 等符号
                             │
                             └── _glfwCreateContextEGL()
                                     └── eglCreateContext()      [Mesa 实现]
                                     └── eglCreateWindowSurface() [Mesa 实现]
```

## 那么 edge 是如何使用图形系统的
ldd  /opt/microsoft/msedge/msedge

```txt
🧀  ldd  /opt/microsoft/msedge/msedge
        linux-vdso.so.1 (0x00007f6c078d6000)
        libdl.so.2 => /lib64/libdl.so.2 (0x00007f6c078b0000)
        libpthread.so.0 => /lib64/libpthread.so.0 (0x00007f6c078ac000)
        libglib-2.0.so.0 => /lib64/libglib-2.0.so.0 (0x00007f6bf0cab000)
        libgobject-2.0.so.0 => /lib64/libgobject-2.0.so.0 (0x00007f6c0784e000)
        libnspr4.so => /lib64/libnspr4.so (0x00007f6c0780a000)
        libnss3.so => /lib64/libnss3.so (0x00007f6bf0b6d000)
        libnssutil3.so => /lib64/libnssutil3.so (0x00007f6c077dc000)
        libsmime3.so => /lib64/libsmime3.so (0x00007f6c077ae000)
        libgio-2.0.so.0 => /lib64/libgio-2.0.so.0 (0x00007f6bf099d000)
        libatk-1.0.so.0 => /lib64/libatk-1.0.so.0 (0x00007f6bf0977000)
        libatk-bridge-2.0.so.0 => /lib64/libatk-bridge-2.0.so.0 (0x00007f6bf093b000)
        libdbus-1.so.3 => /lib64/libdbus-1.so.3 (0x00007f6bf08e6000)
        libcups.so.2 => /lib64/libcups.so.2 (0x00007f6bf0852000)
        libexpat.so.1 => /lib64/libexpat.so.1 (0x00007f6bf0827000)
        libxcb.so.1 => /lib64/libxcb.so.1 (0x00007f6bf07fd000)
        libxkbcommon.so.0 => /lib64/libxkbcommon.so.0 (0x00007f6bf07ad000)
        libasound.so.2 => /lib64/libasound.so.2 (0x00007f6bf0692000)
        libgbm.so.1 => /lib64/libgbm.so.1 (0x00007f6c077a4000)
        libX11.so.6 => /lib64/libX11.so.6 (0x00007f6bf054d000)
        libXext.so.6 => /lib64/libXext.so.6 (0x00007f6c07790000)
        libcairo.so.2 => /lib64/libcairo.so.2 (0x00007f6bf040d000)
        libpango-1.0.so.0 => /lib64/libpango-1.0.so.0 (0x00007f6bf03a3000)
        libudev.so.1 => /lib64/libudev.so.1 (0x00007f6bf035a000)
        libm.so.6 => /lib64/libm.so.6 (0x00007f6bf026b000)
        libXcomposite.so.1 => /lib64/libXcomposite.so.1 (0x00007f6bf0266000)
        libXdamage.so.1 => /lib64/libXdamage.so.1 (0x00007f6bf0261000)
        libXfixes.so.3 => /lib64/libXfixes.so.3 (0x00007f6bf0259000)
        libXrandr.so.2 => /lib64/libXrandr.so.2 (0x00007f6bf024b000)
        libatspi.so.0 => /lib64/libatspi.so.0 (0x00007f6bf020e000)
        libgcc_s.so.1 => /lib64/libgcc_s.so.1 (0x00007f6bf01e2000)
        libc.so.6 => /lib64/libc.so.6 (0x00007f6befff1000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f6c078d8000)
        libpcre2-8.so.0 => /lib64/libpcre2-8.so.0 (0x00007f6beff46000)
        libffi.so.8 => /lib64/libffi.so.8 (0x00007f6beff36000)
        libplc4.so => /lib64/libplc4.so (0x00007f6beff2d000)
        libplds4.so => /lib64/libplds4.so (0x00007f6beff27000)
        libgmodule-2.0.so.0 => /lib64/libgmodule-2.0.so.0 (0x00007f6beff21000)
        libz.so.1 => /lib64/libz.so.1 (0x00007f6befefe000)
        libmount.so.1 => /lib64/libmount.so.1 (0x00007f6befead000)
        libselinux.so.1 => /lib64/libselinux.so.1 (0x00007f6befe7c000)
        libsystemd.so.0 => /lib64/libsystemd.so.0 (0x00007f6befd59000)
        libgssapi_krb5.so.2 => /lib64/libgssapi_krb5.so.2 (0x00007f6befd03000)
        libavahi-common.so.3 => /lib64/libavahi-common.so.3 (0x00007f6befcf3000)
        libavahi-client.so.3 => /lib64/libavahi-client.so.3 (0x00007f6befcde000)
        libgnutls.so.30 => /lib64/libgnutls.so.30 (0x00007f6befa00000)
        libXau.so.6 => /lib64/libXau.so.6 (0x00007f6befcd6000)
        libdrm.so.2 => /lib64/libdrm.so.2 (0x00007f6befcbf000)
        libpng16.so.16 => /lib64/libpng16.so.16 (0x00007f6bef9c5000)
        libfontconfig.so.1 => /lib64/libfontconfig.so.1 (0x00007f6bef975000)
        libfreetype.so.6 => /lib64/libfreetype.so.6 (0x00007f6bef8a9000)
        libXrender.so.1 => /lib64/libXrender.so.1 (0x00007f6befcb2000)
        libxcb-render.so.0 => /lib64/libxcb-render.so.0 (0x00007f6befca1000)
        libxcb-shm.so.0 => /lib64/libxcb-shm.so.0 (0x00007f6befc9d000)
        libpixman-1.so.0 => /lib64/libpixman-1.so.0 (0x00007f6bef7fa000)
        libfribidi.so.0 => /lib64/libfribidi.so.0 (0x00007f6bef7da000)
        libthai.so.0 => /lib64/libthai.so.0 (0x00007f6befc92000)
        libharfbuzz.so.0 => /lib64/libharfbuzz.so.0 (0x00007f6bef6ae000)
        libcap.so.2 => /lib64/libcap.so.2 (0x00007f6bef6a2000)
        libXi.so.6 => /lib64/libXi.so.6 (0x00007f6bef68e000)
        libblkid.so.1 => /lib64/libblkid.so.1 (0x00007f6bef654000)
        libkrb5.so.3 => /lib64/libkrb5.so.3 (0x00007f6bef58a000)
        libk5crypto.so.3 => /lib64/libk5crypto.so.3 (0x00007f6bef573000)
        libcom_err.so.2 => /lib64/libcom_err.so.2 (0x00007f6bef56c000)
        libkrb5support.so.0 => /lib64/libkrb5support.so.0 (0x00007f6bef55c000)
        libkeyutils.so.1 => /lib64/libkeyutils.so.1 (0x00007f6bef555000)
        libcrypto.so.3 => /lib64/libcrypto.so.3 (0x00007f6bef000000)
        libresolv.so.2 => /lib64/libresolv.so.2 (0x00007f6bef543000)
        libp11-kit.so.0 => /lib64/libp11-kit.so.0 (0x00007f6beee73000)
        libidn2.so.0 => /lib64/libidn2.so.0 (0x00007f6bef4f2000)
        libunistring.so.5 => /lib64/libunistring.so.5 (0x00007f6beecc6000)
        libtasn1.so.6 => /lib64/libtasn1.so.6 (0x00007f6bef4dc000)
        libhogweed.so.6 => /lib64/libhogweed.so.6 (0x00007f6beec83000)
        libnettle.so.8 => /lib64/libnettle.so.8 (0x00007f6beec2d000)
        libgmp.so.10 => /lib64/libgmp.so.10 (0x00007f6beeb86000)
        libxml2.so.2 => /lib64/libxml2.so.2 (0x00007f6beea2b000)
        libbz2.so.1 => /lib64/libbz2.so.1 (0x00007f6bef4c5000)
        libbrotlidec.so.1 => /lib64/libbrotlidec.so.1 (0x00007f6bef4b7000)
        libdatrie.so.1 => /lib64/libdatrie.so.1 (0x00007f6beea22000)
        libgraphite2.so.3 => /lib64/libgraphite2.so.3 (0x00007f6beea02000)
        liblzma.so.5 => /lib64/liblzma.so.5 (0x00007f6bee9cd000)
        libbrotlicommon.so.1 => /lib64/libbrotlicommon.so.1 (0x00007f6bee9aa000)
```

```txt
  🎮 GPU / 内核图形接口
   库            作用
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   libgbm.so.1   GBM (Generic Buffer Management) — Mesa 提供的 GPU 缓冲区管理，Chromium 通过它申请显存
   libdrm.so.2   DRM (Direct Rendering Manager) — 内核图形子系统的用户态接口，所有开源驱动都经过它
  ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  🖥️ X11 显示系统
  Edge 在这个系统上跑的是 X11 后端（没走 Wayland）：
   库                   作用
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   libX11.so.6          X11 核心客户端库，创建窗口、事件循环
   libXext.so.6         X11 扩展基础
   libXcomposite.so.1   合成扩展，让 Chromium 可以离屏渲染窗口
   libXdamage.so.1      损伤追踪，只重绘变化区域（对浏览器很关键）
   libXfixes.so.3       光标定位、区域锁定等辅助
   libXrandr.so.2       分辨率、多显示器管理
   libXrender.so.1      矢量图形/抗锯齿渲染扩展
   libXi.so.6           X Input 扩展，处理鼠标/滚轮等
   libxcb.so.1          XCB，X11 的现代异步 C 绑定
   libxcb-render.so.0   XCB 渲染扩展
   libxcb-shm.so.0      XCB 共享内存扩展，加速像素传输
  ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  🎨 2D 绘图 / 合成
   库                 作用
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   libcairo.so.2      2D 矢量图形库，Skia/Chromium 底层可能用到
   libpixman-1.so.0   像素级操作库（合成、alpha 混合），Xorg 和 Cairo 的底层引擎
  ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  🖼️ 图像 & 字体
   库                   作用
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   libpng16.so.16       PNG 图像解码
   libfreetype.so.6     字体栅格化渲染
   libharfbuzz.so.0     文字排版 / shaping（连字、阿拉伯文等）
   libfontconfig.so.1   字体配置与匹配
  ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  总结
  Edge（Chromium）在这个系统上的图形栈大致是：
  你的网页内容
      │
      ▼
  Chromium/Skia 2D 渲染
      │
      ├──► 通过 libgbm.so.1 + libdrm.so.2  →  直接操作 GPU (开源驱动)
      │
      └──► 通过 libX11/libxcb/libXrender    →  提交到 X11 服务器显示

  最核心的是 libgbm.so.1 和 libdrm.so.2，这是 Chromium 在 Linux 上走硬件加速的必经之路；其余 X11 相关的库负责窗口管理和画面呈现。
```
