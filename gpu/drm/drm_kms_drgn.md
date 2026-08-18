# xe DRM/KMS Live 输出解读

## 一句话总结

这份输出说明：**当前真正正在出图的链路只有一条**，即：

```text
HDMI-A-2
  -> encoder DDI TC2/PHY C
  -> crtc pipe A
  -> primary plane 1A -> fb 524
  -> cursor A -> fb 531
```

也就是说，这块 `xe` 设备虽然注册了很多 KMS 对象，但当前只有 `pipe A` 在驱动一块已经接上的 HDMI 显示器，其余 `pipe B/C/D`、overlay plane、其他 encoder 目前都只是“存在能力，但没参与当前显示”。

---

## 测试来源

这是对 live kernel 的实测输出，命令如下：

```bash
bash ./drgn-wrapper.sh -c /proc/kcore ./drm_kms_relationship.py
```

分析对象是其中这一段：

```text
[Device] minor=0 driver=xe bus=0000:00:02.0
...
```

这里的 `driver=xe` 表示 Intel 新一代 DRM 驱动，`bus=0000:00:02.0` 表示对应的 PCI 显卡设备。

---

## Why：这份输出本质上在告诉你什么

这不是“有哪些结构体”的静态罗列，而是一次 **KMS 运行时拓扑快照**：

1. 这块 DRM 设备总共注册了多少个 `connector/crtc/plane/encoder/fb`
2. 哪些对象只是“硬件能力池”
3. 哪些对象此刻真的被路由起来，形成了实际显示链路

理解这份输出的关键是区分两类信息：

- **capacity**：这个 GPU 有哪些可用显示资源
- **live binding**：当前时刻实际怎么接线、怎么扫描输出

---

## What：先看核心关系图

对于这次输出，最重要的运行时关系可以画成：

```text
PCI 0000:00:02.0 (xe DRM device)
│
├── connector: HDMI-A-2 [connected]
│   └── encoder: DDI TC2/PHY C
│       └── crtc: pipe A
│           ├── primary plane: plane 1A
│           │   └── framebuffer: fb-524 (3840x2160, XR30)
│           └── cursor plane: cursor A
│               └── framebuffer: fb-531 (64x64, AR24)
│
├── connector: DP-1 [disconnected]
├── connector: HDMI-A-1 [disconnected]
│
├── crtc: pipe B [idle]
├── crtc: pipe C [idle]
└── crtc: pipe D [idle]
```

这个图已经足够表达这次机器当前的显示事实：

- 当前屏幕走的是 `HDMI-A-2`
- 它由 `pipe A` 驱动
- 主画面来自 `plane 1A -> fb 524`
- 鼠标光标来自 `cursor A -> fb 531`

---

## 第一层：设备级信息怎么读

原始输出：

```text
[Device] minor=0 driver=xe bus=0000:00:02.0
  drm_device: 0xffff88810f478000 registered=yes
  counts: planes=24 crtcs=4 encoders=6 connectors=3 framebuffers=3
```

可以拆成下面几个点：

### 1. `minor=0`

说明这是 `/dev/dri/card0` 对应的 primary DRM 设备。

### 2. `driver=xe`

说明这段输出属于 `xe` 驱动管理的 DRM 设备，不是 `nvidia-drm`，也不是 `vkms`。

### 3. `bus=0000:00:02.0`

这是 PCI BDF，通常就是板载 Intel GPU 所在的位置。

### 4. `registered=yes`

说明这个 `drm_device` 已经完成注册，已经对用户态可见，用户态可以通过 `/dev/dri/card0` 使用它。

### 5. `counts: planes=24 crtcs=4 encoders=6 connectors=3 framebuffers=3`

这几个数字表示“当前这个 DRM 设备维护的对象总数”，不是“活跃数”。

本例里最值得注意的是：

