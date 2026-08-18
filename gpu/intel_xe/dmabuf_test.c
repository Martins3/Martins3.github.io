/**
 * DMA-BUF 测试程序
 * 测试 Intel Xe GPU 是否支持 DMA-BUF 导出/导入
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
#include <linux/dma-buf.h>

#include <xf86drm.h>

// Xe 驱动特定的 DMA-BUF 导出/导入 (如果支持)
#ifndef DRM_XE_GEM_EXPORT_DMABUF
#define DRM_XE_GEM_EXPORT_DMABUF 0x0a
#endif

#ifndef DRM_XE_GEM_IMPORT_DMABUF
#define DRM_XE_GEM_IMPORT_DMABUF 0x0b
#endif

struct drm_xe_gem_export_dmabuf {
    uint32_t handle;
    uint32_t flags;
    int32_t fd;  // 返回的 DMA-BUF fd
    uint32_t pad;
};

struct drm_xe_gem_import_dmabuf {
    int32_t fd;  // 输入的 DMA-BUF fd
    uint32_t flags;
    uint32_t handle;  // 返回的 GEM handle
    uint64_t size;
};

#define CHECK_IOCTL(result, msg) \
    do { if (result < 0) { printf("  [FAIL] %s: %s\n", msg, strerror(errno)); } \
         else { printf("  [OK] %s\n", msg); } } while(0)

int test_dmabuf_basic(int fd) {
    printf("\n=== DMA-BUF 基本功能测试 ===\n");
    
    // 检查内核是否支持 DMA-BUF
    int dmabuf_fd = open("/dev/dma_buf_ctl", O_RDONLY);
    if (dmabuf_fd < 0) {
        printf("DMA-BUF 设备节点: /dev/dma_buf_ctl 不存在 (这是正常的)\n");
    } else {
        printf("DMA-BUF 设备节点: 存在\n");
        close(dmabuf_fd);
    }
    
    // 检查 DRM 驱动信息
    drmVersionPtr version = drmGetVersion(fd);
    if (version) {
        printf("DRM 驱动: %s\n", version->name);
        printf("  日期: %s\n", version->date ? version->date : "unknown");
        printf("  描述: %s\n", version->desc ? version->desc : "unknown");
        drmFreeVersion(version);
    }
    
    return 0;
}

int test_xe_dmabuf_export(int fd) {
    printf("\n=== Xe DMA-BUF 导出测试 ===\n");
    
    // 首先创建一个 GEM 对象
    struct drm_gem_close close_bo;
    struct drm_mode_create_dumb create_req;
    memset(&create_req, 0, sizeof(create_req));
    create_req.width = 1024;
    create_req.height = 1024;
    create_req.bpp = 32;
    
    printf("尝试创建 dumb buffer...\n");
    int ret = ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_req);
    if (ret < 0) {
        printf("  [SKIP] 无法创建 dumb buffer: %s\n", strerror(errno));
        printf("  说明: SR-IOV VF 模式可能不支持 dumb buffer\n");
        return -1;
    }
    
    printf("  [OK] GEM handle: %u, size: %llu\n", 
           create_req.handle, (unsigned long long)create_req.size);
    
    // 尝试导出 DMA-BUF
    struct drm_prime_handle prime_req;
    memset(&prime_req, 0, sizeof(prime_req));
    prime_req.handle = create_req.handle;
    prime_req.flags = DRM_CLOEXEC | DRM_RDWR;
    
    printf("尝试导出 DMA-BUF (DRM_IOCTL_PRIME_HANDLE_TO_FD)...\n");
    ret = ioctl(fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_req);
    if (ret < 0) {
        printf("  [FAIL] 导出 DMA-BUF 失败: %s\n", strerror(errno));
        printf("  说明: Xe 驱动可能不支持 DMA-BUF 导出，或 SR-IOV VF 限制\n");
        
        // 清理
        memset(&close_bo, 0, sizeof(close_bo));
        close_bo.handle = create_req.handle;
        ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_bo);
        return -1;
    }
    
    printf("  [OK] DMA-BUF fd: %d\n", prime_req.fd);
    
    // 测试 DMA-BUF sync
    struct dma_buf_sync sync;
    memset(&sync, 0, sizeof(sync));
    sync.flags = DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW;
    
    printf("测试 DMA-BUF sync (START)...\n");
    ret = ioctl(prime_req.fd, DMA_BUF_IOCTL_SYNC, &sync);
    CHECK_IOCTL(ret, "DMA-BUF SYNC_START");
    
    sync.flags = DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW;
    printf("测试 DMA-BUF sync (END)...\n");
    ret = ioctl(prime_req.fd, DMA_BUF_IOCTL_SYNC, &sync);
    CHECK_IOCTL(ret, "DMA-BUF SYNC_END");
    
    // 尝试 mmap DMA-BUF
    printf("尝试 mmap DMA-BUF...\n");
    void *ptr = mmap(NULL, create_req.size, PROT_READ | PROT_WRITE, MAP_SHARED, 
                     prime_req.fd, 0);
    if (ptr == MAP_FAILED) {
        printf("  [FAIL] mmap 失败: %s\n", strerror(errno));
    } else {
        printf("  [OK] mmap 成功, addr: %p\n", ptr);
        
        // 尝试读写
        printf("测试 DMA-BUF 读写...\n");
        ((char*)ptr)[0] = 0xAB;
        if (((char*)ptr)[0] == 0xAB) {
            printf("  [OK] 读写测试通过\n");
        } else {
            printf("  [FAIL] 读写测试失败\n");
        }
        
        munmap(ptr, create_req.size);
    }
    
    // 关闭 DMA-BUF fd
    close(prime_req.fd);
    
    // 清理 GEM
    memset(&close_bo, 0, sizeof(close_bo));
    close_bo.handle = create_req.handle;
    ioctl(fd, DRM_IOCTL_GEM_CLOSE, &close_bo);
    
    return 0;
}

int test_dmabuf_import(int fd) {
    printf("\n=== DMA-BUF 导入测试 ===\n");
    printf("注意: 需要外部 DMA-BUF fd 才能测试导入\n");
    printf("  跳过 (需要另一个 GPU 或设备创建 DMA-BUF)\n");
    return 0;
}

int test_dmabuf_with_vulkan_memory(void) {
    printf("\n=== DMA-BUF 与 Vulkan 内存共享 ===\n");
    printf("原理说明:\n");
    printf("1. Vulkan 分配设备内存 (vkAllocateMemory)\n");
    printf("2. 使用 VK_EXT_external_memory_dma_buf 扩展\n");
    printf("3. 导出 DMA-BUF fd (vkGetMemoryFdKHR)\n");
    printf("4. 其他设备可以导入该 fd 进行共享\n");
    printf("\n要求:\n");
    printf("- 需要 Vulkan 扩展: VK_EXT_external_memory_dma_buf\n");
    printf("- 需要驱动支持 DMA-BUF 导出\n");
    printf("- SR-IOV VF 模式下可能受限\n");
    return 0;
}

int main(int argc, char** argv) {
    const char* device_path = "/dev/dri/renderD129";
    if (argc > 1) {
        device_path = argv[1];
    }
    
    printf("=== DMA-BUF 功能测试 ===\n");
    printf("设备: %s\n", device_path);
    
    int fd = open(device_path, O_RDWR | O_CLOEXEC);
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
    
    printf("设备打开成功 (fd=%d)\n", fd);
    
    // 运行测试
    test_dmabuf_basic(fd);
    test_xe_dmabuf_export(fd);
    test_dmabuf_import(fd);
    test_dmabuf_with_vulkan_memory();
    
    printf("\n=== 测试总结 ===\n");
    printf("DMA-BUF 关键功能:\n");
    printf("  1. PRIME_HANDLE_TO_FD: 将 GEM 句柄导出为 DMA-BUF fd\n");
    printf("  2. PRIME_FD_TO_HANDLE: 将 DMA-BUF fd 导入为 GEM 句柄\n");
    printf("  3. mmap: 通过 DMA-BUF fd 进行 CPU 访问\n");
    printf("  4. DMA_BUF_IOCTL_SYNC: 同步 DMA-BUF 访问\n");
    printf("\n用于 GPU 间共享:\n");
    printf("  - 导出: GPU1 创建内存 -> 导出 DMA-BUF fd\n");
    printf("  - 导入: GPU2 导入 fd -> 使用共享内存\n");
    printf("  - 零拷贝: 避免数据在 CPU 和 GPU 之间复制\n");
    
    close(fd);
    return 0;
}
