#!/usr/bin/env bash
#===============================================================================
# Intel GPU 驱动恢复脚本: xe -> i915
#===============================================================================

set -E -e -u -o pipefail

INTEL_GPU="0000:00:02.0"

main() {
    if [[ $EUID -ne 0 ]]; then
        echo "ERROR: 需要 root 权限" >&2
        exit 1
    fi
    
    echo "========== 恢复 i915 驱动 =========="
    
    # 解绑 xe
    echo "[*] 解绑 xe..."
    echo "${INTEL_GPU}" > /sys/bus/pci/drivers/xe/unbind 2>/dev/null || true
    sleep 1
    
    # 卸载 xe
    echo "[*] 卸载 xe..."
    modprobe -r xe 2>/dev/null || true
    
    # 加载 i915
    echo "[*] 加载 i915..."
    modprobe i915
    sleep 1
    
    # 验证
    echo ""
    echo "========== 验证结果 =========="
    lspci -k -s 00:02.0 | grep "Kernel driver"
}

main "$@"
