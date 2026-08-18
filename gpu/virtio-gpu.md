# virtio-gpu

内核代码统计
```txt
 ./virtgpu_vq.c                                                                                                                    1304
 ./virtgpu_ioctl.c                                                                                                                  735
 ./virtgpu_submit.c                                                                                                                 542
 ./virtgpu_plane.c                                                                                                                  408
 ./virtgpu_display.c                                                                                                                373
 ./virtgpu_kms.c                                                                                                                    353
 ./virtgpu_gem.c                                                                                                                    296
 ./virtgpu_object.c                                                                                                                 249
 ./virtgpu_vram.c                                                                                                                   228
 ./virtgpu_drv.c                                                                                                                    208
 ./virtgpu_fence.c                                                                                                                  175
 ./virtgpu_prime.c                                                                                                                  170
 ./virtgpu_debugfs.c                                                                                                                111
 ./virtgpu_trace_points.c                                                                                                             5
```

QEMU 的代码 : 大约 2000 行 hw/display/virtio-gpu.c

## 将 virtio-gpu 中配置修改为
code/build/martins3.graphics.config

## mesa 似乎存在类似的支持
https://docs.mesa3d.org/drivers/virgl

## 如何启用 virtio-gpu
```txt
00:02.0 VGA compatible controller: Device 1234:1111 (rev 02)
00:10.0 Display controller: Red Hat, Inc. Virtio 1.0 GPU (rev 01)
```

## 如果在 kernel 中直接将 drm disable 掉，那么 vnc 中输出为 serial 或者 gpu 有区别吗?

### arm 中为什么 serial 输出最后也是可以走 vnc 的

### arm 中 serial mode 的键盘操作是怎么走的 和 gpu mode + usb 的键盘是如何对比的

## 想不到 mesa 中也有对于 virtio-gpu 的支持


这两个函数似乎只有有了驱动才去调用的
```txt
[martins3:virtio_vga_base_update_display:26]
[martins3:vga_update_display:1787]
```
这么思考，其实问题是


## make menuconfig 发现了这个
Cirrus driver for QEMU emulated device


## virtio native context

https://www.reddit.com/r/linux/comments/1i2wpb2/amdgpu_virtio_native_context_merged_native_amd/

https://www.phoronix.com/news/DRM-Native-Contexts-FPS-VM

这个不错了:
https://www.linaro.org/blog/a-closer-look-at-virtio-and-gpu-virtualisation/

想不到 mesa 源码中也有 virtio

> [!NOTE]
> 参考 Deepseeek ，有待验证

Mesa 是一个用户态的开源图形库实现，主要提供：
OpenGL、OpenGL ES、Vulkan、EGL 等 API 的实现
着色器编译（GLSL → NIR → 特定 GPU 指令）
状态跟踪、命令缓冲、资源管理
与窗口系统（X11/Wayland/Android）集成

与 Mesa 配合工作的内核部分是：
DRM (Direct Rendering Manager) + KMS (Kernel Mode Setting)

## 先看这个
https://www.qemu.org/docs/master/interop/vhost-user-gpu.html

## 架构位置

基本结构:
```
虚拟机 (Guest)
├── 应用 (OpenGL/Vulkan/游戏)
├── Mesa 驱动 (用户态)
├── VirtIO GPU 内核驱动 (virtio-gpu.ko)
└── VirtQueue (控制/光标/显示队列)
         ↓ 虚拟化通道
主机 (Host)
├── QEMU (virtio-gpu 设备实现)
├── VirGLrenderer (3D 渲染翻译)
└── 主机 GPU 驱动 (i915/amdgpu/nvidia)
```


## VirtIO GPU 对外提供的功能

### 1. 2D 图形功能（基础）

| 命令 | 功能 | 说明 |
|------|------|------|
| `VIRTIO_GPU_CMD_GET_DISPLAY_INFO` | 获取显示信息 | 查询显示器数量、分辨率 |
| `VIRTIO_GPU_CMD_RESOURCE_CREATE_2D` | 创建 2D 资源 | 创建纹理/缓冲区 |
| `VIRTIO_GPU_CMD_RESOURCE_UNREF` | 释放资源 | 销毁资源 |
| `VIRTIO_GPU_CMD_SET_SCANOUT` | 设置扫描输出 | 将资源绑定到显示器 |
| `VIRTIO_GPU_CMD_RESOURCE_FLUSH` | 刷新资源 | 更新显示内容 |
| `VIRTIO_GPU_CMD_TRANSFER_TO_HOST_2D` | 2D 传输到主机 | 上传像素数据 |
| `VIRTIO_GPU_CMD_RESOURCE_ATTACH_BACKING` | 附加后端存储 | 关联内存 |
| `VIRTIO_GPU_CMD_RESOURCE_DETACH_BACKING` | 分离后端存储 | 解除内存关联 |
| `VIRTIO_GPU_CMD_GET_EDID` | 获取 EDID | 查询显示器信息 |

