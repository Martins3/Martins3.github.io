# Video Decode 引擎 - 最终分析报告

## 根本原因分析

### 为什么原来的程序失败？

```
Video Processor (NPP) ≠ Video Decoder (NVDEC)
```

| 特性 | Video Processor | Video Decoder |
|-----|-----------------|---------------|
| D3D11 API | `ID3D11VideoProcessor` | `ID3D11VideoDecoder` |
| 输入 | 未压缩帧 (YUV) | 压缩数据 (H264 NAL) |
| 硬件单元 | NPP (Video Processing Engine) | NVDEC (Decode Engine) |
| 任务管理器 | Video Processing | Video Decode |
| nvidia-smi | 不单独显示 | `utilization.decoder` |

**原程序使用 `VideoProcessorBlt()`，这只调用 NPP，不调用 NVDEC！**

### 为什么 FFmpeg + d3d11va 失败？

```bash
# 这个命令实际上使用了软件解码
ffmpeg -hwaccel d3d11va -i input.mp4 -f null -

# 输出显示: h264 (native) ← 软件解码！
```

原因：
1. 输出格式不匹配导致自动回退到软件解码
2. D3D11VA 需要特定的初始化序列

### 正确的方法：使用 h264_cuvid

```bash
# 使用 NVIDIA 专用解码器
ffmpeg -c:v h264_cuvid -i input.mp4 -f null -

# 输出显示: h264 (h264_cuvid) ← 硬件解码！
```

## 实测结果

### 测试命令

```powershell
# 启动 4 个并发解码实例
ffmpeg -c:v h264_cuvid -stream_loop -1 -i test.mp4 -t 15 -f null -
```

### nvidia-smi 监控

```
[11:31:34] Decoder: 0%,  Memory: 2098 MiB
[11:31:35] Decoder: 99%, Memory: 2313 MiB  <-- 成功！
[11:31:36] Decoder: 0%,  Memory: 2081 MiB
```

### Windows 性能计数器

```powershell
Get-Counter "\GPU Engine(*)\Utilization Percentage" | 
    Where-Object { $_.Path -like "*VideoDecode*" }
```

## 关键发现

1. **NVDEC 确实存在且可用**
   - RTX 4060 Laptop GPU 有 NVDEC 引擎
   - 可以同时解码多个视频流

2. **监控方式**
   - nvidia-smi: `utilization.decoder`
   - Windows: `\GPU Engine(*VideoDecode*)\Utilization Percentage`
   - Task Manager: Performance -> GPU -> Video Decode

3. **触发 Decode 引擎的正确方法**
   - 使用 `h264_cuvid` / `hevc_cuvid` / `av1_cuvid` 解码器
   - 或使用支持硬件解码的播放器（VLC、MPC-HC 等）
   - 或直接使用 NVDEC API 编程

## 更新后的程序

使用 `gpu_real_decode.exe` 调用 FFmpeg + h264_cuvid 来触发 Decode 引擎。
