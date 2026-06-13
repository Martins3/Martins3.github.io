/*
 * Zero-Copy 演示程序
 * 
 * 对比 RDMA 和 TCP 的数据传输路径
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <infiniband/verbs.h>

#define DATA_SIZE (1024 * 1024)  // 1MB
#define PORT 12345

// 填充数据模式，便于识别
void fill_pattern(void *buf, size_t size) {
    unsigned char *p = buf;
    for (size_t i = 0; i < size; i++) {
        p[i] = (unsigned char)(i & 0xFF);
    }
}

// TCP 发送数据（需要内核拷贝）
int tcp_send_test(const char *server_ip) {
    int sock;
    struct sockaddr_in addr;
    void *buffer;
    
    printf("\n=== TCP Send Test ===\n");
    
    // 分配用户态缓冲区
    buffer = malloc(DATA_SIZE);
    if (!buffer) return -1;
    fill_pattern(buffer, DATA_SIZE);
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        free(buffer);
        return -1;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    inet_pton(AF_INET, server_ip, &addr.sin_addr);
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sock);
        free(buffer);
        return -1;
    }
    
    printf("Sending %d bytes via TCP...\n", DATA_SIZE);
    printf("Data path: user buffer -> kernel socket buffer -> NIC\n");
    printf("(至少一次拷贝: 用户态 -> 内核态)\n\n");
    
    // TCP send: 数据从用户态拷贝到内核 socket buffer
    size_t sent = 0;
    while (sent < DATA_SIZE) {
        ssize_t n = send(sock, (char *)buffer + sent, DATA_SIZE - sent, 0);
        if (n < 0) {
            perror("send");
            break;
        }
        sent += n;
    }
    
    printf("Sent %zu bytes\n", sent);
    
    close(sock);
    free(buffer);
    return 0;
}

// RDMA Write 测试（zero-copy）
int rdma_write_test(const char *server_ip) {
    struct ibv_device **dev_list;
    int num_devices;
    struct ibv_context *ctx = NULL;
    struct ibv_pd *pd = NULL;
    struct ibv_mr *mr = NULL;
    struct ibv_cq *cq = NULL;
    struct ibv_qp *qp = NULL;
    void *buffer = NULL;
    
    printf("\n=== RDMA Write Test (Zero-Copy) ===\n");
    
    dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list || num_devices == 0) {
        printf("No RDMA devices\n");
        return -1;
    }
    
    ctx = ibv_open_device(dev_list[0]);
    if (!ctx) goto cleanup;
    
    pd = ibv_alloc_pd(ctx);
    if (!pd) goto cleanup;
    
    // 分配用户态缓冲区
    buffer = malloc(DATA_SIZE);
    if (!buffer) goto cleanup;
    fill_pattern(buffer, DATA_SIZE);
    
    printf("Registering memory (pinning pages)...\n");
    // 内存注册：将用户态内存 pin 住，建立 DMA 映射
    // 这是 zero-copy 的关键：网卡可以直接访问这块内存
    mr = ibv_reg_mr(pd, buffer, DATA_SIZE,
                    IBV_ACCESS_LOCAL_WRITE | 
                    IBV_ACCESS_REMOTE_READ | 
                    IBV_ACCESS_REMOTE_WRITE);
    if (!mr) {
        perror("ibv_reg_mr");
        goto cleanup;
    }
    
    printf("Memory registered:\n");
    printf("  Virtual Address: %p\n", buffer);
    printf("  LKey (Local Key): 0x%x\n", mr->lkey);
    printf("  RKey (Remote Key): 0x%x\n", mr->rkey);
    printf("\nData path: user buffer -> (DMA) -> NIC\n");
    printf("(Zero-copy: 网卡直接访问用户态内存，无内核拷贝)\n\n");
    
    printf("Key insight:\n");
    printf("  - 数据始终保持在用户态缓冲区\n");
    printf("  - 网卡通过 DMA 直接读写用户态内存\n");
    printf("  - 没有用户态 <-> 内核态的拷贝开销\n");
    
cleanup:
    if (mr) ibv_dereg_mr(mr);
    if (buffer) free(buffer);
    if (pd) ibv_dealloc_pd(pd);
    if (ctx) ibv_close_device(ctx);
    if (dev_list) ibv_free_device_list(dev_list);
    
    return 0;
}

// 展示内存注册细节
void show_memory_registration() {
    printf("\n=== Memory Registration Details (Zero-Copy Core) ===\n\n");
    
    printf("1. 用户态分配内存:\n");
    printf("   void *buf = malloc(size);  // 用户态虚拟地址\n\n");
    
    printf("2. 内存注册 (ibv_reg_mr):\n");
    printf("   - Pin 内存页 (防止被 swap)\n");
    printf("   - 建立虚拟地址 -> 物理地址映射\n");
    printf("   - 配置 DMA 映射 (IOMMU/设备页表)\n");
    printf("   - 返回 LKey/RKey (内存访问密钥)\n\n");
    
    printf("3. 数据传输:\n");
    printf("   - RDMA Write: 本地内存 -> 远程内存 (DMA)\n");
    printf("   - RDMA Read:  远程内存 -> 本地内存 (DMA)\n");
    printf("   - Send/Recv:  本地内存 <-> 远程内存 (DMA)\n\n");
    
    printf("4. Zero-Copy 证明:\n");
    printf("   - 没有 copy_from_user() / copy_to_user()\n");
    printf("   - 没有 socket buffer 拷贝\n");
    printf("   - 网卡直接读写用户态内存\n");
}

int main(int argc, char **argv) {
    const char *server_ip = (argc > 1) ? argv[1] : "127.0.0.1";
    
    printf("========================================\n");
    printf("RDMA Zero-Copy Demonstration\n");
    printf("========================================\n");
    printf("\nZero-copy means:\n");
    printf("  - No kernel socket buffers\n");
    printf("  - No user<->kernel memory copy\n");
    printf("  - NIC DMA directly to/from user memory\n");
    
    show_memory_registration();
    
    // RDMA 测试
    rdma_write_test(server_ip);
    
    printf("\n========================================\n");
    printf("Use strace to observe the difference:\n");
    printf("  TCP: strace -e trace=send ./tcp_program\n");
    printf("  RDMA: strace -e trace=write,ioctl ./rdma_program\n");
    printf("========================================\n");
    
    return 0;
}
