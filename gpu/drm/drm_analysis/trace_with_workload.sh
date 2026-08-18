#!/usr/bin/env bash
# 实际触发GPU负载并trace

set -e

echo "=== DRM 子系统 Trace 实验 (带实际负载) ==="
echo ""

# 检查可用工具
echo "[1/5] 检查环境..."
if ! command -v bpftrace &> /dev/null; then
    echo "[FAIL] bpftrace 未安装"
    exit 1
fi
echo "[OK] bpftrace 可用"
echo ""

# 创建bpftrace脚本
cat > /tmp/trace_drm.bt << 'EOF'
#!/usr/bin/env bpftrace

BEGIN {
    printf("\n=== Trace 开始 ===\n");
    printf("等待GPU活动...\n\n");
}

// 追踪 GEM/TTM 相关调用
kprobe:ttm_bo_evict {
    printf("[%llu] [TTM] ttm_bo_evict\n", nsecs);
}

kprobe:ttm_bo_access {
    printf("[%llu] [TTM] ttm_bo_access\n", nsecs);
}

kprobe:ttm_bo_cleanup_memtype_use {
    printf("[%llu] [TTM] ttm_bo_cleanup_memtype_use\n", nsecs);
}

kprobe:ttm_resource_alloc {
    printf("[%llu] [TTM] ttm_resource_alloc type=%d\n", nsecs, arg1);
}

kprobe:drm_gem_dmabuf_export {
    printf("[%llu] [GEM] drm_gem_dmabuf_export\n", nsecs);
}

kprobe:drm_gem_dmabuf_mmap {
    printf("[%llu] [GEM] drm_gem_dmabuf_mmap\n", nsecs);
}

kprobe:drm_gem_create_mmap_offset {
    printf("[%llu] [GEM] drm_gem_create_mmap_offset\n", nsecs);
}

// Xe 驱动特定
kprobe:xe_bo_eviction_valuable {
    printf("[%llu] [Xe] xe_bo_eviction_valuable\n", nsecs);
}

kprobe:xe_bo_vm_access {
    printf("[%llu] [Xe] xe_bo_vm_access\n", nsecs);
}

// 追踪 ioctl 入口
kprobe:drm_ioctl {
    printf("[%llu] [DRM] drm_ioctl cmd=0x%x\n", nsecs, arg1);
}

// 追踪mmap
kprobe:drm_gem_mmap {
    printf("[%llu] [GEM] drm_gem_mmap\n", nsecs);
}

END {
    printf("\n=== Trace 结束 ===\n");
}
EOF

echo "[2/5] 启动 bpftrace (后台)..."
timeout 15 bpftrace /tmp/trace_drm.bt > /tmp/trace_output.log 2>&1 &
BPFTRACE_PID=$!
sleep 2
echo "[OK] bpftrace PID: $BPFTRACE_PID"
echo ""

echo "[3/5] 运行 GPU 负载测试..."
echo "    测试 1: Vulkan 显存分配 (64MB)"
cd /home/martins3/data/vn/.worktrees/xe/gpu_demo/intel_xe
VK_ICD_FILENAMES=/nix/store/l4myp7qn0q9bqgmkqq4vnnii22ql1r68-mesa-25.0.7/share/vulkan/icd.d/intel_icd.x86_64.json \
    ./vulkan_mem_test.out 64 > /tmp/vulkan_test.log 2>&1 || true
echo "[OK] Vulkan 测试完成"
echo ""

echo "    测试 2: 等待数据处理..."
sleep 3

# 检查bpftrace是否还在运行
if kill -0 $BPFTRACE_PID 2>/dev/null; then
    echo "    停止 bpftrace..."
    kill $BPFTRACE_PID 2>/dev/null || true
    wait $BPFTRACE_PID 2>/dev/null || true
fi
echo ""

echo "[4/5] 分析结果..."
echo ""

# 显示trace结果
echo "=== Trace 捕获结果 ==="
if [ -s /tmp/trace_output.log ]; then
    echo "原始输出 (前50行):"
    grep -v "^Attaching\|^ERROR\|^stdin:" /tmp/trace_output.log | head -50
    
    echo ""
    echo "统计:"
    echo "  TTM 事件:"
    grep -c "\[TTM\]" /tmp/trace_output.log 2>/dev/null || echo "    0"
    echo "  GEM 事件:"
    grep -c "\[GEM\]" /tmp/trace_output.log 2>/dev/null || echo "    0"
    echo "  Xe 事件:"
    grep -c "\[Xe\]" /tmp/trace_output.log 2>/dev/null || echo "    0"
    echo "  DRM 事件:"
    grep -c "\[DRM\]" /tmp/trace_output.log 2>/dev/null || echo "    0"
else
    echo "[WARN] 没有捕获到数据"
fi

echo ""
echo "[5/5] Vulkan 测试输出:"
cat /tmp/vulkan_test.log | grep -E "(成功|失败|分配|显存|GPU)" | head -20 || echo "(无输出)"

echo ""
echo "=== 实验完成 ==="

# 清理
rm -f /tmp/trace_drm.bt /tmp/trace_output.log /tmp/vulkan_test.log
