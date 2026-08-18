#!/bin/bash
# gpu 生成的
# gpu-info.sh - GPU 信息查看工具
# 查看系统中 GPU 的详细信息

echo "================================"
echo "       GPU 信息查看器"
echo "================================"
echo ""

# 1. 检查 DRI 设备
echo "【DRM 设备】"
if [[ -d /sys/class/drm ]]; then
    for card in /sys/class/drm/card*; do
        if [[ -d "$card" ]]; then
            card_name=$(basename "$card")
            device_path=$(readlink -f "$card/device")
            if [[ -f "$device_path/vendor" ]]; then
                vendor_id=$(cat "$device_path/vendor" 2>/dev/null)
                device_id=$(cat "$device_path/device" 2>/dev/null)
                echo "  $card_name: Vendor=$vendor_id Device=$device_id"
            fi

            # 显示连接状态
            if [[ -f "$card/status" ]]; then
                status=$(cat "$card/status" 2>/dev/null)
                echo "    状态: $status"
            fi
        fi
    done
else
    echo "  无 DRM 设备"
fi
echo ""

# 2. 检查 GPU 驱动
echo "【已加载的 GPU 驱动】"
lsmod | grep -E "i915|amdgpu|nouveau|nvidia|virtio_gpu|radeon" | while read line; do
    echo "  $line"
done
echo ""

# 3. 检查 framebuffer
echo "【Framebuffer 设备】"
if [[ -d /sys/class/graphics ]]; then
    for fb in /sys/class/graphics/fb*; do
        if [[ -d "$fb" ]]; then
            fb_name=$(basename "$fb")
            if [[ -f "$fb/name" ]]; then
                fb_dev=$(cat "$fb/name" 2>/dev/null)
                echo "  $fb_name: $fb_dev"
            fi

            # 分辨率
            if [[ -f "$fb/virtual_size" ]]; then
                size=$(cat "$fb/virtual_size" 2>/dev/null)
                echo "    分辨率: $size"
            fi
        fi
    done
else
    echo "  无 Framebuffer 设备"
fi
echo ""

# 4. Intel GPU 信息
echo "【Intel GPU (i915/Xe)】"
if [[ -d /sys/class/drm/card0/device ]]; then
    gpu_dir="/sys/class/drm/card0/device"

    # 检查是否 Intel
    if [[ -f "$gpu_dir/vendor" ]]; then
        vendor=$(cat "$gpu_dir/vendor" 2>/dev/null)
        if [[ "$vendor" == "0x8086" ]]; then
            # Intel GPU
            if [[ -f "$gpu_dir/ gt_cur_freq_mhz" ]]; then
                cur_freq=$(cat "$gpu_dir/gt_cur_freq_mhz" 2>/dev/null)
                echo "  当前频率: ${cur_freq} MHz"
            fi

            if [[ -f "$gpu_dir/ gt_max_freq_mhz" ]]; then
                max_freq=$(cat "$gpu_dir/gt_max_freq_mhz" 2>/dev/null)
                echo "  最大频率: ${max_freq} MHz"
            fi

            # 显存使用
            if [[ -d "$gpu_dir/drm/card0/gt" ]]; then
                for gt in "$gpu_dir/drm/card0/gt/"*; do
                    if [[ -f "$gt/total_gpu_migrations" ]]; then
                        migrations=$(cat "$gt/total_gpu_migrations" 2>/dev/null)
                        echo "  GPU 迁移: $migrations"
                    fi
                done
            fi
        fi
    fi
else
    echo "  无 Intel GPU"
fi
echo ""

# 5. AMD GPU 信息
echo "【AMD GPU (amdgpu)】"
if [[ -d /sys/class/drm/card0/device/pp_dpm_sclk ]]; then
    echo "  核心频率状态:"
    cat /sys/class/drm/card0/device/pp_dpm_sclk 2>/dev/null | head -10

    if [[ -f /sys/class/drm/card0/device/gpu_busy_percent ]]; then
        busy=$(cat /sys/class/drm/card0/device/gpu_busy_percent 2>/dev/null)
        echo "  GPU 利用率: ${busy}%"
    fi

    if [[ -f /sys/class/drm/card0/device/mem_info_vram_used ]]; then
        vram_used=$(cat /sys/class/drm/card0/device/mem_info_vram_used 2>/dev/null)
        vram_total=$(cat /sys/class/drm/card0/device/mem_info_vram_total 2>/dev/null)
        echo "  显存使用: $((vram_used / 1024 / 1024)) MB / $((vram_total / 1024 / 1024)) MB"
    fi
else
    echo "  无 AMD GPU"
fi
echo ""

# 6. VirtIO GPU 信息
echo "【VirtIO GPU】"
if lsmod | grep -q virtio_gpu; then
    echo "  VirtIO GPU 已加载"

    # 检查是否有 VirtIO GPU 设备
    for card in /sys/class/drm/card*; do
        if [[ -d "$card" ]]; then
            device=$(readlink -f "$card/device")
            if [[ -f "$device/vendor" ]]; then
                vendor=$(cat "$device/vendor" 2>/dev/null)
                if [[ "$vendor" == "0x1af4" ]]; then  # Red Hat (VirtIO)
                    echo "  设备: $(basename $card)"

                    # 查看显存大小
                    if [[ -f "$card/virtio0/gfx_features" ]]; then
                        echo "  特性:"
                        cat "$card/virtio0/gfx_features" 2>/dev/null | head -5
                    fi
                fi
            fi
        fi
    done
else
    echo "  无 VirtIO GPU"
fi
echo ""

# 7. NVIDIA GPU (如有 nvidia-smi)
echo "【NVIDIA GPU】"
if command -v nvidia-smi > /dev/null 2>&1; then
    nvidia-smi --query-gpu=name,temperature.gpu,utilization.gpu,utilization.memory,memory.used,memory.total --format=csv,noheader 2>/dev/null | while read line; do
        echo "  $line"
    done
else
    echo "  nvidia-smi 不可用"
fi
echo ""

# 8. PCIe GPU 设备
echo "【PCIe GPU 设备】"
lspci -nn | grep -E "VGA|3D controller|Display controller" | while read line; do
    echo "  $line"
done
echo ""

# 9. 内核参数
echo "【相关内核参数】"
cat /proc/cmdline | tr ' ' '\n' | grep -E "video|drm|i915|amdgpu|nomodeset" | while read line; do
    echo "  $line"
done
echo ""

# 10. Debug 信息
echo "【DRM Debug 信息】"
if [[ -d /sys/kernel/debug/dri ]]; then
    for debug_dir in /sys/kernel/debug/dri/*; do
        if [[ -d "$debug_dir" ]]; then
            name=$(basename "$debug_dir")
            echo "  DRI $name:"

            # 状态
            if [[ -f "$debug_dir/state" ]]; then
                echo "    状态文件: 存在 (使用 'cat $debug_dir/state' 查看详情)"
            fi

            # GEM 对象
            if [[ -f "$debug_dir/gem_objects" ]]; then
                gem_count=$(cat "$debug_dir/gem_objects" 2>/dev/null | wc -l)
                echo "    GEM 对象: $gem_count 个"
            fi

            # Framebuffer
            if [[ -f "$debug_dir/framebuffer" ]]; then
                fb_info=$(cat "$debug_dir/framebuffer" 2>/dev/null | head -1)
                echo "    Framebuffer: $fb_info"
            fi
        fi
    done
else
    echo "  DebugFS 未挂载"
fi
echo ""

echo "================================"
echo "提示: 使用 'sudo cat /sys/kernel/debug/dri/0/state' 查看详细状态"
echo "================================"
