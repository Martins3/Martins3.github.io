/*
 * RDMA Atomic Operations Demo
 * 
 * 演示 RDMA 原子操作：
 * 1. Fetch-and-Add (F&A) - 原子取并加
 * 2. Compare-and-Swap (CAS) - 原子比较并交换
 * 
 * Atomic 操作的特点：
 * - 单端操作：远端 CPU 不参与
 * - 硬件原子性：网卡保证操作的原子性
 * - 返回旧值：操作完成后返回操作前的值
 * 
 * 应用场景：
 * - 分布式锁
 * - 原子计数器
 * - 无锁队列
 * - 同步机制
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <getopt.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <stdint.h>

#include <infiniband/verbs.h>

#define DATA_SIZE 4096
#define PORT 18516

/* 原子操作目标数据结构 */
struct atomic_data {
    uint64_t counter;      // 用于 Fetch-and-Add
    uint64_t lock;         // 用于 Compare-and-Swap (0=unlocked, 1=locked)
    uint64_t value;        // 受保护的数据
};

struct rdma_context {
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_qp *qp;
    struct ibv_mr *mr;
    void *buf;
    
    uint16_t lid;
    uint32_t qpn;
    uint32_t psn;
    union ibv_gid gid;
};

/* 用于交换 RDMA 连接信息 */
struct rdma_info {
    uint64_t addr;
    uint32_t rkey;
    uint32_t qpn;
    uint32_t psn;
    uint16_t lid;
    union ibv_gid gid;
} __attribute__((packed));

static int is_server = 0;
static char *device_name = NULL;
static char *server_ip = NULL;

/* TCP 连接建立 */
static int tcp_connect(const char *ip, int port)
{
    int sockfd;
    struct sockaddr_in addr;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);
    
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    
    return sockfd;
}

static int tcp_listen(int port)
{
    int sockfd, connfd;
    struct sockaddr_in addr;
    int opt = 1;
    
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }
    
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(sockfd);
        return -1;
    }
    
    if (listen(sockfd, 1) < 0) {
        perror("listen");
        close(sockfd);
        return -1;
    }
    
    printf("Server listening on port %d...\n", port);
    connfd = accept(sockfd, NULL, NULL);
    close(sockfd);
    
    return connfd;
}

/* 初始化 RDMA 设备 */
static int init_rdma(struct rdma_context *rdma_ctx)
{
    struct ibv_device **dev_list, *ib_dev = NULL;
    int num_devices, i;
    
    memset(rdma_ctx, 0, sizeof(*rdma_ctx));
    
    dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list || num_devices == 0) {
        fprintf(stderr, "No IB devices found\n");
        return -1;
    }
    
    for (i = 0; i < num_devices; i++) {
        if (!strcmp(ibv_get_device_name(dev_list[i]), device_name)) {
            ib_dev = dev_list[i];
            break;
        }
    }
    
    if (!ib_dev) {
        fprintf(stderr, "Device '%s' not found\n", device_name);
        ibv_free_device_list(dev_list);
        return -1;
    }
    
    rdma_ctx->ctx = ibv_open_device(ib_dev);
    if (!rdma_ctx->ctx) {
        perror("ibv_open_device");
        ibv_free_device_list(dev_list);
        return -1;
    }
    
    ibv_free_device_list(dev_list);
    
    rdma_ctx->pd = ibv_alloc_pd(rdma_ctx->ctx);
    if (!rdma_ctx->pd) {
        perror("ibv_alloc_pd");
        goto err_close;
    }
    
    rdma_ctx->cq = ibv_create_cq(rdma_ctx->ctx, 100, NULL, NULL, 0);
    if (!rdma_ctx->cq) {
        perror("ibv_create_cq");
        goto err_pd;
    }
    
    // 分配并注册内存
    rdma_ctx->buf = malloc(DATA_SIZE);
    if (!rdma_ctx->buf) {
        perror("malloc");
        goto err_cq;
    }
    memset(rdma_ctx->buf, 0, DATA_SIZE);
    
    // 注册内存区域 - 必须包含 REMOTE_ATOMIC 权限
    rdma_ctx->mr = ibv_reg_mr(rdma_ctx->pd, rdma_ctx->buf, DATA_SIZE,
                              IBV_ACCESS_LOCAL_WRITE |
                              IBV_ACCESS_REMOTE_READ |
                              IBV_ACCESS_REMOTE_WRITE |
                              IBV_ACCESS_REMOTE_ATOMIC);  // 原子操作需要这个权限！
    if (!rdma_ctx->mr) {
        perror("ibv_reg_mr");
        goto err_buf;
    }
    
    printf("Memory registered:\n");
    printf("  Address: %p\n", rdma_ctx->buf);
    printf("  Length: %d bytes\n", DATA_SIZE);
    printf("  LKey: 0x%x\n", rdma_ctx->mr->lkey);
    printf("  RKey: 0x%x (包含 REMOTE_ATOMIC 权限)\n", rdma_ctx->mr->rkey);
    
    // 创建 QP
    struct ibv_qp_init_attr qp_init_attr = {
        .send_cq = rdma_ctx->cq,
        .recv_cq = rdma_ctx->cq,
        .qp_type = IBV_QPT_RC,
        .sq_sig_all = 0,
        .cap = {
            .max_send_wr = 10,
            .max_recv_wr = 10,
            .max_send_sge = 1,
            .max_recv_sge = 1,
        },
    };
    
    rdma_ctx->qp = ibv_create_qp(rdma_ctx->pd, &qp_init_attr);
    if (!rdma_ctx->qp) {
        perror("ibv_create_qp");
        goto err_mr;
    }
    
    rdma_ctx->qpn = rdma_ctx->qp->qp_num;
    rdma_ctx->psn = lrand48() & 0xffffff;
    
    struct ibv_port_attr port_attr;
    if (ibv_query_port(rdma_ctx->ctx, 1, &port_attr)) {
        perror("ibv_query_port");
        goto err_qp;
    }
    rdma_ctx->lid = port_attr.lid;
    
    if (ibv_query_gid(rdma_ctx->ctx, 1, 0, &rdma_ctx->gid)) {
        perror("ibv_query_gid");
        goto err_qp;
    }
    
    return 0;

