/**
 * TTM 内存管理实验
 * 
 * TTM (Translation Table Maps) 是 DRM 子系统中的通用 GPU 内存管理器
 * 
 * 内核实现: drivers/gpu/drm/ttm/
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

// 尝试分配大量显存，触发 TTM 的驱逐机制
int test_ttm_memory_pressure(int fd) {
    printf("\n=== TTM 内存压力测试 ===\n");
    printf("说明: 尝试分配大量显存，观察 TTM 的行为\n\n");
    
    // 获取显卡信息
    drmVersionPtr version = drmGetVersion(fd);
    if (version) {
        printf("驱动: %s\n", version->name);
        drmFreeVersion(version);
    }
    
    // 尝试获取 VRAM 大小
    // 注意: 这需要驱动特定的接口
    printf("\nVRAM 信息:\n");
    printf("  注意: 查询 VRAM 大小需要驱动特定接口\n");
    printf("  Intel Xe: /sys/class/drm/card*/device/mem_info_vram_total\n");
    printf("  AMDGPU: /sys/kernel/debug/dri/*/amdgpu_vram_mm\n\n");
    
    // 尝试读取 sysfs
    char path[256];
    snprintf(path, sizeof(path), "/sys/class/drm/card%d/device/mem_info_vram_total", 
             (fd >= 0) ? 0 : 0);
    
    FILE* f = fopen(path, "r");
    if (f) {
        uint64_t vram_total;
        if (fscanf(f, "%lu", &vram_total) == 1) {
            printf("  VRAM 总大小: %.2f MB\n", vram_total / (1024.0 * 1024.0));
        }
        fclose(f);
    } else {
        printf("  无法读取 VRAM 信息\n");
    }
    
    printf("\n测试: 连续分配缓冲区...\n");
    
    #define MAX_BOS 16
    struct dumb_bo {
        uint32_t handle;
        uint64_t size;
        void* map;
    } bos[MAX_BOS];
    
    int num_bos = 0;
    size_t alloc_size = 64 * 1024 * 1024;  // 64MB 每次
    
    for (int i = 0; i < MAX_BOS; i++) {
        // 创建 dumb buffer
        struct drm_mode_create_dumb create;
        memset(&create, 0, sizeof(create));
        create.width = alloc_size / 4;
        create.height = 1;
        create.bpp = 32;
        
        int ret = ioctl(fd, DRM_IOCTL_MODE_CREATE_DUMB, &create);
        if (ret < 0) {
            printf("  分配第 %d 个缓冲区失败: %s\n", i + 1, strerror(errno));
            printf("  可能原因: VRAM 已满，TTM 开始驱逐\n");
            break;
        }
        
        bos[num_bos].handle = create.handle;
        bos[num_bos].size = create.size;
        
        printf("  [OK] BO %d: handle=%u, size=%.2f MB\n", 
               i + 1, create.handle, create.size / (1024.0 * 1024.0));
        
        num_bos++;
        
        // 映射并写入数据，确保内存被真正分配
        struct drm_mode_map_dumb map_req;
        memset(&map_req, 0, sizeof(map_req));
        map_req.handle = create.handle;
        
        ret = ioctl(fd, DRM_IOCTL_MODE_MAP_DUMB, &map_req);
        if (ret == 0) {
            void* ptr = mmap(NULL, create.size, PROT_READ | PROT_WRITE, 
                           MAP_SHARED, fd, map_req.offset);
            if (ptr != MAP_FAILED) {
                // 写入数据，强制分配物理内存
                memset(ptr, i, create.size > 4096 ? 4096 : create.size);
                munmap(ptr, create.size);
            }
        }
        
        printf("     累计分配: %.2f MB\n", (num_bos * alloc_size) / (1024.0 * 1024.0));
    }
    
    printf("\n总共成功分配: %d 个缓冲区 (%.2f MB)\n", 
           num_bos, (num_bos * alloc_size) / (1024.0 * 1024.0));
    
    // 释放缓冲区
    printf("\n释放缓冲区...\n");
    for (int i = 0; i < num_bos; i++) {
        struct drm_mode_destroy_dumb destroy;
        memset(&destroy, 0, sizeof(destroy));
        destroy.handle = bos[i].handle;
        ioctl(fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy);
    }
    printf("[OK] 已释放所有缓冲区\n");
}

