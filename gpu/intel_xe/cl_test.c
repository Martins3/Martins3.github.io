/**
 * OpenCL GPU 显存测试程序
 * 使用 OpenCL 在 Intel Xe 显卡上分配显存
 * 
 * 使用方法:
 *   ./cl_test [显存大小(MB)]
 *   例如: ./cl_test 4096
 */

#define CL_TARGET_OPENCL_VERSION 300
#include <CL/cl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_MEMORY_MB 1024  // 默认 1GB 显存（OpenCL 可能有限制）
#define BUFFER_SIZE_MB 64       // 每个缓冲区 64MB

void print_error(cl_int err, const char* msg) {
    fprintf(stderr, "OpenCL Error %d: %s\n", err, msg);
}

const char* get_error_string(cl_int err) {
    switch(err) {
        case CL_SUCCESS: return "Success";
        case CL_DEVICE_NOT_FOUND: return "Device not found";
        case CL_DEVICE_NOT_AVAILABLE: return "Device not available";
        case CL_MEM_OBJECT_ALLOCATION_FAILURE: return "Memory allocation failure";
        case CL_OUT_OF_HOST_MEMORY: return "Out of host memory";
        case CL_OUT_OF_RESOURCES: return "Out of resources";
        default: return "Unknown error";
    }
}

int main(int argc, char** argv) {
    cl_int err;
    cl_platform_id platform;
    cl_device_id device;
    cl_context context;
    cl_command_queue queue;
    cl_mem* buffers = NULL;
    
    int target_mb = DEFAULT_MEMORY_MB;
    if (argc > 1) {
        target_mb = atoi(argv[1]);
        if (target_mb <= 0) target_mb = DEFAULT_MEMORY_MB;
    }
    
    printf("=== OpenCL GPU 显存测试程序 ===\n");
    printf("目标显存: %d MB\n\n", target_mb);
    
    // 获取平台
    cl_uint num_platforms;
    err = clGetPlatformIDs(1, &platform, &num_platforms);
    if (err != CL_SUCCESS) {
        print_error(err, "获取 OpenCL 平台失败");
        printf("提示: 请安装 OpenCL 驱动\n");
        return 1;
    }
    
    if (num_platforms == 0) {
        printf("错误: 没有发现 OpenCL 平台\n");
        printf("提示: 请安装 intel-compute-runtime\n");
        return 1;
    }
    
    // 获取平台信息
    char platform_name[256];
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(platform_name), platform_name, NULL);
    printf("OpenCL 平台: %s\n", platform_name);
    
    char platform_vendor[256];
    clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, sizeof(platform_vendor), platform_vendor, NULL);
    printf("平台供应商: %s\n", platform_vendor);
    
    // 获取 GPU 设备
    cl_uint num_devices;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &num_devices);
    if (err != CL_SUCCESS) {
        print_error(err, "获取 GPU 设备失败");
        
        // 尝试获取所有设备
        err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &num_devices);
        if (err != CL_SUCCESS) {
            print_error(err, "获取任何设备都失败");
            return 1;
        }
    }
    
    // 获取设备信息
    char device_name[256];
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(device_name), device_name, NULL);
    printf("设备名称: %s\n", device_name);
    
    char device_vendor[256];
    clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(device_vendor), device_vendor, NULL);
    printf("设备供应商: %s\n", device_vendor);
    
    cl_device_type device_type;
    clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(device_type), &device_type, NULL);
    printf("设备类型: %s\n", 
           (device_type == CL_DEVICE_TYPE_GPU) ? "GPU" :
           (device_type == CL_DEVICE_TYPE_CPU) ? "CPU" : "其他");
    
    // 获取显存信息
    cl_ulong global_mem;
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem), &global_mem, NULL);
    printf("全局显存: %lu MB\n", global_mem / (1024 * 1024));
    
    cl_ulong max_alloc;
    clGetDeviceInfo(device, CL_DEVICE_MAX_MEM_ALLOC_SIZE, sizeof(max_alloc), &max_alloc, NULL);
    printf("最大单块分配: %lu MB\n", max_alloc / (1024 * 1024));
    
    cl_ulong local_mem;
    clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem), &local_mem, NULL);
    printf("本地/共享显存: %lu KB\n", local_mem / 1024);
    
    printf("\n");
    
    // 创建上下文
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    if (err != CL_SUCCESS) {
        print_error(err, "创建 OpenCL 上下文失败");
        return 1;
    }
    
    // 创建命令队列
    queue = clCreateCommandQueue(context, device, 0, &err);
    if (err != CL_SUCCESS) {
        print_error(err, "创建命令队列失败");
        clReleaseContext(context);
        return 1;
    }
    
    // 计算需要分配的缓冲区数量
    size_t buffer_size = BUFFER_SIZE_MB * 1024 * 1024;
    int num_buffers = (target_mb + BUFFER_SIZE_MB - 1) / BUFFER_SIZE_MB;
    
    printf("开始分配显存...\n");
    printf("每个缓冲区: %d MB\n", BUFFER_SIZE_MB);
    printf("需要分配: %d 个缓冲区\n\n", num_buffers);
    
    buffers = malloc(num_buffers * sizeof(cl_mem));
    if (!buffers) {
        fprintf(stderr, "分配缓冲区数组失败\n");
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        return 1;
    }
    
    // 分配显存
    int allocated_mb = 0;
    int buffer_count = 0;
    
    for (int i = 0; i < num_buffers; i++) {
        buffers[i] = clCreateBuffer(context, CL_MEM_READ_WRITE, buffer_size, NULL, &err);
        if (err != CL_SUCCESS) {
            fprintf(stderr, "\n分配第 %d 个缓冲区失败: %s\n", i + 1, get_error_string(err));
            fprintf(stderr, "已分配: %d MB\n", allocated_mb);
            break;
        }
        
        allocated_mb += BUFFER_SIZE_MB;
        buffer_count++;
        
        if ((i + 1) % 10 == 0 || i == num_buffers - 1) {
            printf("已分配: %d / %d 个缓冲区 (%d MB / %d MB)\n", 
                   i + 1, num_buffers, allocated_mb, target_mb);
        }
    }
    
    printf("\n=== 显存分配完成 ===\n");
    printf("成功分配: %d 个缓冲区\n", buffer_count);
    printf("占用显存: %d MB\n", allocated_mb);
    
    if (buffer_count > 0) {
        printf("\n按 Enter 键释放显存并退出...\n");
        getchar();
    }
    
    // 清理
    printf("\n正在释放显存...\n");
    for (int i = 0; i < buffer_count; i++) {
        clReleaseMemObject(buffers[i]);
    }
    free(buffers);
    
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    
    printf("显存已释放，程序退出\n");
    return 0;
}
