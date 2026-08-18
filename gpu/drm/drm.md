# drm 入门

GPU 需要有很多的用户态和内核态的数据拷贝吗?

例如，有多个 app 都是需要渲染的

哦，还有 https://gitlab.freedesktop.org/mesa/libdrm 来封装

问了一下 Gwen ，大致是这么说的:
Rust-GPU 着色器 → 编译为 SPIR-V → 由 wgpu 加载 → 调用 Vulkan → Mesa RADV 驱动 → AMD GPU

NVIDIA 专有驱动 + libGL / libvulkan 可以替代 mesa :

自带用户态驱动库（libGLX_nvidia.so, libvulkan.so）
在 Linux 上装了 NVIDIA 专有驱动后，glxinfo 显示的 OpenGL 实现就来自 NVIDIA，而不是 Mesa

## drm edid 错误是什么东西
<!-- f2d2279c-f7d1-4051-b898-8e1be8d0fab8 -->

Linux 内核 DRM 子系统中 `drm_edid.c` 的源码）上下文中，**“kernel EDID has corrupt header”** 的含义是：

> **内核在通过 DDC（Display Data Channel）从显示器读取 EDID（Extended Display Identification Data）数据时，发现其前 8 个字节不符合标准 EDID 头部格式，因此判定该 EDID 头部“损坏”或“不合法”。**

### 具体解释

#### 1. **什么是 EDID？**
EDID 是显示器提供给主机的一段固定格式的数据块（通常 128 字节或更多），描述了：
- 显示器厂商、型号
- 支持的分辨率、刷新率
- 物理尺寸
- 色彩能力等

#### 2. **EDID 的标准头部**
一个合法的 EDID **base block（第一个128字节）** 的前 8 字节必须是：
```c
static const u8 edid_header[] = {
    0x00, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00
};
```
这是 VESA 标准规定的“魔数”，用于快速识别 EDID。

#### 3. **“corrupt header” 的检测逻辑**
在 `drm_edid.c` 中：
- `drm_edid_header_is_valid()` 会逐字节比对读到的 EDID 前 8 字节与 `edid_header`。
- 如果匹配字节数 **少于 `edid_fixup` 模块参数的阈值**（默认是 6），就会被判定为 `EDID_BLOCK_HEADER_CORRUPT`。
- 此时内核会打印：
  ```c
  pr_notice("EDID has corrupt header\n");
  ```
  并可能拒绝使用该 EDID，导致无法正确识别显示器能力（例如分辨率受限、无音频等）。

#### 4. **为什么会发生？**
常见原因包括：
- **显示器固件缺陷**：某些廉价/老旧显示器 EDID 固件写错。
- **DDC 通信干扰**：I²C 总线噪声、线缆质量差、GPU 电平不匹配，导致读取数据出错。
- **EDID 被人为修改（如通过 override）但格式错误**。
- **虚拟机/KVM 环境中虚拟显卡模拟不完善**。

#### 5. **内核如何应对？**
- 如果匹配字节数 ≥6 但 <8，内核会尝试 **自动修复**（`edid_header_fix()`），并标记为 `EDID_BLOCK_HEADER_FIXED`。
- 如果匹配字节数 <6（或 `edid_fixup` 设置得更高），则视为不可修复，丢弃该 EDID。
- 用户可通过 `drm.edid_fixup=N` 内核参数调整容忍度（N=0~8）。

### 小结
“kernel EDID has corrupt header” 并不总是表示显示器坏了，而更可能是：
- **通信错误**（最常见）
- **EDID 数据本身不规范**

你可以通过以下方式诊断：
```bash
# 查看实际读到的 EDID
sudo ddcutil getvcp 0x??  # 或
cat /sys/class/drm/card0-eDP-1/edid | xxd

# 检查内核日志细节
dmesg | grep -i edid
```

如果问题稳定复现，可考虑使用 `drm.edid_firmware=...` 加载正确的 EDID 固件覆盖。

## 这个选项是做什么的?
CONFIG_DRM_ACCEL

可以用 xe 的驱动测试一下

这个代码直接放到 drivers/accel/ 中了，和想象的不一样的
