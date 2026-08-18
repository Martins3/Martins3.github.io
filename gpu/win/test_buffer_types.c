#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxguid.lib")

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <type> [size_mb]\n", argv[0]);
        printf("Types: staging, dynamic, default, immutable\n");
        return 1;
    }
    
    const char* type = argv[1];
    int size_mb = (argc > 2) ? atoi(argv[2]) : 512;
    size_t size = (size_t)size_mb * 1024 * 1024;
    
    ID3D11Device* device = NULL;
    ID3D11DeviceContext* context = NULL;
    D3D_FEATURE_LEVEL featureLevel;
    
    HRESULT hr = D3D11CreateDevice(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, 0,
        NULL, 0, D3D11_SDK_VERSION, &device, &featureLevel, &context);
    if (FAILED(hr)) {
        printf("Failed to create device\n");
        return 1;
    }
    
    D3D11_BUFFER_DESC desc = {0};
    desc.ByteWidth = (UINT)size;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;
    
    if (strcmp(type, "staging") == 0) {
        desc.Usage = D3D11_USAGE_STAGING;
        desc.BindFlags = 0;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_WRITE;
        printf("Creating STAGING buffer: %d MB\n", size_mb);
    } else if (strcmp(type, "dynamic") == 0) {
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        printf("Creating DYNAMIC buffer: %d MB\n", size_mb);
    } else if (strcmp(type, "default") == 0) {
        desc.Usage = D3D11_USAGE_DEFAULT;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        printf("Creating DEFAULT buffer: %d MB\n", size_mb);
    } else if (strcmp(type, "immutable") == 0) {
        desc.Usage = D3D11_USAGE_IMMUTABLE;
        desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        desc.CPUAccessFlags = 0;
        printf("Creating IMMUTABLE buffer: %d MB\n", size_mb);
    } else {
        printf("Unknown type: %s\n", type);
        return 1;
    }
    
    ID3D11Buffer* buffer = NULL;
    hr = device->CreateBuffer(&desc, NULL, &buffer);
    if (FAILED(hr)) {
        printf("Failed to create buffer: 0x%08X\n", hr);
        return 1;
    }
    
    printf("Buffer created successfully\n");
    printf("Press Enter to exit...\n");
    getchar();
    
    buffer->Release();
    context->Release();
    device->Release();
    return 0;
}
