/**
 * GPU Copy Load Generator - Fast Copy Edition
 * 使用 DYNAMIC 缓冲区和 Map/Unmap 提高频率
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// 使用较小缓冲区提高频率
#define BUFFER_SIZE (64 * 1024 * 1024ULL)  // 64MB
#define NUM_BUFFERS 16                      // 更多缓冲区

int main(int argc, char* argv[]) {
    int duration_sec = 30;
    
    if (argc > 1) duration_sec = atoi(argv[1]);

    printf("================================================\n");
    printf("GPU Copy Load Generator - Fast Copy Edition\n");
    printf("================================================\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Buffer size: %d MB x %d buffers\n", 
           (int)(BUFFER_SIZE / (1024*1024)), NUM_BUFFERS);
    printf("Strategy: DYNAMIC buffers + Discard Map\n\n");

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

    // 创建 DYNAMIC 缓冲区 - 用于高速 CPU->GPU 传输
    printf("Creating DYNAMIC GPU buffers...\n");
    ID3D11Buffer* buffers[NUM_BUFFERS];
    D3D11_BUFFER_DESC bd = {0};
    bd.ByteWidth = (UINT)BUFFER_SIZE;
    bd.Usage = D3D11_USAGE_DYNAMIC;  // DYNAMIC 允许快速 Map
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    for (int i = 0; i < NUM_BUFFERS; i++) {
        hr = device->lpVtbl->CreateBuffer(device, &bd, NULL, &buffers[i]);
        if (FAILED(hr)) {
            printf("Failed at buffer %d\n", i);
            return 1;
        }
    }
    printf("OK\n\n");

    // 预热
    printf("Warming up...\n");
    for (int i = 0; i < NUM_BUFFERS; i++) {
        D3D11_MAPPED_SUBRESOURCE mapped;
        if (SUCCEEDED(context->lpVtbl->Map(context, (ID3D11Resource*)buffers[i], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
            // 填充数据
            for (size_t j = 0; j < BUFFER_SIZE; j += 4096) {
                ((BYTE*)mapped.pData)[j] = (BYTE)(i + j);
            }
            context->lpVtbl->Unmap(context, (ID3D11Resource*)buffers[i], 0);
        }
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

    printf("Running...\n");
    
    while (1) {
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
        if (elapsed >= duration_sec) break;

        // 使用 WRITE_DISCARD 快速 Map
        D3D11_MAPPED_SUBRESOURCE mapped;
        hr = context->lpVtbl->Map(context, (ID3D11Resource*)buffers[bufferIdx], 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        
        if (SUCCEEDED(hr)) {
            // 模拟填充数据（实际只需要写一些关键位置）
            volatile BYTE* data = (volatile BYTE*)mapped.pData;
            for (size_t j = 0; j < BUFFER_SIZE; j += 65536) {
                data[j] = (BYTE)operationCount;
            }
            context->lpVtbl->Unmap(context, (ID3D11Resource*)buffers[bufferIdx], 0);
            
            totalBytes += BUFFER_SIZE;
            operationCount++;
        }

        bufferIdx = (bufferIdx + 1) % NUM_BUFFERS;
        
        // 每 64 次操作 Flush 一次
        if ((operationCount & 63) == 0) {
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
    printf("Operations: %d\n", operationCount);
    printf("Data: %.2f GB\n", totalBytes / (1024.0 * 1024 * 1024));
    printf("Time: %.2f seconds\n", totalTime);
    printf("Bandwidth: %.2f GB/s\n", bandwidth);
    printf("Operations/sec: %.0f\n", operationCount / totalTime);
    printf("================================================\n");

    // 清理
    for (int i = 0; i < NUM_BUFFERS; i++) {
        buffers[i]->lpVtbl->Release(buffers[i]);
    }
    context->lpVtbl->Release(context);
    device->lpVtbl->Release(device);

    return 0;
}
