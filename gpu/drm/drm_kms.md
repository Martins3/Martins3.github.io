## 先观察一下 drm_info 的输出

### asahi linux
```txt
/dev/dri/
├── by-path
│   ├── platform-206400000.gpu-card -> ../card2
│   ├── platform-206400000.gpu-render -> ../renderD128
│   ├── platform-228200000.display-pipe-card -> ../card1
│   └── platform-soc:display-subsystem-card -> ../card3
├── card1
├── card2
├── card3
└── renderD128
```

```txt
🧀  sudo drm_info
[sudo] password for martins3:
drmModeGetResources: Operation not supported
Failed to retrieve information from /dev/dri/card2
Node: /dev/dri/card1
├───Driver: adp (Apple Display Pipe DRM Driver) version 0.1.0
│   ├───DRM_CLIENT_CAP_STEREO_3D supported
│   ├───DRM_CLIENT_CAP_UNIVERSAL_PLANES supported
│   ├───DRM_CLIENT_CAP_ATOMIC supported
│   ├───DRM_CLIENT_CAP_ASPECT_RATIO supported
│   ├───DRM_CLIENT_CAP_WRITEBACK_CONNECTORS supported
│   ├───DRM_CLIENT_CAP_CURSOR_PLANE_HOTSPOT not supported
│   ├───DRM_CAP_DUMB_BUFFER = 1
│   ├───DRM_CAP_VBLANK_HIGH_CRTC = 1
│   ├───DRM_CAP_DUMB_PREFERRED_DEPTH = 24
│   ├───DRM_CAP_DUMB_PREFER_SHADOW = 0
│   ├───DRM_CAP_PRIME = 3
│   ├───DRM_CAP_TIMESTAMP_MONOTONIC = 1
│   ├───DRM_CAP_ASYNC_PAGE_FLIP = 0
│   ├───DRM_CAP_CURSOR_WIDTH = 64
│   ├───DRM_CAP_CURSOR_HEIGHT = 64
│   ├───DRM_CAP_ADDFB2_MODIFIERS = 1
│   ├───DRM_CAP_PAGE_FLIP_TARGET = 0
│   ├───DRM_CAP_CRTC_IN_VBLANK_EVENT = 1
│   ├───DRM_CAP_SYNCOBJ = 0
│   ├───DRM_CAP_SYNCOBJ_TIMELINE = 0
│   └───DRM_CAP_ATOMIC_ASYNC_PAGE_FLIP = 0
├───Device: platform apple,t8112-display-pipe apple,h7-display-pipe
│   └───Available nodes: primary
├───Framebuffer size
│   ├───Width: [32, 64]
│   └───Height: [32, 2048]
├───Connectors
│   └───Connector 0
│       ├───Object ID: 37
│       ├───Type: DSI
│       ├───Status: connected
│       ├───Physical size: 0×0 mm
│       ├───Subpixel: unknown
│       ├───Encoders: {0}
│       ├───Modes
│       │   └───60×2008@60.00 preferred driver phsync nvsync
│       └───Properties
│           ├───"EDID" (immutable): blob = 0
│           ├───"DPMS": enum {On, Standby, Suspend, Off} = On
│           ├───"link-status": enum {Good, Bad} = Good
│           ├───"non-desktop" (immutable): range [0, 1] = 1
│           ├───"TILE" (immutable): blob = 0
│           └───"CRTC_ID" (atomic): object CRTC = 35
├───Encoders
│   └───Encoder 0
│       ├───Object ID: 36
│       ├───Type: DSI
│       ├───CRTCS: {0}
│       └───Clones: {0}
├───CRTCs
│   └───CRTC 0
│       ├───Object ID: 35
│       ├───Legacy info
│       │   ├───Mode: 60×2008@60.00 preferred driver phsync nvsync
│       │   └───Gamma size: 0
│       └───Properties
│           ├───"ACTIVE" (atomic): range [0, 1] = 1
│           ├───"MODE_ID" (atomic): blob = 39
│           │   └───60×2008@60.00 preferred driver phsync nvsync
│           ├───"OUT_FENCE_PTR" (atomic): range [0, UINT64_MAX] = 0
│           └───"VRR_ENABLED": range [0, 1] = 0
└───Planes
    └───Plane 0
        ├───Object ID: 33
        ├───CRTCs: {0}
        ├───Legacy info
        │   ├───FB ID: 38
        │   │   ├───Object ID: 38
        │   │   ├───Size: 64×2048
        │   │   ├───Format: XRGB8888 (0x34325258)
        │   │   ├───Modifier: DRM_FORMAT_MOD_LINEAR (0x0000000000000000)
        │   │   └───Planes:
        │   │       └───Plane 0: offset = 0, pitch = 256 bytes
        │   └───Formats:
        │       └───XRGB8888 (0x34325258)
        └───Properties
            ├───"type" (immutable): enum {Overlay, Primary, Cursor} = Primary
            ├───"FB_ID" (atomic): object framebuffer = 38
            │   ├───Object ID: 38
            │   ├───Size: 64×2048
            │   ├───Format: XRGB8888 (0x34325258)
            │   ├───Modifier: DRM_FORMAT_MOD_LINEAR (0x0000000000000000)
            │   └───Planes:
            │       └───Plane 0: offset = 0, pitch = 256 bytes
            ├───"IN_FENCE_FD" (atomic): srange [-1, INT32_MAX] = -1
            ├───"CRTC_ID" (atomic): object CRTC = 35
            ├───"CRTC_X" (atomic): srange [INT32_MIN, INT32_MAX] = 0
            ├───"CRTC_Y" (atomic): srange [INT32_MIN, INT32_MAX] = 0
            ├───"CRTC_W" (atomic): range [0, INT32_MAX] = 60
            ├───"CRTC_H" (atomic): range [0, INT32_MAX] = 2008
            ├───"SRC_X" (atomic): range [0, UINT32_MAX] = 0
            ├───"SRC_Y" (atomic): range [0, UINT32_MAX] = 0
            ├───"SRC_W" (atomic): range [0, UINT32_MAX] = 60
            ├───"SRC_H" (atomic): range [0, UINT32_MAX] = 2008
            └───"IN_FORMATS" (immutable): blob = 34
                └───DRM_FORMAT_MOD_LINEAR (0x0000000000000000)
                    └───XRGB8888 (0x34325258)
Node: /dev/dri/card3
├───Driver: apple (Apple display controller DRM driver) version 1.0.0
│   ├───DRM_CLIENT_CAP_STEREO_3D supported
│   ├───DRM_CLIENT_CAP_UNIVERSAL_PLANES supported
│   ├───DRM_CLIENT_CAP_ATOMIC supported
│   ├───DRM_CLIENT_CAP_ASPECT_RATIO supported
│   ├───DRM_CLIENT_CAP_WRITEBACK_CONNECTORS supported
│   ├───DRM_CLIENT_CAP_CURSOR_PLANE_HOTSPOT not supported
│   ├───DRM_CAP_DUMB_BUFFER = 1
│   ├───DRM_CAP_VBLANK_HIGH_CRTC = 1
│   ├───DRM_CAP_DUMB_PREFERRED_DEPTH = 0
│   ├───DRM_CAP_DUMB_PREFER_SHADOW = 0
│   ├───DRM_CAP_PRIME = 3
│   ├───DRM_CAP_TIMESTAMP_MONOTONIC = 1
│   ├───DRM_CAP_ASYNC_PAGE_FLIP = 0
│   ├───DRM_CAP_CURSOR_WIDTH = 64
│   ├───DRM_CAP_CURSOR_HEIGHT = 64
│   ├───DRM_CAP_ADDFB2_MODIFIERS = 1
│   ├───DRM_CAP_PAGE_FLIP_TARGET = 0
│   ├───DRM_CAP_CRTC_IN_VBLANK_EVENT = 1
│   ├───DRM_CAP_SYNCOBJ = 1
│   ├───DRM_CAP_SYNCOBJ_TIMELINE = 1
│   └───DRM_CAP_ATOMIC_ASYNC_PAGE_FLIP = 0
├───Device: platform apple,display-subsystem
│   └───Available nodes: primary
├───Framebuffer size
│   ├───Width: [32, 16384]
│   └───Height: [32, 16384]
├───Connectors
│   └───Connector 0
│       ├───Object ID: 41
│       ├───Type: eDP
│       ├───Status: connected
│       ├───Physical size: 286×179 mm
│       ├───Subpixel: unknown
│       ├───Encoders: {0}
│       ├───Modes
│       │   └───2560×1600@60.00 preferred driver
│       └───Properties
│           ├───"EDID" (immutable): blob = 0
│           ├───"DPMS": enum {On, Standby, Suspend, Off} = Off
│           ├───"link-status": enum {Good, Bad} = Good
│           ├───"non-desktop" (immutable): range [0, 1] = 0
│           ├───"TILE" (immutable): blob = 0
│           └───"CRTC_ID" (atomic): object CRTC = 0
├───Encoders
│   └───Encoder 0
│       ├───Object ID: 40
│       ├───Type: TMDS
│       ├───CRTCS: {0}
│       └───Clones: {0}
├───CRTCs
│   └───CRTC 0
│       ├───Object ID: 39
│       ├───Legacy info
│       │   └───Gamma size: 0
│       └───Properties
│           ├───"ACTIVE" (atomic): range [0, 1] = 0
│           ├───"MODE_ID" (atomic): blob = 0
│           ├───"OUT_FENCE_PTR" (atomic): range [0, UINT64_MAX] = 0
│           ├───"VRR_ENABLED": range [0, 1] = 0
│           └───"CTM": blob = 0
└───Planes
    ├───Plane 0
    │   ├───Object ID: 33
    │   ├───CRTCs: {0}
    │   ├───Legacy info
    │   │   ├───FB ID: 0
    │   │   └───Formats:
    │   │       ├───XRGB2101010 (0x30335258)
    │   │       ├───XRGB8888 (0x34325258)
    │   │       ├───ARGB8888 (0x34325241)
    │   │       ├───XBGR8888 (0x34324258)
    │   │       └───ABGR8888 (0x34324241)
    │   └───Properties
    │       ├───"type" (immutable): enum {Overlay, Primary, Cursor} = Primary
    │       ├───"FB_ID" (atomic): object framebuffer = 0
    │       ├───"IN_FENCE_FD" (atomic): srange [-1, INT32_MAX] = -1
    │       ├───"CRTC_ID" (atomic): object CRTC = 0
    │       ├───"CRTC_X" (atomic): srange [INT32_MIN, INT32_MAX] = 0
    │       ├───"CRTC_Y" (atomic): srange [INT32_MIN, INT32_MAX] = 0
    │       ├───"CRTC_W" (atomic): range [0, INT32_MAX] = 2560
    │       ├───"CRTC_H" (atomic): range [0, INT32_MAX] = 1600
    │       ├───"SRC_X" (atomic): range [0, UINT32_MAX] = 0
    │       ├───"SRC_Y" (atomic): range [0, UINT32_MAX] = 0
    │       ├───"SRC_W" (atomic): range [0, UINT32_MAX] = 2560
    │       ├───"SRC_H" (atomic): range [0, UINT32_MAX] = 1600
    │       ├───"IN_FORMATS" (immutable): blob = 34
    │       │   └───DRM_FORMAT_MOD_LINEAR (0x0000000000000000)
    │       │       ├───XRGB2101010 (0x30335258)
    │       │       ├───XRGB8888 (0x34325258)
    │       │       ├───ARGB8888 (0x34325241)
    │       │       ├───XBGR8888 (0x34324258)
    │       │       └───ABGR8888 (0x34324241)
    │       └───"zpos" (immutable): range [0, 0] = 0
    └───Plane 1
        ├───Object ID: 36
        ├───CRTCs: {0}
        ├───Legacy info
        │   ├───FB ID: 0
        │   └───Formats:
        │       ├───ARGB8888 (0x34325241)
        │       └───ABGR8888 (0x34324241)
        └───Properties
            ├───"type" (immutable): enum {Overlay, Primary, Cursor} = Overlay
            ├───"FB_ID" (atomic): object framebuffer = 0
            ├───"IN_FENCE_FD" (atomic): srange [-1, INT32_MAX] = -1
            ├───"CRTC_ID" (atomic): object CRTC = 0
            ├───"CRTC_X" (atomic): srange [INT32_MIN, INT32_MAX] = 0
            ├───"CRTC_Y" (atomic): srange [INT32_MIN, INT32_MAX] = 0
            ├───"CRTC_W" (atomic): range [0, INT32_MAX] = 0
            ├───"CRTC_H" (atomic): range [0, INT32_MAX] = 0
            ├───"SRC_X" (atomic): range [0, UINT32_MAX] = 0
            ├───"SRC_Y" (atomic): range [0, UINT32_MAX] = 0
            ├───"SRC_W" (atomic): range [0, UINT32_MAX] = 0
            ├───"SRC_H" (atomic): range [0, UINT32_MAX] = 0
            ├───"IN_FORMATS" (immutable): blob = 37
            │   └───DRM_FORMAT_MOD_LINEAR (0x0000000000000000)
            │       ├───ARGB8888 (0x34325241)
            │       └───ABGR8888 (0x34324241)
            └───"zpos" (immutable): range [1, 1] = 1
```

