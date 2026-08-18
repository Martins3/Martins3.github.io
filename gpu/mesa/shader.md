## shader 的基本工作原理是什么?
<!-- 540ff60b-cf7c-4b36-8dba-57b7dca9e91d -->

1. Shader 的工作是什么？
Shader 是一段运行在 GPU 上的小程序，负责把输入数据（顶点、纹理、 uniform 等）转换成最终的像素颜色。以这个项目里的 09_shader_base 为例：

**Vertex Shader（顶点着色器）— 处理顶点**
```txt
// 09_shader_base.vert
void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    fragColor = colors[gl_VertexIndex];
}
```
它的工作：
• 接收 3 个顶点索引（gl_VertexIndex：0, 1, 2）
• 查表得到每个顶点的位置（positions）和颜色（colors）
• 输出裁剪空间位置（gl_Position）— GPU 靠这个知道三角形画在哪里
• 输出颜色（fragColor）— 传给片段着色器

**Fragment Shader（片段着色器）— 处理像素**
```txt
// 09_shader_base.frag
void main() {
    outColor = vec4(fragColor, 1.0);
}
```
它的工作：
• 对三角形覆盖的每一个像素执行
• 接收顶点着色器插值后的颜色
• 输出最终像素颜色（outColor）到屏幕

也就是画一个三角形，vertex shader 只跑 3 次（每个顶点一次），而 fragment shader 可能跑 成千上万次（覆盖多少像素就跑多少次）。

───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
2. 为什么需要着色器？
因为 GPU 的渲染管线是高度并行、固定流水线的，但具体"算什么"必须由开发者定义。
没有着色器的话，GPU 就不知道：
• 顶点在哪里
• 三角形怎么着色
• 光照怎么计算
• 纹理怎么采样
着色器让开发者可以编程控制 GPU 渲染管线的关键阶段。Vulkan/OpenGL/DirectX 等 API 的渲染流程本质上就是：
CPU 准备数据 → 发给 GPU → GPU 按 Vertex Shader → 图元装配 → Fragment Shader → 帧缓冲 执行

3. 为什么需要专门的语言？

因为 GPU 的编程模型和 CPU 完全不同。
 CPU 代码                GPU Shader
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 串行执行                大规模并行（ thousands of threads 同时跑）
 可以随意读写内存        输入/输出严格受限，按管线阶段定义
 有复杂的控制流          尽量避免分支，偏好 SIMD 风格
 C/C++/Rust 等通用语言   需要表达向量运算（vec3, vec4, mat4）

看这个项目的对比：
GLSL（OpenGL/Vulkan 原生）：
```txt
vec4(positions[gl_VertexIndex], 0.0, 1.0)
```

Slang（这个项目用的现代着色器语言）：
```txt
[shader("vertex")]
VertexOutput vertMain(uint vid : SV_VertexID) { ... }
```
它们都有 CPU 语言不具备的特性：
• 内置向量/矩阵类型（vec3, mat4, float4）
• 内置插值语义（SV_Position, SV_Target, location = 0）
• 显式的管线阶段标记（[shader("vertex")]）
如果用 C++ 写这些，语法会很笨拙；而着色器语言让你用数学公式般的简洁表达来描述光照、变换、纹理采样。

───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
4. 为什么需要编译着色器？
看 15_hello_triangle.cpp 里的这行代码：
vk::raii::ShaderModule shaderModule = createShaderModule(readFile("shaders/slang.spv"));
GPU 不直接执行 GLSL/Slang/HLSL 源码，而是执行 SPIR-V（.spv）这种中间二进制格式。
需要编译的原因：
1. GPU 架构差异巨大
   NVIDIA、AMD、Intel、ARM Mali 的指令集完全不同。SPIR-V 是一种与硬件无关的中间表示，驱动再把它翻译成各自 GPU 能理解的机器码。
2. 运行时编译太慢
   如果把 GLSL 源码直接在程序启动时编译，会显著拖慢启动速度。预编译成 .spv 后，运行时只需加载二进制文件：
   ```txt
   std::vector<char> buffer(file.tellg());
   file.read(buffer.data(), buffer.size());
   vk::ShaderModuleCreateInfo createInfo{.codeSize = code.size(), .pCode = reinterpret_cast<const uint32_t *>(code.data())};
   ```
3. 提前发现错误
   编译阶段就能捕获语法错误、类型不匹配、接口不兼容等问题，而不是等到运行时 GPU 报错。
4. 跨 API 兼容
   SPIR-V 是 Khronos 标准，Vulkan 和 OpenCL 都使用它。你用 Slang/HLSL/GLSL 写的源码，最终都可以编译到同一个 SPIR-V，在不同 API 间复。

## 那么 shader 是什么时候编译的