### 2. 3D 图形功能（通过 VirGL）

| 命令 | 功能 | 说明 |
|------|------|------|
| `VIRTIO_GPU_CMD_CTX_CREATE` | 创建 3D 上下文 | 初始化 VirGL 上下文 |
| `VIRTIO_GPU_CMD_CTX_DESTROY` | 销毁上下文 | 清理资源 |
| `VIRTIO_GPU_CMD_CTX_ATTACH_RESOURCE` | 附加资源到上下文 | 绑定纹理/缓冲区 |
| `VIRTIO_GPU_CMD_CTX_DETACH_RESOURCE` | 分离资源 | 解绑 |
| `VIRTIO_GPU_CMD_RESOURCE_CREATE_3D` | 创建 3D 资源 | 创建 3D 纹理/缓冲区 |
| `VIRTIO_GPU_CMD_TRANSFER_TO_HOST_3D` | 3D 传输到主机 | 上传 3D 数据 |
| `VIRTIO_GPU_CMD_TRANSFER_FROM_HOST_3D` | 从主机传输 | 下载 3D 数据 |
| `VIRTIO_GPU_CMD_SUBMIT_3D` | 提交 3D 命令 | 提交渲染命令 |

### 3. 光标功能

| 命令 | 功能 | 说明 |
|------|------|------|
| `VIRTIO_GPU_CMD_UPDATE_CURSOR` | 更新光标 | 设置光标图像 |
| `VIRTIO_GPU_CMD_MOVE_CURSOR` | 移动光标 | 设置光标位置 |

### 4. Blob 资源功能（新特性）

| 命令 | 功能 | 说明 |
|------|------|------|
| `VIRTIO_GPU_CMD_RESOURCE_CREATE_BLOB` | 创建 Blob 资源 | 创建共享内存资源 |
| `VIRTIO_GPU_CMD_SET_SCANOUT_BLOB` | 设置 Blob 扫描输出 | 显示 Blob 资源 |
| `VIRTIO_GPU_CMD_RESOURCE_ASSIGN_UUID` | 分配 UUID | 跨设备共享 |
| `VIRTIO_GPU_CMD_RESOURCE_MAP_BLOB` | 映射 Blob | CPU 可访问 |
| `VIRTIO_GPU_CMD_RESOURCE_UNMAP_BLOB` | 解映射 Blob | 释放 CPU 访问 |

### 5. Capset 功能（能力查询）

| 命令 | 功能 | 说明 |
|------|------|------|
| `VIRTIO_GPU_CMD_GET_CAPSET_INFO` | 获取能力集信息 | 查询支持的能力 |
| `VIRTIO_GPU_CMD_GET_CAPSET` | 获取能力集 | 获取具体能力数据 |


**路径**: `~/data/kernel/linux-hyperv/drivers/gpu/drm/virtio/`

### 文件结构

| 文件 | 功能 | 代码行数 |
|------|------|----------|
| `virtgpu_drv.c` | 驱动入口、初始化 | ~300 |
| `virtgpu_ioctl.c` | ioctl 命令处理 | ~800 |
| `virtgpu_vq.c` | VirtQueue 操作 | ~1400 |
| `virtgpu_display.c` | 显示/KMS 实现 | ~400 |
| `virtgpu_plane.c` | 平面/图层管理 | ~500 |
| `virtgpu_gem.c` | GEM 内存管理 | ~300 |
| `virtgpu_submit.c` | 命令提交 | ~400 |
| `virtgpu_fence.c` | Fence 同步 | ~200 |
| `virtgpu_debugfs.c` | DebugFS 接口 | ~300 |

### 核心 ioctl 实现

QEMU 实现分析 `~/data/qemu/hw/display/`

### 文件结构

| 文件 | 功能 | 说明 |
|------|------|------|
| `virtio-gpu.c` | 基础 VirtIO GPU 实现 | 2D 显示、资源管理 |
| `virtio-gpu-base.c` | 基类实现 | 公共功能 |
| `virtio-gpu-virgl.c` | VirGL 3D 支持 | 3D 命令处理 |
| `virtio-gpu-gl.c` | OpenGL 后端 | 渲染实现 |
| `virtio-gpu-rutabaga.c` | Rutabaga 支持 | 替代 VirGL 的方案 |
| `virtio-gpu-pci.c` | PCI 设备包装 | PCI 接口 |
