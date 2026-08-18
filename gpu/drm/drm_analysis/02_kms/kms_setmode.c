/**
 * KMS 设置模式实验
 * 
 * 演示如何设置显示模式
 * 
 * 警告: 此实验可能需要 root 权限，且会影响实际显示输出
 * 建议在虚拟机或没有重要显示的机器上运行
 * 
 * 内核实现: drivers/gpu/drm/drm_crtc.c
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

int test_set_mode(int fd) {
    printf("\n=== KMS 设置模式实验 ===\n\n");
    
    printf("警告: 此实验需要 root 权限，且会影响显示输出\n");
    printf("      在 SR-IOV VF 环境中可能无法运行\n\n");
    
    // 获取资源
    drmModeResPtr res = drmModeGetResources(fd);
    if (!res) {
        perror("drmModeGetResources 失败");
        return -1;
    }
    
    printf("发现 %d 个连接器\n", res->count_connectors);
    
    // 查找已连接的连接器
    drmModeConnectorPtr connector = NULL;
    for (int i = 0; i < res->count_connectors; i++) {
        connector = drmModeGetConnector(fd, res->connectors[i]);
        if (connector && connector->connection == DRM_MODE_CONNECTED) {
            printf("连接器 %u 已连接\n", connector->connector_id);
            break;
        }
        if (connector) {
            drmModeFreeConnector(connector);
            connector = NULL;
        }
    }
    
    if (!connector) {
        printf("没有已连接的显示器\n");
        drmModeFreeResources(res);
        return -1;
    }
    
    // 显示支持的模式
    printf("\n支持的显示模式:\n");
    for (int i = 0; i < connector->count_modes; i++) {
        printf("  [%d] %s @ %dHz\n", i, 
               connector->modes[i].name,
               connector->modes[i].vrefresh);
    }
    
    printf("\n说明: 实际设置模式需要 root 权限\n");
    printf("      并且需要完整的 KMS 初始化流程:\n");
    printf("      1. 创建 framebuffer\n");
    printf("      2. 找到匹配的 encoder 和 crtc\n");
    printf("      3. 调用 drmModeSetCrtc()\n\n");
    
    drmModeFreeConnector(connector);
    drmModeFreeResources(res);
    
    return 0;
}

void print_setmode_theory() {
    printf("\n=== KMS 设置模式原理 ===\n\n");
    
    printf("1. 设置显示模式的步骤:\n");
    printf("   a) 获取 KMS 资源 (connectors, encoders, crtcs)\n");
    printf("   b) 找到已连接的显示器\n");
    printf("   c) 创建 framebuffer (GEM 对象)\n");
    printf("   d) 找到匹配的 encoder 和 crtc\n");
    printf("   e) 调用 drmModeSetCrtc() 设置模式\n\n");
    
    printf("2. 核心 API:\n");
    printf("   drmModeGetResources()     - 获取 KMS 资源\n");
    printf("   drmModeGetConnector()     - 获取连接器信息\n");
    printf("   drmModeGetEncoder()       - 获取编码器信息\n");
    printf("   drmModeAddFB()            - 创建 framebuffer\n");
    printf("   drmModeSetCrtc()          - 设置 CRTC 模式\n\n");
    
    printf("3. 代码示例:\n");
    printf("   // 创建 framebuffer\n");
    printf("   uint32_t fb_id;\n");
    printf("   drmModeAddFB(fd, width, height, depth, bpp, pitch,\n");
    printf("                bo_handle, &fb_id);\n");
    printf("   \n");
    printf("   // 设置模式\n");
    printf("   drmModeSetCrtc(fd, crtc_id, fb_id, 0, 0,\n");
    printf("                  &connector_id, 1, &mode);\n\n");
}

int main(int argc, char** argv) {
    const char* device_path = "/dev/dri/card0";
    if (argc > 1) {
        device_path = argv[1];
    }
    
    printf("=== KMS 设置模式实验 ===\n");
    printf("设备: %s\n", device_path);
    
    int fd = open(device_path, O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror("打开设备失败");
        return 1;
    }
    
    test_set_mode(fd);
    print_setmode_theory();
    
    close(fd);
    return 0;
}