```txt
   驱动的两个阶段的实际工作
 阶段     Vulkan API                  驱动做了什么
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 阶段一   vkCreateShaderModule        接收 SPIR-V 二进制，做语法验证、解析成内部 IR（中间表示），通常不做 GPU 机器码生成
 阶段二   vkCreateGraphicsPipelines   拿到完整的管线状态（顶点布局、渲染目标格式、混合模式、深度状态等），将 SPIR-V 编译成 GPU 特定的机器码
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
为什么不在 vkCreateShaderModule 时就编译？
因为 GPU 机器码的生成高度依赖管线状态（Pipeline State）。单独一个 SPIR-V 着色器不足以生成最优代码，驱动需要知道：
1. 顶点输入格式（Vertex Input Layout）— 决定 shader 里怎么读取顶点属性
2. 渲染目标格式（Render Target Format）— 决定 fragment shader 的 SV_Target 输出格式
3. Depth/Stencil 状态 — 决定 Early-Z 测试能否优化掉一些像素
4. Blend 状态 — 决定输出颜色后是否还要做混合
5. Rasterization 状态 — 决定是否做 Flat Shading、Cull 哪个面
这些状态全在 vk::GraphicsPipelineCreateInfo 里提供：
vk::GraphicsPipelineCreateInfo{
    .pVertexInputState   = &vertexInputInfo,
    .pInputAssemblyState = &inputAssembly,
    .pViewportState      = &viewportState,
    .pRasterizationState = &rasterizer,
    .pMultisampleState   = &multisampling,
    .pColorBlendState    = &colorBlending,
    .pDynamicState       = &dynamicState,
    .layout              = pipelineLayout,
    .stageCount          = 2,
    .pStages             = shaderStages  // ← 这里引用了 SPIR-V
}
驱动拿到这个完整的 CreateInfo 后，才能把 SPIR-V 编译成针对当前 GPU 的最优机器码。
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
可以验证的现象
1. Pipeline Creation 耗时明显更长
   vkCreateGraphicsPipelines 往往比 vkCreateShaderModule 慢几个数量级，因为这才是真正的编译+链接过程。
2. Pipeline Cache 的存在
   Vulkan 提供了 vkGetPipelineCacheData / vkCreatePipelineCache，可以把编译好的机器码缓存到磁盘。下次启动直接加载，跳过编译。如果编
   是在 vkCreateShaderModule 时完成的，就不需要这个专门的 Cache 机制了。
3. 不同管线可以复用同一个 Shader Module，但机器码不同
   同一个 vk::ShaderModule 可以被多个不同配置的 vk::Pipeline 引用，每个管线生成的 GPU 机器码可能不同（因为状态不同导致优化策略不同
───────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
例外情况
有些驱动会在 vkCreateShaderModule 时做部分编译（例如把 SPIR-V 转成驱动内部的通用 IR），但真正的后端代码生成（
Backend Code Generation）和硬件指令调度几乎一定延迟到 vkCreateGraphicsPipelines 才做。

所以简单来说：vkCreateShaderModule 是"加载和验证"，``vkCreateGraphicsPipelines` 才是"真正编译成 GPU 机器码"。
```

## pipeline 编译缓存
<!-- a0d0f1d4-5d07-4129-8c76-aa6be9373fed -->

```txt
• 是的，如果不做任何处理，每次运行程序时 vkCreateGraphicsPipelines 都会重新编译。但生产环境绝不会这样干，业界有成熟方案来解决这个问题。
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
1. Pipeline Cache（管线缓存）— Vulkan 原生机制
Vulkan 专门提供了 VkPipelineCache 来解决这个问题。核心思想：
第一次运行：SPIR-V → 驱动编译 → 机器码 → 存入 Cache → 导出保存到磁盘
下次运行：从磁盘加载 Cache → vkCreateGraphicsPipelines 复用 → 几乎瞬间完成
基本用法
// 启动时：尝试从磁盘加载之前保存的 cache 数据
std::vector<char> cacheData = readFileFromDisk("pipeline_cache.bin");

VkPipelineCacheCreateInfo cacheInfo = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
    .initialDataSize = cacheData.size(),
    .pInitialData = cacheData.data()
};
VkPipelineCache cache;
vkCreatePipelineCache(device, &cacheInfo, nullptr, &cache);

// 创建管线时使用 cache
vkCreateGraphicsPipelines(device, cache, 1, &pipelineInfo, nullptr, &pipeline);

// 程序退出前：把 cache 数据保存回磁盘
size_t cacheSize;
vkGetPipelineCacheData(device, cache, &cacheSize, nullptr);
std::vector<char> newCacheData(cacheSize);
vkGetPipelineCacheData(device, cache, &cacheSize, newCacheData.data());
writeFileToDisk("pipeline_cache.bin", newCacheData);
效果：第二次启动时，如果 GPU 驱动版本没变，创建管线的耗时会从数百毫秒降到几毫秒。
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
2. 驱动自身的内部缓存
即使你的程序没有用 VkPipelineCache，现代 GPU 驱动通常也会在系统层面缓存编译结果：
• NVIDIA：~/.cache/nvidia/ 或 Windows 驱动缓存
• AMD：AMDShaderCache 等机制
• Intel：内部也有类似机制
但注意：驱动缓存是全局的、不保证长期保留的。如果用户清空了缓存、更新了驱动、或者换了 GPU，缓存就失效了。因此不能依赖它。
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
3. 预热管线（Pipeline Warmup / 预编译）
一些大型游戏会采用更激进的策略：
 策略           做法
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 启动时预编译   游戏启动画面期间，后台创建所有可能用到的管线，同时显示"正在编译着色器"
 分帧编译       帧率允许时，每帧创建几个新管线，避免一次性卡顿
 PSO Caching    提前在开发机上生成并随游戏分发的 Cache（如 UE5 的 *.upipelinecache）
