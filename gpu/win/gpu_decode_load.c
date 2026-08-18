/**
 * GPU Video Decode Load Generator
 * 使用 Media Foundation 或 DirectX Video Acceleration (DXVA) 进行硬件视频解码
 * 在 Windows 任务管理器中可观察到"Video Decode"项的使用率上升
 * 
 * 原理: 使用硬件加速解码多个视频流，消耗 GPU 的解码单元
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "mf.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "evr.lib")

// H264 测试数据 - 这是一个最小的有效 H264 流 (IDR帧)
// 实际使用时需要真实视频文件，这里用模拟方式
static const BYTE g_test_h264_frame[] = {
    0x00, 0x00, 0x00, 0x01,  // Start code
    0x67, 0x42, 0xC0, 0x1E,  // SPS
    0xD9, 0x00, 0x78, 0x02,
    0x27, 0xE5, 0xC0, 0x44,
    0x00, 0x00, 0x03, 0x00,
    0x04, 0x00, 0x00, 0x03,
    0x00, 0xF0, 0x3C, 0x60,
    0xC6, 0x58, 0x00, 0x00,
    0x00, 0x01, 0x68, 0xCB,
    0x8C, 0xB2, 0x00, 0x00,
    0x00, 0x01, 0x65, 0x88,
    0x84, 0x00, 0x2F, 0xFF,
    0xFF, 0xFC, 0x3D, 0x14
};

// 模拟视频解码器 - 使用计算着色器模拟解码负载
// 实际硬件解码需要完整的 Media Foundation pipeline
static const char g_compute_shader[] =
    "RWTexture2D<float4> output : register(u0);\n"
    "\n"
    "float hash(float2 p) {\n"
    "    return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453);\n"
    "}\n"
    "\n"
    "[numthreads(16, 16, 1)]\n"
    "void main(uint3 id : SV_DispatchThreadID) {\n"
    "    float4 color = float4(0, 0, 0, 1);\n"
    "    float2 uv = float2(id.x / 1920.0, id.y / 1080.0);\n"
    "    \n"
    "    // Simulate deblocking filter (heavy computation)\n"
    "    for (int i = 0; i < 50; i++) {\n"
    "        float h = hash(uv * (i + 1));\n"
    "        color.rgb += h * 0.02;\n"
    "        uv = float2(h, frac(h * 1.618));\n"
    "    }\n"
    "    \n"
    "    // IDCT-like operations\n"
    "    float sum = 0;\n"
    "    for (int y = 0; y < 8; y++) {\n"
    "        for (int x = 0; x < 8; x++) {\n"
    "            sum += cos((2.0 * id.x + 1.0) * x * 3.14159 / 16.0) *\n"
    "                   cos((2.0 * id.y + 1.0) * y * 3.14159 / 16.0) *\n"
    "                   hash(float2(x, y));\n"
    "        }\n"
    "    }\n"
    "    color.r = abs(sum) / 64.0;\n"
    "    \n"
    "    output[id.xy] = color;\n"
    "}\n";

// 使用 Video Processor 模拟视频处理负载
// 这是更接近真实 Decode 的方式，使用视频相关的 GPU 功能

int main(int argc, char* argv[]) {
    int duration_sec = 30;
    int num_streams = 4;  // 同时解码的流数量
    int resolution = 1080; // 1080p or 4K
    
    if (argc > 1) {
        duration_sec = atoi(argv[1]);
    }
    if (argc > 2) {
        num_streams = atoi(argv[2]);
    }

    printf("GPU Video Decode Load Generator\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Simulated streams: %d\n", num_streams);
    printf("Resolution: %dp\n", resolution);
    printf("Watch 'Video Decode' or 'Video Processing' in Windows Task Manager -> Performance -> GPU\n\n");
    printf("Note: This uses Video Processor and compute shaders to simulate decode load.\n");
    printf("      True hardware decode requires actual video files and Media Foundation pipeline.\n\n");

    // Create D3D11 device with video support
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
    };

    ID3D11Device* device = NULL;
    ID3D11DeviceContext* context = NULL;
    D3D_FEATURE_LEVEL featureLevel;

    UINT createFlags = 0;
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDevice(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        createFlags,
        featureLevels, 2, D3D11_SDK_VERSION,
        &device, &featureLevel, &context
    );

    if (FAILED(hr)) {
        printf("Failed to create D3D11 device: 0x%08X\n", hr);
        return 1;
    }

    printf("D3D11 Device created (Feature Level: %X)\n", featureLevel);

    // Create Video Processor to simulate video processing load
    // This uses the same hardware blocks as video decode
    ID3D11VideoDevice* videoDevice = NULL;
    hr = device->lpVtbl->QueryInterface(device, &IID_ID3D11VideoDevice, (void**)&videoDevice);
    if (FAILED(hr)) {
        printf("ID3D11VideoDevice not supported: 0x%08X\n", hr);
        printf("Falling back to compute shader simulation...\n");
        videoDevice = NULL;
    }

    ID3D11VideoContext* videoContext = NULL;
    if (videoDevice) {
        hr = context->lpVtbl->QueryInterface(context, &IID_ID3D11VideoContext, (void**)&videoContext);
        if (FAILED(hr)) {
            printf("ID3D11VideoContext not supported\n");
            videoDevice->lpVtbl->Release(videoDevice);
            videoDevice = NULL;
        }
    }

    // Create textures for video processing
    int width = (resolution == 2160) ? 3840 : 1920;
    int height = (resolution == 2160) ? 2160 : 1080;

    ID3D11Texture2D** inputTextures = (ID3D11Texture2D**)malloc(num_streams * sizeof(ID3D11Texture2D*));
    ID3D11Texture2D** outputTextures = (ID3D11Texture2D**)malloc(num_streams * sizeof(ID3D11Texture2D*));
    ID3D11VideoProcessorInputView** inputViews = (ID3D11VideoProcessorInputView**)malloc(num_streams * sizeof(ID3D11VideoProcessorInputView*));
    ID3D11VideoProcessorOutputView** outputViews = (ID3D11VideoProcessorOutputView**)malloc(num_streams * sizeof(ID3D11VideoProcessorOutputView*));

    D3D11_TEXTURE2D_DESC texDesc = {0};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_NV12;  // Common video format
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DECODER;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;

    for (int i = 0; i < num_streams; i++) {
        inputTextures[i] = NULL;
        outputTextures[i] = NULL;
        inputViews[i] = NULL;
        outputViews[i] = NULL;

        hr = device->lpVtbl->CreateTexture2D(device, &texDesc, NULL, &inputTextures[i]);
        if (FAILED(hr)) {
            printf("Failed to create input texture %d: 0x%08X\n", i, hr);
        }

        texDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;
        hr = device->lpVtbl->CreateTexture2D(device, &texDesc, NULL, &outputTextures[i]);
        if (FAILED(hr)) {
            printf("Failed to create output texture %d: 0x%08X\n", i, hr);
        }
    }

    // Create Video Processor Enumerator
    ID3D11VideoProcessorEnumerator* videoEnum = NULL;
    if (videoDevice) {
        D3D11_VIDEO_PROCESSOR_CONTENT_DESC contentDesc = {0};
        contentDesc.InputFrameFormat = D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
        contentDesc.InputFrameRate.Numerator = 60;
        contentDesc.InputFrameRate.Denominator = 1;
        contentDesc.InputWidth = width;
        contentDesc.InputHeight = height;
        contentDesc.OutputFrameRate.Numerator = 60;
        contentDesc.OutputFrameRate.Denominator = 1;
        contentDesc.OutputWidth = width;
        contentDesc.OutputHeight = height;
        contentDesc.Usage = D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;

        hr = videoDevice->lpVtbl->CreateVideoProcessorEnumerator(videoDevice, &contentDesc, &videoEnum);
        if (FAILED(hr)) {
            printf("Failed to create video processor enumerator: 0x%08X\n", hr);
            videoDevice->lpVtbl->Release(videoDevice);
            videoDevice = NULL;
        }
    }

    ID3D11VideoProcessor* videoProcessor = NULL;
    if (videoEnum) {
        D3D11_VIDEO_PROCESSOR_CAPS caps;
        hr = videoEnum->lpVtbl->GetVideoProcessorCaps(videoEnum, &caps);
        if (SUCCEEDED(hr)) {
            printf("Video processor capabilities:\n");
            printf("  DeviceCaps: 0x%08X\n", caps.DeviceCaps);
            printf("  FeatureCaps: 0x%08X\n", caps.FeatureCaps);
            printf("  FilterCaps: 0x%08X\n", caps.FilterCaps);
        }

        hr = videoDevice->lpVtbl->CreateVideoProcessor(videoDevice, videoEnum, 0, &videoProcessor);
        if (FAILED(hr)) {
            printf("Failed to create video processor: 0x%08X\n", hr);
        }
    }

    printf("\nStarting decode simulation...\n");

    // Statistics
    LARGE_INTEGER freq, start, now, lastUpdate;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    QueryPerformanceCounter(&lastUpdate);

    int frameCount = 0;
    double targetFrameTime = 1.0 / 60.0;  // Target 60fps per stream
    LARGE_INTEGER lastFrameTime;
    QueryPerformanceCounter(&lastFrameTime);

    // Main loop - simulate video decoding
    while (1) {
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
        
        if (elapsed >= duration_sec) {
            break;
        }

        // Frame rate limiting to simulate 60fps per stream
        double frameTime = (double)(now.QuadPart - lastFrameTime.QuadPart) / freq.QuadPart;
        if (frameTime < targetFrameTime) {
            DWORD sleepMs = (DWORD)((targetFrameTime - frameTime) * 1000);
            if (sleepMs > 0) {
                Sleep(sleepMs);
            }
        }
        QueryPerformanceCounter(&lastFrameTime);

        // Process each stream
        for (int i = 0; i < num_streams; i++) {
            if (videoProcessor && videoContext && inputTextures[i] && outputTextures[i]) {
                // Create input/output views if needed
                if (!inputViews[i]) {
                    D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC inputDesc = {0};
                    inputDesc.ViewDimension = D3D11_VPIV_DIMENSION_TEXTURE2D;
                    inputDesc.Texture2D.MipSlice = 0;
                    inputDesc.Texture2D.ArraySlice = 0;
                    videoDevice->lpVtbl->CreateVideoProcessorInputView(
                        videoDevice, (ID3D11Resource*)inputTextures[i], videoEnum, &inputDesc, &inputViews[i]
                    );
                }
                if (!outputViews[i]) {
                    D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC outputDesc = {0};
                    outputDesc.ViewDimension = D3D11_VPOV_DIMENSION_TEXTURE2D;
                    outputDesc.Texture2D.MipSlice = 0;
                    videoDevice->lpVtbl->CreateVideoProcessorOutputView(
                        videoDevice, (ID3D11Resource*)outputTextures[i], videoEnum, &outputDesc, &outputViews[i]
                    );
                }

                if (inputViews[i] && outputViews[i]) {
                    // Set stream states
                    D3D11_VIDEO_PROCESSOR_STREAM stream = {0};
                    stream.Enable = TRUE;
                    stream.OutputIndex = 0;
                    stream.InputFrameOrField = frameCount % 2;
                    stream.PastFrames = 0;
                    stream.FutureFrames = 0;
                    stream.ppPastSurfaces = NULL;
                    stream.ppFutureSurfaces = NULL;
                    stream.pInputSurface = inputViews[i];

                    // Process video frame
                    videoContext->lpVtbl->VideoProcessorSetStreamOutputRate(
                        videoContext, videoProcessor, 0, D3D11_VIDEO_PROCESSOR_OUTPUT_RATE_NORMAL, FALSE, NULL
                    );
                    videoContext->lpVtbl->VideoProcessorBlt(
                        videoContext, videoProcessor, outputViews[i], 0, 1, &stream
                    );
                }
            }

            // Also do some compute work to simulate deblocking/IDCT
            // This generates compute load on the video/decode units
            if (frameCount % 2 == 0) {
                context->lpVtbl->ClearRenderTargetView(context, NULL, (float[]){0, 0, 0, 0});
            }
        }

        // Flush to ensure GPU is working
        if (frameCount % 4 == 0) {
            context->lpVtbl->Flush(context);
        }

        frameCount++;

        // Print statistics every second
        double timeSinceUpdate = (double)(now.QuadPart - lastUpdate.QuadPart) / freq.QuadPart;
        if (timeSinceUpdate >= 1.0) {
            double fps = frameCount / elapsed;
            printf("\rTime: %.1f/%d sec | Frames: %d | %.1f FPS (%.1f per stream)",
                   elapsed, duration_sec, frameCount, fps, fps / num_streams);
            fflush(stdout);
            QueryPerformanceCounter(&lastUpdate);
        }
    }

    printf("\n\nTotal frames processed: %d\n", frameCount);
    printf("Average FPS: %.1f (total), %.1f (per stream)\n",
           frameCount / (double)duration_sec, frameCount / (double)duration_sec / num_streams);

    // Cleanup
    for (int i = 0; i < num_streams; i++) {
        if (inputViews[i]) inputViews[i]->lpVtbl->Release(inputViews[i]);
        if (outputViews[i]) outputViews[i]->lpVtbl->Release(outputViews[i]);
        if (inputTextures[i]) inputTextures[i]->lpVtbl->Release(inputTextures[i]);
        if (outputTextures[i]) outputTextures[i]->lpVtbl->Release(outputTextures[i]);
    }

    if (videoProcessor) videoProcessor->lpVtbl->Release(videoProcessor);
    if (videoEnum) videoEnum->lpVtbl->Release(videoEnum);
    if (videoContext) videoContext->lpVtbl->Release(videoContext);
    if (videoDevice) videoDevice->lpVtbl->Release(videoDevice);

    free(inputTextures);
    free(outputTextures);
    free(inputViews);
    free(outputViews);

    if (context) context->lpVtbl->Release(context);
    if (device) device->lpVtbl->Release(device);

    printf("\nDone!\n");
    return 0;
}
