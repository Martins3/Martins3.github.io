/**
 * GPU Copy Load Generator
 * 在 CPU 和 GPU 之间进行大量数据传输，占用 Copy 引擎
 * 在 Windows 任务管理器中可观察到"Copy"项的使用率上升
 * 
 * 原理: Copy 引擎负责 CPU↔GPU 之间的数据传输
 * 通过频繁调用 UpdateSubresource / Map-Unmap 可以产生大量 copy 操作
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// 缓冲区大小: 256MB
#define BUFFER_SIZE (256 * 1024 * 1024)

// 同时存在的缓冲区数量
#define NUM_BUFFERS 4

int main(int argc, char* argv[]) {
    int duration_sec = 30;
    int copy_mode = 0;  // 0 = upload (CPU->GPU), 1 = download (GPU->CPU), 2 = bidirectional
    
    if (argc > 1) {
        duration_sec = atoi(argv[1]);
    }
    if (argc > 2) {
        copy_mode = atoi(argv[2]);
    }

    printf("GPU Copy Load Generator\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Mode: ");
    switch (copy_mode) {
        case 0: printf("Upload (CPU -> GPU VRAM)\n"); break;
        case 1: printf("Download (GPU VRAM -> CPU)\n"); break;
        default: printf("Bidirectional\n"); break;
    }
    printf("Buffer size: %d MB per buffer\n", BUFFER_SIZE / (1024 * 1024));
    printf("Watch 'Copy' in Windows Task Manager -> Performance -> GPU\n\n");

    // Create D3D11 device (no window needed)
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device* device = NULL;
    ID3D11DeviceContext* context = NULL;

    UINT createFlags = 0;
#ifdef _DEBUG
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    HRESULT hr = D3D11CreateDevice(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        createFlags,
        NULL, 0, D3D11_SDK_VERSION,
        &device, &featureLevel, &context
    );

    if (FAILED(hr)) {
        printf("Failed to create D3D11 device: 0x%08X\n", hr);
        return 1;
    }

    printf("D3D11 Device created (Feature Level: %d)\n", featureLevel);

    // Allocate CPU buffers
    BYTE** cpuBuffers = (BYTE**)malloc(NUM_BUFFERS * sizeof(BYTE*));
    for (int i = 0; i < NUM_BUFFERS; i++) {
        cpuBuffers[i] = (BYTE*)_aligned_malloc(BUFFER_SIZE, 4096);
        if (!cpuBuffers[i]) {
            printf("Failed to allocate CPU buffer %d\n", i);
            return 1;
        }
        // Fill with random data to prevent compression
        for (size_t j = 0; j < BUFFER_SIZE; j += 4096) {
            cpuBuffers[i][j] = (BYTE)(rand() % 256);
        }
    }

    // Create GPU buffers
    ID3D11Buffer** gpuBuffers = (ID3D11Buffer**)malloc(NUM_BUFFERS * sizeof(ID3D11Buffer*));
    D3D11_BUFFER_DESC bd = {0};
    bd.ByteWidth = BUFFER_SIZE;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;
    bd.StructureByteStride = 0;

    for (int i = 0; i < NUM_BUFFERS; i++) {
        hr = device->lpVtbl->CreateBuffer(device, &bd, NULL, &gpuBuffers[i]);
        if (FAILED(hr)) {
            printf("Failed to create GPU buffer %d: 0x%08X\n", i, hr);
            return 1;
        }
    }

    // Create staging buffer for download
    ID3D11Buffer* stagingBuffer = NULL;
    D3D11_BUFFER_DESC stagingDesc = {0};
    stagingDesc.ByteWidth = BUFFER_SIZE;
    stagingDesc.Usage = D3D11_USAGE_STAGING;
    stagingDesc.BindFlags = 0;
    stagingDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    
    if (copy_mode == 1 || copy_mode == 2) {
        hr = device->lpVtbl->CreateBuffer(device, &stagingDesc, NULL, &stagingBuffer);
        if (FAILED(hr)) {
            printf("Failed to create staging buffer: 0x%08X\n", hr);
            return 1;
        }
    }

    printf("Buffers created successfully\n");
    printf("Starting copy operations...\n\n");

    // Statistics
    LARGE_INTEGER freq, start, now, lastUpdate;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    QueryPerformanceCounter(&lastUpdate);

    long long totalBytesCopied = 0;
    int operationCount = 0;

    // Main loop
    while (1) {
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
        
        if (elapsed >= duration_sec) {
            break;
        }

        // Rotate through buffers
        int bufferIdx = operationCount % NUM_BUFFERS;

        if (copy_mode == 0 || copy_mode == 2) {
            // Upload: CPU -> GPU
            D3D11_BOX box;
            box.left = 0;
            box.top = 0;
            box.front = 0;
            box.right = BUFFER_SIZE;
            box.bottom = 1;
            box.back = 1;

            context->lpVtbl->UpdateSubresource(
                context,
                (ID3D11Resource*)gpuBuffers[bufferIdx],
                0,
                &box,
                cpuBuffers[bufferIdx],
                BUFFER_SIZE,
                0
            );
            totalBytesCopied += BUFFER_SIZE;
        }

        if (copy_mode == 1 || copy_mode == 2) {
            // Download: GPU -> CPU (via staging buffer)
            // First, copy GPU buffer to staging buffer
            context->lpVtbl->CopyResource(
                context,
                (ID3D11Resource*)stagingBuffer,
                (ID3D11Resource*)gpuBuffers[bufferIdx]
            );

            // Then map staging buffer to read (forces synchronization)
            D3D11_MAPPED_SUBRESOURCE mapped;
            hr = context->lpVtbl->Map(context, (ID3D11Resource*)stagingBuffer, 0, D3D11_MAP_READ, 0, &mapped);
            if (SUCCEEDED(hr)) {
                // Touch the data to ensure it's really copied
                volatile BYTE* data = (volatile BYTE*)mapped.pData;
                BYTE dummy = data[0];
                (void)dummy;
                context->lpVtbl->Unmap(context, (ID3D11Resource*)stagingBuffer, 0);
            }
            totalBytesCopied += BUFFER_SIZE;
        }

        // Flush every 4 operations to keep GPU busy
        if (operationCount % 4 == 0) {
            context->lpVtbl->Flush(context);
        }

        operationCount++;

        // Print statistics every second
        double timeSinceUpdate = (double)(now.QuadPart - lastUpdate.QuadPart) / freq.QuadPart;
        if (timeSinceUpdate >= 1.0) {
            double bandwidth = (totalBytesCopied / (1024.0 * 1024.0 * 1024.0)) / elapsed;  // GB/s
            double opsPerSec = operationCount / elapsed;
            printf("\rTime: %.1f/%d sec | Ops: %d | Bandwidth: %.2f GB/s | Avg: %.0f ops/sec",
                   elapsed, duration_sec, operationCount, bandwidth, opsPerSec);
            fflush(stdout);
            lastUpdate = now;
        }
    }

    // Final statistics
    QueryPerformanceCounter(&now);
    double totalTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
    double avgBandwidth = (totalBytesCopied / (1024.0 * 1024.0 * 1024.0)) / totalTime;

    printf("\n\n");
    printf("Total operations: %d\n", operationCount);
    printf("Total data transferred: %.2f GB\n", totalBytesCopied / (1024.0 * 1024.0 * 1024.0));
    printf("Average bandwidth: %.2f GB/s\n", avgBandwidth);
    printf("Average operations/sec: %.0f\n", operationCount / totalTime);

    // Cleanup
    for (int i = 0; i < NUM_BUFFERS; i++) {
        if (gpuBuffers[i]) gpuBuffers[i]->lpVtbl->Release(gpuBuffers[i]);
        _aligned_free(cpuBuffers[i]);
    }
    if (stagingBuffer) stagingBuffer->lpVtbl->Release(stagingBuffer);
    free(cpuBuffers);
    free(gpuBuffers);

    if (context) context->lpVtbl->Release(context);
    if (device) device->lpVtbl->Release(device);

    printf("\nDone!\n");
    return 0;
}