### kunpeng
```txt
/dev/dri/
├── by-path
│   └── pci-0000:03:00.0-card -> ../card0
└── card0
```

```txt
🧀  sudo drm_info
[sudo] password for martins3:
drmModeGetFB: No such device
drmModeGetFB: No such device
Node: /dev/dri/card0
├───Driver: hibmc (hibmc drm driver) version 1.0.0 (20160828)
│   ├───DRM_CLIENT_CAP_STEREO_3D supported
│   ├───DRM_CLIENT_CAP_UNIVERSAL_PLANES supported
│   ├───DRM_CLIENT_CAP_ATOMIC supported
│   ├───DRM_CLIENT_CAP_ASPECT_RATIO supported
│   ├───DRM_CLIENT_CAP_WRITEBACK_CONNECTORS supported
│   ├───DRM_CLIENT_CAP_CURSOR_PLANE_HOTSPOT not supported
│   ├───DRM_CAP_DUMB_BUFFER = 1
│   ├───DRM_CAP_VBLANK_HIGH_CRTC = 1
│   ├───DRM_CAP_DUMB_PREFERRED_DEPTH = 24
│   ├───DRM_CAP_DUMB_PREFER_SHADOW = 1
│   ├───DRM_CAP_PRIME = 0
│   ├───DRM_CAP_TIMESTAMP_MONOTONIC = 1
│   ├───DRM_CAP_ASYNC_PAGE_FLIP = 0
│   ├───DRM_CAP_CURSOR_WIDTH = 64
│   ├───DRM_CAP_CURSOR_HEIGHT = 64
│   ├───DRM_CAP_ADDFB2_MODIFIERS = 0
│   ├───DRM_CAP_PAGE_FLIP_TARGET = 0
│   ├───DRM_CAP_CRTC_IN_VBLANK_EVENT = 1
│   ├───DRM_CAP_SYNCOBJ = 0
│   ├───DRM_CAP_SYNCOBJ_TIMELINE not supported
│   └───DRM_CAP_ATOMIC_ASYNC_PAGE_FLIP not supported
├───Device: PCI 19e5:1711 Huawei Technologies Co., Ltd. Hi171x Series [iBMC Intelligent Management system chip w/VGA support]
│   └───Available nodes: primary
├───Framebuffer size
│   ├───Width: [0, 1920]
│   └───Height: [0, 1200]
├───Connectors
│   └───Connector 0
│       ├───Object ID: 30
│       ├───Type: VGA
│       ├───Status: connected
│       ├───Physical size: 0×0 mm
│       ├───Subpixel: unknown
│       ├───Encoders: {0}
│       ├───Modes
│       │   ├───640×480@60.00 userdef nhsync pvsync
│       │   ├───1024×768@60.00 preferred driver nhsync nvsync
│       │   ├───1920×1200@59.88 driver nhsync pvsync
│       │   ├───1920×1200@59.95 driver phsync nvsync
│       │   ├───1920×1080@60.00 driver nhsync nvsync
│       │   ├───1600×1200@60.00 driver phsync pvsync
│       │   ├───1600×900@60.00 driver phsync pvsync
│       │   ├───1280×1024@60.02 driver phsync pvsync
│       │   ├───1440×900@59.89 driver nhsync pvsync
│       │   ├───1440×900@59.90 driver phsync nvsync
│       │   ├───1280×960@60.00 driver phsync pvsync
│       │   ├───1280×768@59.87 driver nhsync pvsync
│       │   ├───1280×768@59.99 driver phsync nvsync
│       │   ├───1280×720@60.00 driver phsync pvsync
│       │   ├───800×600@60.32 driver phsync pvsync
│       │   └───640×480@59.94 driver nhsync nvsync
│       └───Properties
│           ├───"EDID" (immutable): blob = 0
│           ├───"DPMS": enum {On, Standby, Suspend, Off} = On
│           ├───"link-status": enum {Good, Bad} = Good
│           ├───"non-desktop" (immutable): range [0, 1] = 0
│           └───"CRTC_ID" (atomic): object CRTC = 29
├───Encoders
│   └───Encoder 0
│       ├───Object ID: 31
│       ├───Type: DAC
│       ├───CRTCS: {0}
│       └───Clones: {}
├───CRTCs
│   └───CRTC 0
│       ├───Object ID: 29
│       ├───Legacy info
│       │   ├───Mode: 640×480@60.00 userdef nhsync pvsync
│       │   └───Gamma size: 256
│       └───Properties
│           ├───"ACTIVE" (atomic): range [0, 1] = 1
│           ├───"MODE_ID" (atomic): blob = 41
│           │   └───640×480@60.00 userdef nhsync pvsync
│           └───"OUT_FENCE_PTR" (atomic): range [0, UINT64_MAX] = 0
└───Planes
    └───Plane 0
        ├───Object ID: 28
        ├───CRTCs: {0}
        ├───Legacy info
        │   ├───FB ID: 40
        │   └───Formats:
        │       ├───RGB565 (0x36314752)
        │       ├───BGR565 (0x36314742)
        │       ├───RGB888 (0x34324752)
        │       ├───BGR888 (0x34324742)
        │       ├───XRGB8888 (0x34325258)
        │       ├───XBGR8888 (0x34324258)
        │       ├───RGBA8888 (0x34324152)
        │       ├───BGRA8888 (0x34324142)
        │       ├───ARGB8888 (0x34325241)
        │       └───ABGR8888 (0x34324241)
        └───Properties
            ├───"type" (immutable): enum {Overlay, Primary, Cursor} = Primary
            ├───"FB_ID" (atomic): object framebuffer = 40
            ├───"IN_FENCE_FD" (atomic): srange [-1, INT32_MAX] = -1
            ├───"CRTC_ID" (atomic): object CRTC = 29
            ├───"CRTC_X" (atomic): srange [INT32_MIN, INT32_MAX] = 0
            ├───"CRTC_Y" (atomic): srange [INT32_MIN, INT32_MAX] = 0
            ├───"CRTC_W" (atomic): range [0, INT32_MAX] = 640
            ├───"CRTC_H" (atomic): range [0, INT32_MAX] = 480
            ├───"SRC_X" (atomic): range [0, UINT32_MAX] = 0
            ├───"SRC_Y" (atomic): range [0, UINT32_MAX] = 0
            ├───"SRC_W" (atomic): range [0, UINT32_MAX] = 640
            └───"SRC_H" (atomic): range [0, UINT32_MAX] = 480
```

