#!/usr/bin/env bash
# =============================================================================
# RDMA Zero-Copy 观察脚本
# =============================================================================

set -e

echo "========================================"
echo "RDMA Zero-Copy 观察方法"
echo "========================================"
echo ""

cd /home/martins3/data/vn/rdma-demo

# 方法1: 观察内存注册 (ibv_reg_mr) 的效果
echo "=== 方法1: 观察内存注册 (ibv_reg_mr) ==="
echo ""
echo "内存注册是 zero-copy 的核心，它会:"
echo "  1. Pin 内存页 (防止 swap)"
echo "  2. 建立 DMA 映射"
echo "  3. 返回 LKey/RKey"
echo ""

# 创建一个简单的内存注册测试程序
cat > /tmp/mr_test.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <infiniband/verbs.h>

int main() {
    struct ibv_device **dev_list;
    int num_devices;
    
    dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list || num_devices == 0) {
        printf("No RDMA devices\n");
        return 1;
    }
    
    struct ibv_context *ctx = ibv_open_device(dev_list[0]);
    struct ibv_pd *pd = ibv_alloc_pd(ctx);
    
    // 分配 10MB 内存
    size_t size = 10 * 1024 * 1024;
    void *buf = malloc(size);
    if (!buf) return 1;
    
    printf("PID: %d\n", getpid());
    printf("Buffer: %p - %p (size: %zu MB)\n", buf, (char*)buf + size, size / (1024*1024));
    printf("Before ibv_reg_mr: 查看 /proc/%d/smaps\n", getpid());
    printf("\nPress Enter to continue...\n");
    getchar();
    
    // 注册内存
    struct ibv_mr *mr = ibv_reg_mr(pd, buf, size,
        IBV_ACCESS_LOCAL_WRITE | IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE);
    
    printf("\nAfter ibv_reg_mr:\n");
    printf("  LKey: 0x%x, RKey: 0x%x\n", mr->lkey, mr->rkey);
    printf("  再次查看 /proc/%d/smaps\n", getpid());
    printf("  观察是否有 'rdma' 或 'pinned' 相关的内存区域\n");
    printf("\nPress Enter to continue...\n");
    getchar();
    
    ibv_dereg_mr(mr);
    free(buf);
    ibv_dealloc_pd(pd);
    ibv_close_device(ctx);
    ibv_free_device_list(dev_list);
    return 0;
}
EOF

gcc -O2 /tmp/mr_test.c -o /tmp/mr_test \
    -I/nix/store/55srcgw80lncngmvs0p0m98695wc1hkb-rdma-core-60.0-dev/include \
    -L/nix/store/ln64vgzhlx1k4h5gqlxkmwprfz7xwlqs-rdma-core-60.0/lib \
    -libverbs -Wl,-rpath,/nix/store/ln64vgzhlx1k4h5gqlxkmwprfz7xwlqs-rdma-core-60.0/lib 2>/dev/null

echo "运行内存注册测试..."
echo "你可以从另一个终端观察:"
echo "  cat /proc/\$(pgrep mr_test)/smaps | grep -E '(Anonymous|Locked|VmFlags)'"
echo ""

# 后台运行并自动发送回车
(/tmp/mr_test < /dev/null) &
PID=$!
sleep 2

if kill -0 $PID 2>/dev/null; then
    echo "程序 PID: $PID"
    echo ""
    echo "观察 smaps 中的变化:"
    cat /proc/$PID/smaps 2>/dev/null | grep -E "(Anonymous|Locked|VmFlags|Size)" | head -20 || true
    
    kill $PID 2>/dev/null || true
fi

echo ""
echo "========================================"
echo ""

# 方法2: strace 观察系统调用
echo "=== 方法2: strace 观察系统调用 ==="
echo ""
echo "对比 RDMA 和 TCP 的系统调用差异:"
echo ""
echo "RDMA 数据传输 (ibv_post_send/recv):"
echo "  - 无 read/write/send/recv 系统调用"
echo "  - 只有初始化的 ioctl"
echo "  - 数据传输完全绕过内核"
echo ""
echo "TCP 数据传输:"
echo "  - send() / recv() 系统调用"
echo "  - 数据从用户态拷贝到内核 socket buffer"
echo "  - 内核再将数据拷贝到网卡"
echo ""

echo "演示: strace 追踪 RDMA pingpong 程序..."
echo "(只显示前30行相关系统调用)"
echo ""

# 启动 RDMA 服务端
ssh -f root@10.0.3.2 "timeout 10 /tmp/rdma_pingpong_poll.out -s -d mlx5_0 -i 5" > /tmp/srv_zc.log 2>&1
sleep 2

# 追踪客户端
strace -f -e trace=send,recv,write,read,ioctl ./rdma_pingpong_poll.out -d rocep0s6 -i 5 10.0.3.2 2>&1 | head -30 || true

echo ""
echo "========================================"
echo ""

# 方法3: 观察 DMA 映射
echo "=== 方法3: 观察 DMA 映射 ==="
echo ""
echo "查看网卡设备的 DMA 相关信息:"
echo ""

# 查看 mlx5 设备的内核日志
echo "1. 内核日志中的内存注册信息:"
dmesg 2>/dev/null | grep -iE "(mlx5|rdma|mr|dma)" | tail -10 || echo "  (需要 root 权限查看 dmesg)"

echo ""
echo "2. 查看网卡统计中的 DMA 信息:"
cat /sys/class/infiniband/rocep0s6/device/infiniband/rocep0s6/ports/1/counters/* 2>/dev/null | head -10 || echo "  (计数器信息)"

echo ""
echo "3. 查看内存区域锁定的统计:"
cat /proc/meminfo 2>/dev/null | grep -E "(AnonPages|Mlocked|HugePages)" || echo "  (meminfo 信息)"

echo ""
echo "========================================"
echo ""

# 方法4: 使用 perf 观察缓存行为
echo "=== 方法4: 使用 perf 观察缓存行为 ==="
echo ""
echo "Zero-copy 减少了数据拷贝，会表现为:"
echo "  - 更低的 cache-misses"
echo "  - 更低的 dTLB-load-misses"
echo "  - 没有 memcpy 相关的开销"
echo ""
echo "运行 perf stat 对比 (示例):"
echo "  perf stat -e cache-misses,dTLB-load-misses ./rdma_pingpong_poll.out ..."
echo ""

# 方法5: 观察中断和事件
echo "========================================"
echo ""
echo "=== 方法5: 观察中断处理 ==="
echo ""
echo "RDMA zero-copy 并不意味着零中断:"
echo "  - Polling 模式: 无中断，但 CPU 100%"
echo "  - Interrupt 模式: 有中断，CPU 低"
echo ""
echo "查看 RDMA 相关中断:"
cat /proc/interrupts 2>/dev/null | grep -E "(mlx|roce|rdma)" | head -5 || echo "  中断信息"

echo ""
echo "========================================"
echo "Zero-Copy 观察总结"
echo "========================================"
echo ""
echo "关键观察点:"
echo "1. 内存注册后，smaps 中显示 Locked 页"
echo "2. 数据传输时无 send/write 系统调用"
echo "3. 低 cache-miss 率 (无拷贝开销)"
echo "4. 网卡直接访问用户态内存 (DMA)"
echo ""
echo "详细分析见: ZERO_COPY_ANALYSIS.md"
echo "========================================"