- `crtcs=4`：有四条显示控制流水线，分别是 `pipe A/B/C/D`
- `planes=24`：平均每个 pipe 有 6 个 plane
- `connectors=3`：暴露出 3 个物理/逻辑输出口
- `encoders=6`：有 6 个 encoder 对象
- `framebuffers=3`：当前内核里还挂着 3 个 framebuffer 对象

其中，很多对象只是注册存在，不代表在当前时刻真正参与出图。

---

## 第二层：`live graph` 才是当前真实显示链路

原始输出：

```text
  live graph
    HDMI-A-2
      -> encoder[id=520] DDI TC2/PHY C type=TMDS
      -> crtc[id=148] pipe A index=0
         -> plane 1A -> fb[id=524] fb-524 3840x2160 fmt=XR30
         -> cursor A -> fb[id=531] fb-531 64x64 fmt=AR24
```

这一段最重要，因为它直接给出“谁连到谁”。

### 1. `HDMI-A-2`

当前接上的显示输出口是 `HDMI-A-2`。从后面的 connector 状态也能看到它是 `connected`。

### 2. `-> encoder DDI TC2/PHY C`

表示这个 connector 当前选择的 encoder 是 `DDI TC2/PHY C`。

直观理解：

- `connector` 是外部插口
- `encoder` 是把像素流变成 HDMI/DP 电信号格式的中间硬件块

### 3. `-> crtc pipe A`

表示当前真正驱动这块显示器的是 `pipe A`。

CRTC 可以理解成“扫描输出控制器”：

- 决定时序
- 驱动扫描
- 接收 plane 混合结果
- 将结果送给 encoder

### 4. `plane 1A -> fb 524`

这是主画面层。

- `plane 1A` 是 `pipe A` 的 primary plane
- 它绑定到 `fb 524`
- `3840x2160` 说明当前主画面 framebuffer 的逻辑分辨率是 4K
- `fmt=XR30` 说明像素格式是 30-bit RGB 家族格式，通常可理解为 10bit/通道的 RGB scanout 格式

### 5. `cursor A -> fb 531`

这是独立硬件光标层。

- 它没有把鼠标光标画进主 framebuffer
- 而是使用单独的 cursor plane
- 这个 plane 绑定了一个小的 framebuffer：`64x64`
- `fmt=AR24` 说明它是一个带 alpha 的 32-bit 光标图像

这正是现代显示控制器常见做法：主图像一个 plane，光标一个专用 plane。

---

## 第三层：connector 段说明“哪些口接上了，哪些没接”

原始输出里三个 connector：

```text
connector[id=502] DP-1 status=disconnected
connector[id=515] HDMI-A-1 status=disconnected
connector[id=521] HDMI-A-2 status=connected
```

结论很直接：

- `DP-1` 没插显示器
- `HDMI-A-1` 没插显示器
- `HDMI-A-2` 插着显示器，并且已经被路由到 `pipe A`

更细一点看：

```text
possible_encoders: DDI TC2/PHY C
current_encoder: DDI TC2/PHY C
current_crtc: pipe A
```

这说明：

- `HDMI-A-2` 并不是任意 encoder 都能驱动
- 它当前只能/至少可以走 `DDI TC2/PHY C`
- 而且此刻确实就是这样连的

对于 disconnected 的 connector：

```text
current_encoder: (none)
current_crtc: (none)
```

这说明它们当前没有参与显示链路。

---

## 第四层：encoder 段说明“编码器资源池”和当前谁被占用

原始输出里一共 6 个 encoder：

- `DDI TC1/PHY B`
- `DP-MST A`
- `DP-MST B`
- `DP-MST C`
- `DP-MST D`
- `DDI TC2/PHY C`

其中真正活跃的只有：

```text
encoder[id=520] DDI TC2/PHY C
  current_crtc: pipe A
  attached_connectors: HDMI-A-2
```

其他 encoder 都是：

```text
current_crtc: (none)
attached_connectors: (none)
```

这表示它们目前没有绑定到任何输出链路。

