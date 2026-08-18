## 测试看看

https://news.ycombinator.com/item?id=43931845
	- https://github.com/martinfama/fui

关键文档:
Documentation/fb/framebuffer.rst

可以跑一个 perf 看看整个流程


## 关键问题 : fb 和 drm 是什么关系?
<!-- c71d97b0-febb-4acb-a520-86c3f7d41763 -->

CONFIG_FB_CIRRUS
CONFIG_DRM_CIRRUS_QEMU

在 Linux 图形子系统中，**framebuffer（fbdev）** 与 **DRM（Direct Rendering Manager）** 不是并列或等价的概念，而是**历史演进关系 + 功能层级关系**。一句话先给出结论：

> **framebuffer 是早期的、简单的显示抽象；DRM 是现代的、完整的图形与显示管理框架。**
> **fbdev 关注“如何把像素显示出来”，DRM 关注“如何管理 GPU、显示管线和渲染”。**

下面从架构、职责、内核实现以及实际使用角度系统说明二者关系。

### 一、framebuffer（fbdev）是什么

#### 1. 基本定位

**framebuffer（fbdev）** 是 Linux 早期的显示子系统，目标非常简单：

* 提供一个 **线性显存缓冲区**
* 用户空间 **直接往内存写像素**
* 内核/硬件负责把这块内存扫描到屏幕

典型设备：

```text
/dev/fb0
```

典型工作模型：

```
用户空间 → mmap(/dev/fb0) → 写像素 → 显示
```

#### 2. fbdev 的能力边界

fbdev **不关心**：

* GPU 3D 能力
* 多平面（plane）
* 显示模式切换的复杂性
* 显示输出（HDMI / DP / eDP）拓扑
* 多屏 / 原子更新 / 同步

它只提供：

* 一块 framebuffer
* 简单的分辨率、像素格式
* 基本的 pan / blank

---

### 二、DRM 是什么

#### 1. DRM 的真实角色

**DRM（Direct Rendering Manager）** 是 Linux 现代图形栈的**内核基础设施**，负责：

* GPU 资源管理（GEM / VRAM）
* 显示管线管理（KMS）
* 原子模式设置（atomic modeset）
* buffer 同步（fence）
* 安全的 GPU 共享（多进程）

用户接口：

```text
/dev/dri/card0
/dev/dri/renderD128
```

#### 2. DRM 的两大核心子系统

1. KMS（Kernel Mode Setting）

负责：

* 显示模式（分辨率 / 刷新率）
* CRTC / encoder / connector 管理
* plane / overlay / cursor
* 多屏 & 热插拔

2. GEM / TTM

负责：

* GPU buffer 对象
* 显存 / system memory 管理
* mmap / dma-buf 共享


## 三、二者的**核心关系**

从演进关系看：

```
fbdev  ────────────┐
                   ├── DRM + KMS
XFree86 DRM (早期) ┘
```

* fbdev：**只解决“显示”**
* DRM：**解决“渲染 + 显示 + 管理”**

DRM 通常提供了模拟 framebuffer
```text
CONFIG_DRM_FBDEV_EMULATION
```

对应内核模块：

```text
drm_fbdev_generic
drm_fbdev_emulation
```

效果是：

* 基于 DRM/KMS
* 向用户空间暴露：

  ```text
  /dev/fb0
  ```
* 供：

  * early console
  * legacy 程序
  * initramfs 阶段使用

👉 **这个 fb0 并不是真正的老 fbdev 驱动，而是“DRM-backed fbdev”**


现在的趋势是：

| 类型                    | 状态          |
| --------------------- | ----------- |
| 原生 fbdev 驱动           | 维护最小化 / 不推荐 |
| DRM + fbdev emulation | 主流          |
| 纯 DRM（无 fbdev）        | 桌面/嵌入式新系统   |


### 四、数据路径对比

#### 原生 framebuffer

```
userspace
  └─ mmap(/dev/fb0)
      └─ framebuffer memory
          └─ display scanout
```

问题：

