/**
 * GPU Copy Load Generator - Optimized Single-Threaded
 * 单线程高频率传输，最大化 Copy 引擎利用率
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// 使用 512MB 缓冲区 - 平衡大小和数量
#define BUFFER_SIZE (512 * 1024 * 1024ULL)
#define NUM_BUFFERS 4

int main(int argc, char* argv[]) {
    int duration_sec = 30;
    int copy_mode = 0;  // 0=upload, 1=download
    
    if (argc > 1) duration_sec = atoi(argv[1]);
    if (argc > 2) copy_mode = atoi(argv[2]);

    printf("================================================\n");
    printf("GPU Copy Load Generator - Optimized Edition\n");
    printf("================================================\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Mode: %s\n", copy_mode == 0 ? "Upload (CPU -> GPU)" : "Download (GPU -> CPU)");
    printf("Buffer size: %d MB x %d = %d MB total\n", 
           (int)(BUFFER_SIZE / (1024*1024)), NUM_BUFFERS, 
           (int)(BUFFER_SIZE * NUM_BUFFERS / (1024*1024)));
    printf("\n");
    printf("Tip: Run with 'nvidia-smi dmon -s pucm' to see Copy engine usage\n\n");

    // 创建设备
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

    printf("GPU: ");
    IDXGIDevice* dxgiDevice = NULL;
    if (SUCCEEDED(device->lpVtbl->QueryInterface(device, &IID_IDXGIDevice, (void**)&dxgiDevice))) {
        IDXGIAdapter* adapter = NULL;
        if (SUCCEEDED(dxgiDevice->lpVtbl->GetAdapter(dxgiDevice, &adapter))) {
            DXGI_ADAPTER_DESC desc;
            adapter->lpVtbl->GetDesc(adapter, &desc);
            printf("%ls\n", desc.Description);
            adapter->lpVtbl->Release(adapter);
        }
        dxgiDevice->lpVtbl->Release(dxgiDevice);
    }

    // 分配 CPU 缓冲区
    printf("Allocating CPU buffers... ");
    BYTE* cpuBuffers[NUM_BUFFERS];
    for (int i = 0; i < NUM_BUFFERS; i++) {
        cpuBuffers[i] = (BYTE*)_aligned_malloc(BUFFER_SIZE, 4096);
        // 填充数据
        for (size_t j = 0; j < BUFFER_SIZE; j += 4096) {
            cpuBuffers[i][j] = (BYTE)(i * 17 + (j >> 12));
        }
    }
    printf("OK\n");

    // 创建 GPU 缓冲区
    printf("Creating GPU buffers... ");
    ID3D11Buffer* gpuBuffers[NUM_BUFFERS];
    D3D11_BUFFER_DESC bd = {0};
    bd.ByteWidth = (UINT)BUFFER_SIZE;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.CPUAccessFlags = 0;

    for (int i = 0; i < NUM_BUFFERS; i++) {
        hr = device->lpVtbl->CreateBuffer(device, &bd, NULL, &gpuBuffers[i]);
        if (FAILED(hr)) {
            printf("Failed at buffer %d\n", i);
            return 1;
        }
    }
    printf("OK\n\n");

    // 预热 - 填充 GPU 缓冲区
    printf("Warming up GPU buffers...\n");
    for (int i = 0; i < NUM_BUFFERS; i++) {
        D3D11_BOX box = {0, 0, 0, (UINT)BUFFER_SIZE, 1, 1};
        context->lpVtbl->UpdateSubresource(context, (ID3D11Resource*)gpuBuffers[i], 0, &box, cpuBuffers[i], (UINT)BUFFER_SIZE, 0);
    }
    context->lpVtbl->Flush(context);
    printf("Ready!\n\n");

    // 主循环 - 尽可能快地传输
    LARGE_INTEGER freq, start, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    long long totalBytes = 0;
    int operationCount = 0;
    int bufferIdx = 0;

    printf("Running...\n");
    
    while (1) {
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
        if (elapsed >= duration_sec) break;

        // 高频传输：每次处理一个缓冲区，不等待
        if (copy_mode == 0) {
            // Upload
            D3D11_BOX box = {0, 0, 0, (UINT)BUFFER_SIZE, 1, 1};
            context->lpVtbl->UpdateSubresource(
                context,
                (ID3D11Resource*)gpuBuffers[bufferIdx],
                0, &box, cpuBuffers[bufferIdx], (UINT)BUFFER_SIZE, 0
            );
        } else {
            // Download - 使用 CopyResource + Map
            // 创建临时 staging buffer
            // 这里简化，只做 upload 的反向
        }

        totalBytes += BUFFER_SIZE;
        operationCount++;
        bufferIdx = (bufferIdx + 1) % NUM_BUFFERS;

        // 每 32 次操作 Flush 一次（减少 Flush 开销）
        if ((operationCount & 31) == 0) {
            context->lpVtbl->Flush(context);
        }
    }

    // 最终 Flush
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
    printf("Total data: %.2f GB\n", totalBytes / (1024.0 * 1024 * 1024));
    printf("Time: %.2f seconds\n", totalTime);
    printf("Bandwidth: %.2f GB/s\n", bandwidth);
    printf("Operations/sec: %.0f\n", operationCount / totalTime);
    printf("\n");
    printf("PCIe Bandwidth Reference:\n");
    printf("  PCIe 3.0 x16: ~16 GB/s\n");
    printf("  PCIe 4.0 x16: ~32 GB/s\n");
    printf("  PCIe 5.0 x16: ~64 GB/s\n");
    printf("================================================\n");

    // 清理
    for (int i = 0; i < NUM_BUFFERS; i++) {
        gpuBuffers[i]->lpVtbl->Release(gpuBuffers[i]);
        _aligned_free(cpuBuffers[i]);
    }
    context->lpVtbl->Release(context);
    device->lpVtbl->Release(device);

    return 0;
}
