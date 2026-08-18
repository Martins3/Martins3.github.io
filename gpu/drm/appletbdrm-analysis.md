# `appletbdrm` 驱动实现分析


源文件：`drivers/gpu/drm/tiny/appletbdrm.c`

## 1. 驱动定位

`appletbdrm` 不是传统意义上的显示控制器驱动，而是一个通过 USB bulk 传输把 DRM framebuffer 内容发送到 Apple Touch Bar 设备的小型 DRM 驱动。

它的核心思路是：

- 在 DRM 侧暴露一个固定模式的显示设备
- 使用 shadow plane 让 CPU 能访问 framebuffer 内容
- 利用 damage tracking 只上传发生变化的矩形区域
- 把这些矩形按设备私有协议打包后通过 USB 发给 Touch Bar

从结构上看，这个驱动更像“USB 显示协议适配层”，而不是“直接驱动显示硬件扫描输出”的 KMS 驱动。

## 2. 私有数据结构

### 2.1 `struct appletbdrm_device`

这个结构保存驱动实例的核心状态，定义见 `appletbdrm.c` 中 `struct appletbdrm_device`：

- USB bulk in/out endpoint
- 设备上报的 `width` / `height`
- DRM 对象：
  - `drm_device`
  - `drm_display_mode`
  - `drm_connector`
  - `drm_plane`
  - `drm_crtc`
  - `drm_encoder`

这说明驱动的 DRM 建模非常简化，本质上就是一个单输出、单 primary plane 的显示设备。

### 2.2 协议结构体

驱动里定义了若干 USB 协议结构体：

- `struct appletbdrm_msg_request_header`
- `struct appletbdrm_msg_response_header`
- `struct appletbdrm_msg_simple_request`
- `struct appletbdrm_msg_information`
- `struct appletbdrm_frame`
- `struct appletbdrm_fb_request`
- `struct appletbdrm_fb_request_footer`
- `struct appletbdrm_fb_request_response`

可以看到其中有很多字段名是 `unk_xx`。这通常意味着协议并非公开文档定义，而是基于逆向或实验得到的最小可用实现。驱动真正关心的字段只有：

- 消息类型
- 长度
- 分辨率
- 像素格式
- 时间戳
- 每个 damage frame 的坐标和像素数据

## 3. 支持的协议消息

代码里定义了四类消息：

- `APPLETBDRM_MSG_CLEAR_DISPLAY`：`CLRD`
- `APPLETBDRM_MSG_GET_INFORMATION`：`GINF`
- `APPLETBDRM_MSG_UPDATE_COMPLETE`：`UDCL`
- `APPLETBDRM_MSG_SIGNAL_READINESS`：`REDY`

对应关系如下：

- `GINF`：获取设备分辨率、像素格式等信息
- `REDY`：向设备发送 readiness 信号
- `CLRD`：清屏
- `UDCL`：设备对 framebuffer 更新的完成响应

## 4. probe 流程

入口是 `appletbdrm_probe()`，整体时序如下：

1. 通过 `usb_find_common_endpoints()` 找到 bulk in/out endpoint
2. 通过 `devm_drm_dev_alloc()` 分配 DRM 设备对象
3. 调用 `appletbdrm_get_information()` 读取设备显示信息
4. 调用 `appletbdrm_signal_readiness()` 发送 readiness 消息
5. 调用 `appletbdrm_setup_mode_config()` 初始化 DRM mode setting 对象
6. 调用 `drm_dev_register()` 注册 DRM 设备
7. 调用 `appletbdrm_clear_display()` 清空 Touch Bar 当前显示内容

这条路径说明设备初始化依赖一个设备私有的 USB 握手流程，DRM 只是其上层抽象。

## 5. USB 消息收发逻辑

### 5.1 `appletbdrm_send_request()`

这个函数负责通过 `usb_bulk_msg()` 向设备发送请求。它做了两件很严格的校验：

- 发送返回值必须为 0
- `actual_size` 必须等于预期 `size`

如果长度不匹配，直接报错并返回 `-EIO`。

### 5.2 `appletbdrm_read_response()`

这个函数负责接收设备响应，同样通过 `usb_bulk_msg()` 从 bulk-in endpoint 读取。

这里有一个很关键的设备行为兼容：

- 在 USB 配置完成后的某个时间窗口内，设备可能会对“第一条请求”先回一个 `REDY`
- 驱动如果第一次读到 `REDY`，会重试再读一次真正响应
- 如果连续两次都读到 `REDY`，则认为异常并返回错误

这说明设备的协议时序并不完全干净，驱动专门为这种上电阶段行为做了处理。

### 5.3 `appletbdrm_send_msg()`

