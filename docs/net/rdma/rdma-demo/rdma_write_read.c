/*
 * RDMA WRITE/READ Demo
 * 
 * 演示真正的 RDMA 单端操作：
 * - WRITE: 本端直接写入远端内存
 * - READ: 本端直接从远端内存读取
 * 
 * 与 SEND/RECV 的区别：
 * - SEND/RECV: 双端操作，两端 CPU 都需要参与
 * - WRITE/READ: 单端操作，远端 CPU 不参与数据传输
 * 
 * 流程：
 * 1. 服务端注册内存，通过 TCP 将 rkey 和地址发送给客户端
 * 2. 客户端使用 WRITE 直接写入服务端内存
 * 3. 客户端使用 READ 直接从服务端内存读取
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
#include <netdb.h>
#include <time.h>

#include <infiniband/verbs.h>

#define DATA_SIZE 1024
#define PORT 18515

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
    uint64_t addr;      // 内存地址
    uint32_t rkey;      // 远程访问密钥
    uint32_t qpn;       // QP 号
    uint32_t psn;       // 包序列号
    uint16_t lid;       // 本地标识符
    union ibv_gid gid;  // 全局标识符
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
    
    // 注册内存区域 - 这是 zero-copy 的关键
    // 允许本地写入、远程读取、远程写入
    rdma_ctx->mr = ibv_reg_mr(rdma_ctx->pd, rdma_ctx->buf, DATA_SIZE,
                              IBV_ACCESS_LOCAL_WRITE |
                              IBV_ACCESS_REMOTE_READ |
                              IBV_ACCESS_REMOTE_WRITE);
    if (!rdma_ctx->mr) {
        perror("ibv_reg_mr");
        goto err_buf;
    }
    
    printf("Memory registered:\n");
    printf("  Address: %p\n", rdma_ctx->buf);
    printf("  Length: %d bytes\n", DATA_SIZE);
    printf("  LKey: 0x%x (Local Key)\n", rdma_ctx->mr->lkey);
    printf("  RKey: 0x%x (Remote Key - 用于远端访问)\n", rdma_ctx->mr->rkey);
    
    // 创建 QP
    struct ibv_qp_init_attr qp_init_attr = {
        .send_cq = rdma_ctx->cq,
        .recv_cq = rdma_ctx->cq,
        .qp_type = IBV_QPT_RC,  // Reliable Connection
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
        .qp_access_flags = IBV_ACCESS_REMOTE_READ | IBV_ACCESS_REMOTE_WRITE,
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
    int max_polls = timeout_ms * 100;  // 粗略估计
    
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

/* 执行 RDMA WRITE 操作 */
static int post_write(struct rdma_context *rdma_ctx, 
                      uint64_t remote_addr, uint32_t rkey, size_t len)
{
    struct ibv_sge sge = {
        .addr = (uint64_t)rdma_ctx->buf,  // 本地内存地址
        .length = len,
        .lkey = rdma_ctx->mr->lkey,
    };
    
    struct ibv_send_wr wr = {
        .wr_id = 1,
        .opcode = IBV_WR_RDMA_WRITE,      // RDMA WRITE 操作！
        .send_flags = IBV_SEND_SIGNALED,
        .sg_list = &sge,
        .num_sge = 1,
        .wr.rdma = {
            .remote_addr = remote_addr,    // 远端内存地址
            .rkey = rkey,                  // 远端内存访问密钥
        },
    };
    
    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(rdma_ctx->qp, &wr, &bad_wr)) {
        perror("ibv_post_send (WRITE)");
        return -1;
    }
    
    printf("  [WRITE] Local %p -> Remote 0x%lx (len=%zu)\n", 
           rdma_ctx->buf, remote_addr, len);
    
    // 等待完成
    return poll_cq(rdma_ctx->cq, 2000);
}

