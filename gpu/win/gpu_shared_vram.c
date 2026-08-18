/**
 * GPU Shared Memory Allocator
 * 分配 GPU 共享内存（系统内存被 GPU 使用）
 * 在 Windows 任务管理器中可观察到"共享 GPU 内存"的使用量上升
 * 
 * 原理:
 * 1. 集成显卡: 使用系统内存作为显存，所有 GPU 内存都是"共享"
 * 2. 独立显卡: 专用显存用尽后，会溢出使用系统内存（但驱动通常不透明处理）
 * 
 * 为了显示共享内存增长，我们使用 STAGING 缓冲区，这些缓冲区:
 * - 位于系统内存中 (CPU 可访问)
 * - 可被 GPU 访问 (用于数据上传/下载)
 * - 在任务管理器中显示为"共享 GPU 内存"
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// 每次分配的块大小: 256MB
#define ALLOC_CHUNK_SIZE (256ULL * 1024 * 1024)

int main(int argc, char* argv[]) {
    int target_mb = 1024;  // 默认目标: 1GB
    int duration_sec = 0;  // 默认: 一直运行直到按键
    
    if (argc > 1) {
        target_mb = atoi(argv[1]);
    }
    if (argc > 2) {
        duration_sec = atoi(argv[2]);
    }

    printf("GPU Shared Memory Allocator\n");
    printf("Target allocation: %d MB (%.2f GB)\n", target_mb, target_mb / 1024.0);
    if (duration_sec > 0) {
        printf("Duration: %d seconds\n", duration_sec);
    } else {
        printf("Duration: unlimited (press Enter to exit)\n");
    }
    printf("Watch 'Shared GPU memory' in Windows Task Manager -> Performance -> GPU\n\n");

    // Create D3D11 device
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

    // Get GPU info
    IDXGIDevice* dxgiDevice = NULL;
    hr = device->lpVtbl->QueryInterface(device, &IID_IDXGIDevice, (void**)&dxgiDevice);
    if (SUCCEEDED(hr)) {
        IDXGIAdapter* adapter = NULL;
        hr = dxgiDevice->lpVtbl->GetAdapter(dxgiDevice, &adapter);
        if (SUCCEEDED(hr)) {
            DXGI_ADAPTER_DESC desc;
            adapter->lpVtbl->GetDesc(adapter, &desc);
            printf("GPU: %ls\n", desc.Description);
            printf("Dedicated Video Memory: %.0f MB\n", desc.DedicatedVideoMemory / (1024.0 * 1024.0));
            printf("Shared System Memory: %.0f MB\n", desc.SharedSystemMemory / (1024.0 * 1024.0));
            adapter->lpVtbl->Release(adapter);
        }
        dxgiDevice->lpVtbl->Release(dxgiDevice);
    }

    // Check if this is an integrated GPU
    D3D11_FEATURE_DATA_D3D11_OPTIONS2 options2 = {0};
    hr = device->lpVtbl->CheckFeatureSupport(device, D3D11_FEATURE_D3D11_OPTIONS2, &options2, sizeof(options2));
    if (SUCCEEDED(hr)) {
        printf("Unified Memory Architecture: %s\n", options2.UnifiedMemoryArchitecture ? "Yes" : "No");
        if (options2.UnifiedMemoryArchitecture) {
            printf("  (This is an integrated GPU - all GPU memory is 'shared')\n");
        }
    }

    printf("\n");

    // Calculate number of buffers needed
    // Use 512MB chunks for efficiency
    const size_t BUFFER_SIZE = ALLOC_CHUNK_SIZE;
    int numBuffers = (target_mb * 1024ULL * 1024 + BUFFER_SIZE - 1) / BUFFER_SIZE;
    size_t actualAlloc = (size_t)numBuffers * BUFFER_SIZE;

    printf("Allocation plan:\n");
    printf("  Buffer size: %.0f MB each\n", BUFFER_SIZE / (1024.0 * 1024.0));
    printf("  Number of buffers: %d\n", numBuffers);
    printf("  Total allocation: %.0f MB\n\n", actualAlloc / (1024.0 * 1024.0));

    // Allocate staging buffers (shared memory)
    ID3D11Buffer** stagingBuffers = (ID3D11Buffer**)calloc(numBuffers, sizeof(ID3D11Buffer*));
    if (!stagingBuffers) {
        printf("Failed to allocate buffer array\n");
        return 1;
    }

    D3D11_BUFFER_DESC desc = {0};
    desc.ByteWidth = (UINT)BUFFER_SIZE;
    desc.Usage = D3D11_USAGE_STAGING;  // <-- 这是关键：STAGING = 系统内存，GPU可访问
    desc.BindFlags = 0;                // 不能绑定到渲染管线
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    printf("Allocating STAGING buffers (shared memory)...\n");
    int allocated = 0;
    size_t totalAllocated = 0;

    for (int i = 0; i < numBuffers; i++) {
        hr = device->lpVtbl->CreateBuffer(device, &desc, NULL, &stagingBuffers[i]);
        if (FAILED(hr)) {
            printf("  Failed to allocate buffer %d: 0x%08X\n", i + 1, hr);
            break;
        }

        allocated++;
        totalAllocated += BUFFER_SIZE;

        // Progress update
        if ((i + 1) % 2 == 0 || i == numBuffers - 1) {
            printf("\r  Allocated: %d/%d buffers (%.0f MB)",
                   allocated, numBuffers, totalAllocated / (1024.0 * 1024.0));
            fflush(stdout);
        }
    }

    printf("\n\nAllocation complete!\n");
    printf("Successfully allocated: %.2f GB\n", totalAllocated / (1024.0 * 1024.0 * 1024.0));

    // Touch the memory to ensure it's committed
    printf("\nTouching memory to ensure it's committed...\n");
    for (int i = 0; i < allocated; i++) {
        D3D11_MAPPED_SUBRESOURCE mapped;
        hr = context->lpVtbl->Map(context, (ID3D11Resource*)stagingBuffers[i], 0, 
                                  D3D11_MAP_WRITE, 0, &mapped);
        if (SUCCEEDED(hr)) {
            // Write some data
            BYTE* data = (BYTE*)mapped.pData;
            for (size_t j = 0; j < BUFFER_SIZE; j += 4096) {
                data[j] = (BYTE)(i & 0xFF);
            }
            context->lpVtbl->Unmap(context, (ID3D11Resource*)stagingBuffers[i], 0);
        }
    }
    printf("Memory touched and flushed\n");

    // Wait
    printf("\n");
    if (duration_sec > 0) {
        printf("Holding for %d seconds...", duration_sec);
        fflush(stdout);
        for (int i = 0; i < duration_sec; i++) {
            Sleep(1000);
            printf(".");
            fflush(stdout);
        }
        printf("\n");
    } else {
        printf("Memory allocated. Press Enter to release...\n");
        fflush(stdout);
        getchar();
    }

    // Cleanup
    printf("\nReleasing memory...\n");
    for (int i = 0; i < allocated; i++) {
        if (stagingBuffers[i]) stagingBuffers[i]->lpVtbl->Release(stagingBuffers[i]);
    }
    free(stagingBuffers);

    if (context) context->lpVtbl->Release(context);
    if (device) device->lpVtbl->Release(device);

    printf("Done! Check Task Manager - Shared GPU memory should decrease.\n");
    return 0;
}
