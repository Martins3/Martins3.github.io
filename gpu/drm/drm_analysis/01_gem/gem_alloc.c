/**
 * GEM 分配实验
 * 
 * 演示如何创建和释放 GEM 对象
 * 
 * 内核实现: drivers/gpu/drm/drm_gem.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

// GEM 创建 IOCTL (通用 DRM)
#ifndef DRM_IOCTL_GEM_CREATE
struct drm_gem_create {
    uint64_t size;
    uint32_t handle;
    uint32_t padding;
};
#define DRM_IOCTL_GEM_CREATE 0xc0206440  // _IOWR(DRM_IOCTL_BASE, 0x40, struct drm_gem_create)
#endif

// 打印 GEM 对象信息
void print_gem_info(int fd, uint32_t handle) {
    // 获取 GEM 对象名称（用于调试）
    struct drm_gem_flink flink;
    memset(&flink, 0, sizeof(flink));
    flink.handle = handle;
    
    int ret = ioctl(fd, DRM_IOCTL_GEM_FLINK, &flink);
    if (ret == 0) {
        printf("  GEM Name: 0x%08x\n", flink.name);
    }
}

int test_gem_create(int fd) {
    printf("\n=== GEM 对象创建实验 ===\n\n");
    
    // 测试不同大小的 GEM 分配
    size_t test_sizes[] = {4096, 64*1024, 1024*1024, 32*1024*1024};
    const char* size_names[] = {"4KB", "64KB", "1MB", "32MB"};
    
    for (int i = 0; i < 4; i++) {
        size_t size = test_sizes[i];
        printf("测试 %s 分配:\n", size_names[i]);
        
        // 方法 1: 使用 dumb buffer (如果支持)
        struct drm_mode_create_dumb create_req;
        memset(&create_req, 0, sizeof(create_req));
        create_req.width = size / 4;
        create_req.height = 1;
        create_req.bpp = 32;
        
        int ret = ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req);
        if (ret < 0) {
            printf("  [FAIL] DRM_IOCTL_MODE_CREATE_DUMB: %s\n", strerror(errno));
            printf("  说明: SR-IOV VF 模式可能不支持 dumb buffer\n");
        } else {
            printf("  [OK] Dumb buffer 创建成功\n");
            printf("  Handle: %u, Size: %llu bytes\n", 
                   create_req.handle, (unsigned long long)create_req.size);
            
            // 获取 GEM 信息
            struct drm_mode_map_dumb map_req;
            memset(&map_req, 0, sizeof(map_req));
            map_req.handle = create_req.handle;
            
            if (ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req) == 0) {
                printf("  Offset: 0x%llx\n", (unsigned long long)map_req.offset);
            }
            
            // 销毁 dumb buffer
            struct drm_mode_destroy_dumb destroy_req;
            memset(&destroy_req, 0, sizeof(destroy_req));
            destroy_req.handle = create_req.handle;
            ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
            printf("  [OK] 已释放\n");
        }
        
        printf("\n");
    }
    
    return 0;
}

int test_gem_prime(int fd) {
    printf("\n=== GEM PRIME (DMA-BUF) 实验 ===\n\n");
    
    // 创建 dumb buffer
    struct drm_mode_create_dumb create_req;
    memset(&create_req, 0, sizeof(create_req));
    create_req.width = 1024;
    create_req.height = 1024;
    create_req.bpp = 32;
    
    int ret = ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req);
    if (ret < 0) {
        printf("[SKIP] 无法创建 dumb buffer: %s\n", strerror(errno));
        return -1;
    }
    
    printf("[OK] 创建 GEM 对象: handle=%u, size=%llu\n",
           create_req.handle, (unsigned long long)create_req.size);
    
    // 导出为 DMA-BUF
    struct drm_prime_handle prime_req;
    memset(&prime_req, 0, sizeof(prime_req));
    prime_req.handle = create_req.handle;
    prime_req.flags = DRM_CLOEXEC;
    
    ret = ioctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_req);
    if (ret < 0) {
        printf("[FAIL] 导出 DMA-BUF 失败: %s\n", strerror(errno));
    } else {
        printf("[OK] DMA-BUF fd: %d\n", prime_req.fd);
        
        // 导入 DMA-BUF
        struct drm_prime_handle import_req;
        memset(&import_req, 0, sizeof(import_req));
        import_req.fd = prime_req.fd;
        import_req.flags = 0;
        
        ret = ioctl(fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &import_req);
        if (ret < 0) {
            printf("[FAIL] 导入 DMA-BUF 失败: %s\n", strerror(errno));
        } else {
            printf("[OK] 导入后的 handle: %u\n", import_req.handle);
            
            // 关闭导入的 handle
            struct drm_gem_close close_req;
            memset(&close_req, 0, sizeof(close_req));
            close_req.handle = import_req.handle;
            ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_req);
        }
        
        close(prime_req.fd);
    }
    
    // 清理
    struct drm_mode_destroy_dumb destroy_req;
    memset(&destroy_req, 0, sizeof(destroy_req));
    destroy_req.handle = create_req.handle;
    ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
    
    printf("\n说明: GEM PRIME 允许跨设备/跨进程共享显存\n");
    
    return 0;
}

void print_gem_theory() {
    printf("\n=== GEM 原理说明 ===\n\n");
    
    printf("1. GEM 对象是什么?\n");
    printf("   GEM (Graphics Execution Manager) 对象是 DRM 子系统中\n");
    printf("   表示一块 GPU 显存的抽象。每个 GEM 对象有:\n");
    printf("   - 唯一的 handle (用户空间标识)\n");
    printf("   - 引用计数 (自动释放)\n");
    printf("   - 文件指针 (支持 mmap)\n\n");
    
    printf("2. 为什么需要 GEM?\n");
    printf("   - 统一管理显存分配\n");
    printf("   - 支持内存共享 (PRIME/DMA-BUF)\n");
    printf("   - 安全隔离不同应用\n\n");
    
    printf("3. GEM vs TTM\n");
    printf("   - GEM: 高层 API，管理显存对象\n");
    printf("   - TTM: 底层实现，管理物理内存页\n\n");
}

int main(int argc, char** argv) {
    const char* device_path = "/dev/dri/card0";
    if (argc > 1) {
        device_path = argv[1];
    }
    
    printf("=== GEM (Graphics Execution Manager) 实验 ===\n");
    printf("设备: %s\n", device_path);
    
    int fd = open(device_path, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("打开设备失败");
        return 1;
    }
    
    // 获取 DRM 驱动信息
    drmVersionPtr version = drmGetVersion(fd);
    if (version) {
        printf("驱动: %s (%s)\n\n", version->name, version->desc);
        drmFreeVersion(version);
    }
    
    // 运行实验
    test_gem_create(fd);
    test_gem_prime(fd);
    print_gem_theory();
    
    close(fd);
    return 0;
}