### 13900k

### 看看 hyperv 下虚拟机有何不同


## 判断使用的是哪一个显卡
13900k 中，同时有内部的集成显卡和 nvidia 显卡:
1. 看哪个 DRM 连接器真的接了显示器


```sh
for f in /sys/class/drm/card*-*/status; do
    printf '%s: ' "$f"
    < "$f" tr -d '\n'
    printf '\n'
done
```
其中的结果为:
```txt
/sys/class/drm/card0-DP-1/status: disconnected
/sys/class/drm/card0-HDMI-A-1/status: disconnected
/sys/class/drm/card0-HDMI-A-2/status: connected
/sys/class/drm/card1-DP-2/status: disconnected
/sys/class/drm/card1-DP-3/status: disconnected
/sys/class/drm/card1-DP-4/status: disconnected
/sys/class/drm/card1-DVI-D-1/status: disconnected
/sys/class/drm/card1-HDMI-A-3/status: disconnected
```

3. 把 cardN 映射回 PCI 设备，确认是哪张卡

```sh
for d in /sys/class/drm/card[0-9]; do
    printf '%s -> %s\n' "$d" "$(readlink -f "$d/device")"
done
ls -l /dev/dri/by-path
```

```txt
/sys/class/drm/card0 -> /sys/devices/pci0000:00/0000:00:02.0
/sys/class/drm/card1 -> /sys/devices/pci0000:00/0000:00:01.0/0000:01:00.0
lrwxrwxrwx - root 16 Apr 21:26 pci-0000:00:02.0-card -> ../card0
lrwxrwxrwx - root 16 Apr 21:26 pci-0000:00:02.0-render -> ../renderD128
lrwxrwxrwx - root 16 Apr 21:26 pci-0000:01:00.0-card -> ../card1
lrwxrwxrwx - root 16 Apr 21:26 pci-0000:01:00.0-render -> ../renderD129
```