这是对简单控制消息的封装函数，用来发送：

- `GINF`
- `REDY`
- `CLRD`

它构造一个简单 request，填好消息 ID 和大小后直接发出。

## 6. 显示信息获取

`appletbdrm_get_information()` 负责向设备发送 `GINF`，再读取 `struct appletbdrm_msg_information` 响应。

这个函数会提取：

- `width`
- `height`
- `bits_per_pixel`
- `pixel_format`

然后做两项校验：

- `bits_per_pixel` 必须等于 `24`
- `pixel_format` 必须等于 `APPLETBDRM_PIXEL_FORMAT`

如果设备返回其他像素格式或位深，驱动直接报错退出。这说明它几乎只支持被验证过的那一种 Touch Bar 协议格式，而不是通用显示协议。

## 7. DRM 模型

### 7.1 基本对象

`appletbdrm_setup_mode_config()` 建立了一条最小 DRM 显示管线：

- 一个 primary plane
- 一个 CRTC
- 一个 encoder
- 一个 connector

驱动没有 overlay plane，也没有复杂的 display pipe。实现目标很明确：只需要把一块 framebuffer 显示到 Touch Bar。

### 7.2 framebuffer 格式

primary plane 支持两种格式：

- `DRM_FORMAT_BGR888`
- `DRM_FORMAT_XRGB8888`

其中 `XRGB8888` 是“软件模拟支持”，因为设备真正要求的是 24-bit BGR888。提交时如果发现 framebuffer 是 `XRGB8888`，驱动会在 CPU 上做格式转换。

### 7.3 shadow plane 和 damage clips

这个驱动的 DRM 设计重点在于：

- 使用 `DRM_GEM_SHADOW_PLANE_HELPER_FUNCS`
- 调用 `drm_plane_enable_fb_damage_clips()`

这两点结合起来意味着：

- framebuffer 内容会有一份可供 CPU 处理的 shadow 数据
- atomic commit 时只需要处理发生变化的 damage 矩形

这正好匹配 USB 小屏设备的使用场景，因为整帧反复传输成本更高，增量更新更合适。

## 8. 坐标系转换

驱动中最容易让人困惑的一点是设备坐标系和 framebuffer 坐标系并不一致。

源码里的注释说明了：

- 设备的 x/y 轴与 framebuffer 的 x/y 轴是交换的
- 设备的 y 轴方向还是反向的

因此：

- 设备上报的 `height` 实际成为 DRM framebuffer 的 `width`
- 设备上报的 `width` 实际成为 DRM framebuffer 的 `height`

所以 `appletbdrm_setup_mode_config()` 里 mode 初始化写成了：

- 宽度使用 `adev->height`
- 高度使用 `adev->width`

这不是写反了，而是在适配设备原生坐标系。

## 9. atomic check 阶段做什么

`appletbdrm_primary_plane_helper_atomic_check()` 负责为一次显示更新提前准备传输缓冲区。

主要流程：

1. 使用 `drm_atomic_helper_check_plane_state()` 做基础合法性检查
2. 如果 plane 不可见，直接返回
3. 使用 damage iterator 遍历所有脏矩形
4. 计算所有脏矩形所需的总 `frames_size`
5. 为这次 commit 分配：
   - `request`
   - `response`
6. 记录：
   - `request_size`
   - `frames_size`

这个设计有两个特点：

- 每次 atomic commit 的协议缓冲区都挂在新的 plane state 上
- 真正提交前就把内存准备好，避免 update 阶段再做复杂分配

## 10. framebuffer 刷新路径

真正的数据上传发生在 `appletbdrm_flush_damage()`。

### 10.1 总体流程

函数执行步骤如下：

1. 如果没有 damage，直接返回
2. 调用 `drm_gem_fb_begin_cpu_access()` 开始 CPU 访问 framebuffer
3. 填充 request header
4. 遍历所有 damage 矩形
5. 为每个 damage 构造一个 `struct appletbdrm_frame`
6. 把对应区域的像素数据拷贝到 frame buffer 中
7. 在所有 frame 后面追加 footer
8. footer 中填入 timestamp
9. 通过 USB 发送整个 request
10. 接收设备返回的 `UDCL`
11. 校验响应里的 timestamp 是否与请求一致
12. 调用 `drm_gem_fb_end_cpu_access()` 结束 CPU 访问

### 10.2 单个 damage frame 的含义

每个 `struct appletbdrm_frame` 描述一个待更新矩形：

- `begin_x`
- `begin_y`
- `width`
- `height`
- `buf_size`
- `buf[]`

其中 `buf[]` 保存这个矩形区域的原始像素数据。

