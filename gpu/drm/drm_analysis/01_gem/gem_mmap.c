/**
 * GEM mmap 实验
 * 
 * 演示如何将 GEM 对象映射到用户空间
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

int test_gem_mmap(int fd) {
    printf("\n=== GEM mmap 实验 ===\n\n");
    
    // 创建 dumb buffer
    struct drm_mode_create_dumb create_req;
    memset(&create_req, 0, sizeof(create_req));
    create_req.width = 1024;
    create_req.height = 1024;
    create_req.bpp = 32;
    
    printf("创建 dumb buffer (1K x 1K x 4 bytes = 4MB)...\n");
    int ret = ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req);
    if (ret < 0) {
        printf("[FAIL] 创建失败: %s\n", strerror(errno));
        printf("说明: SR-IOV VF 模式可能不支持 dumb buffer\n");
        return -1;
    }
    
    printf("[OK] 创建成功:\n");
    printf("  Handle: %u\n", create_req.handle);
    printf("  Pitch: %u bytes\n", create_req.pitch);
    printf("  Size: %llu bytes\n", (unsigned long long)create_req.size);
    
    // 获取 mmap 偏移
    struct drm_mode_map_dumb map_req;
    memset(&map_req, 0, sizeof(map_req));
    map_req.handle = create_req.handle;
    
    ret = ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req);
    if (ret < 0) {
        printf("[FAIL] 获取 mmap 偏移失败: %s\n", strerror(errno));
        
        // 清理
        struct drm_mode_destroy_dumb destroy_req;
        memset(&destroy_req, 0, sizeof(destroy_req));
        destroy_req.handle = create_req.handle;
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
        return -1;
    }
    
    printf("[OK] 获取 mmap 偏移: 0x%llx\n", (unsigned long long)map_req.offset);
    
    // mmap 映射
    printf("\n映射 GEM 对象到用户空间...\n");
    void* ptr = mmap(NULL, create_req.size, PROT_READ | PROT_WRITE, 
                     MAP_SHARED, fd, map_req.offset);
    if (ptr == MAP_FAILED) {
        printf("[FAIL] mmap 失败: %s\n", strerror(errno));
        
        // 清理
        struct drm_mode_destroy_dumb destroy_req;
        memset(&destroy_req, 0, sizeof(destroy_req));
        destroy_req.handle = create_req.handle;
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
        return -1;
    }
    
    printf("[OK] mmap 成功:\n");
    printf("  用户空间地址: %p\n", ptr);
    printf("  大小: %llu bytes\n", (unsigned long long)create_req.size);
    
    // 测试读写
    printf("\n测试内存访问...\n");
    
    // 写入模式
    printf("  写入测试模式...\n");
    uint32_t* pixels = (uint32_t*)ptr;
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            pixels[y * (create_req.pitch / 4) + x] = (x << 16) | (y << 8) | 0xFF;
        }
    }
    
    // 读取验证
    printf("  读取验证...\n");
    int errors = 0;
    for (int y = 0; y < 10; y++) {
        for (int x = 0; x < 10; x++) {
            uint32_t expected = (x << 16) | (y << 8) | 0xFF;
            uint32_t actual = pixels[y * (create_req.pitch / 4) + x];
            if (actual != expected) {
                errors++;
            }
        }
    }
    
    if (errors == 0) {
        printf("  [OK] 读写测试通过!\n");
    } else {
        printf("  [FAIL] 读写测试失败 (%d 个错误)\n", errors);
    }
    
    // 显示一些像素值
    printf("\n  前 5x5 像素值 (十六进制):\n");
    for (int y = 0; y < 5; y++) {
        printf("    ");
        for (int x = 0; x < 5; x++) {
            printf("%08X ", pixels[y * (create_req.pitch / 4) + x]);
        }
        printf("\n");
    }
    
    // 清理
    printf("\n清理...\n");
    munmap(ptr, create_req.size);
    
    struct drm_mode_destroy_dumb destroy_req;
    memset(&destroy_req, 0, sizeof(destroy_req));
    destroy_req.handle = create_req.handle;
    ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_req);
    
    printf("[OK] 完成\n");
    return 0;
}

void print_mmap_theory() {
    printf("\n=== GEM mmap 原理 ===\n\n");
    
    printf("1. 为什么需要 mmap?\n");
    printf("   GPU 显存在内核空间分配，用户空间需要\n");
    printf("   通过 mmap 才能直接访问。\n\n");
    
    printf("2. mmap 工作流程:\n");
    printf("   a) 分配 GEM 对象 (创建显存缓冲区)\n");
    printf("   b) 获取 mmap 偏移 (通过 DRM_IOCTL_MODE_MAP_DUMB)\n");
    printf("   c) mmap 映射 (建立页表映射)\n");
    printf("   d) 直接读写 (像访问普通内存一样)\n\n");
    
    printf("3. 内存映射示意图:\n");
    printf("   用户空间虚拟地址\n");
    printf("         │\n");
    printf("         ▼\n");
    printf("   ┌─────────────┐\n");
    printf("   │  页表 (PTE)  │ ← CPU MMU\n");
    printf("   └─────────────┘\n");
    printf("         │\n");
    printf("         ▼\n");
    printf("   GPU 显存物理地址\n\n");
    
    printf("4. 注意:\n");
    printf("   - CPU 访问 GPU 显存通常比访问系统内存慢\n");
    printf("   - 对于频繁 CPU 访问的数据，应该用系统内存\n");
    printf("   - GPU 访问自己的显存最快\n\n");
}

int main(int argc, char** argv) {
    const char* device_path = "/dev/dri/renderD128";
    if (argc > 1) {
        device_path = argv[1];
    }
    
    printf("=== GEM mmap 实验 ===\n");
    printf("设备: %s\n", device_path);
    
    int fd = open(device_path, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("打开设备失败");
        return 1;
    }
    
    test_gem_mmap(fd);
    print_mmap_theory();
    
    close(fd);
    return 0;
}