err_qp:
    ibv_destroy_qp(rdma_ctx->qp);
err_mr:
    ibv_dereg_mr(rdma_ctx->mr);
err_buf:
    free(rdma_ctx->buf);
err_cq:
    ibv_destroy_cq(rdma_ctx->cq);
err_pd:
    ibv_dealloc_pd(rdma_ctx->pd);
err_close:
    ibv_close_device(rdma_ctx->ctx);
    return -1;
}

/* 转换 QP 状态 */
static int modify_qp_to_init(struct rdma_context *rdma_ctx)
{
    struct ibv_qp_attr attr = {
        .qp_state = IBV_QPS_INIT,
        .pkey_index = 0,
        .port_num = 1,
        .qp_access_flags = IBV_ACCESS_REMOTE_READ | 
                           IBV_ACCESS_REMOTE_WRITE | 
                           IBV_ACCESS_REMOTE_ATOMIC,  // 允许原子操作
    };
    
    if (ibv_modify_qp(rdma_ctx->qp, &attr,
                      IBV_QP_STATE | IBV_QP_PKEY_INDEX |
                      IBV_QP_PORT | IBV_QP_ACCESS_FLAGS)) {
        perror("ibv_modify_qp to INIT");
        return -1;
    }
    return 0;
}

static int modify_qp_to_rtr(struct rdma_context *rdma_ctx, 
                            struct rdma_info *remote)
{
    struct ibv_qp_attr attr = {
        .qp_state = IBV_QPS_RTR,
        .path_mtu = IBV_MTU_1024,
        .dest_qp_num = remote->qpn,
        .rq_psn = remote->psn,
        .max_dest_rd_atomic = 1,
        .min_rnr_timer = 12,
        .ah_attr = {
            .is_global = 0,
            .dlid = remote->lid,
            .sl = 0,
            .src_path_bits = 0,
            .port_num = 1,
        },
    };
    
    if (remote->gid.global.interface_id) {
        attr.ah_attr.is_global = 1;
        attr.ah_attr.grh.hop_limit = 1;
        attr.ah_attr.grh.dgid = remote->gid;
        attr.ah_attr.grh.sgid_index = 0;
    }
    
    if (ibv_modify_qp(rdma_ctx->qp, &attr,
                      IBV_QP_STATE | IBV_QP_AV | IBV_QP_PATH_MTU |
                      IBV_QP_DEST_QPN | IBV_QP_RQ_PSN |
                      IBV_QP_MAX_DEST_RD_ATOMIC | IBV_QP_MIN_RNR_TIMER)) {
        perror("ibv_modify_qp to RTR");
        return -1;
    }
    return 0;
}