### 10.3 坐标转换公式

对于每个 damage rect，驱动使用下面的映射：

- `begin_x = damage.y1`
- `begin_y = adev->height - damage.x2`
- `width = rect_height`
- `height = rect_width`

原因就是前面提到的：

- x/y 轴互换
- y 轴翻转

### 10.4 像素格式转换

如果 framebuffer 是 `DRM_FORMAT_XRGB8888`，驱动调用：

- `drm_fb_xrgb8888_to_bgr888()`

将像素转成设备需要的 `BGR888`。

否则就直接：

- `drm_fb_memcpy()`

把数据拷过去。

因此这个驱动的数据路径是明确的 CPU copy / CPU convert 路线，不依赖 GPU 做 blit 或格式转换。

### 10.5 footer 和时间戳

所有 frame 数据之后会附加一个 `struct appletbdrm_fb_request_footer`。

其中最关键的字段是：

- `timestamp`

设备在返回 `struct appletbdrm_fb_request_response` 时也会附带 timestamp，驱动据此确认响应和本次提交是一一对应的。如果时间戳不匹配，驱动会报错。

这说明该协议至少提供了一个轻量级的请求-应答关联机制。

## 11. atomic update 和 disable

### 11.1 `atomic_update`

`appletbdrm_primary_plane_helper_atomic_update()` 很短，它只做一件事：

- 调用 `appletbdrm_flush_damage()`

真正的数据搬运和 USB 提交都在后者中完成。

### 11.2 `atomic_disable`

`appletbdrm_primary_plane_helper_atomic_disable()` 调用：

- `appletbdrm_clear_display()`

即 plane 被禁用时，直接让 Touch Bar 清屏。这也解释了为什么系统关机时显示内容能被清掉。

## 12. plane state 生命周期

驱动扩展了 plane state，定义了 `struct appletbdrm_plane_state`，在 `drm_shadow_plane_state` 基础上增加：

- `request`
- `response`
- `request_size`
- `frames_size`

生命周期大致如下：

- `reset`：创建新的私有 plane state
- `duplicate_state`：复制 shadow plane state，但不复制 request/response 缓冲
- `atomic_check`：为这次新提交分配 request/response
- `destroy_state`：释放 request/response，并清理 shadow plane state

也就是说，这些 USB 传输缓冲区是一次 atomic state 专属的临时资源，不会跨提交复用。

## 13. disconnect 和 shutdown

### 13.1 `disconnect`

设备拔出时：

- 先 `drm_dev_unplug()`
- 再 `drm_atomic_helper_shutdown()`

这符合 DRM 设备热拔插时的常见处理方式。

### 13.2 `shutdown`

系统关机时只调用：

- `drm_atomic_helper_shutdown()`

源码注释明确指出，Touch Bar 的 framebuffer 内容会跨重启保留，因此需要在关机时确保屏幕被清空。这个清空动作最终通过 shutdown 路径中的 plane disable 回调完成。

## 14. 这个驱动的关键实现特点

可以把这个驱动总结成下面几点：

1. 它是基于 USB bulk 传输的私有显示协议驱动。
2. DRM 侧只实现最小必要的显示对象，没有复杂显示管线。
3. 它依赖 shadow plane，因此本质是 CPU 读写 framebuffer。
4. 它使用 damage-based 更新，只上传变化区域。
5. 它在提交阶段把多个矩形合并进一次 USB request，减少往返次数。
6. 它显式处理了 Touch Bar 特殊的坐标系。
7. 它对协议字段和格式假设较强，明显是面向特定硬件的实现。

## 15. 阅读这个驱动时最值得抓住的主线

如果从实现上抓主线，建议按下面的顺序理解：

1. `appletbdrm_probe()`：看设备是怎么初始化的
2. `appletbdrm_get_information()`：看设备能力如何探测
3. `appletbdrm_setup_mode_config()`：看 DRM 怎么建模
4. `appletbdrm_primary_plane_helper_atomic_check()`：看提交前怎么准备传输缓冲
5. `appletbdrm_flush_damage()`：看脏矩形如何变成 USB 协议包
6. `appletbdrm_read_response()`：看设备应答和异常时序如何处理

真正的核心代码其实就在 `appletbdrm_flush_damage()`，因为这个函数把 DRM damage、格式转换、坐标映射和 USB 协议封装全部串在了一起。

## 16. 一句话总结

`appletbdrm` 的实现本质上是：

“把 DRM 的增量 framebuffer 更新，转换成 Apple Touch Bar 设备能理解的 USB 批量矩形更新协议，并用一个最小 atomic KMS 模型把它接入 Linux DRM 子系统。”