### 为什么会有 4 个 `DP-MST` encoder 但都没用

`DP-MST` 是 DisplayPort Multi-Stream Transport，对应一个 DP 口上分叉多路显示流的能力。

这次机器上：

- DP 相关 encoder 对象已经注册出来了
- 但当前没有 DP 显示链路在工作
- 所以它们都处于 idle 状态

这很正常，说明驱动把能力建模出来了，但当前没有使用。

---

## 第五层：CRTC 段说明“有哪些 pipe，谁在真正扫描输出”

原始输出：

```text
crtc[id=148] pipe A
  primary_plane: plane 1A
  cursor_plane: cursor A
  connectors: HDMI-A-2
  encoders: DDI TC2/PHY C
  active_planes: plane 1A, cursor A
```

这说明 `pipe A` 是唯一活跃的显示控制器。

而 `pipe B/C/D` 都是：

```text
connectors: (none)
encoders: (none)
active_planes: (none)
```

这表示：

- 这些 pipe 硬件上存在
- DRM/KMS 也为它们创建了对象
- 但当前没有任何 connector 绑定到这些 pipe
- 因此它们不在扫描输出

### 如何理解 `primary_plane` 和 `cursor_plane`

例如：

```text
primary_plane: plane 1A
cursor_plane: cursor A
```

表示 `pipe A` 至少天然拥有两类关键 plane：

- `primary plane`：承载主桌面内容
- `cursor plane`：承载鼠标光标

这也是 KMS 最典型的最小配置。

---

## 第六层：plane 段说明“图层池很大，但当前只用了两个”

最关键的 plane 是这两个：

```text
plane[id=33] plane 1A type=PRIMARY
  possible_crtcs: pipe A
  current_crtc: pipe A
  current_fb: fb 524
  state: crtc_xy=(0, 0) crtc_wh=(3840x2160) zpos=0

plane[id=143] cursor A type=CURSOR
  possible_crtcs: pipe A
  current_crtc: pipe A
  current_fb: fb 531
  state: crtc_xy=(1310, 658) crtc_wh=(64x64) zpos=5
```

### `plane 1A`

这是主图层：

- 只能挂到 `pipe A`
- 当前确实挂在 `pipe A`
- 当前扫描的是 `fb 524`
- `crtc_xy=(0, 0)` 说明它从屏幕左上角开始显示
- `crtc_wh=(3840x2160)` 说明它覆盖整个 4K 目标区域
- `zpos=0` 说明它在底层

### `cursor A`

这是硬件光标层：

- 当前也挂到 `pipe A`
- 当前绑定 `fb 531`
- `crtc_xy=(1310, 658)` 说明光标当前位于屏幕上的这个坐标
- `crtc_wh=(64x64)` 说明光标平面尺寸是 64x64
- `zpos=5` 说明它在更高层，压在主画面之上

### 为什么其他 `plane 2A/3A/4A/5A` 都是空的

例如：

```text
current_crtc: (none)
current_fb: (none)
state: crtc_wh=(0x0)
```

说明这些 overlay plane 当前没启用。

这通常意味着：

- 当前桌面合成器没有使用额外硬件 overlay
- 或者当前 workload 不需要多层直接 scanout

所以虽然每个 pipe 都有一组丰富的 plane 资源，但这次实测只用到了：

- 一个 primary plane
- 一个 cursor plane

---

## 第七层：framebuffer 段说明“哪些像素缓存真的被引用”

原始输出：

```text
fb[id=524] fb-524 3840x2160 fmt=XR30
  modifier: 0x100000000000002
  pitches: 15360, 0, 0, 0
  users: plane 1A

fb[id=527] fb-527 3840x2160 fmt=XR24
  modifier: 0x0
  pitches: 16384, 0, 0, 0
  users: (none)

fb[id=531] fb-531 64x64 fmt=AR24
  modifier: 0x0
  pitches: 256, 0, 0, 0
  users: cursor A
```

### `fb 524`

