# Intel Xe GPU 测试 (SR-IOV VF 模式)

本目录包含在 KVM 虚拟机中测试 Intel Xe GPU (SR-IOV VF 模式) 的各种程序。

## 环境信息

- **GPU**: Intel UHD Graphics 770 (Raptor Lake-S GT1)
- **设备 ID**: 0x8086:0xA780
- **驱动**: xe 1.1.0
- **模式**: SR-IOV VF (虚拟功能)
- **显存**: 约 7.6 GB (共享内存)
- **渲染节点**: /dev/dri/renderD129

## 快速开始

### 一键测试

```bash
# 进入环境并运行所有测试
nix-shell --run "make test"
```

### 分步使用

```bash
# 进入开发环境
nix-shell

# 编译
make              # 只编译 Vulkan 测试
make all          # 编译所有测试

# 运行
make run          # 运行 Vulkan 测试 (64MB)
make run SIZE=128 # 运行 Vulkan 测试 (指定大小)
make test         # 运行所有测试程序

# 清理
make clean
```

## 文件说明

### 核心文件
| 文件 | 说明 |
|------|------|
| `Makefile` | 编译管理 |
| `shell.nix` | Nix 开发环境 |
| `run_gpu_test.sh` | 一键运行脚本 |
| `README.md` | 本文档 |

### 源代码
| 文件 | 说明 | 状态 |
|------|------|------|
| `vulkan_mem_test.cpp` | Vulkan 显存分配测试 | [OK] |
| `vulkan_compute_test.cpp` | Vulkan 计算着色器框架 | [OK] |
| `gpu_test.c` | EGL/GLES 显存测试 | [NEED CONFIG] |
| `cl_test.c` | OpenCL 测试 | [FAIL] VF 不支持 |
| `drm_gpu_test.c` | DRM 直接测试 | [FAIL] VF 不支持 |

## 编译方法

### 使用 Makefile

```bash
# 进入 nix-shell 环境
nix-shell

# 编译所有程序
make all

# 只编译 Vulkan 测试 (推荐)
make vulkan_mem_test

# 编译特定测试
make gpu_test      # EGL/GLES
make cl_test       # OpenCL
make drm_gpu_test  # DRM

# 清理
make clean
```

### 手动编译

```bash
nix-shell -p vulkan-loader vulkan-headers --run 'make vulkan_mem_test'
```

## 测试结果 (实际测试)

运行 `make test` 测试结果：

| 测试 | 状态 | 说明 |
|------|------|------|
| Vulkan 显存 | [OK] | 显存分配成功，约 5.7GB 可用 |
| Vulkan 计算 | [OK] | 计算着色器框架工作正常 |
| EGL/GLES | [FAIL] | 获取 EGL 显示失败，需要图形环境 |
| OpenCL | [FAIL] | 初始化超时，SR-IOV VF 不支持 |
| DRM | [FAIL] | dumb buffer 权限被拒绝，SR-IOV VF 不支持 |
| **DMA-BUF** | **[OK]** | **通过 Vulkan 导出 DMA-BUF fd 成功** |

### 结论
- **推荐**: Vulkan 计算、DMA-BUF 共享 (工作正常)
- **不推荐**: OpenCL, DRM (SR-IOV VF 模式不支持)
- **有条件**: EGL/OpenGL (需要 X11/Wayland)

## DMA-BUF 支持 (重要发现)

Intel Xe GPU 在 SR-IOV VF 模式下**支持 DMA-BUF**！

### 测试方法
```bash
# 使用 Vulkan DMA-BUF 测试
./vulkan_dmabuf_test.out
```

### 测试结果
```
[OK] DMA-BUF fd 导出成功: 13
[OK] mmap 成功, addr: 0x7f43905fc000
[OK] CPU 读写测试通过
```

### DMA-BUF 工作原理
```
Vulkan 分配设备内存
  -> vkGetMemoryFdKHR() 
    -> 导出 DMA-BUF fd
      -> mmap() 映射到 CPU 地址空间
        -> 零拷贝共享给其他设备
```

### 应用场景
1. **视频编码**: GPU 渲染画面 -> DMA-BUF -> VAAPI 编码器
2. **显示输出**: GPU 渲染 -> DMA-BUF -> DRM/KMS 显示
3. **AI 推理**: 模型输入 -> DMA-BUF -> NPU/GPU 推理
4. **容器共享**: 跨容器/跨虚拟机共享显存

### 关键扩展
- `VK_EXT_external_memory_dma_buf`: 启用 DMA-BUF 支持
- `VK_KHR_external_memory_fd`: fd 句柄导出
- `DRM_IOCTL_PRIME_HANDLE_TO_FD`: DRM 级别的 DMA-BUF 导出

## AI 推理方案

Intel Xe GPU 可以用于 AI 推理：

### 1. OpenVINO (推荐)
```bash
nix-shell -p openvino --run "your_inference_app"
```

### 2. Vulkan 计算着色器
使用 `vulkan_compute_test.cpp` 作为框架，编写 SPIR-V 计算着色器。

### 3. oneAPI/DPC++
```bash
nix-shell -p intel-oneapi-dpcpp-cpp
```

## 监控 GPU

```bash
# 查看 GPU 信息
cat /sys/class/drm/card1/device/uevent

# 查看驱动版本
dmesg | grep xe | head -5

# 实时监控 (在 nix-shell 中)
sudo intel_gpu_top
```

## 注意事项

1. **SR-IOV VF 模式限制**: OpenCL 和部分 DRM 功能不可用
2. **ICD 路径**: 已由 shell.nix 自动设置 `VK_ICD_FILENAMES`
3. **警告信息**: "Support for this platform is experimental with Xe KMD" 是正常的

## 故障排除

### "没有发现渲染设备"
```bash
ls -la /dev/dri/renderD*
# 如果没有，检查 dmesg 看驱动是否加载
dmesg | grep xe
```

### "Vulkan Error: 创建实例失败"
```bash
# 检查 ICD 路径
echo $VK_ICD_FILENAMES
# 应该是: /nix/store/...-mesa-.../share/vulkan/icd.d/intel_icd.x86_64.json
```

### "显存分配失败"
SR-IOV VF 的显存是共享的，尝试减小分配大小：
```bash
./vulkan_mem_test 32  # 减小到 32MB
```

## 参考资料

- [Intel Xe 驱动文档](https://www.kernel.org/doc/html/latest/gpu/xe.html)
- [SR-IOV 简介](https://www.intel.com/content/www/us/en/developer/articles/technical/single-root-io-virtualization-sr-iov-with-linux-containers.html)
- [Vulkan 计算着色器教程](https://vulkan-tutorial.com/)
