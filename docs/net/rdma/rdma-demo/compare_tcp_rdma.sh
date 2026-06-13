#!/usr/bin/env bash
# =============================================================================
# TCP vs RDMA Zero-Copy 对比测试
# =============================================================================

set -e

echo "========================================"
echo "TCP vs RDMA Zero-Copy 对比"
echo "========================================"
echo ""

cd /home/martins3/data/vn/rdma-demo

# 创建一个简单的 TCP 测试程序用于对比
cat > /tmp/tcp_dummy.c << 'EOF'
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define DATA_SIZE (1024 * 1024)  // 1MB

int main(int argc, char **argv) {
    int sock;
    struct sockaddr_in addr;
    char *buffer = malloc(DATA_SIZE);
    
    if (!buffer) return 1;
    memset(buffer, 'A', DATA_SIZE);
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        // 连接失败，模拟发送操作
        printf("[TCP] Simulating send() of %d bytes...\n", DATA_SIZE);
        printf("[TCP] System call: send(sock, buffer, %d, 0)\n", DATA_SIZE);
        printf("[TCP] Data path: user buffer -> kernel socket buffer -> NIC\n");
        printf("[TCP] Copies: 2 (user->kernel, kernel->NIC)\n");
    } else {
        send(sock, buffer, DATA_SIZE, 0);
    }
    
    close(sock);
    free(buffer);
    return 0;
}
EOF

gcc -O2 /tmp/tcp_dummy.c -o /tmp/tcp_dummy 2>/dev/null

echo "=== 1. 系统调用对比 ==="
echo ""
echo "TCP 程序的系统调用 (会有 send/write):"
echo "---"
strace -e trace=send,write,read,recv /tmp/tcp_dummy 2>&1 | grep -E "(send|write)" | head -3 || echo "  (模拟输出)"
echo ""

echo "RDMA 程序的系统调用 (无 send/write，只有 ioctl):"
echo "---"
# 使用 zero_copy_demo 程序
strace -e trace=write,send,ioctl ./zero_copy_demo.out 2>&1 | grep -E "(ioctl|write|send)" | head -5 || true
echo ""

echo "========================================"
echo ""
echo "=== 2. 数据传输路径对比 ==="
echo ""
echo "TCP 路径:"
echo "  应用缓冲区"
echo "      |"
echo "      v (copy)
  内核 socket buffer"
echo "      |"
echo "      v (copy)
  网卡"
echo "  [2次拷贝]"
echo ""

echo "RDMA 路径:"
echo "  应用缓冲区 (已注册)"
echo "      |"
echo "      | (DMA, 无拷贝)"
echo "      v"
echo "  网卡"
echo "  [0次拷贝 - Zero-Copy!]"
echo ""

echo "========================================"
echo ""
echo "=== 3. 内存注册观察 ==="
echo ""
./zero_copy_demo.out 2>&1 | grep -A 10 "Memory registered"
echo ""

echo "========================================"
echo ""
echo "=== 4. 关键区别总结 ==="
echo ""
printf "%-20s %-15s %-15s\n" "特性" "TCP" "RDMA"
printf "%-20s %-15s %-15s\n" "----" "---" "----"
printf "%-20s %-15s %-15s\n" "用户态拷贝" "有 (send)" "无"
printf "%-20s %-15s %-15s\n" "内核态拷贝" "有" "无"
printf "%-20s %-15s %-15s\n" "DMA 直接访问" "否" "是"
printf "%-20s %-15s %-15s\n" "内存注册" "无需" "需要"
printf "%-20s %-15s %-15s\n" "延迟" "高" "低"
printf "%-20s %-15s %-15s\n" "CPU 占用" "高" "低"
echo ""

echo "========================================"
echo "详细分析见: ZERO_COPY_ANALYSIS.md"
echo "========================================"