static int modify_qp_to_rts(struct rdma_context *rdma_ctx)
{
    struct ibv_qp_attr attr = {
        .qp_state = IBV_QPS_RTS,
        .sq_psn = rdma_ctx->psn,
        .max_rd_atomic = 1,
        .retry_cnt = 7,
        .rnr_retry = 7,
        .timeout = 14,
    };
    
    if (ibv_modify_qp(rdma_ctx->qp, &attr,
                      IBV_QP_STATE | IBV_QP_SQ_PSN |
                      IBV_QP_MAX_QP_RD_ATOMIC | IBV_QP_RETRY_CNT |
                      IBV_QP_RNR_RETRY | IBV_QP_TIMEOUT)) {
        perror("ibv_modify_qp to RTS");
        return -1;
    }
    return 0;
}

/* 等待 CQ 完成 */
static int poll_cq(struct ibv_cq *cq, int timeout_ms)
{
    struct ibv_wc wc;
    int ret;
    int poll_count = 0;
    int max_polls = timeout_ms * 100;
    
    do {
        ret = ibv_poll_cq(cq, 1, &wc);
        if (ret < 0) {
            fprintf(stderr, "ibv_poll_cq failed\n");
            return -1;
        }
        if (++poll_count > max_polls) {
            fprintf(stderr, "poll_cq timeout\n");
            return -1;
        }
        usleep(10);
    } while (ret == 0);
    
    if (wc.status != IBV_WC_SUCCESS) {
        fprintf(stderr, "WC error: %s\n", ibv_wc_status_str(wc.status));
        return -1;
    }
    
    return 0;
}

/* 执行 Fetch-and-Add 原子操作 */
static int atomic_fetch_add(struct rdma_context *rdma_ctx,
                            uint64_t remote_addr, uint32_t rkey,
                            uint64_t add_val, uint64_t *old_val)
{
    // 本地缓冲区用于接收旧值
    uint64_t *result = (uint64_t *)rdma_ctx->buf;
    *result = 0;
    
    struct ibv_sge sge = {
        .addr = (uint64_t)result,     // 本地缓冲区（接收旧值）
        .length = sizeof(uint64_t),
        .lkey = rdma_ctx->mr->lkey,
    };
    
    struct ibv_send_wr wr = {
        .wr_id = 1,
        .opcode = IBV_WR_ATOMIC_FETCH_AND_ADD,  // Fetch-and-Add 操作！
        .send_flags = IBV_SEND_SIGNALED,
        .sg_list = &sge,
        .num_sge = 1,
        .wr.atomic = {
            .remote_addr = remote_addr,   // 远端内存地址
            .rkey = rkey,                 // 远端内存密钥
            .compare_add = add_val,       // 要增加的值 (F&A 使用 compare_add 字段)
        },
    };
    
    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(rdma_ctx->qp, &wr, &bad_wr)) {
        perror("ibv_post_send (FETCH_AND_ADD)");
        return -1;
    }
    
    if (poll_cq(rdma_ctx->cq, 2000) < 0)
        return -1;
    
    *old_val = *result;
    return 0;
}

/* 执行 Compare-and-Swap 原子操作 */
static int atomic_cmp_swap(struct rdma_context *rdma_ctx,
                           uint64_t remote_addr, uint32_t rkey,
                           uint64_t expected, uint64_t new_val,
                           uint64_t *old_val)
{
    // 本地缓冲区用于接收旧值
    uint64_t *result = (uint64_t *)rdma_ctx->buf;
    *result = 0;
    
    struct ibv_sge sge = {
        .addr = (uint64_t)result,     // 本地缓冲区（接收旧值）
        .length = sizeof(uint64_t),
        .lkey = rdma_ctx->mr->lkey,
    };
    
    struct ibv_send_wr wr = {
        .wr_id = 2,
        .opcode = IBV_WR_ATOMIC_CMP_AND_SWP,  // Compare-and-Swap 操作！
        .send_flags = IBV_SEND_SIGNALED,
        .sg_list = &sge,
        .num_sge = 1,
        .wr.atomic = {
            .remote_addr = remote_addr,   // 远端内存地址
            .rkey = rkey,                 // 远端内存密钥
            .compare_add = expected,      // 期望值
            .swap = new_val,              // 新值
        },
    };
    
    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(rdma_ctx->qp, &wr, &bad_wr)) {
        perror("ibv_post_send (CMP_AND_SWP)");
        return -1;
    }
    
    if (poll_cq(rdma_ctx->cq, 2000) < 0)
        return -1;
    
    *old_val = *result;
    return 0;
}

