/**
 * GPU Shared Memory Allocator v2
 * 使用多种策略占用"共享 GPU 内存"
 * 
 * 分析：
 * Windows "共享 GPU 内存"包括：
 * 1. STAGING 缓冲区（CPU 可访问的系统内存）
 * 2. 溢出到系统内存的 GPU 资源（当专用显存不足时）
 * 3. GPU 直接访问的系统内存（零拷贝等）
 * 
 * 策略：
 * - 创建大量 DYNAMIC 缓冲区（频繁 CPU->GPU 传输）
 * - 使用 Map/Unmap 强制提交内存
 * - 创建多个小缓冲区而不是一个大缓冲区
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// 缓冲区大小：128MB
#define BUFFER_SIZE (128ULL * 1024 * 1024)

int main(int argc, char* argv[]) {
    int target_mb = 1024;  // 默认 1GB
    int duration_sec = 0;
    int strategy = 0;  // 0 = 多个 STAGING 缓冲区，1 = DYNAMIC 缓冲区
    
    if (argc > 1) target_mb = atoi(argv[1]);
    if (argc > 2) duration_sec = atoi(argv[2]);
    if (argc > 3) strategy = atoi(argv[3]);

    printf("GPU Shared Memory Allocator v2\n");
    printf("==============================\n");
    printf("Target: %d MB\n", target_mb);
    if (duration_sec > 0) {
        printf("Duration: %d seconds\n", duration_sec);
    } else {
        printf("Duration: unlimited (press Enter to exit)\n");
    }
    
    const char* strategyName = (strategy == 0) ? "Multiple STAGING buffers" : 
                               (strategy == 1) ? "DYNAMIC buffers with Map" : "Mixed";
    printf("Strategy: %s\n", strategyName);
    printf("\n");
    printf("Watch 'Shared GPU memory' in Task Manager -> Performance -> GPU\n");
    printf("Or run: typeperf \"\\GPU Adapter Memory(*)\\Shared Usage\" -sc 1\n\n");

    // 创建 D3D11 设备
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device* device = NULL;
    ID3D11DeviceContext* context = NULL;

    HRESULT hr = D3D11CreateDevice(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
        NULL, 0, D3D11_SDK_VERSION,
        &device, &featureLevel, &context
    );

    if (FAILED(hr)) {
        printf("Failed to create D3D11 device: 0x%08X\n", hr);
        return 1;
    }

    printf("D3D11 Device created (Feature Level: %d)\n", featureLevel);

    // 获取 GPU 信息
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

    printf("\n");

    // 计算需要分配的缓冲区数量
    int numBuffers = (target_mb * 1024ULL * 1024 + BUFFER_SIZE - 1) / BUFFER_SIZE;
    printf("Allocation plan:\n");
    printf("  Buffer size: %d MB each\n", (int)(BUFFER_SIZE / (1024 * 1024)));
    printf("  Number of buffers: %d\n", numBuffers);
    printf("  Total target: %.0f MB\n\n", (double)numBuffers * BUFFER_SIZE / (1024 * 1024));

    // 分配缓冲区数组
    ID3D11Buffer** buffers = (ID3D11Buffer**)calloc(numBuffers, sizeof(ID3D11Buffer*));
    BYTE** cpuData = (BYTE**)calloc(numBuffers, sizeof(BYTE*));
    
    if (!buffers || !cpuData) {
        printf("Failed to allocate arrays\n");
        return 1;
    }

    D3D11_BUFFER_DESC desc = {0};
    desc.ByteWidth = (UINT)BUFFER_SIZE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    if (strategy == 0) {
        // 策略 0: 多个 STAGING 缓冲区
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    } else if (strategy == 1) {
        // 策略 1: DYNAMIC 缓冲区
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    } else {
        // 策略 2: 混合
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
    }

    printf("Allocating buffers...\n");
    int allocated = 0;
    
    for (int i = 0; i < numBuffers; i++) {
        // 对于混合策略，交替使用不同类型
        if (strategy == 2) {
            if (i % 2 == 0) {
                desc.Usage = D3D11_USAGE_STAGING;
                desc.BindFlags = 0;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
            } else {
                desc.Usage = D3D11_USAGE_DYNAMIC;
                desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
                desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
            }
        }

        hr = device->lpVtbl->CreateBuffer(device, &desc, NULL, &buffers[i]);
        if (FAILED(hr)) {
            printf("  Failed to allocate buffer %d: 0x%08X\n", i + 1, hr);
            break;
        }

        allocated++;

        // 策略 1 和 2：Map 缓冲区来强制提交内存
        if (strategy >= 1 || i % 2 == 1) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            D3D11_MAP mapType = (desc.Usage == D3D11_USAGE_DYNAMIC) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE;
            hr = context->lpVtbl->Map(context, (ID3D11Resource*)buffers[i], 0, mapType, 0, &mapped);
            if (SUCCEEDED(hr)) {
                // 写入数据以强制提交
                BYTE* data = (BYTE*)mapped.pData;
                // 只写入部分数据（每页第一个字节）
                for (size_t j = 0; j < BUFFER_SIZE; j += 4096) {
                    data[j] = (BYTE)(i & 0xFF);
                }
                context->lpVtbl->Unmap(context, (ID3D11Resource*)buffers[i], 0);
            }
        }

        if ((i + 1) % 4 == 0 || i == numBuffers - 1) {
            printf("\r  Allocated: %d/%d buffers (%.0f MB)",
                   allocated, numBuffers, (double)allocated * BUFFER_SIZE / (1024 * 1024));
            fflush(stdout);
        }
    }

    printf("\n\nAllocation complete!\n");
    printf("Successfully allocated: %.2f GB\n", (double)allocated * BUFFER_SIZE / (1024 * 1024 * 1024));

    // 再次 Map 所有缓冲区确保内存被提交
    if (strategy == 0) {
        printf("\nTouching memory to ensure commit...\n");
        for (int i = 0; i < allocated; i++) {
            D3D11_MAPPED_SUBRESOURCE mapped;
            hr = context->lpVtbl->Map(context, (ID3D11Resource*)buffers[i], 0, 
                                      D3D11_MAP_WRITE, 0, &mapped);
            if (SUCCEEDED(hr)) {
                BYTE* data = (BYTE*)mapped.pData;
                // 写入数据
                for (size_t j = 0; j < BUFFER_SIZE; j += 4096) {
                    data[j] = (BYTE)(i & 0xFF);
                }
                context->lpVtbl->Unmap(context, (ID3D11Resource*)buffers[i], 0);
            }
            if ((i + 1) % 4 == 0) {
                printf("\r  Touched: %d/%d buffers", i + 1, allocated);
                fflush(stdout);
            }
        }
        printf("\n");
    }

    // 等待
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

    // 清理
    printf("\nReleasing memory...\n");
    for (int i = 0; i < allocated; i++) {
        if (buffers[i]) buffers[i]->lpVtbl->Release(buffers[i]);
    }
    free(buffers);
    free(cpuData);

    if (context) context->lpVtbl->Release(context);
    if (device) device->lpVtbl->Release(device);

    printf("Done!\n");
    return 0;
}
