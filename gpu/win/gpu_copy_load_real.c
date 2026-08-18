/**
 * GPU Copy Load Generator - Real Copy Edition
 * 使用 STAGING 缓冲区进行真正的 CPU<->GPU 数据传输
 * 
 * 关键优化：
 * 1. 使用 STAGING 缓冲区进行回读（强制真正传输）
 * 2. 多个缓冲区流水线
 * 3. 异步操作减少等待
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// 256MB 缓冲区 - 平衡大小和延迟
#define BUFFER_SIZE (256 * 1024 * 1024ULL)
#define NUM_BUFFERS 6

int main(int argc, char* argv[]) {
    int duration_sec = 30;
    int mode = 0;  // 0=upload, 1=download, 2=both
    
    if (argc > 1) duration_sec = atoi(argv[1]);
    if (argc > 2) mode = atoi(argv[2]);

    printf("================================================\n");
    printf("GPU Copy Load Generator - Real Copy Edition\n");
    printf("================================================\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Mode: ");
    switch (mode) {
        case 0: printf("Upload (CPU -> GPU)\n"); break;
        case 1: printf("Download (GPU -> CPU)\n"); break;
        default: printf("Bidirectional\n"); break;
    }
    printf("Buffer size: %d MB x %d = %d MB\n", 
           (int)(BUFFER_SIZE / (1024*1024)), NUM_BUFFERS,
           (int)(BUFFER_SIZE * NUM_BUFFERS / (1024*1024)));
    printf("\n");

    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device* device = NULL;
    ID3D11DeviceContext* context = NULL;

    HRESULT hr = D3D11CreateDevice(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
        NULL, 0, D3D11_SDK_VERSION,
        &device, &featureLevel, &context
    );

    if (FAILED(hr)) {
        printf("Failed to create D3D11 device\n");
        return 1;
    }

    // GPU 信息
    IDXGIDevice* dxgiDevice = NULL;
    if (SUCCEEDED(device->lpVtbl->QueryInterface(device, &IID_IDXGIDevice, (void**)&dxgiDevice))) {
        IDXGIAdapter* adapter = NULL;
        if (SUCCEEDED(dxgiDevice->lpVtbl->GetAdapter(dxgiDevice, &adapter))) {
            DXGI_ADAPTER_DESC desc;
            adapter->lpVtbl->GetDesc(adapter, &desc);
            printf("GPU: %ls\n", desc.Description);
            printf("Dedicated VRAM: %.0f MB\n", desc.DedicatedVideoMemory / (1024.0 * 1024.0));
            adapter->lpVtbl->Release(adapter);
        }
        dxgiDevice->lpVtbl->Release(dxgiDevice);
    }
    printf("\n");

    // 创建 GPU 缓冲区 (DEFAULT)
    printf("Creating GPU buffers... ");
    ID3D11Buffer* gpuBuffers[NUM_BUFFERS];
    D3D11_BUFFER_DESC gpuDesc = {0};
    gpuDesc.ByteWidth = (UINT)BUFFER_SIZE;
    gpuDesc.Usage = D3D11_USAGE_DEFAULT;
    gpuDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    gpuDesc.CPUAccessFlags = 0;

    for (int i = 0; i < NUM_BUFFERS; i++) {
        hr = device->lpVtbl->CreateBuffer(device, &gpuDesc, NULL, &gpuBuffers[i]);
        if (FAILED(hr)) {
            printf("Failed at buffer %d\n", i);
            return 1;
        }
    }
    printf("OK\n");

    // 创建 STAGING 缓冲区用于回读
    printf("Creating STAGING buffers... ");
    ID3D11Buffer* stagingBuffers[NUM_BUFFERS];
    D3D11_BUFFER_DESC stagingDesc = {0};
    stagingDesc.ByteWidth = (UINT)BUFFER_SIZE;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;

    for (int i = 0; i < NUM_BUFFERS; i++) {
        hr = device->lpVtbl->CreateBuffer(device, &stagingDesc, NULL, &stagingBuffers[i]);
        if (FAILED(hr)) {
            printf("Failed at staging buffer %d\n", i);
            return 1;
        }
    }
    printf("OK\n");

    // 分配 CPU 缓冲区
    printf("Allocating CPU buffers... ");
    BYTE* cpuBuffers[NUM_BUFFERS];
    for (int i = 0; i < NUM_BUFFERS; i++) {
        cpuBuffers[i] = (BYTE*)_aligned_malloc(BUFFER_SIZE, 4096);
        for (size_t j = 0; j < BUFFER_SIZE; j += 4096) {
            cpuBuffers[i][j] = (BYTE)(i * 13 + (j >> 12));
        }
    }
    printf("OK\n\n");

    // 预热 GPU 缓冲区
    printf("Warming up...\n");
    for (int i = 0; i < NUM_BUFFERS; i++) {
        D3D11_BOX box = {0, 0, 0, (UINT)BUFFER_SIZE, 1, 1};
        context->lpVtbl->UpdateSubresource(context, (ID3D11Resource*)gpuBuffers[i], 0, &box, cpuBuffers[i], (UINT)BUFFER_SIZE, 0);
    }
    context->lpVtbl->Flush(context);
    printf("Ready!\n\n");

    // 主循环
    LARGE_INTEGER freq, start, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    long long totalBytes = 0;
    int operationCount = 0;
    int bufferIdx = 0;

    printf("Running (this uses real PCIe transfers)...\n");
    
    while (1) {
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
        if (elapsed >= duration_sec) break;

        if (mode == 0) {
            // ===== Upload: CPU -> GPU =====
            // 方法: UpdateSubresource (同步上传)
            D3D11_BOX box = {0, 0, 0, (UINT)BUFFER_SIZE, 1, 1};
            context->lpVtbl->UpdateSubresource(
                context,
                (ID3D11Resource*)gpuBuffers[bufferIdx],
                0, &box, cpuBuffers[bufferIdx], (UINT)BUFFER_SIZE, 0
            );
            totalBytes += BUFFER_SIZE;
        }
        else if (mode == 1) {
            // ===== Download: GPU -> CPU =====
            // 步骤 1: GPU -> STAGING (CopyResource)
            context->lpVtbl->CopyResource(
                context,
                (ID3D11Resource*)stagingBuffers[bufferIdx],
                (ID3D11Resource*)gpuBuffers[bufferIdx]
            );
            
            // 步骤 2: Map STAGING (强制同步和传输)
            D3D11_MAPPED_SUBRESOURCE mapped;
            hr = context->lpVtbl->Map(context, (ID3D11Resource*)stagingBuffers[bufferIdx], 0, D3D11_MAP_READ, 0, &mapped);
            if (SUCCEEDED(hr)) {
                // 读第一个字节强制数据传输完成
                volatile BYTE dummy = ((BYTE*)mapped.pData)[0];
                (void)dummy;
                context->lpVtbl->Unmap(context, (ID3D11Resource*)stagingBuffers[bufferIdx], 0);
            }
            totalBytes += BUFFER_SIZE;
        }
        else {
            // ===== Bidirectional =====
            // Upload
            D3D11_BOX box = {0, 0, 0, (UINT)BUFFER_SIZE, 1, 1};
            context->lpVtbl->UpdateSubresource(
                context,
                (ID3D11Resource*)gpuBuffers[bufferIdx],
                0, &box, cpuBuffers[bufferIdx], (UINT)BUFFER_SIZE, 0
            );
            
            // Download (不同的缓冲区)
            int downloadIdx = (bufferIdx + NUM_BUFFERS/2) % NUM_BUFFERS;
            context->lpVtbl->CopyResource(
                context,
                (ID3D11Resource*)stagingBuffers[downloadIdx],
                (ID3D11Resource*)gpuBuffers[downloadIdx]
            );
            
            D3D11_MAPPED_SUBRESOURCE mapped;
            if (SUCCEEDED(context->lpVtbl->Map(context, (ID3D11Resource*)stagingBuffers[downloadIdx], 0, D3D11_MAP_READ, 0, &mapped))) {
                volatile BYTE dummy = ((BYTE*)mapped.pData)[0];
                (void)dummy;
                context->lpVtbl->Unmap(context, (ID3D11Resource*)stagingBuffers[downloadIdx], 0);
            }
            
            totalBytes += BUFFER_SIZE * 2;
        }

        operationCount++;
        bufferIdx = (bufferIdx + 1) % NUM_BUFFERS;

        // 每 8 次操作 Flush 一次
        if ((operationCount & 7) == 0) {
            context->lpVtbl->Flush(context);
        }
    }

    context->lpVtbl->Flush(context);

    // 统计
    QueryPerformanceCounter(&now);
    double totalTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
    double bandwidth = (totalBytes / (1024.0 * 1024 * 1024)) / totalTime;

    printf("\n");
    printf("================================================\n");
    printf("Results:\n");
    printf("================================================\n");
    printf("Total operations: %d\n", operationCount);
    printf("Total data transferred: %.2f GB\n", totalBytes / (1024.0 * 1024 * 1024));
    printf("Total time: %.2f seconds\n", totalTime);
    printf("Average bandwidth: %.2f GB/s\n", bandwidth);
    printf("Operations per second: %.0f\n", operationCount / totalTime);
    printf("\n");
    printf("PCIe Bandwidth Limits (theoretical max):\n");
    printf("  PCIe 3.0 x8:  ~8 GB/s\n");
    printf("  PCIe 3.0 x16: ~16 GB/s\n");
    printf("  PCIe 4.0 x8:  ~16 GB/s\n");
    printf("  PCIe 4.0 x16: ~32 GB/s\n");
    printf("  PCIe 5.0 x16: ~64 GB/s\n");
    printf("\n");
    printf("Note: Laptop GPUs typically use PCIe x8, not x16\n");
    printf("RTX 4060 Laptop likely uses PCIe 4.0 x8 (~16 GB/s max)\n");
    printf("================================================\n");

    // 清理
    for (int i = 0; i < NUM_BUFFERS; i++) {
        gpuBuffers[i]->lpVtbl->Release(gpuBuffers[i]);
        stagingBuffers[i]->lpVtbl->Release(stagingBuffers[i]);
        _aligned_free(cpuBuffers[i]);
    }
    context->lpVtbl->Release(context);
    device->lpVtbl->Release(device);

    return 0;
}
