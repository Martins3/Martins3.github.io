/**
 * GPU Load Generator - CUDA Driver API Edition
 * 
 * 使用 CUDA Driver API 直接操作 GPU，不依赖 CUDA Runtime
 * 只需 nvcuda.dll (随显卡驱动安装)
 * 
 * 参考 llama.cpp 的 CUDA 实现风格
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// CUDA Driver API 类型定义
typedef int CUresult;
typedef void* CUcontext;
typedef void* CUdevice;
typedef unsigned long long CUdeviceptr;
typedef void* CUstream;
typedef void* CUmodule;
typedef void* CUfunction;

#define CUDA_SUCCESS 0
#define CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT 16
#define CU_DEVICE_ATTRIBUTE_CLOCK_RATE 13
#define CU_DEVICE_ATTRIBUTE_MEMORY_CLOCK_RATE 21
#define CU_DEVICE_ATTRIBUTE_GLOBAL_MEMORY_BUS_WIDTH 37
#define CU_DEVICE_ATTRIBUTE_CONCURRENT_KERNELS 31
#define CU_DEVICE_ATTRIBUTE_ASYNC_ENGINE_COUNT 128

#define CU_MEMHOSTALLOC_PORTABLE 0x01
#define CU_MEMHOSTALLOC_DEVICEMAP 0x02
#define CU_MEMHOSTALLOC_WRITECOMBINED 0x04

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
typedef CUresult (*cuMemAllocHost_t)(void** pp, size_t bytesize);
typedef CUresult (*cuMemFreeHost_t)(void* p);
typedef CUresult (*cuMemcpyHtoD_t)(CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount);
typedef CUresult (*cuMemcpyDtoH_t)(void* dstHost, CUdeviceptr srcDevice, size_t ByteCount);
typedef CUresult (*cuMemcpyHtoDAsync_t)(CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount, CUstream hStream);
typedef CUresult (*cuMemcpyDtoHAsync_t)(void* dstHost, CUdeviceptr srcDevice, size_t ByteCount, CUstream hStream);
typedef CUresult (*cuStreamCreate_t)(CUstream* phStream, unsigned int Flags);
typedef CUresult (*cuStreamDestroy_t)(CUstream hStream);
typedef CUresult (*cuStreamSynchronize_t)(CUstream hStream);
typedef CUresult (*cuLaunchKernel_t)(CUfunction f, unsigned int gridDimX, unsigned int gridDimY, unsigned int gridDimZ,
                                     unsigned int blockDimX, unsigned int blockDimY, unsigned int blockDimZ,
                                     unsigned int sharedMemBytes, CUstream hStream, void** kernelParams, void** extra);
typedef CUresult (*cuModuleLoad_t)(CUmodule* module, const char* fname);
typedef CUresult (*cuModuleGetFunction_t)(CUfunction* hfunc, CUmodule hmod, const char* name);
typedef CUresult (*cuModuleUnload_t)(CUmodule hmod);

// 全局函数指针
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
cuMemAllocHost_t cuMemAllocHost_ptr = NULL;
cuMemFreeHost_t cuMemFreeHost_ptr = NULL;
cuMemcpyHtoD_t cuMemcpyHtoD_ptr = NULL;
cuMemcpyDtoH_t cuMemcpyDtoH_ptr = NULL;
cuMemcpyHtoDAsync_t cuMemcpyHtoDAsync_ptr = NULL;
cuMemcpyDtoHAsync_t cuMemcpyDtoHAsync_ptr = NULL;
cuStreamCreate_t cuStreamCreate_ptr = NULL;
cuStreamDestroy_t cuStreamDestroy_ptr = NULL;
cuStreamSynchronize_t cuStreamSynchronize_ptr = NULL;

// 加载 CUDA Driver API
HMODULE load_cuda_driver() {
    HMODULE cuda = LoadLibraryA("nvcuda.dll");
    if (!cuda) {
        fprintf(stderr, "Failed to load nvcuda.dll\n");
        fprintf(stderr, "Please ensure NVIDIA GPU drivers are installed.\n");
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
    cuMemAllocHost_ptr = (cuMemAllocHost_t)GetProcAddress(cuda, "cuMemAllocHost");
    cuMemFreeHost_ptr = (cuMemFreeHost_t)GetProcAddress(cuda, "cuMemFreeHost");
    cuMemcpyHtoD_ptr = (cuMemcpyHtoD_t)GetProcAddress(cuda, "cuMemcpyHtoD");
    cuMemcpyDtoH_ptr = (cuMemcpyDtoH_t)GetProcAddress(cuda, "cuMemcpyDtoH");
    cuMemcpyHtoDAsync_ptr = (cuMemcpyHtoDAsync_t)GetProcAddress(cuda, "cuMemcpyHtoDAsync_v2");
    cuMemcpyDtoHAsync_ptr = (cuMemcpyDtoHAsync_t)GetProcAddress(cuda, "cuMemcpyDtoHAsync_v2");
    cuStreamCreate_ptr = (cuStreamCreate_t)GetProcAddress(cuda, "cuStreamCreate");
    cuStreamDestroy_ptr = (cuStreamDestroy_t)GetProcAddress(cuda, "cuStreamDestroy");
    cuStreamSynchronize_ptr = (cuStreamSynchronize_t)GetProcAddress(cuda, "cuStreamSynchronize");
    
    if (!cuInit_ptr || !cuDeviceGetCount_ptr || !cuMemAlloc_ptr) {
        fprintf(stderr, "Failed to load required CUDA functions\n");
        FreeLibrary(cuda);
        return NULL;
    }
    
    return cuda;
}

#define CUDA_DRV_CHECK(call)                                                   \
    do {                                                                       \
        CUresult result = (call);                                              \
        if (result != CUDA_SUCCESS) {                                          \
            fprintf(stderr, "CUDA Driver error %d at %s:%d\n", result, __FILE__, __LINE__); \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

// 打印 GPU 信息
void print_gpu_info(CUdevice device) {
    char name[256] = {0};
    int mpCount = 0, clockRate = 0, memClock = 0, busWidth = 0;
    int concurrentKernels = 0, asyncEngines = 0;
    size_t totalMem = 0;
    
    cuDeviceGetName_ptr(name, sizeof(name), device);
    cuDeviceGetAttribute_ptr(&mpCount, CU_DEVICE_ATTRIBUTE_MULTIPROCESSOR_COUNT, device);
    cuDeviceGetAttribute_ptr(&clockRate, CU_DEVICE_ATTRIBUTE_CLOCK_RATE, device);
    cuDeviceGetAttribute_ptr(&memClock, CU_DEVICE_ATTRIBUTE_MEMORY_CLOCK_RATE, device);
    cuDeviceGetAttribute_ptr(&busWidth, CU_DEVICE_ATTRIBUTE_GLOBAL_MEMORY_BUS_WIDTH, device);
    cuDeviceGetAttribute_ptr(&concurrentKernels, CU_DEVICE_ATTRIBUTE_CONCURRENT_KERNELS, device);
    cuDeviceGetAttribute_ptr(&asyncEngines, CU_DEVICE_ATTRIBUTE_ASYNC_ENGINE_COUNT, device);
    cuDeviceTotalMem_ptr(&totalMem, device);
    
    printf("GPU: %s\n", name);
    printf("  Multiprocessors: %d\n", mpCount);
    printf("  GPU Clock: %.0f MHz\n", clockRate / 1000.0);
    printf("  Memory Clock: %.0f MHz\n", memClock / 1000.0);
    printf("  Memory Bus Width: %d bits\n", busWidth);
    printf("  Peak Memory Bandwidth: %.2f GB/s\n", 2.0 * memClock * (busWidth / 8) / 1.0e6);
    printf("  Total Memory: %.2f GB\n", totalMem / (1024.0 * 1024 * 1024));
    printf("  Concurrent Kernels: %s\n", concurrentKernels ? "Yes" : "No");
    printf("  Async Copy Engines: %d\n", asyncEngines);
}

// 测试 1: Copy 引擎负载
void test_copy_load(CUcontext ctx, int duration_sec, int copy_mode) {
    printf("\n=== CUDA Driver Copy Load Test ===\n");
    printf("Duration: %d seconds\n", duration_sec);
    
    const char* modeStr = (copy_mode == 0) ? "Host to Device (Upload)" :
                          (copy_mode == 1) ? "Device to Host (Download)" :
                                             "Bidirectional";
    printf("Mode: %s\n\n", modeStr);
    
    const size_t BUFFER_SIZE = 512 * 1024 * 1024;  // 512MB
    
    // 分配设备内存
    CUdeviceptr d_mem1, d_mem2;
    CUDA_DRV_CHECK(cuMemAlloc_ptr(&d_mem1, BUFFER_SIZE));
    CUDA_DRV_CHECK(cuMemAlloc_ptr(&d_mem2, BUFFER_SIZE));
    
    // 分配主机内存 (pinned)
    void* h_mem;
    CUDA_DRV_CHECK(cuMemAllocHost_ptr(&h_mem, BUFFER_SIZE));
    
    // 初始化数据
    memset(h_mem, 0xAB, BUFFER_SIZE);
    CUDA_DRV_CHECK(cuMemcpyHtoD_ptr(d_mem1, h_mem, BUFFER_SIZE));
    
    // 创建异步流
    CUstream stream;
    CUDA_DRV_CHECK(cuStreamCreate_ptr(&stream, 0));
    
    LARGE_INTEGER freq, start, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&start);
    
    long long totalBytes = 0;
    int operationCount = 0;
    
    printf("Running...\n");
    
    while (1) {
        QueryPerformanceCounter(&now);
        double elapsed = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
        if (elapsed >= duration_sec) break;
        
        if (copy_mode == 0 || copy_mode == 2) {
            // H2D - use sync version for compatibility
            CUDA_DRV_CHECK(cuMemcpyHtoD_ptr(d_mem2, h_mem, BUFFER_SIZE));
            totalBytes += BUFFER_SIZE;
        }
        
        if (copy_mode == 1 || copy_mode == 2) {
            // D2H - use sync version for compatibility
            CUDA_DRV_CHECK(cuMemcpyDtoH_ptr(h_mem, d_mem1, BUFFER_SIZE));
            totalBytes += BUFFER_SIZE;
        }
        
        operationCount++;
    }
    
    CUDA_DRV_CHECK(cuStreamSynchronize_ptr(stream));
    
    QueryPerformanceCounter(&now);
    double totalTime = (double)(now.QuadPart - start.QuadPart) / freq.QuadPart;
    double bandwidth = (totalBytes / (1024.0 * 1024 * 1024)) / totalTime;
    
    printf("\nTotal operations: %d\n", operationCount);
    printf("Total data: %.2f GB\n", totalBytes / (1024.0 * 1024 * 1024));
    printf("Time: %.2f seconds\n", totalTime);
    printf("Bandwidth: %.2f GB/s\n", bandwidth);
    
    // 清理
    cuStreamDestroy_ptr(stream);
    cuMemFree_ptr(d_mem1);
    cuMemFree_ptr(d_mem2);
    cuMemFreeHost_ptr(h_mem);
}

// 测试 2: 显存分配
void test_memory_allocation(CUcontext ctx, int dedicated_mb) {
    printf("\n=== CUDA Driver Memory Allocation Test ===\n");
    printf("Allocating: %d MB\n\n", dedicated_mb);
    
    size_t size = (size_t)dedicated_mb * 1024 * 1024;
    CUdeviceptr d_mem;
    
    printf("Allocating GPU memory... ");
    CUresult err = cuMemAlloc_ptr(&d_mem, size);
    if (err == CUDA_SUCCESS) {
        printf("OK\n");
        
        // 填充数据
        printf("Filling with data... ");
        void* h_temp = malloc(1024 * 1024);  // 1MB temp buffer
        memset(h_temp, 0xCD, 1024 * 1024);
        
        for (size_t offset = 0; offset < size; offset += 1024 * 1024) {
            size_t copySize = (offset + 1024 * 1024 > size) ? (size - offset) : (1024 * 1024);
            cuMemcpyHtoD_ptr(d_mem + offset, h_temp, copySize);
        }
        
        free(h_temp);
        printf("OK\n");
        
        printf("\nAllocated %d MB. Press Enter to release...\n", dedicated_mb);
        getchar();
        
        cuMemFree_ptr(d_mem);
        printf("Memory released.\n");
    } else {
        printf("Failed: %d\n", err);
    }
}

void print_help(const char* prog) {
    printf("Usage: %s <command> [options]\n", prog);
    printf("\nCommands:\n");
    printf("  info                    Show GPU information\n");
    printf("  copy <seconds> [mode]  Run copy load (mode: 0=H2D, 1=D2H, 2=both)\n");
    printf("  memory <dedicated_mb>  Allocate memory\n");
    printf("\nExamples:\n");
    printf("  %s info\n", prog);
    printf("  %s copy 30 1\n", prog);
    printf("  %s memory 2048\n", prog);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }
    
    // 加载 CUDA Driver
    HMODULE cudaDll = load_cuda_driver();
    if (!cudaDll) {
        return 1;
    }
    
    // 初始化 CUDA
    CUDA_DRV_CHECK(cuInit_ptr(0));
    
    // 获取设备
    int deviceCount = 0;
    CUDA_DRV_CHECK(cuDeviceGetCount_ptr(&deviceCount));
    
    if (deviceCount == 0) {
        fprintf(stderr, "No CUDA devices found\n");
        FreeLibrary(cudaDll);
        return 1;
    }
    
    printf("CUDA Driver API Loaded\n");
    printf("Devices: %d\n\n", deviceCount);
    
    // 使用第一个设备
    CUdevice device;
    CUDA_DRV_CHECK(cuDeviceGet_ptr(&device, 0));
    
    const char* cmd = argv[1];
    
    if (strcmp(cmd, "info") == 0) {
        print_gpu_info(device);
    }
    else if (strcmp(cmd, "copy") == 0) {
        int duration = (argc > 2) ? atoi(argv[2]) : 30;
        int mode = (argc > 3) ? atoi(argv[3]) : 0;
        
        // 创建上下文
        CUcontext ctx;
        CUDA_DRV_CHECK(cuCtxCreate_ptr(&ctx, 0, device));
        
        test_copy_load(ctx, duration, mode);
        
        cuCtxDestroy_ptr(ctx);
    }
    else if (strcmp(cmd, "memory") == 0) {
        int dedicated = (argc > 2) ? atoi(argv[2]) : 1024;
        
        CUcontext ctx;
        CUDA_DRV_CHECK(cuCtxCreate_ptr(&ctx, 0, device));
        
        test_memory_allocation(ctx, dedicated);
        
        cuCtxDestroy_ptr(ctx);
    }
    else {
        print_help(argv[0]);
        FreeLibrary(cudaDll);
        return 1;
    }
    
    FreeLibrary(cudaDll);
    return 0;
}
