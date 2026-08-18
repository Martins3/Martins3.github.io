# Video Decode 引擎分析

## 问题：为什么之前的程序不触发 Decode 引擎？

### 1. GPU 视频架构

现代 GPU（以 NVIDIA 为例）有**三个独立的视频硬件单元**：

```
┌─────────────────────────────────────────────────────────────┐
│                     NVIDIA GPU 视频管线                      │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌──────────────┐    ┌──────────────┐    ┌──────────────┐  │
│  │    NVDEC     │───>│     NPP      │───>│   显存输出   │  │
│  │  (Decode)    │    │  (Process)   │    │              │  │
│  │              │    │              │    │              │  │
│  │ • H264 解码  │    │ • 缩放       │    │ • NV12       │  │
│  │ • HEVC 解码  │    │ • 去隔行     │    │ • RGBA       │  │
│  │ • AV1 解码   │    │ • 颜色转换   │    │              │  │
│  │ • VP9 解码   │    │              │    │              │  │
│  └──────────────┘    └──────────────┘    └──────────────┘  │
│          │                    │                            │
│          │                    │                            │
│          ▼                    ▼                            │
│    Task Manager:         Task Manager:                     │
│    "Video Decode"        "Video Processing"                │
│    nvidia-smi:           (或合并到 3D)                     │
│    "utilization.decoder"                                   │
│                                                             │
└─────────────────────────────────────────────────────────────┘
```

### 2. 关键区别

| 特性 | Video Decoder (NVDEC) | Video Processor (NPP) |
|-----|----------------------|----------------------|
| **输入** | 压缩视频数据 (H264 NAL) | 未压缩帧 (YUV/RGB) |
| **输出** | 未压缩帧 (NV12) | 处理后的帧 |
| **D3D11 API** | `ID3D11VideoDecoder` | `ID3D11VideoProcessor` |
| **主要方法** | `DecodeFrame()` | `VideoProcessorBlt()` |
| **专用硬件** | ✅ 是 (NVDEC) | ✅ 是 (NPP) |
| **任务管理器** | Video Decode | Video Processing |

### 3. 为什么 Video Processor 不触发 Decode 引擎？

**Video Processor** 只处理**已经解码**的帧：

```c
// 这是 Video Processor 的工作流程
// 输入已经是未压缩的纹理
D3D11_VIDEO_PROCESSOR_STREAM stream = {
    .pInputSurface = inputView,  // ← 已经是解码后的帧 (NV12/YUV)
    ...
};

// 这个操作只使用 NPP 引擎，不涉及 NVDEC
videoContext->VideoProcessorBlt(processor, outputView, 0, 1, &stream);
```

要触发 **Decode 引擎**，必须提供**压缩数据**：

```c
// 真正的解码流程
D3D11_VIDEO_DECODER_BUFFER_DESC buffers[] = {
    {
        .BufferType = D3D11_VIDEO_DECODER_BUFFER_BITSTREAM,
        .DataOffset = 0,
        .DataSize = h264_data_size,
        // ... 指向 H264 NAL unit 数据
    }
};

// 这会调用 NVDEC 硬件
videoContext->SubmitDecoderBuffers(decoder, 1, buffers);
```

### 4. 为什么不能简单模拟？

构造有效的 H264 解码数据流需要：

1. **SPS (Sequence Parameter Set)** - 序列参数集
2. **PPS (Picture Parameter Set)** - 图像参数集  
3. **Slice Header** - 片头信息
4. **Macroblock Data** - 宏块数据
5. **正确的 NAL unit 格式**

这些数据必须符合 H264 标准，否则解码器会拒绝或报错。

### 5. 真正能触发 Decode 引擎的方法

#### 方法 1: 使用真实视频文件 + Media Foundation (推荐)

```c
// 使用 Media Foundation 的硬件解码路径
// 它会自动调用 D3D11 Video Decoder
IMFSourceReader* reader;
MFCreateSourceReaderFromURL(L"test.mp4", NULL, &reader);

// 配置硬件加速
IMFAttributes* attrs;
reader->SetStreamSelection(MF_SOURCE_READER_FIRST_VIDEO_STREAM, TRUE);

// 读取并解码样本
IMFSample* sample;
reader->ReadSample(MF_SOURCE_READER_FIRST_VIDEO_STREAM, 0, ...);
// 这会触发 NVDEC 解码
```

#### 方法 2: 使用 FFmpeg + DXVA/D3D11VA

```bash
# 使用 FFmpeg 播放视频并强制硬件解码
ffmpeg -hwaccel d3d11va -i input.mp4 -f null -

# 这会调用 D3D11 Video Decoder，触发 Decode 引擎
```

#### 方法 3: 使用现有工具

```bash
# 方法 A: 使用 ffmpeg 生成解码负载
# 先准备一个测试视频
curl -o test.mp4 "https://www.w3schools.com/html/mov_bbb.mp4"

# 循环解码（触发 NVDEC）
ffmpeg -stream_loop -1 -hwaccel d3d11va -i test.mp4 -f null -

# 方法 B: 使用 VLC 播放视频（启用硬件解码）
vlc test.mp4 --hwdec=d3d11va

# 方法 C: 使用 Chrome/Firefox 播放 4K YouTube 视频
```

### 6. 验证 Decode 引擎是否工作

```bash
# 监控 Decode 利用率
nvidia-smi dmon -s pucm

# 或
nvidia-smi --query-gpu=utilization.decoder --format=csv -l 1

# 在 Windows 任务管理器中查看
# Task Manager -> Performance -> GPU -> Video Decode
```

### 7. 结论

| 方法 | 复杂度 | 效果 |
|-----|-------|------|
| Video Processor Blit | 低 | ❌ 不触发 Decode |
| 构造 H264 数据流 | 高 | ✅ 触发 Decode |
| Media Foundation | 中 | ✅ 触发 Decode |
| FFmpeg + DXVA | 低 | ✅ 触发 Decode |

**推荐方案**：编写一个程序调用 FFmpeg API 或使用 Media Foundation 解码真实视频文件。
