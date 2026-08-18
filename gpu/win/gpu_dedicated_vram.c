/**
 * GPU Dedicated VRAM Allocator
 * 分配 GPU 专用显存（独占的 VRAM）
 * 在 Windows 任务管理器中可观察到"专用 GPU 内存"的使用量上升
 * 
 * 原理: 在 GPU 上创建大的纹理/缓冲区，这些内存是 GPU 独占的
 * 专用显存 = GPU 卡上的物理显存（GDDR6/GDDR6X/HBM2等）
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

// 每次分配的块大小: 512MB
#define ALLOC_CHUNK_SIZE (512ULL * 1024 * 1024)

int main(int argc, char* argv[]) {
    int target_mb = 2048;  // 默认目标: 2GB
    int duration_sec = 0;  // 默认: 一直运行直到按键
    
    if (argc > 1) {
        target_mb = atoi(argv[1]);
    }
    if (argc > 2) {
        duration_sec = atoi(argv[2]);
    }

    printf("GPU Dedicated VRAM Allocator\n");
    printf("Target allocation: %d MB (%.2f GB)\n", target_mb, target_mb / 1024.0);
    if (duration_sec > 0) {
        printf("Duration: %d seconds\n", duration_sec);
    } else {
        printf("Duration: unlimited (press Enter to exit)\n");
    }
    printf("Watch 'Dedicated GPU memory' in Windows Task Manager -> Performance -> GPU\n\n");

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

    // Calculate number of textures needed
    // Use 4K RGBA32 textures (4096 * 4096 * 4 = 64MB each)
    // Or larger ones for efficiency
    const int TEX_WIDTH = 8192;
    const int TEX_HEIGHT = 8192;
    const size_t BYTES_PER_TEX = (size_t)TEX_WIDTH * TEX_HEIGHT * 4;  // RGBA32 = 256MB

    int numTextures = (target_mb * 1024ULL * 1024 + BYTES_PER_TEX - 1) / BYTES_PER_TEX;
    size_t actualAlloc = (size_t)numTextures * BYTES_PER_TEX;

    printf("\nAllocation plan:\n");
    printf("  Texture size: %dx%d (%.0f MB each)\n", TEX_WIDTH, TEX_HEIGHT, BYTES_PER_TEX / (1024.0 * 1024.0));
    printf("  Number of textures: %d\n", numTextures);
    printf("  Total allocation: %.0f MB\n\n", actualAlloc / (1024.0 * 1024.0));

    // Allocate textures
    ID3D11Texture2D** textures = (ID3D11Texture2D**)calloc(numTextures, sizeof(ID3D11Texture2D*));
    if (!textures) {
        printf("Failed to allocate texture array\n");
        return 1;
    }

    D3D11_TEXTURE2D_DESC desc = {0};
    desc.Width = TEX_WIDTH;
    desc.Height = TEX_HEIGHT;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
    desc.Usage = D3D11_USAGE_DEFAULT;
    // Use RENDER_TARGET to ensure it's in dedicated VRAM
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;

    printf("Allocating textures...\n");
    int allocated = 0;
    size_t totalAllocated = 0;

    for (int i = 0; i < numTextures; i++) {
        hr = device->lpVtbl->CreateTexture2D(device, &desc, NULL, &textures[i]);
        if (FAILED(hr)) {
            printf("  Failed to allocate texture %d: 0x%08X\n", i + 1, hr);
            break;
        }

        allocated++;
        totalAllocated += BYTES_PER_TEX;

        // Progress update every 4 textures or on last one
        if ((i + 1) % 4 == 0 || i == numTextures - 1) {
            printf("\r  Allocated: %d/%d textures (%.0f MB)",
                   allocated, numTextures, totalAllocated / (1024.0 * 1024.0));
            fflush(stdout);
        }
    }

    printf("\n\nAllocation complete!\n");
    printf("Successfully allocated: %.2f GB\n", totalAllocated / (1024.0 * 1024.0 * 1024.0));

    // Verify by touching the memory (optional - forces it to be committed)
    printf("\nTouching memory to ensure it's committed...\n");
    
    ID3D11RenderTargetView** rtvs = (ID3D11RenderTargetView**)calloc(allocated, sizeof(ID3D11RenderTargetView*));
    for (int i = 0; i < allocated; i++) {
        device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)textures[i], NULL, &rtvs[i]);
    }

    // Clear each texture to force GPU to actually use the memory
    float colors[][4] = {
        {1.0f, 0.0f, 0.0f, 1.0f},  // Red
        {0.0f, 1.0f, 0.0f, 1.0f},  // Green
        {0.0f, 0.0f, 1.0f, 1.0f},  // Blue
        {1.0f, 1.0f, 0.0f, 1.0f},  // Yellow
    };

    for (int i = 0; i < allocated; i++) {
        context->lpVtbl->ClearRenderTargetView(context, rtvs[i], colors[i % 4]);
    }
    context->lpVtbl->Flush(context);
    printf("Memory touched and flushed to GPU\n");

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
        if (rtvs[i]) rtvs[i]->lpVtbl->Release(rtvs[i]);
        if (textures[i]) textures[i]->lpVtbl->Release(textures[i]);
    }
    free(rtvs);
    free(textures);

    if (context) context->lpVtbl->Release(context);
    if (device) device->lpVtbl->Release(device);

    printf("Done! Check Task Manager - GPU memory should decrease.\n");
    return 0;
}