这就是为什么你玩大型游戏时，偶尔会遇到：
• 第一次启动或更新驱动后，出现"正在优化着色器"的提示
• 某些新场景第一次进入时会轻微卡顿（新管线的首次编译）
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
4. 一个具体例子
假设你的 15_hello_triangle.cpp 中只创建 1 个管线：
graphicsPipeline = vk::raii::Pipeline(device, nullptr, pipelineCreateInfo);
• 第一次运行：驱动编译 SPIR-V → 机器码，耗时约 1~10ms（足够快，几乎无感）
• 第二次运行（有 Pipeline Cache）：驱动发现 cache 命中，耗时约 0.1ms
但如果是复杂游戏，可能有 数千个管线（不同材质、不同顶点格式、不同渲染状态组合），总编译时间可能达到 数秒。没有缓存就不可接受了。
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
一句话总结

是的，不做优化的话每次都要编译；但 Vulkan 提供了 VkPipelineCache 机制，可以把编译结果持久化到磁盘。实际项目几乎都会用它，再配合预编译策略，来避免运行时卡顿。
```

## 编译 shader 的驱动在那里的


```txt
• 找到了，这台机器上装了不少 Vulkan 驱动。结果如下：
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
NVIDIA 私有驱动
ICD 配置文件：/usr/share/vulkan/icd.d/nvidia_icd.x86_64.json
{
    "ICD": {
        "library_path": "/usr/lib64/libGLX_nvidia.so.0",
        "api_version" : "1.4.312"
    }
}
实际的 so：/usr/lib64/libGLX_nvidia.so.0 → /usr/lib64/libGLX_nvidia.so.580.142
NVIDIA 比较特殊，它的 Vulkan 入口不是单独的 libnvidia-glvkspirv.so，而是打包在 libGLX_nvidia.so 里（同时包含 OpenGL 和 Vulkan 实现）。驱动版本 5
80.142，支持 Vulkan 1.4.312。
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Mesa 开源驱动（一堆）
ICD 配置文件目录：/usr/share/vulkan/icd.d/
 GPU 厂商                ICD JSON                    实际 so 文件                        API 版本
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
 Intel                   intel_icd.x86_64.json       /usr/lib64/libvulkan_intel.so       1.4.311
 AMD/ATI                 radeon_icd.x86_64.json      /usr/lib64/libvulkan_radeon.so      1.4.311
 Apple Silicon           asahi_icd.x86_64.json       /usr/lib64/libvulkan_asahi.so       -
 Broadcom (树莓派)       broadcom_icd.x86_64.json    /usr/lib64/libvulkan_broadcom.so    -
 Qualcomm                freedreno_icd.x86_64.json   /usr/lib64/libvulkan_freedreno.so   -
 NVIDIA (开源 nouveau)   nouveau_icd.x86_64.json     /usr/lib64/libvulkan_nouveau.so     -
 ARM Mali                panfrost_icd.x86_64.json    /usr/lib64/libvulkan_panfrost.so    -
 软渲染 (LLVMpipe)       lvp_icd.x86_64.json         /usr/lib64/libvulkan_lvp.so         -
 虚拟 GPU                virtio_icd.x86_64.json      /usr/lib64/libvulkan_virtio.so      -
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
Vulkan Loader（分发层）
/usr/lib64/libvulkan.so -> libvulkan.so.1 -> libvulkan.so.1.4.313
这是 Khronos 官方的 Vulkan Loader，你的程序链接的就是它。它不负责编译，只负责：
1. 读取 /usr/share/vulkan/icd.d/*.json
2. 根据当前使用的 GPU，把 vkCreateGraphicsPipelines 等调用转发给对应的 ICD 驱动
3. 加载验证层、扩展等
────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────────
总结这张图
你的程序 (15_hello_triangle.cpp)
    ↓ 链接
libvulkan.so.1.4.313  (Khronos Loader, ~524KB)
    ↓ 读取 ICD 配置
    ├─→ libGLX_nvidia.so.580.142  (NVIDIA 私有驱动, Vulkan 1.4.312)
    ├─→ libvulkan_intel.so        (Mesa Intel 驱动, Vulkan 1.4.311)
    ├─→ libvulkan_radeon.so       (Mesa AMD 驱动, Vulkan 1.4.311)
    └─→ libvulkan_*.so            (其他 Mesa 驱动...)

真正执行 SPIR-V → GPU 机器码编译的，就是这些 libGLX_nvidia.so 或 libvulkan_*.so，全在用户态。
```
补充说明: "ICD"（Installable Client Driver）这个概念 OpenGL/OpenCL 也有，但 Vulkan 的 JSON 配置文件机制（/usr/share/vulkan/icd.d/*.json）是它独有的标准化设计，目的是让多厂商驱动共存、验证层可插拔、行为完全显式可控。
