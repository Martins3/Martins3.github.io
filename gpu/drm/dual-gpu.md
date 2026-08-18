## linux 是如何管理两个 GPU 的
<!-- 90041f0e-4f88-440b-a53c-c764d6d85b6e -->

• 当前这棵内核是 v7.0-1-gafd597f0ba30。核心结论：Linux 内核不把多个 GPU 合成一个“全局显示设备”，而是每个 GPU 驱动注册一个独立的 DRM/KMS 设备，最终桌面如
  何横跨多个 GPU，主要由用户态 compositor / Xorg 决定。

  单个 GPU 在内核里对应一个 struct drm_device，它可以有多个显示头，也就是多个 CRTC / connector。KMS 暴露的显示链路大致是：

  framebuffer -> plane -> CRTC -> encoder/bridge -> connector -> monitor

  文档里对应的是 Documentation/gpu/drm-kms.rst:57。struct drm_device 也明确被描述为“a complete card that may contain multiple heads”，见 include/drm/
  drm_device.h:70。

  如果你有两个 GPU、两个显示器分别接在两个 GPU 上，通常会是：

  GPU0 -> /dev/dri/card0 -> connector HDMI-A-1 -> 显示器 A
  GPU1 -> /dev/dri/card1 -> connector DP-1     -> 显示器 B

  每个 cardN 有自己的 KMS 对象、自己的 CRTC、自己的 connector、自己的 atomic commit。内核负责每块卡自己的模式设置、page flip、vblank、hotplug、EDID、
  buffer 导入导出等；它不会在内核里统一管理“一个大桌面”。

  真正把两个屏幕拼成一个桌面的，是用户态显示服务器：

  Wayland compositor / Xorg
      打开 card0 和 card1
      在两个 DRM device 上分别取得 DRM master
      分别设置两个显示器的 mode
      分别提交 atomic commit / page flip
      维护全局桌面坐标和窗口布局

  DRM master 是按 drm_device 管的，一个设备同时只有一个 current master，见 drivers/gpu/drm/drm_auth.c:42。
  所以 compositor 可以同时成为 card0 和 card1 的 master，但这仍然是两个独立设备，不是一个跨 GPU 的原子事务。

  跨 GPU 的关键是 buffer 共享。内核通过 PRIME / dma-buf 让一个 GPU 产生的 buffer 可以被另一个 GPU 导入，
  文档见 Documentation/gpu/drm-mm.rst:234。代码入口
  是 `drm_gem_prime_handle_to_fd()` 导出和 `drm_gem_prime_fd_to_handle()` 导入，
  见 drivers/gpu/drm/drm_prime.c:279 和 drivers/gpu/drm/drm_prime.c:510。

  所以实际流程常见是：

  应用 / compositor 在某个 GPU 上渲染
      -> 产出 dma-buf
      -> 另一个 GPU 通过 PRIME 导入
      -> compositor 在对应显示 GPU 上合成或直接 scanout
      -> 每个显示器由自己连接的 GPU 扫描输出

  如果格式、modifier、tiling、显存位置或驱动能力不兼容，用户态就要做拷贝，可能是 GPU copy，也可能退化到 CPU/system memory copy。
  能不能零拷贝，取决于两个驱动和硬件的 dma-buf/PRIME 支持。

  另外，renderD* 节点只用于渲染，不能 modeset；真正控制显示输出的是 cardN primary node。
  文档在 Documentation/gpu/drm-uapi.rst:126 和 Documentation/gpu/drm-uapi.rst:141。

  简化一句话：两个显示器分别接两个 GPU 时，内核把它们作为两个独立 DRM/KMS 设备暴露；
  每块 GPU 只驱动自己物理连接的显示器；桌面跨屏、窗口拖动、哪个 GPU 渲染、
  是否用 PRIME/dma-buf 零拷贝，主要由 Wayland compositor 或 Xorg 决策。


## 如何操作?
我靠，天然就是支持的，这样太酷炫了。

```txt
• 你的环境信息：
   项目   内容
  ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
   系统   Fedora 42 (物理机)
   显示   Wayland + GNOME
   集显   Intel UHD Graphics 770 (当前在用)
   独显   NVIDIA GeForce GTX 1060 3GB (驱动已加载)
  ───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
  切换方案
  方案 1：仅特定程序使用 NVIDIA（推荐）
  如果你只是想让某个程序（如游戏、GPU 计算）跑在 NVIDIA 上，用环境变量即可，不需要全局切换：
  __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia <程序名>
  例如：
  __NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia glxinfo | grep "OpenGL renderer"

  方案 2：GNOME 应用右键切换
  对于支持的应用，在 GNOME 中右键点击应用图标，选择 "Launch using Discrete Graphics Card"（使用独立显卡启动）即可。

  方案 3：全局默认切换到 NVIDIA
  如果你想让整个桌面默认使用 NVIDIA 显卡，需要在 GDM 登录时切换：
  1. 注销当前用户，回到 GDM 登录界面
  2. 点击登录界面右上角的齿轮图标或用户名旁边的设置
  3. 选择 "NVIDIA" 或 "Discrete Graphics" 选项
  4. 重新登录
  ▌ 如果登录界面没有该选项，可以在终端执行：
  ▌ echo "a" | sudo -S env XDG_CONFIG_DIRS="" gdm-runtime-config set daemon PreferredDisplayserver x11
  ▌ 但 Fedora 42 Wayland 下通常不需要改 X11。
  验证是否切换成功
  glxinfo | grep "OpenGL renderer"
  • 输出 NVIDIA GeForce GTX 1060 表示成功切换到独显
  • 输出 Mesa Intel 表示仍在使用集显
```

的确，这么运行就是可以了:
```txt
__NV_PRIME_RENDER_OFFLOAD=1 __GLX_VENDOR_LIBRARY_NAME=nvidia glxinfo | grep "OpenGL renderer"
```

