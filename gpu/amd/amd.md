https://news.ycombinator.com/item?id=43071983

## HipKittens
https://hazyresearch.stanford.edu/blog/2025-11-09-amd-brr
## 为什么 AMD GPU 的源码这么大

让 codex 看了一下，但是我的核心问题没有帮我解释，
那就是真的存在这么多的寄存器吗?

> [!NOTE]
> 参考神奇海螺的意见，有待验证

• 在这棵内核树里，tokei drivers/gpu/drm/amd 的结果是 Code = 5,111,450。但这个数字很容易误导，因为 AMD 这边最大的不是普通 .c 逻辑，而是海量硬件寄存器/位域
  头文件。

  按 Code 列拆开看：

  - drivers/gpu/drm/amd/include: 4,262,722，约占整个 AMD DRM 代码量 83.4%
  - drivers/gpu/drm/amd/include/asic_reg: 4,168,089，单独就占总量 81.5%
  - drivers/gpu/drm/amd/display: 404,652
  - drivers/gpu/drm/amd/amdgpu: 259,201
  - drivers/gpu/drm/amd/pm: 137,952
  - drivers/gpu/drm/amd/amdkfd: 37,795
  - drivers/gpu/drm/i915: 291,874
  - drivers/gpu/drm/nouveau: 172,989

  所以第一结论是：AMD “特别大”，首先是因为它把非常多的硬件描述直接带进了内核源码里。asic_reg 里一共 450 个头文件，其中 202 个是 *_sh_mask.h，160 个是
  *_offset.h。像 drivers/gpu/drm/amd/include/asic_reg/dcn/dcn_3_2_0_sh_mask.h:1 这种文件，基本全是寄存器 bitfield/mask 宏，不是复杂控制流逻辑
。tokei 会把
  这些宏都算进 Code，所以总量会被显著拉高。

  这些头文件主要在描述什么：

  - 显示控制器相关最多：dcn 1,346,582，dpcs 405,649，dce 198,931
  - 图形核心：gc 484,752
  - I/O/总线：nbio 961,845
  - 内存管理 hub：mmhub 205,903

  第二结论是：去掉 include 以后，真正“逻辑代码”的主体其实是显示栈。5,111,450 - 4,262,722 = 848,728，而 display 自己就有 404,652，接近一半。这
里主要做的
  是：

  - 显示管线和资源分配：pipe、clock、bandwidth、watermark、timing 校验
  - DP/HDMI/eDP 链路训练与显示输出控制
  - Freesync、HDCP、DSC、颜色管理
  - DMUB 固件交互和 DRM/KMS 对接

  最大的几个 .c 文件也说明了这一点：

  - drivers/gpu/drm/amd/display/dc/dml2_0/dml21/src/dml2_core/dml2_core_dcn4_calcs.c:1
  - drivers/gpu/drm/amd/display/dc/dml2_0/display_mode_core.c:1
  - drivers/gpu/drm/amd/display/amdgpu_dm/amdgpu_dm.c:1

  第三块是 amdgpu 核心驱动本体，主要做：

  - 命令提交、ring、调度、VM/GTT/VRAM 管理
  - 各代 GFX/IP block 支持
  - PSP 安全处理器、RAS、中断、视频编解码引擎

  从 drivers/gpu/drm/amd/amdgpu/Makefile:1 可以直接看到它同时维护了很多代的 gfx_v6/7/8/9/10/11/12、gmc/gfxhub/mmhub、psp_v*、sdma_v*、uvd/vce/vcn/jpeg。
  像 drivers/gpu/drm/amd/amdgpu/gfx_v10_0.c:1 这种单文件就非常大。

  第四块是电源管理 pm，主要是：

  - powerplay
  - swsmu
  - legacy-dpm

  这里做的是时钟、电压、风扇、温度、功耗表、SMU 固件接口，而且按代际拆得很细，smu11/12/13/14 和大量 *_ppt.c 文件就是这个来源。
