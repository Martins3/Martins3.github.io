## 有的机器需要配置 video=efifb:on

这个功能的作用到底是什么来着?

可以用 qemu 来测试吗?

https://gitee.com/openeuler/kernel/issues/I4E6K1

原来我们内部配置了这个:
video=efifb:off

还是需要完成 /home/martins3/data/vn/code/src/m/gpu/README.md 中的测试啊

加上 video=efifb:off 后：
- 内核不再加载或启用 efifb
- 由固件提供的屏幕输出不再使用
- 早期启动阶段可能不再有图形输出（或退回 VGA/文本）


## 如何理解这这段日志
-std vga
```txt
[    4.731382] bochs-drm 0000:00:02.0: vgaarb: deactivate vga console
[    4.733488] Console: switching to colour dummy device 80x25
[    4.733664] [drm] Found bochs VGA, ID 0xb0c5.
[    4.733665] [drm] Framebuffer size 16384 kB @ 0xfd000000, mmio @ 0xfc2d0000.
[    4.733723] [TTM] Zone  kernel: Available graphics memory: 4065298 KiB
[    4.733724] [TTM] Zone   dma32: Available graphics memory: 2097152 KiB
[    4.733725] [TTM] Initializing pool allocator
[    4.733727] [TTM] Initializing DMA pool allocator
[    4.734430] [drm] Found EDID data blob.
[    4.734497] [drm] Initialized bochs-drm 1.0.0 20130925 for 0000:00:02.0 on minor 0
[    4.736484] fbcon: bochs-drmdrmfb (fb0) is primary device
[    4.738146] Console: switching to colour frame buffer device 160x50
[    4.738446] bochs-drm 0000:00:02.0: [drm] fb0: bochs-drmdrmfb frame buffer device
```

-device virtio-gpu-pci 内核启动日志:
```txt
[    3.776902] [drm] pci: virtio-gpu-pci detected at 0000:00:0c.0
[    3.776977] ipmi_si: Trying PCI-specified kcs state machine at i/o address 0xc390, slave address 0x0, irq 10
[    3.776985] [drm] features: -virgl +edid -resource_blob -host_visible
[    3.776986] [drm] features: -context_init
[    3.777479] [drm] number of scanouts: 1
[    3.777482] [drm] number of cap sets: 0
[    3.777734] virtio-pci 0000:00:0c.0: [drm] Registered 1 planes with drm panic
[    3.777735] [drm] Initialized virtio_gpu 0.1.0 for 0000:00:0c.0 on minor 0
[    3.777897] ipmi_si 0000:00:0a.0: Using irq 10
[    3.780271] scsi 3:2:1:0: Direct-Access     QEMU     QEMU HARDDISK    2.5+ PQ: 0 ANSI: 5
[    3.782818] Console: switching to colour frame buffer device 160x50
[    3.783129] virtio-pci 0000:00:0c.0: [drm] fb0: virtio_gpudrmfb frame buffer device
```

## 真的有哇
/sys/bus/platform/drivers/efi-framebuffer

drivers/video/fbdev/efifb.c

drivers/gpu/drm/sysfb/efidrm.c
drivers/gpu/drm/sysfb/simpledrm.c

## 问问 chatgpt
似乎这个解释是合理的:
本质是固件给了“画布”，内核负责“写像素”

内核之所以需要实现 **efifb**（EFI framebuffer），而不能完全依赖 GPU 专用驱动，有明确的体系架构与现实需求驱动。关键原因在于：

1. **启动早期必须有显示输出**

GPU 专用驱动（i915、amdgpu、nouveau、nvidia 等）只有在内核后期阶段才能加载，它依赖于：

* 总线/PCI 设备枚举完成
* IOMMU、BAR 映射准备就绪
* 时钟/电源/显示管线初始化可用
* DRM/KMS 子系统上线

在此之前，系统无法由 GPU 驱动负责显示。
为了在这些组件准备好之前仍然能输出信息（控制台、日志、panic dump），内核需要一个**可立即工作的显示接口**，而 UEFI 已经将设备初始化好了，并留下了 framebuffer，因此 efifb 作为“过渡层”出现。

> efifb = GPU 驱动到位前的临时安全着陆区。

---

2. **UEFI 已经为我们初始化好了显卡**

UEFI 的 GOP（Graphics Output Protocol）在开机阶段已经做了：

* 显卡寄存器基本初始化
* 显存映射与物理地址提供
* 分辨率和显示模式设定

所以 efifb 的作用是“直接使用现成资源”，而不是重新初始化硬件。

如果直接让 GPU 驱动接管：

* GPU 驱动必须重新设置显示模式
* 模式切换可能导致屏幕黑闪
* 如果 GPU 驱动加载失败，你连调试日志都看不到

efifb 为系统提供了一条**最低风险回退路径**。

4. **特殊工作负载需要避免 GPU 驱动接管**

在 VM 直通/IOMMU/VFIO 场景中常见这种需求：

```
GPU 不由宿主机驱动，而提供给虚拟机独占使用
```

此时 efifb 让宿主机仍保有基本显示能力，而不会占用 GPU 资源。

(这个似乎有所耳闻了)


**架构定位：efifb 是“桥梁”，不是“替代者”**

从 Linux 显示架构看：

```
[Boot Firmware]
      │ GOP/UGA
      ▼
  efifb / simpledrm        ← 启动早期、最低限度显示
      ▼
GPU DRM/KMS drivers        ← 真正驱动显卡
      ▼
X11/Wayland/DE/桌面环境
```

efifb 的定位是：

* 不负责加速
* 不负责 modeset
* 不负责输出管线控制
* 不负责 3D/渲染

只有 GPU 驱动才具备这些能力。

(基本上感觉是合理的，但是无法调试我们想要的问题)


