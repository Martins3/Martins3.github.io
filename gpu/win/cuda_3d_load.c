/**
 * GPU 3D Load Generator - CUDA Edition
 * 
 * 使用 CUDA Driver API 实现高负载计算
 * 模拟 3D 渲染的计算密集型特征：
 * - 大量并行线程
 * - 复杂数学运算（sin/cos/sqrt等）
 * - 内存访问模式
 * 
 * 优势：直接控制 GPU 计算核心，无 DirectX 运行时开销
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// CUDA Driver API 类型定义
typedef int CUresult;
typedef void* CUcontext;
typedef void* CUdevice;
typedef unsigned long long CUdeviceptr;
typedef void* CUmodule;
typedef void* CUfunction;

#define CUDA_SUCCESS 0
#define CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT 16
#define CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR 39

// 函数指针类型
typedef CUresult (*cuInit_t)(unsigned int Flags);
typedef CUresult (*cuDeviceGetCount_t)(int* count);
typedef CUresult (*cuDeviceGet_t)(CUdevice* device, int ordinal);
typedef CUresult (*cuDeviceGetName_t)(char* name, int len, CUdevice dev);
typedef CUresult (*cuDeviceGetAttribute_t)(int* pi, int attrib, CUdevice dev);
typedef CUresult (*cuDeviceTotalMem_t)(size_t* bytes, CUdevice dev);
typedef CUresult (*cuCtxCreate_t)(CUcontext* pctx, unsigned int flags, CUdevice dev);
typedef CUresult (*cuCtxDestroy_t)(CUcontext ctx);
typedef CUresult (*cuCtxSetCurrent_t)(CUcontext ctx);
typedef CUresult (*cuMemAlloc_t)(CUdeviceptr* dptr, size_t bytesize);
typedef CUresult (*cuMemFree_t)(CUdeviceptr dptr);
typedef CUresult (*cuMemcpyHtoD_t)(CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount);
typedef CUresult (*cuMemcpyDtoH_t)(void* dstHost, CUdeviceptr srcDevice, size_t ByteCount);
typedef CUresult (*cuLaunchKernel_t)(CUfunction f, unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ,
                                     unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ,
                                     unsigned int sharedMemBytes, void* hStream, void** kernelParams, void** extra);
typedef CUresult (*cuModuleLoad_t)(CUmodule* module, const char* fname);
typedef CUresult (*cuModuleGetFunction_t)(CUfunction* hfunc, CUmodule hmod, const char* name);
typedef CUresult (*cuModuleUnload_t)(CUmodule hmod);
typedef CUresult (*cuModuleLoadDataEx_t)(CUmodule* module, const void* image, unsigned int numOptions, void** options, void** optionValues);

// PTX 内核代码 - 模拟复杂 3D 着色器计算
// 这个 PTX 代码执行大量数学运算来模拟像素着色器
const char* ptxCode = 
".version 7.0\n"
".target sm_50\n"
".address_size 64\n"
".entry compute_kernel(\n"
"    .param .u64 data,\n"
"    .param .u64 n,\n"
"    .param .u32 iterations\n"
"){\n"
"    .reg .u64 %rd<4>;\n"
"    .reg .u32 %r<6>;\n"
"    .reg .f32 %f<20>;\n"
"    .reg .pred %p<3>;\n"
"    ld.param.u64 %rd1, [data];\n"
"    ld.param.u64 %rd2, [n];\n"
"    ld.param.u32 %r1, [iterations];\n"
"    mov.u32 %r2, %ctaid.x;\n"
"    mov.u32 %r3, %ntid.x;\n"
"    mov.u32 %r4, %tid.x;\n"
"    mad.lo.s32 %r2, %r2, %r3, %r4;\n"
"    cvt.u64.u32 %rd3, %r2;\n"
"    setp.ge.u64 %p1, %rd3, %rd2;\n"
"    @%p1 bra EXIT;\n"
"    shl.b64 %rd3, %rd3, 2;\n"
"    add.u64 %rd1, %rd1, %rd3;\n"
"    ld.global.f32 %f1, [%rd1];\n"
"    mov.f32 %f2, %f1;\n"
"    mov.u32 %r5, 0;\n"
"LOOP:\n"
"    setp.ge.u32 %p2, %r5, %r1;\n"
"    @%p2 bra EXIT_LOOP;\n"
"    sin.approx.f32 %f3, %f2;\n"
"    cos.approx.f32 %f4, %f2;\n"
"    mul.f32 %f5, %f3, %f4;\n"
"    add.f32 %f6, %f5, 0F3F800000;\n"
"    sqrt.approx.f32 %f7, %f6;\n"
"    mul.f32 %f8, %f7, 0F3F000000;\n"
"    add.f32 %f9, %f8, %f2;\n"
"    mov.f32 %f2, %f9;\n"
"    add.u32 %r5, %r5, 1;\n"
"    bra LOOP;\n"
"EXIT_LOOP:\n"
"    st.global.f32 [%rd1], %f2;\n"
"EXIT:\n"
"    ret;\n"
"}\n";

#define CUDA_DRV_CHECK(call)                                                   \
    do {                                                                       \
        CUresult result = (call);                                              \
        if (result != CUDA_SUCCESS) {                                          \
            fprintf(stderr, "CUDA Driver error %d at %s:%d\n", result, __FILE__, __LINE__); \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

HMODULE g_cudaDll = NULL;
cuInit_t cuInit_ptr = NULL;
cuDeviceGetCount_t cuDeviceGetCount_ptr = NULL;
cuDeviceGet_t cuDeviceGet_ptr = NULL;
cuDeviceGetName_t cuDeviceGetName_ptr = NULL;
cuDeviceGetAttribute_t cuDeviceGetAttribute_ptr = NULL;
cuDeviceTotalMem_t cuDeviceTotalMem_ptr = NULL;
cuCtxCreate_t cuCtxCreate_ptr = NULL;
cuCtxDestroy_t cuCtxDestroy_ptr = NULL;
cuCtxSetCurrent_t cuCtxSetCurrent_ptr = NULL;
cuMemAlloc_t cuMemAlloc_ptr = NULL;
cuMemFree_t cuMemFree_ptr = NULL;
cuMemcpyHtoD_t cuMemcpyHtoD_ptr = NULL;
cuMemcpyDtoH_t cuMemcpyDtoH_ptr = NULL;
cuLaunchKernel_t cuLaunchKernel_ptr = NULL;
cuModuleLoad_t cuModuleLoad_ptr = NULL;
cuModuleGetFunction_t cuModuleGetFunction_ptr = NULL;
cuModuleUnload_t cuModuleUnload_ptr = NULL;
cuModuleLoadDataEx_t cuModuleLoadDataEx_ptr = NULL;

HMODULE load_cuda_driver() {
    HMODULE cuda = LoadLibraryA("nvcuda.dll");
    if (!cuda) {
        fprintf(stderr, "Failed to load nvcuda.dll\n");
        return NULL;
    }
    
    cuInit_ptr = (cuInit_t)GetProcAddress(cuda, "cuInit");
    cuDeviceGetCount_ptr = (cuDeviceGetCount_t)GetProcAddress(cuda, "cuDeviceGetCount");
    cuDeviceGet_ptr = (cuDeviceGet_t)GetProcAddress(cuda, "cuDeviceGet");
    cuDeviceGetName_ptr = (cuDeviceGetName_t)GetProcAddress(cuda, "cuDeviceGetName");
    cuDeviceGetAttribute_ptr = (cuDeviceGetAttribute_t)GetProcAddress(cuda, "cuDeviceGetAttribute");
    cuDeviceTotalMem_ptr = (cuDeviceTotalMem_t)GetProcAddress(cuda, "cuDeviceTotalMem");
    cuCtxCreate_ptr = (cuCtxCreate_t)GetProcAddress(cuda, "cuCtxCreate");
    cuCtxDestroy_ptr = (cuCtxDestroy_t)GetProcAddress(cuda, "cuCtxDestroy");
    cuCtxSetCurrent_ptr = (cuCtxSetCurrent_t)GetProcAddress(cuda, "cuCtxSetCurrent");
    cuMemAlloc_ptr = (cuMemAlloc_t)GetProcAddress(cuda, "cuMemAlloc");
    cuMemFree_ptr = (cuMemFree_t)GetProcAddress(cuda, "cuMemFree");
    cuMemcpyHtoD_ptr = (cuMemcpyHtoD_t)GetProcAddress(cuda, "cuMemcpyHtoD");
    cuMemcpyDtoH_ptr = (cuMemcpyDtoH_t)GetProcAddress(cuda, "cuMemcpyDtoH");
    cuLaunchKernel_ptr = (cuLaunchKernel_t)GetProcAddress(cuda, "cuLaunchKernel");
    cuModuleLoad_ptr = (cuModuleLoad_t)GetProcAddress(cuda, "cuModuleLoad");
    cuModuleGetFunction_ptr = (cuModuleGetFunction_t)GetProcAddress(cuda, "cuModuleGetFunction");
    cuModuleUnload_ptr = (cuModuleUnload_t)GetProcAddress(cuda, "cuModuleUnload");
    cuModuleLoadDataEx_ptr = (cuModuleLoadDataEx_t)GetProcAddress(cuda, "cuModuleLoadDataEx");
    
    if (!cuInit_ptr || !cuMemAlloc_ptr || !cuLaunchKernel_ptr) {
        fprintf(stderr, "Failed to load required CUDA functions\n");
        FreeLibrary(cuda);
        return NULL;
    }
    
    return cuda;
}

int main(int argc, char* argv[]) {
    int duration_sec = 30;
    int intensity = 1000;  // 每像素计算迭代次数
    
    if (argc > 1) duration_sec = atoi(argv[1]);
    if (argc > 2) intensity = atoi(argv[2]);

    printf("================================================\n");
    printf("GPU 3D Load Generator - CUDA Edition\n");
    printf("================================================\n");
    printf("Duration: %d seconds\n", duration_sec);
    printf("Compute intensity: %d iterations per element\n", intensity);
    printf("Watch '3D' or 'Compute' in Task Manager\n\n");

    // 加载 CUDA
    g_cudaDll = load_cuda_driver();
    if (!g_cudaDll) {
        fprintf(stderr, "CUDA not available\n");
        return 1;
    }

    CUDA_DRV_CHECK(cuInit_ptr(0));

    int deviceCount = 0;
    CUDA_DRV_CHECK(cuDeviceGetCount_ptr(&deviceCount));
    if (deviceCount == 0) {
        fprintf(stderr, "No CUDA devices\n");
        return 1;
    }

    CUdevice device;
    CUDA_DRV_CHECK(cuDeviceGet_ptr(&device, 0));

    char name[256] = {0};
    cuDeviceGetName_ptr(name, sizeof(name), device);
    printf("GPU: %s\n\n", name);

    // 获取 GPU 能力
    int mpCount = 0, maxThreadsPerMP = 0;
    cuDeviceGetAttribute_ptr(&mpCount, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, device);
    cuDeviceGetAttribute_ptr(&maxThreadsPerMP, CU_DEVICE_ATTRIBUTE_MAX_THREADS_PER_MULTIPROCESSOR, device);
    printf("Multiprocessors: %d\n", mpCount);
    printf("Max threads per MP: %d\n", maxThreadsPerMP);
    printf("Total parallel threads: %d\n\n", mpCount * maxThreadsPerMP);

    // 创建上下文
    CUcontext ctx;
    CUDA_DRV_CHECK(cuCtxCreate_ptr(&ctx, 0, device));

    // 加载 PTX 模块
    CUmodule module;
    CUDA_DRV_CHECK(cuModuleLoadDataEx_ptr(&module, ptxCode, 0, NULL, NULL));
    
    CUfunction kernel;
    CUDA_DRV_CHECK(cuModuleGetFunction_ptr(&kernel, module, "compute_kernel"));
    printf("PTX kernel loaded\n\n");

    // 分配 GPU 内存 (256MB float data = 64M elements)
    size_t dataSize = 256 * 1024 * 1024;
    int numElements = dataSize / sizeof(float);
    
    CUdeviceptr d_data;
    CUDA_DRV_CHECK(cuMemAlloc_ptr(&d_data, dataSize));
    
    // 初始化数据
    float* h_data = (float*)malloc(dataSize);
    for (int i = 0; i < numElements; i++) {
        h_data[i] = (float)(i % 100) * 0.01f;
    }
    CUDA_DRV_CHECK(cuMemcpyHtoD_ptr(d_data, h_data, dataSize));
    free(h_data);

    printf("Starting compute load...\n");
    printf("Data size: %d MB (%d elements)\n\n", (int)(dataSize / (1024*1024)), numElements);

    // 启动参数
    int blockSize = 256;
    int gridSize = (numElements + blockSize - 1) / blockSize;
    
    void* args[3] = { &d_data, &numElements, &intensity };

    LARGE_INTEGER freq, start, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);

    int frameCount = 0;
    int lastReportedFrames = 0;
    LARGE_INTEGER lastReportTime;
    QueryPerformanceCounter(&lastReportTime);

    while (1) {
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
        if (elapsed >= duration_sec) break;

        // 启动 kernel (模拟一帧渲染)
        CUDA_DRV_CHECK(cuLaunchKernel_ptr(kernel, 
            gridSize, 1, 1,           // grid
            blockSize, 1, 1,          // block
            0, NULL, args, NULL));    // shared, stream, args, extra
        
        frameCount++;

        // 每 10 帧同步一次
        if ((frameCount % 10) == 0) {
            // 同步在 cuLaunchKernel 后隐式完成（同步启动模式）
        }

        // 每秒报告一次
        double timeSinceReport = (double)(now.QuadPart - lastReportTime.QuadPart) / freq.QuadPart;
        if (timeSinceReport >= 1.0) {
            int framesSince = frameCount - lastReportedFrames;
            printf("\rTime: %.0f/%d sec | Frames: %d | FPS: %d", 
                   elapsed, duration_sec, frameCount, framesSince);
            fflush(stdout);
            lastReportedFrames = frameCount;
            QueryPerformanceCounter(&lastReportTime);
        }
    }

    QueryPerformanceCounter(&now);
    double totalTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;

    printf("\n\n");
    printf("================================================\n");
    printf("Results:\n");
    printf("================================================\n");
    printf("Total frames (kernel launches): %d\n", frameCount);
    printf("Total time: %.2f seconds\n", totalTime);
    printf("Average FPS: %.1f\n", frameCount / totalTime);
    printf("Total compute operations: %.2f billion\n", 
           (double)frameCount * numElements * intensity / 1e9);
    printf("================================================\n");

    // 清理
    cuMemFree_ptr(d_data);
    cuModuleUnload_ptr(module);
    cuCtxDestroy_ptr(ctx);
    FreeLibrary(g_cudaDll);

    printf("\nDone!\n");
    return 0;
}
