/**
 * KMS 信息显示实验
 * 
 * 演示如何查询 KMS (Kernel Mode Setting) 资源
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

// 连接器类型名称
const char* connector_type_name(int type) {
    switch (type) {
        case DRM_MODE_CONNECTOR_Unknown:     return "Unknown";
        case DRM_MODE_CONNECTOR_VGA:         return "VGA";
        case DRM_MODE_CONNECTOR_DVII:        return "DVI-I";
        case DRM_MODE_CONNECTOR_DVID:        return "DVI-D";
        case DRM_MODE_CONNECTOR_DVIA:        return "DVI-A";
        case DRM_MODE_CONNECTOR_Composite:   return "Composite";
        case DRM_MODE_CONNECTOR_SVIDEO:      return "S-Video";
        case DRM_MODE_CONNECTOR_LVDS:        return "LVDS";
        case DRM_MODE_CONNECTOR_Component:   return "Component";
        case DRM_MODE_CONNECTOR_9PinDIN:     return "9-pin DIN";
        case DRM_MODE_CONNECTOR_DisplayPort: return "DisplayPort";
        case DRM_MODE_CONNECTOR_HDMIA:       return "HDMI-A";
        case DRM_MODE_CONNECTOR_HDMIB:       return "HDMI-B";
        case DRM_MODE_CONNECTOR_TV:          return "TV";
        case DRM_MODE_CONNECTOR_eDP:         return "eDP";
        case DRM_MODE_CONNECTOR_VIRTUAL:     return "Virtual";
        case DRM_MODE_CONNECTOR_DSI:         return "DSI";
        default:                             return "Other";
    }
}

// 编码器类型名称
const char* encoder_type_name(int type) {
    switch (type) {
        case DRM_MODE_ENCODER_NONE:  return "None";
        case DRM_MODE_ENCODER_DAC:   return "DAC";
        case DRM_MODE_ENCODER_TMDS:  return "TMDS";
        case DRM_MODE_ENCODER_LVDS:  return "LVDS";
        case DRM_MODE_ENCODER_TVDAC: return "TVDAC";
        case DRM_MODE_ENCODER_VIRTUAL: return "Virtual";
        case DRM_MODE_ENCODER_DSI:   return "DSI";
        case DRM_MODE_ENCODER_DPMST: return "DP MST";
        default:                     return "Other";
    }
}

// 打印显示模式
void print_mode(const drmModeModeInfo* mode) {
    printf("    %s @ %dHz (preferred: %d)\n", 
           mode->name, mode->vrefresh,
           mode->type & DRM_MODE_TYPE_PREFERRED ? 1 : 0);
    printf("      Clock: %d kHz, %dx%d\n", 
           mode->clock, mode->hdisplay, mode->vdisplay);
    printf("      H: %d %d %d %d, V: %d %d %d %d\n",
           mode->hdisplay, mode->hsync_start, mode->hsync_end, mode->htotal,
           mode->vdisplay, mode->vsync_start, mode->vsync_end, mode->vtotal);
}

int test_kms_resources(int fd) {
    printf("\n=== KMS 资源查询 ===\n\n");
    
    // 获取 KMS 资源
    drmModeResPtr res = drmModeGetResources(fd);
    if (!res) {
        perror("drmModeGetResources 失败");
        return -1;
    }
    
    printf("KMS 资源概览:\n");
    printf("  CRTCs: %d\n", res->count_crtcs);
    printf("  Encoders: %d\n", res->count_encoders);
    printf("  Connectors: %d\n", res->count_connectors);
    printf("  Framebuffers: %d\n", res->count_fbs);
    printf("  Min/Max 分辨率: %dx%d - %dx%d\n\n",
           res->min_width, res->min_height,
           res->max_width, res->max_height);
    
    // 查询每个 CRTC
    printf("CRTCs (显示控制器):\n");
    for (int i = 0; i < res->count_crtcs; i++) {
        drmModeCrtcPtr crtc = drmModeGetCrtc(fd, res->crtcs[i]);
        if (crtc) {
            printf("  [%d] CRTC %u:\n", i, crtc->crtc_id);
            printf("    缓冲区: %u\n", crtc->buffer_id);
            printf("    位置: (%d, %d)\n", crtc->x, crtc->y);
            printf("    尺寸: %dx%d\n", crtc->width, crtc->height);
            if (crtc->mode_valid) {
                printf("    当前模式: %s @ %dHz\n", 
                       crtc->mode.name, crtc->mode.vrefresh);
            }
            drmModeFreeCrtc(crtc);
        }
    }
    printf("\n");
    
    // 查询每个 Encoder
    printf("Encoders (编码器):\n");
    for (int i = 0; i < res->count_encoders; i++) {
        drmModeEncoderPtr enc = drmModeGetEncoder(fd, res->encoders[i]);
        if (enc) {
            printf("  [%d] Encoder %u: %s\n", 
                   i, enc->encoder_id, encoder_type_name(enc->encoder_type));
            printf("    CRTC: %u, Possible CRTCs: 0x%x\n", 
                   enc->crtc_id, enc->possible_crtcs);
            drmModeFreeEncoder(enc);
        }
    }
    printf("\n");
    
    // 查询每个 Connector
    printf("Connectors (连接器):\n");
    for (int i = 0; i < res->count_connectors; i++) {
        drmModeConnectorPtr conn = drmModeGetConnector(fd, res->connectors[i]);
        if (conn) {
            printf("  [%d] Connector %u: %s\n", 
                   i, conn->connector_id, 
                   connector_type_name(conn->connector_type));
            
            // 连接状态
            const char* status = conn->connection == DRM_MODE_CONNECTED 
                                 ? "已连接" : "未连接";
            printf("    状态: %s\n", status);
            
            if (conn->connection == DRM_MODE_CONNECTED) {
                printf("    物理尺寸: %dmm x %dmm\n", 
                       conn->mmWidth, conn->mmHeight);
                
                // 显示支持的模式
                printf("    支持 %d 种显示模式:\n", conn->count_modes);
                for (int j = 0; j < conn->count_modes && j < 5; j++) {
                    print_mode(&conn->modes[j]);
                }
                if (conn->count_modes > 5) {
                    printf("    ... 还有 %d 种模式\n", conn->count_modes - 5);
                }
            }
            
            drmModeFreeConnector(conn);
        }
    }
    
    drmModeFreeResources(res);
    return 0;
}

void print_kms_architecture() {
    printf("\n=== KMS 架构说明 ===\n\n");
    
    printf("1. KMS 解决了什么问题?\n");
    printf("   传统 UMS (User Mode Setting):\n");
    printf("     - X Server 直接操作硬件寄存器\n");
    printf("     - 需要 root 权限\n");
    printf("     - 切换显示模式时容易崩溃\n\n");
    
    printf("   KMS (Kernel Mode Setting):\n");
    printf("     - 内核统一管理显示硬件\n");
    printf("     - 用户空间通过 ioctl 安全访问\n");
    printf("     - 支持无缝显示切换\n\n");
    
    printf("2. KMS 组件关系:\n");
    printf("   ┌─────────────┐     ┌─────────────┐     ┌─────────────┐\n");
    printf("   │   CRTC      │────→│   Encoder   │────→│  Connector  │\n");
    printf("   │ (扫描缓冲区) │     │ (信号编码)   │     │ (物理接口)   │\n");
    printf("   └──────┬──────┘     └─────────────┘     └─────────────┘\n");
    printf("          │                                               │\n");
    printf("          │ 扫描 Framebuffer                               │ 连接\n");
    printf("          ▼                                               ▼\n");
    printf("   ┌─────────────┐                                ┌─────────────┐\n");
    printf("   │ Framebuffer │                                │   显示器     │\n");
    printf("   │ (GEM 对象)  │                                │             │\n");
    printf("   └─────────────┘                                └─────────────┘\n\n");
    
    printf("3. 工作流程:\n");
    printf("   a) 应用绘制 → Framebuffer (GEM 显存)\n");
    printf("   b) CRTC 扫描 → 读取像素数据\n");
    printf("   c) Encoder 编码 → 转换为 HDMI/DP 信号\n");
    printf("   d) Connector 输出 → 到显示器\n\n");
    
    printf("4. Plane (现代 KMS 新增):\n");
    printf("   - 支持多层叠加 (类似于 Android SurfaceFlinger)\n");
    printf("   - 硬件合成，无需 CPU 参与\n");
    printf("   - 支持透明度、缩放、旋转\n\n");
}

int main(int argc, char** argv) {
    const char* device_path = "/dev/dri/card0";
    if (argc > 1) {
        device_path = argv[1];
    }
    
    printf("=== KMS (Kernel Mode Setting) 实验 ===\n");
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
    
    // 获取驱动信息
    drmVersionPtr version = drmGetVersion(fd);
    if (version) {
        printf("驱动: %s\n\n", version->name);
        drmFreeVersion(version);
    }
    
    // 运行实验
    test_kms_resources(fd);
    print_kms_architecture();
    
    close(fd);
    return 0;
}