* 无同步
* 无 page flip
* 无多 buffer

---

#### DRM（现代）

```
userspace (Wayland/Xorg/GBM)
  └─ GEM buffer
      └─ KMS plane
          └─ CRTC
              └─ encoder
                  └─ connector
                      └─ display
```

特点：

* 原子更新
* page flip
* vblank 同步
* 多 plane 合成

---

## 五、为什么 DRM 取代 framebuffer

1. 多显示 & 热插拔

fbdev **无法表达**：

* HDMI 插拔
* DP MST
* 多屏拓扑

DRM/KMS 是为此设计的。

2. GPU 与显示统一管理

fbdev 假设：

* 显示 = 一块内存

现实：

* scanout buffer 可能在 VRAM
* buffer 需要 cache / IOMMU / fence
* GPU 和 display pipeline 强耦合

DRM 才能正确管理。

3. 安全与多进程

fbdev：

* 任意进程可写显存

DRM：

* buffer ownership
* render node
* 权限隔离

---

## 六、实际系统中你看到的“共存”

你可能看到：

```text
/dev/fb0
/dev/dri/card0
```

这并不矛盾：

* `/dev/dri/card0`：**DRM 原生接口**
* `/dev/fb0`：**DRM fbdev 模拟层**

它们**共享同一套 DRM/KMS 硬件资源**。



### 八、一句话总结（工程视角）

> **framebuffer 是“把像素送到屏幕”的最低抽象；DRM 是“管理 GPU 与显示硬件”的现代内核框架。**
> **今天的 framebuffer 往往只是 DRM 的一个兼容层，而不是独立的显示子系统。**

如果你愿意，我可以：

* 结合 **efifb → simpledrm → amdgpu/i915** 的接管流程画一条完整启动链
* 或从 **内核代码（drivers/gpu/drm/）** 角度逐层对照 fbdev 与 DRM 的实现差异

## 如何理解这个东西

```txt
[    9.607409] fbcon: Taking over console
```

## 这几个问题得回答下吧
```txt
# 1. 如果不用图形，可以直接把 DRM 也关闭 ，VIRTIO_GPU 都可以关闭
# 2. 才意识到，需要 FB 才可以显示界面，而且 FB 和 VIRTIO_GPU 是互相不用关联的
# 3. 不过，这个是默认关闭的吗?
CONFIG_DRM=m
CONFIG_FB=m
CONFIG_DRM_VIRTIO_GPU=m
即便是打开了以上选项，但是配置 CONFIG_VT is not set ，那么 vnc 还是没有内容
```
这个观察其实非常的有意思，
不过需要确认一下，当 drm 关闭的时候，如果仅仅配置 CONFIG_VT ，那么会有内容吗?
是会完全没有吗?

此外，需要注意，CONFIG_FB_DEVICE 和 CONFIG_FB 中的内容是不同的。

这个需要我们重新构建内核，但是现在的环境需要考虑 nvidia 了。


还没法测试 fb ，fedora 的内核中不存在这个东西:

```txt
  - 内核配置里是 CONFIG_FB=y、CONFIG_DRM_FBDEV_EMULATION=y，但同时 # CONFIG_FB_DEVICE is not set
  - 启动日志里确实出现过 fb0：
      - simpledrm ... fb0: simpledrmdrmfb
      - xe ... fb0: xedrmfb frame buffer device

  这说明：

  - 启动阶段内核有 framebuffer console / DRM fbdev emulation
  - 但当前内核没有把老的 framebuffer 设备接口导出给用户态
  - 所以不能在当前环境直接跑依赖 /dev/fb0 的 framebuffer 测试程序

  如果你的目标是“显示输出测试”，当前机器更适合测 DRM/KMS，而不是 legacy fbdev。
  如果你的目标就是 /dev/fb0，那需要换环境或改内核配置，关键是打开 CONFIG_FB_DEVICE。如果你要，我可以继续帮你判断该用 VM、改内核，还是
  改成 DRM/KMS 测试。
```