这是当前主显示 framebuffer：

- 分辨率 `3840x2160`
- 被 `plane 1A` 使用
- 因此它就是当前桌面主画面的扫描源

`pitches: 15360` 可以这样理解：

- 每行 15360 字节
- 3840 像素宽
- 平均每像素 4 字节

这和常见 32-bit/像素 scanout 排布是一致的。

### `fb 531`

这是 cursor framebuffer：

- 大小只有 `64x64`
- 被 `cursor A` 使用
- 明确是硬件光标专用的小 buffer

### `fb 527`

这个 framebuffer 当前没有 `users`：

- 它还在 `fb_list` 上
- 但没有任何 plane 正在引用它

这通常意味着它可能是：

- 旧帧残留
- 预分配但未提交
- 曾参与显示但当前已切走

总之，**它当前不在扫描路径上**。

---

## How：把这份输出翻译成“当前屏幕是怎么刷出来的”

更贴近硬件的理解方式是：

```text
1. pipe A 被启用，负责产生当前显示时序
2. plane 1A 从 fb 524 中取 3840x2160 的主画面像素
3. cursor A 从 fb 531 中取 64x64 的光标像素
4. pipe A 将两个 plane 按 zpos 混合
5. 混合结果送到 encoder DDI TC2/PHY C
6. encoder 将像素流转换成 HDMI 信号
7. HDMI-A-2 把信号送到外部显示器
```

这正是 DRM/KMS 的核心运行模型。

---

## How Safe：这份输出里可以推断出的并发和状态特征

虽然输出本身没直接打印锁，但结合 DRM/KMS 语义，可以读出几件事：

### 1. 这是一次“当前原子状态”的观测

脚本优先读取：

- `drm_connector.state`
- `drm_plane.state`

因此看到的是 atomic KMS 视角下的当前状态，而不只是 legacy 字段。

### 2. `current_*` 不等于 `possible_*`

这体现了 KMS 很重要的两层含义：

- `possible_crtcs` / `possible_encoders`：能力约束
- `current_crtc` / `current_encoder` / `current_fb`：当前绑定状态

能力大，不代表现在就在用。

### 3. `fb_list` 比 active planes 更大是正常的

因为 framebuffer 生命周期并不要求“只要不在扫就立即销毁”。

所以：

- `framebuffers=3`
- 但 active plane 只引用了其中 2 个

这不是异常。

---

## 这份输出最值得记住的 6 个结论

1. 这台机器上 `xe` 设备有 4 条 pipe，但当前只有 `pipe A` 在工作。
2. 当前唯一接上的显示输出是 `HDMI-A-2`。
3. 当前活跃链路是 `connector HDMI-A-2 -> encoder DDI TC2/PHY C -> crtc pipe A`。
4. 当前真正参与扫描的 plane 只有两个：`plane 1A` 和 `cursor A`。
5. 主桌面像素来自 `fb 524`，鼠标光标像素来自 `fb 531`。
6. 其余 plane、crtc、encoder 大多只是空闲资源，不代表配置错误。

---

## 什么时候这份输出会变

如果发生下面这些事件，这份 live 拓扑就会变化：

- 插上 DP 显示器
- 把显示器从 `HDMI-A-2` 切到别的口
- 开启第二块显示器
- 合成器启用 overlay plane 直接扫描
- 分辨率/色深变化
- 光标移动

其中最容易立刻观察到的变化是：

- `cursor A` 的 `crtc_xy`
- `current_connector/current_crtc/current_fb`
- `active_planes`

---

## 后续建议

如果要继续深入，我建议顺着下面三个方向继续做：

1. 把 `fb 524` 的来源继续往上追，分析它对应的 GEM/BO 对象是谁分配的。
2. 对比 `xe`、`nvidia-drm`、`vkms` 三段输出，看看不同驱动的 KMS 对象建模差异。
3. 在插拔显示器、移动光标、切换分辨率前后各抓一次输出，做动态对比。