/* 清理资源 */
static void cleanup_rdma(struct rdma_context *rdma_ctx)
{
    if (rdma_ctx->qp) ibv_destroy_qp(rdma_ctx->qp);
    if (rdma_ctx->mr) ibv_dereg_mr(rdma_ctx->mr);
    if (rdma_ctx->buf) free(rdma_ctx->buf);
    if (rdma_ctx->cq) ibv_destroy_cq(rdma_ctx->cq);
    if (rdma_ctx->pd) ibv_dealloc_pd(rdma_ctx->pd);
    if (rdma_ctx->ctx) ibv_close_device(rdma_ctx->ctx);
}

/* 服务端主函数 */
static int run_server(void)
{
    struct rdma_context rdma_ctx;
    int connfd;
    struct rdma_info local_info, remote_info;
    struct atomic_data *data;
    
    printf("========================================\n");
    printf("=== RDMA Atomic Operations Server ===\n");
    printf("========================================\n\n");
    
    if (init_rdma(&rdma_ctx) < 0)
        return -1;
    
    // 初始化原子数据
    data = (struct atomic_data *)rdma_ctx.buf;
    data->counter = 100;   // 初始计数器值
    data->lock = 0;        // 未锁定
    data->value = 9999;    // 受保护的数据
    
    printf("Initial atomic data:\n");
    printf("  counter = %lu\n", data->counter);
    printf("  lock = %lu (0=unlocked, 1=locked)\n", data->lock);
    printf("  value = %lu\n\n", data->value);
    
    // 准备本地信息
    local_info.addr = (uint64_t)rdma_ctx.buf;
    local_info.rkey = rdma_ctx.mr->rkey;
    local_info.qpn = rdma_ctx.qpn;
    local_info.psn = rdma_ctx.psn;
    local_info.lid = rdma_ctx.lid;
    memcpy(&local_info.gid, &rdma_ctx.gid, sizeof(local_info.gid));
    
    // 等待客户端连接
    connfd = tcp_listen(PORT);
    if (connfd < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    // 交换信息
    if (read(connfd, &remote_info, sizeof(remote_info)) != sizeof(remote_info) ||
        write(connfd, &local_info, sizeof(local_info)) != sizeof(local_info)) {
        perror("exchange info");
        close(connfd);
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    close(connfd);
    
    printf("Exchanged RDMA info with client\n\n");
    
    // 设置 QP
    if (modify_qp_to_init(&rdma_ctx) < 0 ||
        modify_qp_to_rtr(&rdma_ctx, &remote_info) < 0 ||
        modify_qp_to_rts(&rdma_ctx) < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    printf("=== Waiting for client atomic operations ===\n");
    printf("(Server CPU is not involved!)\n\n");
    
    // 等待客户端完成原子操作
    sleep(3);
    
    printf("=== Atomic data after client operations ===\n");
    printf("  counter = %lu\n", data->counter);
    printf("  lock = %lu\n", data->lock);
    printf("  value = %lu\n\n", data->value);
    
    printf("========================================\n");
    printf("Server complete - CPU was never involved!\n");
    printf("========================================\n");
    
    cleanup_rdma(&rdma_ctx);
    return 0;
}

/* 客户端主函数 */
static int run_client(const char *server_ip)
{
    struct rdma_context rdma_ctx;
    int sockfd;
    struct rdma_info local_info, remote_info;
    uint64_t old_val;
    uint64_t remote_counter_addr;
    uint64_t remote_lock_addr;
    
    printf("========================================\n");
    printf("=== RDMA Atomic Operations Client ===\n");
    printf("========================================\n\n");
    
    if (init_rdma(&rdma_ctx) < 0)
        return -1;
    
    // 准备本地信息
    local_info.addr = (uint64_t)rdma_ctx.buf;
    local_info.rkey = rdma_ctx.mr->rkey;
    local_info.qpn = rdma_ctx.qpn;
    local_info.psn = rdma_ctx.psn;
    local_info.lid = rdma_ctx.lid;
    memcpy(&local_info.gid, &rdma_ctx.gid, sizeof(local_info.gid));
    
    // 连接到服务端
    sockfd = tcp_connect(server_ip, PORT);
    if (sockfd < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    // 交换信息
    if (write(sockfd, &local_info, sizeof(local_info)) != sizeof(local_info) ||
        read(sockfd, &remote_info, sizeof(remote_info)) != sizeof(remote_info)) {
        perror("exchange info");
        close(sockfd);
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    close(sockfd);
    
    printf("Exchanged RDMA info with server\n\n");
    
    // 计算远端原子变量的地址
    remote_counter_addr = remote_info.addr + offsetof(struct atomic_data, counter);
    remote_lock_addr = remote_info.addr + offsetof(struct atomic_data, lock);
    
    // 设置 QP
    if (modify_qp_to_init(&rdma_ctx) < 0 ||
        modify_qp_to_rtr(&rdma_ctx, &remote_info) < 0 ||
        modify_qp_to_rts(&rdma_ctx) < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    // ========== 演示 1: Fetch-and-Add ==========
    printf("=== Demo 1: Fetch-and-Add ===\n");
    printf("Atomically add 10 to remote counter...\n\n");
    
    if (atomic_fetch_add(&rdma_ctx, remote_counter_addr, remote_info.rkey, 
                         10, &old_val) < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    printf("  Fetch-and-Add complete!\n");
    printf("  Old value: %lu\n", old_val);
    printf("  New value: %lu (old + 10)\n\n", old_val + 10);
    
    // ========== 演示 2: Compare-and-Swap (尝试获取锁) ==========
    printf("=== Demo 2: Compare-and-Swap (Lock Acquisition) ===\n");
    printf("Trying to acquire distributed lock...\n");
    printf("  Expected: 0 (unlocked)\n");
    printf("  New value: 1 (locked)\n\n");
    
    if (atomic_cmp_swap(&rdma_ctx, remote_lock_addr, remote_info.rkey,
                        0, 1, &old_val) < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    printf("  Compare-and-Swap complete!\n");
    printf("  Old value: %lu\n", old_val);
    
    if (old_val == 0) {
        printf("  => Lock acquired successfully!\n\n");
        
        // 模拟：持有锁时访问受保护的数据
        printf("  (Lock held - can safely access protected data)\n\n");
        
        // 释放锁
        printf("Releasing lock...\n");
        if (atomic_cmp_swap(&rdma_ctx, remote_lock_addr, remote_info.rkey,
                            1, 0, &old_val) < 0) {
            cleanup_rdma(&rdma_ctx);
            return -1;
        }
        printf("  Lock released!\n\n");
    } else {
        printf("  => Lock already held by someone else\n\n");
    }
    
    // ========== 演示 3: 多个 Fetch-and-Add ==========
    printf("=== Demo 3: Multiple Fetch-and-Add ===\n");
    printf("Incrementing counter 5 times...\n\n");
    
    for (int i = 0; i < 5; i++) {
        if (atomic_fetch_add(&rdma_ctx, remote_counter_addr, remote_info.rkey,
                             1, &old_val) < 0) {
            cleanup_rdma(&rdma_ctx);
            return -1;
        }
        printf("  [%d] Old: %lu, New: %lu\n", i+1, old_val, old_val + 1);
    }
    
    printf("\n========================================\n");
    printf("All atomic operations completed!\n");
    printf("Server CPU was NOT involved.\n");
    printf("========================================\n");
    
    cleanup_rdma(&rdma_ctx);
    return 0;
}

static void usage(const char *prog)
{
    printf("Usage: %s [options] [server_ip]\n", prog);
    printf("\n");
    printf("Options:\n");
    printf("  -s, --server         Run as server\n");
    printf("  -d, --device=NAME    IB device name\n");
    printf("  -h, --help           Show help\n");
    printf("\n");
    printf("Examples:\n");
    printf("  Server: %s -s -d rocep0s6\n", prog);
    printf("  Client: %s -d rocep0s6 10.0.3.2\n", prog);
    printf("\n");
    printf("Atomic Operations:\n");
    printf("  1. Fetch-and-Add (F&A) - 原子取并加\n");
    printf("  2. Compare-and-Swap (CAS) - 原子比较并交换\n");
}

int main(int argc, char **argv)
{
    static struct option long_options[] = {
        {"server", no_argument, 0, 's'},
        {"device", required_argument, 0, 'd'},
        {"help",   no_argument, 0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
    while ((opt = getopt_long(argc, argv, "sd:h", long_options, NULL)) != -1) {
        switch (opt) {
        case 's':
            is_server = 1;
            break;
        case 'd':
            device_name = optarg;
            break;
        case 'h':
            usage(argv[0]);
            return 0;
        default:
            usage(argv[0]);
            return 1;
        }
    }
    
    if (!device_name) {
        fprintf(stderr, "Error: Device name required (-d)\n");
        return 1;
    }
    
    if (!is_server && optind < argc) {
        server_ip = argv[optind];
    } else if (!is_server) {
        fprintf(stderr, "Error: Server IP required for client mode\n");
        return 1;
    }
    
    srand48(time(NULL));
    
    if (is_server) {
        return run_server();
    } else {
        return run_client(server_ip);
    }
}
