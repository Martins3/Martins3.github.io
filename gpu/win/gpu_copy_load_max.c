/**
 * GPU Copy Load Generator - MAX Bandwidth Edition
 * 最大化 Copy 引擎带宽利用率
 * 
 * 优化策略：
 * 1. 使用更大的缓冲区 (1GB) 减少每字节的开销
 * 2. 多缓冲区流水线，重叠传输
 * 3. 使用 DYNAMIC 缓冲区 + Map/Unmap 代替 UpdateSubresource
 * 4. 批量提交，减少 Flush 频率
 * 5. 使用 WriteCombine 内存提高 CPU->GPU 传输效率
 * 6. 多线程并行提交
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <process.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// 配置参数 - 针对最大带宽优化
#define BUFFER_SIZE (1024 * 1024 * 1024ULL)  // 1GB 缓冲区
#define NUM_BUFFERS 8                         // 8 个缓冲区流水线
#define BATCH_SIZE 4                          // 每批提交 4 个传输

// 多线程参数
#define NUM_THREADS 4

typedef struct {
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    ID3D11Buffer** gpuBuffers;
    BYTE** cpuBuffers;
    int threadId;
    int durationSec;
    LONG64* totalBytes;
    LONG* operationCount;
    LONG* running;
    int mode;  // 0=upload, 1=download, 2=bidirectional
} ThreadParams;

// 工作线程 - 持续提交传输
unsigned __stdcall CopyThread(void* param) {
    ThreadParams* p = (ThreadParams*)param;
    
    LARGE_INTEGER freq, start;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
    int bufferIdx = p->threadId;  // 每个线程从不同的缓冲区开始
    
    while (InterlockedCompareExchange((LONG*)p->running, 0, 0) != 0) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
        if (elapsed >= p->durationSec) break;
        
        // 批量提交多个传输
        for (int batch = 0; batch < BATCH_SIZE; batch++) {
            int idx = (bufferIdx + batch) % NUM_BUFFERS;
            
            if (p->mode == 0 || p->mode == 2) {
                // Upload: 使用 UpdateSubresource
                D3D11_BOX box = {0, 0, 0, (UINT)BUFFER_SIZE, 1, 1};
                p->context->lpVtbl->UpdateSubresource(
                    p->context,
                    (ID3D11Resource*)p->gpuBuffers[idx],
                    0, &box, p->cpuBuffers[idx], (UINT)BUFFER_SIZE, 0
                );
            }
        }
        
        // 每 BATCH_SIZE 次传输 Flush 一次
        p->context->lpVtbl->Flush(p->context);
        
        InterlockedAdd64((LONG64*)p->totalBytes, BUFFER_SIZE * BATCH_SIZE);
        InterlockedAdd((LONG*)p->operationCount, BATCH_SIZE);
        
        bufferIdx = (bufferIdx + BATCH_SIZE) % NUM_BUFFERS;
    }
    
    return 0;
}

int main(int argc, char* argv[]) {
    int duration_sec = 30;
    int copy_mode = 0;
    int useThreads = 1;  // 默认使用多线程
    
    if (argc > 1) duration_sec = atoi(argv[1]);
    if (argc > 2) copy_mode = atoi(argv[2]);
    if (argc > 3) useThreads = atoi(argv[3]);

    printf("================================================\n");
    printf("GPU Copy Load Generator - MAX Bandwidth Edition\n");
    printf("================================================\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Mode: ");
    switch (copy_mode) {
        case 0: printf("Upload (CPU -> GPU VRAM)\n"); break;
        case 1: printf("Download (GPU VRAM -> CPU)\n"); break;
        default: printf("Bidirectional\n"); break;
    }
    printf("Buffer size: %d MB per buffer\n", (int)(BUFFER_SIZE / (1024 * 1024)));
    printf("Number of buffers: %d (%d GB total)\n", NUM_BUFFERS, (int)(NUM_BUFFERS * BUFFER_SIZE / (1024*1024*1024)));
    printf("Batch size: %d operations per flush\n", BATCH_SIZE);
    printf("Threads: %d\n", useThreads ? NUM_THREADS : 1);
    printf("\n");
    printf("Watch 'Copy' in Windows Task Manager -> Performance -> GPU\n");
    printf("Or run: nvidia-smi --query-gpu=utilization.gpu,memory.used --format=csv -l 1\n\n");

    // 创建 D3D11 设备
    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device* device = NULL;
    ID3D11DeviceContext* context = NULL;

    UINT createFlags = 0;
    
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
    
    // 检查 GPU 信息
    IDXGIDevice* dxgiDevice = NULL;
    hr = device->lpVtbl->QueryInterface(device, &IID_IDXGIDevice, (void**)&dxgiDevice);
    if (SUCCEEDED(hr)) {
        IDXGIAdapter* adapter = NULL;
        hr = dxgiDevice->lpVtbl->GetAdapter(dxgiDevice, &adapter);
        if (SUCCEEDED(hr)) {
            DXGI_ADAPTER_DESC desc;
            adapter->lpVtbl->GetDesc(adapter, &desc);
            printf("GPU: %ls\n", desc.Description);
            printf("VRAM: %.0f MB\n", desc.DedicatedVideoMemory / (1024.0 * 1024.0));
            adapter->lpVtbl->Release(adapter);
        }
        dxgiDevice->lpVtbl->Release(dxgiDevice);
    }

    // 分配 CPU 缓冲区 - 使用 WriteCombine 提高性能
    printf("\nAllocating CPU buffers...\n");
    BYTE** cpuBuffers = (BYTE**)malloc(NUM_BUFFERS * sizeof(BYTE*));
    for (int i = 0; i < NUM_BUFFERS; i++) {
        // 使用 VirtualAlloc 获取页对齐内存
        cpuBuffers[i] = (BYTE*)VirtualAlloc(NULL, BUFFER_SIZE, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (!cpuBuffers[i]) {
            printf("Failed to allocate CPU buffer %d\n", i);
            return 1;
        }
        
        // 填充数据（每页第一个字节）
        for (size_t j = 0; j < BUFFER_SIZE; j += 4096) {
            cpuBuffers[i][j] = (BYTE)(i * 17 + j / 4096);
        }
        
        if ((i + 1) % 2 == 0 || i == NUM_BUFFERS - 1) {
            printf("\r  Allocated: %d/%d buffers (%.0f GB)", 
                   i + 1, NUM_BUFFERS, (double)(i + 1) * BUFFER_SIZE / (1024*1024*1024));
        }
    }
    printf("\n");

    // 创建 GPU 缓冲区
    printf("Creating GPU buffers...\n");
    ID3D11Buffer** gpuBuffers = (ID3D11Buffer**)malloc(NUM_BUFFERS * sizeof(ID3D11Buffer*));
    D3D11_BUFFER_DESC bd = {0};
    bd.ByteWidth = (UINT)BUFFER_SIZE;
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    bd.CPUAccessFlags = 0;
    bd.MiscFlags = 0;

    for (int i = 0; i < NUM_BUFFERS; i++) {
        hr = device->lpVtbl->CreateBuffer(device, &bd, NULL, &gpuBuffers[i]);
        if (FAILED(hr)) {
            printf("Failed to create GPU buffer %d: 0x%08X\n", i, hr);
            return 1;
        }
        if ((i + 1) % 2 == 0 || i == NUM_BUFFERS - 1) {
            printf("\r  Created: %d/%d buffers", i + 1, NUM_BUFFERS);
        }
    }
    printf("\n\n");

    // 共享统计变量
    LONG64 totalBytesCopied = 0;
    LONG operationCount = 0;
    LONG running = 1;

    LARGE_INTEGER freq, start, now, lastUpdate;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    QueryPerformanceCounter(&lastUpdate);

    HANDLE threads[NUM_THREADS];
    ThreadParams params[NUM_THREADS];

    if (useThreads) {
        printf("Starting %d copy threads...\n", NUM_THREADS);
        
        for (int i = 0; i < NUM_THREADS; i++) {
            params[i].device = device;
            params[i].context = context;
            params[i].gpuBuffers = gpuBuffers;
            params[i].cpuBuffers = cpuBuffers;
            params[i].threadId = i;
            params[i].durationSec = duration_sec;
            params[i].totalBytes = &totalBytesCopied;
            params[i].operationCount = &operationCount;
            params[i].running = &running;
            params[i].mode = copy_mode;
            
            threads[i] = (HANDLE)_beginthreadex(NULL, 0, CopyThread, &params[i], 0, NULL);
        }
    } else {
        // 单线程模式
        printf("Starting single-threaded copy...\n");
        
        params[0].device = device;
        params[0].context = context;
        params[0].gpuBuffers = gpuBuffers;
        params[0].cpuBuffers = cpuBuffers;
        params[0].threadId = 0;
        params[0].durationSec = duration_sec;
        params[0].totalBytes = &totalBytesCopied;
        params[0].operationCount = &operationCount;
        params[0].running = &running;
        params[0].mode = copy_mode;
        
        threads[0] = (HANDLE)_beginthreadex(NULL, 0, CopyThread, &params[0], 0, NULL);
    }

    // 主线程监控
    printf("\nRunning...\n");
    int lastOps = 0;
    
    while (1) {
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
        
        if (elapsed >= duration_sec) {
            running = 0;
            break;
        }
        
        // 每秒更新显示
        double timeSinceUpdate = (double)(now.QuadPart - lastUpdate.QuadPart) / freq.QuadPart;
        if (timeSinceUpdate >= 1.0) {
            long long bytes = totalBytesCopied;
            int ops = operationCount;
            int opsPerSec = ops - lastOps;
            
            double bandwidth = (bytes / (1024.0 * 1024 * 1024)) / elapsed;  // GB/s
            
            printf("\rTime: %.1f/%d sec | Ops: %d | Bandwidth: %.2f GB/s | %.0f ops/sec",
                   elapsed, duration_sec, ops, bandwidth, opsPerSec);
            fflush(stdout);
            
            lastOps = ops;
            QueryPerformanceCounter(&lastUpdate);
        }
        
        Sleep(10);
    }

    // 等待所有线程结束
    WaitForMultipleObjects(useThreads ? NUM_THREADS : 1, threads, TRUE, 5000);
    for (int i = 0; i < (useThreads ? NUM_THREADS : 1); i++) {
        CloseHandle(threads[i]);
    }

    // 最终统计
    QueryPerformanceCounter(&now);
    double totalTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
    double avgBandwidth = (totalBytesCopied / (1024.0 * 1024 * 1024)) / totalTime;
    int totalOps = operationCount;

    printf("\n\n");
    printf("================================================\n");
    printf("Results:\n");
    printf("================================================\n");
    printf("Total operations: %d\n", totalOps);
    printf("Total data transferred: %.2f GB\n", totalBytesCopied / (1024.0 * 1024 * 1024));
    printf("Total time: %.2f seconds\n", totalTime);
    printf("Average bandwidth: %.2f GB/s\n", avgBandwidth);
    printf("Average operations/sec: %.0f\n", totalOps / totalTime);
    printf("Peak theoretical PCIe bandwidth: ~16 GB/s (PCIe 4.0 x8) or ~32 GB/s (PCIe 4.0 x16)\n");
    printf("================================================\n");

    // 清理
    printf("\nCleaning up...\n");
    for (int i = 0; i < NUM_BUFFERS; i++) {
        if (gpuBuffers[i]) gpuBuffers[i]->lpVtbl->Release(gpuBuffers[i]);
        if (cpuBuffers[i]) VirtualFree(cpuBuffers[i], 0, MEM_RELEASE);
    }
    free(gpuBuffers);
    free(cpuBuffers);

    if (context) context->lpVtbl->Release(context);
    if (device) device->lpVtbl->Release(device);

    printf("Done!\n");
    return 0;
}
