/**
 * DRM GPU 显存测试程序
 * 使用通用 libdrm API 操作 GPU
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

#define DEFAULT_MEMORY_MB 1024  // 默认 1GB 显存
#define BUFFER_SIZE_MB 64       // 每个 BO 64MB

int main(int argc, char** argv) {
    int fd;
    int target_mb = DEFAULT_MEMORY_MB;
    const char* device_path = "/dev/dri/renderD129";
    
    if (argc > 1) {
        target_mb = atoi(argv[1]);
        if (target_mb <= 0) target_mb = DEFAULT_MEMORY_MB;
    }
    
    printf("=== DRM GPU 显存测试程序 ===\n");
    printf("目标显存: %d MB\n", target_mb);
    
    // 打开 GPU 设备
    printf("\n正在打开设备: %s\n", device_path);
    fd = open(device_path, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("打开设备失败");
        device_path = "/dev/dri/renderD128";
        printf("尝试备用设备: %s\n", device_path);
        fd = open(device_path, O_RDWR | O_CLOEXEC);
        if (fd < 0) {
            perror("打开备用设备也失败");
            return 1;
        }
    }
    printf("成功打开: %s (fd=%d)\n", device_path, fd);
    
    // 打印 GPU 信息
    printf("\n[GPU 信息]\n");
    drmVersionPtr version = drmGetVersion(fd);
    if (version) {
        printf("DRM 驱动名称: %s\n", version->name ? version->name : "unknown");
        printf("DRM 驱动描述: %s\n", version->desc ? version->desc : "unknown");
        drmFreeVersion(version);
    }
    
    // 获取设备信息
    drmDevicePtr device_info;
    if (drmGetDevice2(fd, 0, &device_info) == 0) {
        printf("设备节点: %s\n", device_info->nodes[DRM_NODE_RENDER] ? device_info->nodes[DRM_NODE_RENDER] : "unknown");
        drmFreeDevice(&device_info);
    }
    
    // 检查是否为 Intel Xe
    version = drmGetVersion(fd);
    if (version && version->name && strcmp(version->name, "xe") == 0) {
        printf("GPU 类型: Intel Xe\n");
    } else {
        printf("GPU 类型: 其他\n");
    }
    if (version) drmFreeVersion(version);
    
    // 获取显存限制
    (void)device_info;  // 标记为有意不使用
    if (drmGetDevice2(fd, 0, &device_info) == 0) {
        // 尝试读取显存信息
        // 注意：通用 DRM API 不直接提供显存查询
        printf("显存信息: 需要通过特定驱动接口查询\n");
        drmFreeDevice(&device_info);
    }
    
    printf("\n[显存分配测试]\n");
    printf("每个 BO: %d MB\n", BUFFER_SIZE_MB);
    
    int num_buffers = (target_mb + BUFFER_SIZE_MB - 1) / BUFFER_SIZE_MB;
    printf("需要分配: %d 个 BO\n\n", num_buffers);
    
    // 使用 drmPrimeHandleToFD 或 mmap 来分配显存
    // 这里我们使用简单的 mmap 测试
    
    printf("使用 mmap 测试显存访问...\n");
    
    // 对于 Intel Xe，我们可以尝试创建 dumb buffer
    int allocated_mb = 0;
    int buffer_count = 0;
    
    for (int i = 0; i < num_buffers; i++) {
        struct drm_mode_create_dumb create_req;
        memset(&create_req, 0, sizeof(create_req));
        create_req.width = 4096;
        create_req.height = 4096;
        create_req.bpp = 32;
        
        if (drmIoctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req) < 0) {
            fprintf(stderr, "\n分配第 %d 个 dumb buffer 失败: %s\n", i + 1, strerror(errno));
            fprintf(stderr, "已分配: %d MB\n", allocated_mb);
            break;
        }
        
        // 计算实际大小 (约 64MB)
        int actual_mb = create_req.size / (1024 * 1024);
        allocated_mb += actual_mb;
        buffer_count++;
        
        // 映射内存
        struct drm_mode_map_dumb map_req;
        memset(&map_req, 0, sizeof(map_req));
        map_req.handle = create_req.handle;
        
        if (drmIoctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req) == 0) {
            void* ptr = mmap(0, create_req.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, map_req.offset);
            if (ptr != MAP_FAILED) {
                // 写入一些数据确保内存被分配
                memset(ptr, 0xAB, 4096);  // 只写一部分
                munmap(ptr, create_req.size);
            }
        }
        
        // 销毁 buffer
        struct drm_mode_destroy_dumb destroy_req;
        memset(&destroy_req, 0, sizeof(destroy_req));
        destroy_req.handle = create_req.handle;
        drmIoctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
        
        if ((i + 1) % 10 == 0 || i == num_buffers - 1) {
            printf("已测试: %d / %d 个 BO (%d MB / %d MB)\n", 
                   i + 1, num_buffers, allocated_mb, target_mb);
        }
    }
    
    printf("\n=== 显存测试完成 ===\n");
    printf("成功测试: %d 个 BO\n", buffer_count);
    printf("测试显存: %d MB\n", allocated_mb);
    
    close(fd);
    
    printf("\n注意: dumb buffer 是临时分配的，已自动释放\n");
    printf("程序退出\n");
    return 0;
}