/* 执行 RDMA READ 操作 */
static int post_read(struct rdma_context *rdma_ctx,
                     uint64_t remote_addr, uint32_t rkey, size_t len)
{
    struct ibv_sge sge = {
        .addr = (uint64_t)rdma_ctx->buf,  // 本地内存地址（数据读到这里）
        .length = len,
        .lkey = rdma_ctx->mr->lkey,
    };
    
    struct ibv_send_wr wr = {
        .wr_id = 2,
        .opcode = IBV_WR_RDMA_READ,       // RDMA READ 操作！
        .send_flags = IBV_SEND_SIGNALED,
        .sg_list = &sge,
        .num_sge = 1,
        .wr.rdma = {
            .remote_addr = remote_addr,    // 远端内存地址（从这里读）
            .rkey = rkey,                  // 远端内存访问密钥
        },
    };
    
    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(rdma_ctx->qp, &wr, &bad_wr)) {
        perror("ibv_post_send (READ)");
        return -1;
    }
    
    printf("  [READ] Remote 0x%lx -> Local %p (len=%zu)\n",
           remote_addr, rdma_ctx->buf, len);
    
    // 等待完成
    return poll_cq(rdma_ctx->cq, 2000);
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
    int sockfd, connfd;
    struct rdma_info local_info, remote_info;
    
    printf("=== RDMA WRITE/READ Server ===\n\n");
    
    // 初始化 RDMA
    if (init_rdma(&rdma_ctx) < 0)
        return -1;
    
    // 准备本地信息（关键是 addr 和 rkey）
    local_info.addr = (uint64_t)rdma_ctx.buf;
    local_info.rkey = rdma_ctx.mr->rkey;
    local_info.qpn = rdma_ctx.qpn;
    local_info.psn = rdma_ctx.psn;
    local_info.lid = rdma_ctx.lid;
    memcpy(&local_info.gid, &rdma_ctx.gid, sizeof(local_info.gid));
    
    // 填充测试数据
    snprintf(rdma_ctx.buf, DATA_SIZE, 
             "Hello from Server! This data can be read by RDMA READ.");
    printf("Initial data in buffer: '%s'\n\n", (char*)rdma_ctx.buf);
    
    // 等待客户端连接
    connfd = tcp_listen(PORT);
    if (connfd < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    // 接收对端信息
    if (read(connfd, &remote_info, sizeof(remote_info)) != sizeof(remote_info)) {
        perror("read remote_info");
        close(connfd);
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    // 发送本地信息（包含 addr 和 rkey，这是 WRITE/READ 的关键）
    if (write(connfd, &local_info, sizeof(local_info)) != sizeof(local_info)) {
        perror("write local_info");
        close(connfd);
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    close(connfd);
    
    printf("Exchanged RDMA info with client:\n");
    printf("  Local:  addr=0x%lx, rkey=0x%x\n", local_info.addr, local_info.rkey);
    printf("  Remote: addr=0x%lx, rkey=0x%x\n\n", remote_info.addr, remote_info.rkey);
    
    // 设置 QP
    if (modify_qp_to_init(&rdma_ctx) < 0 ||
        modify_qp_to_rtr(&rdma_ctx, &remote_info) < 0 ||
        modify_qp_to_rts(&rdma_ctx) < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    printf("=== Waiting for client operations ===\n");
    printf("(Server CPU is not involved in data transfer!)\n\n");
    
    // 服务端不主动操作，只是等待
    // 客户端会通过 WRITE 写入数据，然后通过 READ 读取数据
    printf("Waiting 5 seconds for client to complete operations...\n");
    sleep(5);
    
    printf("\n=== Server buffer content after client operations ===\n");
    printf("Data: '%s'\n\n", (char*)rdma_ctx.buf);
    
    printf("=== Server Complete ===\n");
    printf("Notice: Server CPU was not involved in WRITE/READ operations!\n");
    
    cleanup_rdma(&rdma_ctx);
    return 0;
}

/* 客户端主函数 */
static int run_client(const char *server_ip)
{
    struct rdma_context rdma_ctx;
    int sockfd;
    struct rdma_info local_info, remote_info;
    
    printf("=== RDMA WRITE/READ Client ===\n\n");
    
    // 初始化 RDMA
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
    
    // 发送本地信息
    if (write(sockfd, &local_info, sizeof(local_info)) != sizeof(local_info)) {
        perror("write local_info");
        close(sockfd);
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    // 接收服务端信息（关键是 addr 和 rkey）
    if (read(sockfd, &remote_info, sizeof(remote_info)) != sizeof(remote_info)) {
        perror("read remote_info");
        close(sockfd);
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    close(sockfd);
    
    printf("Exchanged RDMA info with server:\n");
    printf("  Local:  addr=0x%lx, rkey=0x%x\n", local_info.addr, local_info.rkey);
    printf("  Remote: addr=0x%lx, rkey=0x%x\n\n", remote_info.addr, remote_info.rkey);
    
    // 设置 QP
    if (modify_qp_to_init(&rdma_ctx) < 0 ||
        modify_qp_to_rtr(&rdma_ctx, &remote_info) < 0 ||
        modify_qp_to_rts(&rdma_ctx) < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    // ========== 演示 RDMA READ ==========
    printf("=== Step 1: RDMA READ from server ===\n");
    printf("Reading data from server's memory without server CPU involvement...\n");
    
    if (post_read(&rdma_ctx, remote_info.addr, remote_info.rkey, DATA_SIZE) < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    printf("  Read complete! Data: '%s'\n\n", (char*)rdma_ctx.buf);
    
    // ========== 演示 RDMA WRITE ==========
    printf("=== Step 2: RDMA WRITE to server ===\n");
    printf("Writing data to server's memory without server CPU involvement...\n");
    
    // 准备要写入的数据
    snprintf(rdma_ctx.buf, DATA_SIZE,
             "Hello from Client! Written via RDMA WRITE.");
    
    if (post_write(&rdma_ctx, remote_info.addr, remote_info.rkey, DATA_SIZE) < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    printf("  Write complete!\n\n");
    
    // ========== 再次 READ 验证 ==========
    printf("=== Step 3: RDMA READ again to verify WRITE ===\n");
    printf("Reading back the data we just wrote...\n");
    
    if (post_read(&rdma_ctx, remote_info.addr, remote_info.rkey, DATA_SIZE) < 0) {
        cleanup_rdma(&rdma_ctx);
        return -1;
    }
    
    printf("  Read complete! Data: '%s'\n\n", (char*)rdma_ctx.buf);
    
    printf("=== Client Complete ===\n");
    printf("Successfully demonstrated RDMA WRITE and READ operations!\n");
    printf("Server CPU was NOT involved in any data transfer.\n");
    
    cleanup_rdma(&rdma_ctx);
    return 0;
}

static void usage(const char *prog)
{
    printf("Usage: %s [options] [server_ip]\n", prog);
    printf("Options:\n");
    printf("  -s, --server         Run as server\n");
    printf("  -d, --device=NAME    IB device name\n");
    printf("  -h, --help           Show help\n");
    printf("\nExamples:\n");
    printf("  Server: %s -s -d rocep0s6\n", prog);
    printf("  Client: %s -d rocep0s6 10.0.3.2\n", prog);
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
