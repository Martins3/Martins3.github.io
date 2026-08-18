/**
 * GPU Shared Memory Real Generator
 * 真正触发 Windows "共享 GPU 内存"增加
 * 
 * 原理：先填满专用显存，迫使驱动将额外分配溢出到系统内存
 * 
 * Windows 任务管理器中的 "共享 GPU 内存" = 
 * 专用显存溢出到系统内存的部分
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

#define GB (1024ULL * 1024 * 1024)
#define MB (1024ULL * 1024)

// 纹理大小: 256MB
#define TEX_SIZE (256 * MB)

void print_memory_stats() {
    // 使用 Windows 性能计数器读取内存状态
    // 这里简化输出，实际应该使用 PDH API
    printf("  (Check Task Manager or run: typeperf \"\\GPU Adapter Memory(*)\\Shared Usage\" -sc 1)\n");
}

int main(int argc, char* argv[]) {
    int dedicated_target_gb = 6;  // 默认填满 6GB 专用显存
    int shared_target_gb = 2;     // 默认再分配 2GB（溢出到共享内存）
    int duration_sec = 10;
    
    if (argc > 1) dedicated_target_gb = atoi(argv[1]);
    if (argc > 2) shared_target_gb = atoi(argv[2]);
    if (argc > 3) duration_sec = atoi(argv[3]);

    printf("================================================\n");
    printf("GPU Shared Memory Real Generator\n");
    printf("================================================\n");
    printf("Step 1: Fill %d GB dedicated VRAM\n", dedicated_target_gb);
    printf("Step 2: Allocate %d GB (will overflow to shared memory)\n", shared_target_gb);
    printf("Duration: %d seconds\n", duration_sec);
    printf("\n");
    printf("Watch in Task Manager -> Performance -> GPU:\n");
    printf("  - Dedicated GPU memory (should max out)\n");
    printf("  - Shared GPU memory (should increase)\n");
    printf("\n");

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
            printf("Dedicated Video Memory: %.0f MB (%.2f GB)\n", 
                   desc.DedicatedVideoMemory / (1024.0 * 1024.0),
                   desc.DedicatedVideoMemory / (double)GB);
            printf("Shared System Memory: %.0f MB (%.2f GB)\n",
                   desc.SharedSystemMemory / (1024.0 * 1024.0),
                   desc.SharedSystemMemory / (double)GB);
            adapter->lpVtbl->Release(adapter);
        }
        dxgiDevice->lpVtbl->Release(dxgiDevice);
    }

    printf("\n");

    // 步骤 1: 填满专用显存
    printf("[Step 1] Filling dedicated VRAM (%d GB)...\n", dedicated_target_gb);
    
    size_t dedicated_bytes = (size_t)dedicated_target_gb * GB;
    int numDedicatedTex = (int)(dedicated_bytes / TEX_SIZE);
    
    ID3D11Texture2D** dedicatedTextures = (ID3D11Texture2D**)calloc(numDedicatedTex, sizeof(ID3D11Texture2D*));
    ID3D11RenderTargetView** rtvs = (ID3D11RenderTargetView**)calloc(numDedicatedTex, sizeof(ID3D11RenderTargetView*));
    
    D3D11_TEXTURE2D_DESC texDesc = {0};
    texDesc.Width = 8192;
    texDesc.Height = 8192;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_RENDER_TARGET;

    int dedicatedAllocated = 0;
    for (int i = 0; i < numDedicatedTex; i++) {
        hr = device->lpVtbl->CreateTexture2D(device, &texDesc, NULL, &dedicatedTextures[i]);
        if (FAILED(hr)) {
            printf("  Failed to allocate texture %d, stopping\n", i + 1);
            break;
        }
        
        // 创建 RTV 确保内存被提交
        device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)dedicatedTextures[i], NULL, &rtvs[i]);
        
        dedicatedAllocated++;
        
        if ((i + 1) % 4 == 0 || i == numDedicatedTex - 1) {
            printf("\r  Allocated: %d textures (%.0f MB)", 
                   dedicatedAllocated, (double)dedicatedAllocated * TEX_SIZE / MB);
            fflush(stdout);
        }
    }
    printf("\n");
    printf("  Dedicated VRAM filled: %.2f GB\n", (double)dedicatedAllocated * TEX_SIZE / GB);
    
    printf("\n  [Check Task Manager - Dedicated GPU memory should be near maximum]\n");
    print_memory_stats();
    
    Sleep(2000);  // 给用户时间观察

    // 步骤 2: 分配额外的内存（溢出到共享内存）
    printf("\n[Step 2] Allocating additional %d GB (will overflow to shared memory)...\n", shared_target_gb);
    
    size_t shared_bytes = (size_t)shared_target_gb * GB;
    int numSharedTex = (int)(shared_bytes / TEX_SIZE);
    
    ID3D11Texture2D** sharedTextures = (ID3D11Texture2D**)calloc(numSharedTex, sizeof(ID3D11Texture2D*));
    ID3D11RenderTargetView** sharedRtvs = (ID3D11RenderTargetView**)calloc(numSharedTex, sizeof(ID3D11RenderTargetView*));
    
    int sharedAllocated = 0;
    for (int i = 0; i < numSharedTex; i++) {
        hr = device->lpVtbl->CreateTexture2D(device, &texDesc, NULL, &sharedTextures[i]);
        if (FAILED(hr)) {
            printf("  Failed to allocate texture %d (this is expected when memory is full)\n", i + 1);
            break;
        }
        
        device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)sharedTextures[i], NULL, &sharedRtvs[i]);
        
        // 清除颜色以强制 GPU 使用内存
        float color[4] = { (i % 4) * 0.25f, ((i + 1) % 4) * 0.25f, ((i + 2) % 4) * 0.25f, 1.0f };
        context->lpVtbl->ClearRenderTargetView(context, sharedRtvs[i], color);
        
        sharedAllocated++;
        
        if ((i + 1) % 2 == 0 || i == numSharedTex - 1) {
            printf("\r  Allocated: %d textures (%.0f MB)", 
                   sharedAllocated, (double)sharedAllocated * TEX_SIZE / MB);
            fflush(stdout);
        }
    }
    printf("\n");
    printf("  Additional memory: %.2f GB (should be in shared memory)\n", 
           (double)sharedAllocated * TEX_SIZE / GB);
    
    context->lpVtbl->Flush(context);
    
    printf("\n  [Check Task Manager - Shared GPU memory should have increased!]\n");
    print_memory_stats();

    // 等待
    printf("\n");
    if (duration_sec > 0) {
        printf("Holding for %d seconds...", duration_sec);
        for (int i = 0; i < duration_sec; i++) {
            Sleep(1000);
            printf(".");
            if ((i + 1) % 10 == 0) {
                printf(" %d/%d", i + 1, duration_sec);
            }
        }
        printf("\n");
    } else {
        printf("Press Enter to release memory...\n");
        getchar();
    }

    // 清理
    printf("\nReleasing memory...\n");
    
    for (int i = 0; i < sharedAllocated; i++) {
        if (sharedRtvs[i]) sharedRtvs[i]->lpVtbl->Release(sharedRtvs[i]);
        if (sharedTextures[i]) sharedTextures[i]->lpVtbl->Release(sharedTextures[i]);
    }
    
    for (int i = 0; i < dedicatedAllocated; i++) {
        if (rtvs[i]) rtvs[i]->lpVtbl->Release(rtvs[i]);
        if (dedicatedTextures[i]) dedicatedTextures[i]->lpVtbl->Release(dedicatedTextures[i]);
    }
    
    free(sharedTextures);
    free(sharedRtvs);
    free(dedicatedTextures);
    free(rtvs);
    
    if (context) context->lpVtbl->Release(context);
    if (device) device->lpVtbl->Release(device);

    printf("Done! Check Task Manager - both memory types should decrease.\n");
    return 0;
}
