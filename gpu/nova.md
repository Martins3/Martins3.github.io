## nova core
https://docs.kernel.org/gpu/nova/index.html

https://kangrejos.com/2025/DRM%20and%20Nova%20GPU%20Driver%20(Update).pdf

https://rust-for-linux.com/nova-gpu-driver

## 在 virtme VM 中测试 nova-core

环境：`~/data/hack/vm/virtme`，vfio 直通 host 的 NVIDIA GPU（`opt/vfio`
里 0000:01:00.0 + 0000:01:00.1），内核树 `~/data/kernel/linux-drm`。

构建（build/ 封装流程）：
- `build/def/drm.config`：`CONFIG_DRM=y`（nova Kconfig 硬性依赖 `DRM=y`，
  不能是 m）+ `CONFIG_DRM_NOVA=m`（自动 select `NOVA_CORE`），
  Rust 由 `def/plus` 的 `CONFIG_RUST=y` 提供
- `./build/build-config.sh drm && ./build/build.sh drm`
- 产物：`linux-drm.mod/lib/modules/<kver>/kernel/drivers/gpu/nova-core/nova_core.ko`
  和 `.../drm/nova/nova.ko`

guest 内测试（`collei/scripts/collei.py` 重启 VM 后，`ge` 走 vsock 登录）：
- `virtme-init.sh` 会把 `/lib/modules/<kver>` 软链到共享的
  `linux-drm.mod`，guest 里直接 `sudo modprobe nova_core` 即可
- 测试循环：host 改代码 → `build.sh drm` 增量编译 → guest `rmmod` /
  `modprobe` → 看 dmesg，不用重启 guest
- nova-core 按 PCI class 匹配所有 NVIDIA VGA/3D 设备
  （`drivers/gpu/nova-core/driver.rs`），不需要逐设备 ID
- GSP 固件路径格式 `nvidia/<chip>/gsp/*-570.144.bin`（modinfo 可见），
  来自 linux-firmware，guest 通过共享 rootfs 直接读 host 的
  `/lib/firmware`

2026-07 实测（RTX 5060 Ti / GB206 Blackwell，内核 7.1.2）：
- 链路全部打通：vfio 直通、Rust 模块加载、probe、BAR0 映射、寄存器读取
- 但 `modprobe nova_core` 失败于芯片检测：
  `Unsupported chipset: boot42 = 0x1b6a1000 (architecture 0x1b, implementation 0x6)`，
  probe 返回 -EINVAL，模块可干净 `rmmod`
- 原因：`drivers/gpu/nova-core/gpu.rs` 的 chipset 列表只到
  Ada（AD107 = 0x197），0x1b 是 Blackwell；nova 的 Blackwell 支持还在
  drm-rust-next 开发中。Blackwell 目前只能用 nouveau（6.16+）
- 要测真功能需要 Turing/Ampere/Ada 的卡

完整的探测日志为:
```txt
[  237.859271] NovaCore 0000:00:09.0: vgaarb: pci_notify
[  237.859448] NovaCore 0000:00:09.0: runtime IRQ mapping not provided by arch
[  237.861116] NovaCore 0000:00:09.0: Unsupported chipset: boot42 = 0x1b6a1000 (architecture 0x1b, implementation 0x6)
[  237.861382] NovaCore 0000:00:09.0: probe with driver NovaCore failed with error -22
[  237.861580] NovaCore 0000:00:09.0: vgaarb: pci_notify
```
