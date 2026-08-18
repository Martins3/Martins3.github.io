/**
 * GPU 3D Load Generator
 * 使用 DirectX 11 渲染大量三角形来占用 3D 引擎
 * 在 Windows 任务管理器中可观察到"3D"项的使用率上升
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "user32.lib")

// 简单的顶点着色器 (HLSL compiled to bytes)
// float4 main(float4 pos : POSITION) : SV_POSITION { return pos; }
static const BYTE g_vs_code[] = {
    0x44, 0x58, 0x42, 0x43, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x24, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// 复杂的像素着色器 - 进行大量数学运算以增加 GPU 负载
// 实际代码会在运行时编译
static const char g_ps_source[] =
    "float4 main() : SV_TARGET {\n"
    "    float4 color = float4(0.5, 0.5, 0.5, 1.0);\n"
    "    for(int i = 0; i < 1000; i++) {\n"
    "        color.x = sin(color.x * 1.618 + i * 0.01);\n"
    "        color.y = cos(color.y * 2.718 + i * 0.01);\n"
    "        color.z = sin(color.z * 3.14159 + i * 0.01);\n"
    "    }\n"
    "    return color;\n"
    "}\n";

// 简单的顶点着色器源码
static const char g_vs_source[] =
    "float4 main(float4 pos : POSITION) : SV_POSITION {\n"
    "    return pos;\n"
    "}\n";

struct Vertex {
    float x, y, z, w;
};

static HWND g_hwnd = NULL;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

// Windows entry point for /subsystem:windows
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Convert command line to argc/argv format and call main
    int argc = 0;
    char** argv = NULL;
    
    // Simple command line parsing
    wchar_t* cmdLine = GetCommandLineW();
    int wideLen = lstrlenW(cmdLine);
    int len = WideCharToMultiByte(CP_UTF8, 0, cmdLine, wideLen, NULL, 0, NULL, NULL);
    char* cmdLineA = (char*)malloc(len + 1);
    WideCharToMultiByte(CP_UTF8, 0, cmdLine, wideLen, cmdLineA, len, NULL, NULL);
    cmdLineA[len] = '\0';
    
    // Count arguments
    argc = 1;
    for (char* p = cmdLineA; *p; p++) {
        if (*p == ' ') argc++;
    }
    
    argv = (char**)malloc((argc + 1) * sizeof(char*));
    int idx = 0;
    char* token = strtok(cmdLineA, " ");
    while (token && idx < argc) {
        argv[idx++] = token;
        token = strtok(NULL, " ");
    }
    argv[idx] = NULL;
    argc = idx;
    
    int result = main(argc, argv);
    free(argv);
    free(cmdLineA);
    return result;
}

int main(int argc, char* argv[]) {
    int duration_sec = 30;
    int target_fps = 0;  // 0 = unlimited
    
    if (argc > 1) {
        duration_sec = atoi(argv[1]);
    }
    if (argc > 2) {
        target_fps = atoi(argv[2]);
    }

    printf("GPU 3D Load Generator\n");
    printf("Duration: %d seconds\n", duration_sec);
    if (target_fps > 0) {
        printf("Target FPS: %d (throttled)\n", target_fps);
    } else {
        printf("Target FPS: unlimited (max 3D load)\n");
    }
    printf("Watch '3D' in Windows Task Manager -> Performance -> GPU\n\n");

    // Create window
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"GPU3DLoad";
    RegisterClassEx(&wc);

    g_hwnd = CreateWindowEx(
        0, L"GPU3DLoad", L"GPU 3D Load Generator",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600, NULL, NULL, GetModuleHandle(NULL), NULL
    );
    
    ShowWindow(g_hwnd, SW_SHOW);

    // Create D3D11 device
    DXGI_SWAP_CHAIN_DESC sd = {0};
    sd.BufferCount = 1;
    sd.BufferDesc.Width = 800;
    sd.BufferDesc.Height = 600;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    sd.Flags = 0;

    D3D_FEATURE_LEVEL featureLevel;
    ID3D11Device* device = NULL;
    ID3D11DeviceContext* context = NULL;
    IDXGISwapChain* swapChain = NULL;

    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        NULL, D3D_DRIVER_TYPE_HARDWARE, NULL,
        D3D11_CREATE_DEVICE_SINGLETHREADED,
        NULL, 0, D3D11_SDK_VERSION,
        &sd, &swapChain, &device, &featureLevel, &context
    );

    if (FAILED(hr)) {
        printf("Failed to create D3D11 device: 0x%08X\n", hr);
        return 1;
    }

    printf("D3D11 Device created (Feature Level: %d)\n", featureLevel);

    // Create render target view
    ID3D11Texture2D* backBuffer = NULL;
    hr = swapChain->lpVtbl->GetBuffer(swapChain, 0, &IID_ID3D11Texture2D, (void**)&backBuffer);
    if (FAILED(hr)) {
        printf("Failed to get back buffer\n");
        return 1;
    }

    ID3D11RenderTargetView* rtv = NULL;
    hr = device->lpVtbl->CreateRenderTargetView(device, (ID3D11Resource*)backBuffer, NULL, &rtv);
    backBuffer->lpVtbl->Release(backBuffer);
    if (FAILED(hr)) {
        printf("Failed to create render target view\n");
        return 1;
    }

    // Compile shaders
    ID3DBlob* vsBlob = NULL;
    ID3DBlob* psBlob = NULL;
    ID3DBlob* errorBlob = NULL;

    hr = D3DCompile(g_vs_source, strlen(g_vs_source), NULL, NULL, NULL,
                    "main", "vs_4_0", 0, 0, &vsBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            printf("VS Compile error: %s\n", (char*)errorBlob->lpVtbl->GetBufferPointer(errorBlob));
            errorBlob->lpVtbl->Release(errorBlob);
        }
        return 1;
    }

    hr = D3DCompile(g_ps_source, strlen(g_ps_source), NULL, NULL, NULL,
                    "main", "ps_4_0", 0, 0, &psBlob, &errorBlob);
    if (FAILED(hr)) {
        if (errorBlob) {
            printf("PS Compile error: %s\n", (char*)errorBlob->lpVtbl->GetBufferPointer(errorBlob));
            errorBlob->lpVtbl->Release(errorBlob);
        }
        return 1;
    }

    ID3D11VertexShader* vertexShader = NULL;
    ID3D11PixelShader* pixelShader = NULL;
    device->lpVtbl->CreateVertexShader(device, vsBlob->lpVtbl->GetBufferPointer(vsBlob),
                                       vsBlob->lpVtbl->GetBufferSize(vsBlob), NULL, &vertexShader);
    device->lpVtbl->CreatePixelShader(device, psBlob->lpVtbl->GetBufferPointer(psBlob),
                                      psBlob->lpVtbl->GetBufferSize(psBlob), NULL, &pixelShader);

    // Create input layout
    D3D11_INPUT_ELEMENT_DESC layout[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0}
    };
    ID3D11InputLayout* inputLayout = NULL;
    device->lpVtbl->CreateInputLayout(device, layout, 1,
                                      vsBlob->lpVtbl->GetBufferPointer(vsBlob),
                                      vsBlob->lpVtbl->GetBufferSize(vsBlob), &inputLayout);

    vsBlob->lpVtbl->Release(vsBlob);
    psBlob->lpVtbl->Release(psBlob);

    // Create vertex buffer with many triangles
    // 10000 triangles = 30000 vertices
    const int NUM_TRIANGLES = 10000;
    const int NUM_VERTICES = NUM_TRIANGLES * 3;
    
    struct Vertex* vertices = (struct Vertex*)malloc(NUM_VERTICES * sizeof(struct Vertex));
    if (!vertices) {
        printf("Failed to allocate vertex buffer\n");
        return 1;
    }

    for (int i = 0; i < NUM_VERTICES; i++) {
        vertices[i].x = ((float)(rand() % 2000) / 1000.0f) - 1.0f;
        vertices[i].y = ((float)(rand() % 2000) / 1000.0f) - 1.0f;
        vertices[i].z = 0.5f;
        vertices[i].w = 1.0f;
    }

    D3D11_BUFFER_DESC bd = {0};
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.ByteWidth = NUM_VERTICES * sizeof(struct Vertex);
    bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    bd.CPUAccessFlags = 0;

    D3D11_SUBRESOURCE_DATA initData = {0};
    initData.pSysMem = vertices;

    ID3D11Buffer* vertexBuffer = NULL;
    hr = device->lpVtbl->CreateBuffer(device, &bd, &initData, &vertexBuffer);
    free(vertices);
    if (FAILED(hr)) {
        printf("Failed to create vertex buffer\n");
        return 1;
    }

    // Set viewport
    D3D11_VIEWPORT viewport = {0};
    viewport.Width = 800;
    viewport.Height = 600;
    viewport.MinDepth = 0.0f;
    viewport.MaxDepth = 1.0f;
    viewport.TopLeftX = 0;
    viewport.TopLeftY = 0;
    context->lpVtbl->RSSetViewports(context, 1, &viewport);

    // Main loop
    LARGE_INTEGER freq, start, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    int frameCount = 0;
    double targetFrameTime = target_fps > 0 ? (1.0 / target_fps) : 0;
    LARGE_INTEGER lastFrameTime;
    QueryPerformanceCounter(&lastFrameTime);

    printf("\nRunning... Press Ctrl+C to stop early\n");

    while (1) {
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
        
        if (elapsed >= duration_sec) {
            break;
        }

        // Frame rate limiting
        if (target_fps > 0) {
            double frameTime = (double)(now.QuadPart - lastFrameTime.QuadPart) / freq.QuadPart;
            if (frameTime < targetFrameTime) {
                DWORD sleepMs = (DWORD)((targetFrameTime - frameTime) * 1000);
                if (sleepMs > 0) Sleep(sleepMs);
            }
            QueryPerformanceCounter(&lastFrameTime);
        }

        // Process messages
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (msg.message == WM_QUIT) {
                goto cleanup;
            }
        }

        // Render multiple times per frame to increase load
        const int RENDERS_PER_FRAME = target_fps > 0 ? 1 : 10;
        
        for (int r = 0; r < RENDERS_PER_FRAME; r++) {
            float clearColor[4] = {0.0f, 0.2f, 0.4f, 1.0f};
            context->lpVtbl->ClearRenderTargetView(context, rtv, clearColor);
            context->lpVtbl->OMSetRenderTargets(context, 1, &rtv, NULL);

            UINT stride = sizeof(struct Vertex);
            UINT offset = 0;
            context->lpVtbl->IASetVertexBuffers(context, 0, 1, &vertexBuffer, &stride, &offset);
            context->lpVtbl->IASetInputLayout(context, inputLayout);
            context->lpVtbl->IASetPrimitiveTopology(context, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            context->lpVtbl->VSSetShader(context, vertexShader, NULL, 0);
            context->lpVtbl->PSSetShader(context, pixelShader, NULL, 0);

            context->lpVtbl->Draw(context, NUM_VERTICES, 0);
        }

        swapChain->lpVtbl->Present(swapChain, 0, 0);
        frameCount++;

        if (frameCount % 100 == 0) {
            printf("\rFrame: %d, Time: %.1f/%d sec", frameCount, elapsed, duration_sec);
            fflush(stdout);
        }
    }

cleanup:
    printf("\n\nTotal frames rendered: %d\n", frameCount);
    printf("Average FPS: %.1f\n", frameCount / (double)duration_sec);

    // Cleanup
    if (vertexBuffer) vertexBuffer->lpVtbl->Release(vertexBuffer);
    if (inputLayout) inputLayout->lpVtbl->Release(inputLayout);
    if (vertexShader) vertexShader->lpVtbl->Release(vertexShader);
    if (pixelShader) pixelShader->lpVtbl->Release(pixelShader);
    if (rtv) rtv->lpVtbl->Release(rtv);
    if (swapChain) swapChain->lpVtbl->Release(swapChain);
    if (context) context->lpVtbl->Release(context);
    if (device) device->lpVtbl->Release(device);

    DestroyWindow(g_hwnd);
    UnregisterClass(L"GPU3DLoad", GetModuleHandle(NULL));

    printf("Done!\n");
    return 0;
}
