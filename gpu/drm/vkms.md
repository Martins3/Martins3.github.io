
• drivers/gpu/drm/vkms/ 是 Linux DRM 子系统里的 VKMS 驱动目录，也就是 Virtual Kernel Mode Setting。它不是给真实显示硬件用的，而是提供
  一个“纯软件的虚拟显示管线”，主要用途是测试 DRM/KMS 框架、跑 IGT/KUnit、以及在没有实体显示器或没有真实输出能力的机器上跑 X/Wayland/
  compositor。drivers/gpu/drm/vkms/Kconfig:3 里也写得很直接：用于 testing 和 headless machine；drivers/gpu/drm/vkms/vkms_drv.c:4 的注
  释说明它是 “software-only model of a KMS driver”。

  这个目录里的代码基本是在“模拟一张显示卡的显示输出部分”：
  vkms_drv.c 是驱动入口，drivers/gpu/drm/vkms/vkms_output.c:8 负责把 plane、CRTC、encoder、connector 这些 DRM 对象组起来，
  vkms_writeback.c 提供 writeback connector，vkms_configfs.c 支持通过 configfs 动态创建多个虚拟 VKMS 实例，tests/ 下面是它自己的
  KUnit 测试。官方文档也在 Documentation/gpu/vkms.rst:1。

  如果你愿意，我可以继续按“文件分工”把这个目录逐个文件讲一遍。

## vkms 具体做了什么东西:


• 整体结构
  vkms 可以理解成“把一条 DRM/KMS 显示流水线用软件实现一遍”。它不驱动真实显示器，也不写真实硬件寄存器，但会向 DRM core 暴露正常的 KMS 对象和行为，所以用户态看上去它像一块能 modeset 的显卡。

  核心分工大致是：

  - drivers/gpu/drm/vkms/vkms_drv.c:65 是入口。它注册一个 drm_driver，初始化 mode config、vblank、fbdev/shmem helper，并创建默认 VKMS 设备。
  - drivers/gpu/drm/vkms/vkms_output.c:8 按配置把 plane、CRTC、encoder、connector 组装起来，形成一条完整显示管线。
  - drivers/gpu/drm/vkms/vkms_plane.c:124 处理 plane 的 framebuffer 准备、映射、atomic check/update，并为不同像素格式选择对应的读像素 函数。
  - drivers/gpu/drm/vkms/vkms_crtc.c:15 负责 vblank、page flip event、CRC source、以及把 compositor worker 挂到 vblank 节奏上。
  - drivers/gpu/drm/vkms/vkms_composer.c:471 是“软件出图”的核心：逐行读 plane、做旋转/裁剪/混合/颜色变换/LUT，最后算 CRC，必要时写到 writeback buffer。
  - drivers/gpu/drm/vkms/vkms_formats.c:14 负责各种像素格式的读取和转成统一内部格式，尤其是 RGB/YUV/低位深格式。
  - drivers/gpu/drm/vkms/vkms_writeback.c:34 实现虚拟 writeback connector，把“最终画面”写回一个 framebuffer。
  - drivers/gpu/drm/vkms/vkms_config.c:34 和 drivers/gpu/drm/vkms/vkms_configfs.c:689 负责描述和动态创建不同拓扑的 VKMS 实例，比如多
    个 plane/CRTC/connector、writeback、热插拔状态。

  关键执行路径
  一次典型 atomic commit，大致是这样跑的：

  - 用户态提交 atomic state，vkms_atomic_check() 做基础校验，然后大量复用 DRM helper 路径，drivers/gpu/drm/vkms/vkms_drv.c:105。
  - plane 在 prepare_fb 里把 GEM/shmem framebuffer vmap 到内核地址，后面 compositor 直接按内存读像素，drivers/gpu/drm/vkms/
    vkms_plane.c:181。
  - CRTC 在 flush 阶段设置 event、更新当前 compositor state；vblank 超时处理函数会周期性触发 worker，drivers/gpu/drm/vkms/
    vkms_crtc.c:170 和 drivers/gpu/drm/vkms/vkms_crtc.c:15。
  - worker 调 drivers/gpu/drm/vkms/vkms_composer.c:536，逐行混合所有 active planes，计算整帧 CRC；如果开启 writeback，就把结果写回目
    标 fb，drivers/gpu/drm/vkms/vkms_composer.c:594。
  - 最后把 CRC 通过 drm_crtc_add_crc_entry() 上报给 DRM core，writeback 通过 drm_writeback_signal_completion() 完成，drivers/gpu/drm/
    vkms/vkms_composer.c:648。

  也就是说，VKMS 并不是“假装成功返回”，而是真的把 framebuffer 内容按 KMS 规则走了一遍软件合成。这是它测试价值的关键。

  为什么它很适合做测试

  - 它覆盖的是 DRM/KMS 的“协议和行为”，不是某家 GPU 的寄存器细节。atomic check/commit、plane 约束、vblank、flip event、writeback、
    CRC、connector hotplug 这些接口都能测到。
  - 它是纯软件、确定性强。输出帧来自内存里的软件合成，结果可以直接转成 CRC，比真实硬件更稳定，更适合 CI。
  - 它没有真实显示器依赖，headless/VM 里也能跑。Documentation/gpu/drm-uapi.rst:633 明说它适合在 VM 里验证 DRM API。
  - 它能模拟多种显示能力。模块参数能开关 cursor/overlay/writeback/pipeline；configfs 还能动态拼不同拓扑和 connector 状态，drivers/
    gpu/drm/vkms/vkms_drv.c:43 和 drivers/gpu/drm/vkms/vkms_configfs.c:702。
  - 它方便把算法逻辑单独做 KUnit。比如颜色插值/LUT/CTM 在 drivers/gpu/drm/vkms/tests/vkms_color_test.c:91，YUV 到 ARGB 转换在
    drivers/gpu/drm/vkms/tests/vkms_format_test.c:215，拓扑配置合法性在 drivers/gpu/drm/vkms/tests/vkms_config_test.c:89。
  - 它大量复用 DRM helper，因此能很好地验证 “DRM core + helper + 用户态接口” 这一层是否回归，而不是只测 VKMS 自己。

  一句话说，vkms 的价值在于：它把“显示驱动最值得测的通用 KMS 行为”抽出来，用一个没有硬件依赖、结果可验证的纯软件模型实现了。

  如果你想继续，我可以下一步按“从 modprobe vkms 到用户态看到 /dev/dri/cardX”这条路径，再顺一遍调用链。

