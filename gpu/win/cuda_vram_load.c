/**
 * GPU VRAM Load Generator - CUDA Edition
 *
 * 使用 CUDA Driver API 分配 GPU 显存
 * - 专用显存: cudaMalloc
 * - 共享/统一内存: cudaMallocManaged
 *
 * 优势：直接控制 GPU 内存分配，精确的内存类型控制
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

#define CUDA_SUCCESS 0
#define CU_DEVICE_ATTRIBUTE_TOTAL_CONSTANT_MEMORY 9
#define CU_DEVICE_ATTRIBUTE_MAX_SHARED_MEMORY_PER_BLOCK 8

// 函数指针
typedef CUresult (*cuInit_t)(unsigned int Flags);
typedef CUresult (*cuDeviceGetCount_t)(int* count);
typedef CUresult (*cuDeviceGet_t)(CUdevice* device, int ordinal);
typedef CUresult (*cuDeviceGetName_t)(char* name, int len, CUdevice dev);
typedef CUresult (*cuDeviceTotalMem_t)(size_t* bytes, CUdevice dev);
typedef CUresult (*cuCtxCreate_t)(CUcontext* pctx, unsigned int flags, CUdevice dev);
typedef CUresult (*cuCtxDestroy_t)(CUcontext ctx);
typedef CUresult (*cuCtxSetCurrent_t)(CUcontext ctx);
typedef CUresult (*cuMemAlloc_t)(CUdeviceptr* dptr, size_t bytesize);
typedef CUresult (*cuMemFree_t)(CUdeviceptr dptr);
typedef CUresult (*cuMemAllocManaged_t)(CUdeviceptr* dptr, size_t bytesize, unsigned int flags);
typedef CUresult (*cuMemsetD8_t)(CUdeviceptr dstDevice, unsigned char uc, size_t N);
typedef CUresult (*cuMemcpyHtoD_t)(CUdeviceptr dstDevice, const void* srcHost, size_t ByteCount);

// 内存分配类型
#define ALLOC_TYPE_DEDICATED 0
#define ALLOC_TYPE_MANAGED   1
#define ALLOC_TYPE_MIXED     2

HMODULE g_cudaDll = NULL;
cuInit_t cuInit_ptr = NULL;
cuDeviceGetCount_t cuDeviceGetCount_ptr = NULL;
cuDeviceGet_t cuDeviceGet_ptr = NULL;
cuDeviceGetName_t cuDeviceGetName_ptr = NULL;
cuDeviceTotalMem_t cuDeviceTotalMem_ptr = NULL;
cuCtxCreate_t cuCtxCreate_ptr = NULL;
cuCtxDestroy_t cuCtxDestroy_ptr = NULL;
cuCtxSetCurrent_t cuCtxSetCurrent_ptr = NULL;
cuMemAlloc_t cuMemAlloc_ptr = NULL;
cuMemFree_t cuMemFree_ptr = NULL;
cuMemAllocManaged_t cuMemAllocManaged_ptr = NULL;
cuMemsetD8_t cuMemsetD8_ptr = NULL;
cuMemcpyHtoD_t cuMemcpyHtoD_ptr = NULL;

#define CUDA_DRV_CHECK(call)                                                   \
    do {                                                                       \
        CUresult result = (call);                                              \
        if (result != CUDA_SUCCESS) {                                          \
            fprintf(stderr, "CUDA error %d at %s:%d\n", result, __FILE__, __LINE__); \
            exit(1);                                                           \
        }                                                                      \
    } while (0)

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
    cuDeviceTotalMem_ptr = (cuDeviceTotalMem_t)GetProcAddress(cuda, "cuDeviceTotalMem");
    cuCtxCreate_ptr = (cuCtxCreate_t)GetProcAddress(cuda, "cuCtxCreate");
    cuCtxDestroy_ptr = (cuCtxDestroy_t)GetProcAddress(cuda, "cuCtxDestroy");
    cuCtxSetCurrent_ptr = (cuCtxSetCurrent_t)GetProcAddress(cuda, "cuCtxSetCurrent");
    cuMemAlloc_ptr = (cuMemAlloc_t)GetProcAddress(cuda, "cuMemAlloc");
    cuMemFree_ptr = (cuMemFree_t)GetProcAddress(cuda, "cuMemFree");
    cuMemAllocManaged_ptr = (cuMemAllocManaged_t)GetProcAddress(cuda, "cuMemAllocManaged");
    cuMemsetD8_ptr = (cuMemsetD8_t)GetProcAddress(cuda, "cuMemsetD8");
    cuMemcpyHtoD_ptr = (cuMemcpyHtoD_t)GetProcAddress(cuda, "cuMemcpyHtoD");

    if (!cuInit_ptr || !cuMemAlloc_ptr) {
        fprintf(stderr, "Failed to load required CUDA functions\n");
        FreeLibrary(cuda);
        return NULL;
    }

    return cuda;
}

void print_memory_stats() {
    printf("  (Check Task Manager: Performance -> GPU)\n");
    printf("  (Or run: nvidia-smi --query-gpu=memory.used,memory.free --format=csv)\n");
}

int main(int argc, char* argv[]) {
    int target_mb = 2048;  // 默认 2GB
    int duration_sec = 0;  // 0 = 等待按键
    int alloc_type = ALLOC_TYPE_DEDICATED;

    if (argc > 1) target_mb = atoi(argv[1]);
    if (argc > 2) duration_sec = atoi(argv[2]);
    if (argc > 3) alloc_type = atoi(argv[3]);

    printf("================================================\n");
    printf("GPU VRAM Load Generator - CUDA Edition\n");
    printf("================================================\n");
    printf("Target allocation: %d MB (%.2f GB)\n", target_mb, target_mb / 1024.0);
    if (duration_sec > 0) {
        printf("Duration: %d seconds\n", duration_sec);
    } else {
        printf("Duration: unlimited (press Enter to exit)\n");
    }

    const char* typeStr = (alloc_type == ALLOC_TYPE_DEDICATED) ? "Dedicated (cudaMalloc)" :
                          (alloc_type == ALLOC_TYPE_MANAGED)   ? "Managed (cudaMallocManaged)" :
                                                                 "Mixed";
    printf("Allocation type: %s\n", typeStr);
    printf("\n");
    printf("Watch 'Dedicated GPU memory' in Task Manager\n\n");

    // 加载 CUDA
    g_cudaDll = load_cuda_driver();
    if (!g_cudaDll) return 1;

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
    size_t totalMem = 0;
    cuDeviceGetName_ptr(name, sizeof(name), device);
    cuDeviceTotalMem_ptr(&totalMem, device);

    printf("GPU: %s\n", name);
    printf("Total VRAM: %.2f GB\n\n", totalMem / (1024.0 * 1024 * 1024));

    // 创建上下文
    CUcontext ctx;
    CUDA_DRV_CHECK(cuCtxCreate_ptr(&ctx, 0, device));

    // 计算分配参数
    size_t chunkSize = 512 * 1024 * 1024;  // 512MB chunks
    int numChunks = (target_mb * 1024ULL * 1024 + chunkSize - 1) / chunkSize;

    printf("Allocation plan:\n");
    printf("  Chunk size: %d MB\n", (int)(chunkSize / (1024 * 1024)));
    printf("  Number of chunks: %d\n", numChunks);
    printf("  Total: %.0f MB\n\n", (double)numChunks * chunkSize / (1024 * 1024));

    // 分配内存
    CUdeviceptr* allocations = (CUdeviceptr*)calloc(numChunks, sizeof(CUdeviceptr));
    int allocated = 0;

    printf("Allocating memory...\n");

    for (int i = 0; i < numChunks; i++) {
        int type = alloc_type;
        if (alloc_type == ALLOC_TYPE_MIXED) {
            type = (i % 2 == 0) ? ALLOC_TYPE_DEDICATED : ALLOC_TYPE_MANAGED;
        }

        CUresult result;
        if (type == ALLOC_TYPE_DEDICATED || !cuMemAllocManaged_ptr) {
            result = cuMemAlloc_ptr(&allocations[i], chunkSize);
        } else {
            result = cuMemAllocManaged_ptr(&allocations[i], chunkSize, 1); // CU_MEM_ATTACH_GLOBAL
        }

        if (result != CUDA_SUCCESS) {
            printf("  Failed to allocate chunk %d: %d\n", i + 1, result);
            break;
        }

        allocated++;

        // 填充数据确保内存被提交
        if (cuMemsetD8_ptr) {
            cuMemsetD8_ptr(allocations[i], (unsigned char)(i & 0xFF), chunkSize);
        }

        if ((i + 1) % 2 == 0 || i == numChunks - 1) {
            printf("\r  Allocated: %d/%d chunks (%.0f MB)",
                   allocated, numChunks, (double)allocated * chunkSize / (1024 * 1024));
            fflush(stdout);
        }
    }

    printf("\n\nAllocation complete!\n");
    printf("Successfully allocated: %.2f GB\n", (double)allocated * chunkSize / (1024 * 1024 * 1024));

    print_memory_stats();

    // 等待
    printf("\n");
    if (duration_sec > 0) {
        printf("Holding for %d seconds...", duration_sec);
        for (int i = 0; i < duration_sec; i++) {
            Sleep(1000);
            printf(".");
            fflush(stdout);
        }
        printf("\n");
    } else {
        printf("Memory allocated. Press Enter to release...\n");
        getchar();
    }

    // 清理
    printf("\nReleasing memory...\n");
    for (int i = 0; i < allocated; i++) {
        cuMemFree_ptr(allocations[i]);
    }
    free(allocations);

    cuCtxDestroy_ptr(ctx);
    FreeLibrary(g_cudaDll);

    printf("Done! Check Task Manager - memory should decrease.\n");
    return 0;
}