void print_ttm_theory() {
    printf("\n=== TTM 原理详解 ===\n\n");
    
    printf("1. TTM 是什么?\n");
    printf("   TTM = Translation Table Maps\n");
    printf("   是 Linux DRM 的通用 GPU 内存管理器\n\n");
    
    printf("2. TTM 管理的内存类型:\n");
    printf("   ┌─────────────────────────────────────────────────┐\n");
    printf("   │ TTM_PL_SYSTEM  - 系统内存 (CPU 访问快, GPU 慢)   │\n");
    printf("   │ TTM_PL_TT      - GART/PCIe 映射内存             │\n");
    printf("   │ TTM_PL_VRAM    - 显卡显存 (GPU 快, CPU 慢)      │\n");
    printf("   └─────────────────────────────────────────────────┘\n\n");
    
    printf("3. TTM 的核心功能:\n");
    printf("   a) 内存分配/释放\n");
    printf("   b) 内存迁移 (System ↔ VRAM)\n");
    printf("   c) 内存驱逐 (VRAM 满时换出到 System)\n");
    printf("   d) 分页管理 (处理不连续的物理内存)\n\n");
    
    printf("4. 内存迁移示例:\n");
    printf("   场景: GPU 需要处理在系统内存中的纹理\n");
    printf("   \n");
    printf("   步骤 1: TTM 在 VRAM 分配空间\n");
    printf("   步骤 2: 使用 DMA 将数据复制到 VRAM\n");
    printf("   步骤 3: 更新 Buffer Object 指向 VRAM\n");
    printf("   步骤 4: GPU 高速访问 VRAM\n");
    printf("   步骤 5: 释放系统内存\n\n");
    
    printf("5. 驱逐 (Eviction) 机制:\n");
    printf("   当 VRAM 满时:\n");
    printf("   - TTM 查找最久未使用的 Buffer Object\n");
    printf("   - 将该 BO 的数据复制到系统内存\n");
    printf("   - 释放 VRAM 空间\n");
    printf("   - 下次访问时再从系统内存迁移回来\n\n");
    
    printf("6. TTM vs GEM:\n");
    printf("   GEM: 高层 API，管理显存对象句柄\n");
    printf("   TTM: 底层实现，管理物理内存页\n");
    printf("   现代驱动 (如 Intel Xe) 两者结合使用\n\n");
    
    printf("7. 观察 TTM:\n");
    printf("   # 查看 TTM 内存状态\n");
    printf("   cat /sys/kernel/debug/dri/0/ttm_buffer_objects\n");
    printf("   \n");
    printf("   # 查看驱逐统计\n");
    printf("   cat /sys/kernel/debug/dri/0/ttm_page_pool\n");
}

void print_ttm_observation() {
    printf("\n=== 如何观察 TTM 行为 ===\n\n");
    
    printf("1. 通过 debugfs:\n");
    printf("   sudo cat /sys/kernel/debug/dri/*/ttm_buffer_objects\n");
    printf("   sudo cat /sys/kernel/debug/dri/*/amdgpu_vram_mm  # AMD\n\n");
    
    printf("2. 通过 dmesg:\n");
    printf("   sudo dmesg | grep -i ttm\n\n");
    
    printf("3. 通过性能监控:\n");
    printf("   sudo intel_gpu_top  # Intel\n");
    printf("   sudo radeon-profile  # AMD\n\n");
    
    printf("4. 本实验观察:\n");
    printf("   - 分配大量缓冲区，观察何时失败\n");
    printf("   - 失败时说明 VRAM 已满，TTM 正在工作\n\n");
}

int main(int argc, char** argv) {
    const char* device_path = "/dev/dri/card0";
    if (argc > 1) {
        device_path = argv[1];
    }
    
    printf("=== TTM (Translation Table Maps) 实验 ===\n");
    printf("设备: %s\n", device_path);
    
    int fd = open(device_path, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("打开设备失败");
        device_path = "/dev/dri/card1";
        printf("尝试: %s\n", device_path);
        fd = open(device_path, O_RDWR | O_CLOEXEC);
        if (fd < 0) {
            perror("打开也失败");
            return 1;
        }
    }
    
    // 运行实验
    test_ttm_memory_pressure(fd);
    print_ttm_theory();
    print_ttm_observation();
    
    close(fd);
    return 0;
}
