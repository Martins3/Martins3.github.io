## mesa 的驱动真的有趣的

https://docs.mesa3d.org/drivers/llvmpipe.html


https://learnopengl.com/
https://nehe.gamedev.net/
https://docs.mesa3d.org/


### 问 : 为什么 src/virtio 有 10w 行的代码
 因为 src/virtio 在 Mesa 里不是“几个 virtio 帮助文件”，而是一个完整的
  虚拟 GPU 子系统目录。

  最主要的一块是 venus Vulkan 驱动。src/meson.build 明确把它当成独立模
  块编进来：src/meson.build:71。而 src/virtio/vulkan/meson.build:56 里
  可以看到它直接构建 vulkan_virtio 这个 ICD，并且源码按 Vulkan 对象拆
  成很多文件：instance/device/queue/image/pipeline/descriptor/
  command_buffer/...。这不是简单封装，而是一整套 Vulkan userspace
  driver，所以文件数本来就会接近其他 Vulkan 驱动。

  其次，venus 还要自己处理“把 Vulkan 调用发到宿主机”这套传输机制，不只
  是转发 API。比如：

  - 命令序列化在 src/virtio/vulkan/vn_cs.h:43
  - ring buffer 传输在 src/virtio/vulkan/vn_ring.h:13
  - renderer/BO/sync 抽象在 src/virtio/vulkan/vn_renderer.h:31
  - 它还分真实 virtgpu 后端和 vtest 测试后端，见 src/virtio/vulkan/
    meson.build:110

  再者，src/virtio 里还有很多“公共协议和复用层”，不只给 venus 用。比如
  src/virtio/vdrm/vdrm.h:6 的注释就写了，它专门抽象 vtest 和真实
  virtgpu 的差异；而 src/virtio/venus-protocol/vn_protocol_driver.h:1
  还是生成出来的一整套协议头。virtio-gpu/drm_hw.h 这类头也被 AMD、
  Asahi、Freedreno、Intel/Gallium 侧复用。

  所以你看到“源码很多”，根本原因是：

  - src/virtio/vulkan 本身就是完整 Vulkan 驱动，不是小插件
  - 它还包含传输层、同步、内存、WSI、测试后端
  - src/virtio 还顺手放了公共协议头、vdrm 适配层、Rust 协议代码和 CI
    配置

### 问，venus 是什么东西

• venus 可以直接理解成：Mesa 里的 virtio Vulkan 虚拟化驱动。

  它的作用不是自己真正驱动一块本地 GPU，而是把 guest 里的 Vulkan API 调用编码成一套 venus 协议命令，发给宿主机上的 renderer 去执行，然后再把结果返回给 guest。
  代码里很直白：

  - 它被构建成 Vulkan ICD vulkan_virtio，见 src/virtio/vulkan/meson.build:18 和 src/virtio/vulkan/meson.build:139
  - 初始化时会先连接 renderer，并检查 wire format、vk xml version、VK_MESA_venus_protocol 版本，见 src/virtio/vulkan/vn_instance.c:169
  - 它通过 ring 传输“plain venus commands”，见 src/virtio/vulkan/vn_ring.h:13

  更准确地说，venus 处在这条链上：

  Vulkan 应用
  -> guest 里的 vulkan_virtio / venus
  -> 把 Vulkan 参数序列化成 venus protocol
  -> 通过 virtio-gpu / 共享内存 / ring 送到宿主机 renderer
  -> 宿主机真正的 Vulkan 驱动或 GPU 执行

  所以它解决的问题是：让虚拟机里的应用能用 Vulkan，而且尽量保留 Vulkan 对象模型和语义，而不是把 Vulkan 再翻译成另一套 3D API。

  这也是它和 virgl 最容易混淆、但本质不同的地方：

  - virgl 更像是 Gallium/OpenGL 那套虚拟 3D 协议
  - venus 是 Vulkan 专用协议，按 Vulkan 的对象、命令、同步、内存模型来传

  从这个角度看，venus-protocol 那些自动生成头文件，其实就是“Vulkan 参数 <-> 协议字节流”的编解码器，不是业务逻辑本身。

  再压缩成一句话：

  `venus = guest 侧 Vulkan front-end + virtio 上的 Vulkan 协议层；真正渲染通常在 host 侧完成。

  如果你愿意，我下一步可以继续讲这三个词怎么区分最不容易混：

  - virtio-gpu
  - virgl
  - venus


### 问 : 似乎这里的词汇有点


    amd
    android_stub
    asahi
    broadcom
    c11
    compiler
    drm-shim
    egl
    etnaviv
    freedreno
    gallium
    gbm
    getopt
    gfxstream
    glx
    gtest
    imagination
    imgui
    intel
    kosmickrisp
    loader
    mesa
    microsoft
    nouveau
    panfrost
    poly
    tool
    util
    virtio
    vulkan
    x11

那么这里的 glx 和 egl 都是什么意思?



## 在终端中启动 kitty 的时候，可以看到这个错误
kitty
MESA: warning: Support for this platform is experimental with Xe KMD, bug reports may be ignored.
[0.100] Application escape mode is not supported, the extended keyboard protocol should be used instead
[0.100] Application escape mode is not supported, the extended keyboard protocol should be used instead

### 如何理解 alacritty 所说的 GPU 加速是什么意思?

## mesa 是 linux 特有的，那么 windows 上用的叫什么?
https://github.com/pal1000/mesa-dist-win
