/*
 * RDMA Ping-Pong Demo
 * 
 * Supports two completion notification modes:
 * 1. Polling mode (USE_POLLING=1): Spin on CQ for completions
 * 2. Event mode (USE_POLLING=0): Use completion channel with interrupt
 * 
 * Usage:
 *   Server: ./rdma_pingpong -s -d <device>
 *   Client: ./rdma_pingpong -d <device> <server_ip>
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
#include <pthread.h>
#include <time.h>

#include <infiniband/verbs.h>

#define MSG_SIZE 1024
#define MAX_POLL_CQ_TIMEOUT 2000  /* ms */

struct rdma_context {
    struct ibv_context *ctx;
    struct ibv_pd *pd;
    struct ibv_cq *cq;
    struct ibv_comp_channel *comp_channel;  /* For event mode */
    struct ibv_qp *qp;
    struct ibv_mr *mr;
    void *buf;
    
    uint16_t lid;
    uint32_t qpn;
    uint32_t psn;
    union ibv_gid gid;
    
    int use_event;
    int gid_idx;
};

struct conn_info {
    uint16_t lid;
    uint32_t qpn;
    uint32_t psn;
    union ibv_gid gid;
} __attribute__((packed));

static int page_size;
static int is_server = 0;
static char *device_name = NULL;
static char *server_ip = NULL;
static int iterations = 1000;

/* Print usage */
static void usage(const char *prog)
{
    printf("Usage: %s [options] [server_ip]\n", prog);
    printf("Options:\n");
    printf("  -s, --server         Run as server\n");
    printf("  -d, --device=NAME    IB device name (e.g., rocep0s6, mlx5_0)\n");
    printf("  -i, --iterations=N   Number of iterations (default: 1000)\n");
    printf("  -e, --event          Use event-driven completion (interrupt mode)\n");
    printf("  -h, --help           Show this help\n");
    printf("\nCompilation modes:\n");
#if USE_POLLING
    printf("  Current: POLLING mode (spin on CQ)\n");
#else
    printf("  Current: EVENT mode (interrupt-driven)\n");
#endif
    printf("\nExamples:\n");
    printf("  Server: %s -s -d rocep0s6\n", prog);
    printf("  Client: %s -d rocep0s6 10.0.3.2\n", prog);
}

/* Parse command line */
static int parse_args(int argc, char **argv)
{
    static struct option long_options[] = {
        {"server",     no_argument,       0, 's'},
        {"device",     required_argument, 0, 'd'},
        {"iterations", required_argument, 0, 'i'},
        {"event",      no_argument,       0, 'e'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };
    
    int opt;
#if USE_POLLING
    int use_event = 0;
#else
    int use_event = 1;
#endif
    
    while ((opt = getopt_long(argc, argv, "sd:i:eh", long_options, NULL)) != -1) {
        switch (opt) {
        case 's':
            is_server = 1;
            break;
        case 'd':
            device_name = optarg;
            break;
        case 'i':
            iterations = atoi(optarg);
            break;
        case 'e':
            use_event = 1;
            break;
        case 'h':
            usage(argv[0]);
            exit(0);
        default:
            usage(argv[0]);
            return -1;
        }
    }
    
    if (!device_name) {
        fprintf(stderr, "Error: Device name required (-d)\n");
        return -1;
    }
    
    if (!is_server && optind < argc) {
        server_ip = argv[optind];
    } else if (!is_server) {
        fprintf(stderr, "Error: Server IP required for client mode\n");
        return -1;
    }
    
    return use_event;
}

/* Get timestamp in microseconds */
static uint64_t get_usec(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

/* TCP socket helpers for connection setup */
static int tcp_server_listen(int port)
{
    int sockfd;
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
    return sockfd;
}

static int tcp_client_connect(const char *ip, int port)
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
    
    printf("Connecting to %s:%d...\n", ip, port);
    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect");
        close(sockfd);
        return -1;
    }
    
    printf("Connected.\n");
    return sockfd;
}

static int tcp_accept(int listen_fd)
{
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int conn_fd;
    
    conn_fd = accept(listen_fd, (struct sockaddr *)&addr, &len);
    if (conn_fd < 0) {
        perror("accept");
        return -1;
    }
    
    printf("Client connected from %s\n", inet_ntoa(addr.sin_addr));
    return conn_fd;
}

/* Exchange connection info over TCP */
static int exchange_conn_info(int sockfd, struct rdma_context *rdma_ctx,
                              struct conn_info *remote_info)
{
    struct conn_info local_info;
    
    local_info.lid = rdma_ctx->lid;
    local_info.qpn = rdma_ctx->qpn;
    local_info.psn = rdma_ctx->psn;
    memcpy(&local_info.gid, &rdma_ctx->gid, sizeof(local_info.gid));
    
    printf("Local: LID=0x%x, QPN=0x%x, PSN=0x%x\n",
           local_info.lid, local_info.qpn, local_info.psn);
    
    if (write(sockfd, &local_info, sizeof(local_info)) != sizeof(local_info)) {
        perror("write conn_info");
        return -1;
    }
    
    if (read(sockfd, remote_info, sizeof(*remote_info)) != sizeof(*remote_info)) {
        perror("read conn_info");
        return -1;
    }
    
    printf("Remote: LID=0x%x, QPN=0x%x, PSN=0x%x\n",
           remote_info->lid, remote_info->qpn, remote_info->psn);
    
    return 0;
}

/* Initialize RDMA device */
static int init_rdma(struct rdma_context *rdma_ctx, int use_event)
{
    struct ibv_device **dev_list, *ib_dev = NULL;
    int num_devices, i;
    
    memset(rdma_ctx, 0, sizeof(*rdma_ctx));
    rdma_ctx->use_event = use_event;
    
    /* Get device list */
    dev_list = ibv_get_device_list(&num_devices);
    if (!dev_list || num_devices == 0) {
        fprintf(stderr, "No IB devices found\n");
        return -1;
    }
    
    /* Find requested device */
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
    
    /* Open device */
    rdma_ctx->ctx = ibv_open_device(ib_dev);
    if (!rdma_ctx->ctx) {
        perror("ibv_open_device");
        ibv_free_device_list(dev_list);
        return -1;
    }
    
    ibv_free_device_list(dev_list);
    
    /* Allocate Protection Domain */
    rdma_ctx->pd = ibv_alloc_pd(rdma_ctx->ctx);
    if (!rdma_ctx->pd) {
        perror("ibv_alloc_pd");
        goto err_close;
    }
    
    /* Create completion channel for event mode */
    if (use_event) {
        rdma_ctx->comp_channel = ibv_create_comp_channel(rdma_ctx->ctx);
        if (!rdma_ctx->comp_channel) {
            perror("ibv_create_comp_channel");
            goto err_pd;
        }
    }
    
    /* Create Completion Queue */
    rdma_ctx->cq = ibv_create_cq(rdma_ctx->ctx, 100, NULL,
                                  rdma_ctx->comp_channel, 0);
    if (!rdma_ctx->cq) {
        perror("ibv_create_cq");
        goto err_channel;
    }
    
    /* Request notifications for event mode */
    if (use_event) {
        if (ibv_req_notify_cq(rdma_ctx->cq, 0)) {
            perror("ibv_req_notify_cq");
            goto err_cq;
        }
    }
    
    /* Allocate buffer */
    page_size = sysconf(_SC_PAGESIZE);
    if (posix_memalign(&rdma_ctx->buf, page_size, MSG_SIZE)) {
        perror("posix_memalign");
        goto err_cq;
    }
    memset(rdma_ctx->buf, 0, MSG_SIZE);
    
    /* Register Memory Region */
    rdma_ctx->mr = ibv_reg_mr(rdma_ctx->pd, rdma_ctx->buf, MSG_SIZE,
                              IBV_ACCESS_LOCAL_WRITE |
                              IBV_ACCESS_REMOTE_READ |
                              IBV_ACCESS_REMOTE_WRITE);
    if (!rdma_ctx->mr) {
        perror("ibv_reg_mr");
        goto err_buf;
    }
    
    /* Create Queue Pair */
    struct ibv_qp_init_attr qp_init_attr = {
        .send_cq = rdma_ctx->cq,
        .recv_cq = rdma_ctx->cq,
        .qp_type = IBV_QPT_RC,  /* Reliable Connection */
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
    
    /* Get LID */
    struct ibv_port_attr port_attr;
    if (ibv_query_port(rdma_ctx->ctx, 1, &port_attr)) {
        perror("ibv_query_port");
        goto err_qp;
    }
    rdma_ctx->lid = port_attr.lid;
    
    /* Get GID for RoCE (if needed) */
    if (ibv_query_gid(rdma_ctx->ctx, 1, 0, &rdma_ctx->gid)) {
        perror("ibv_query_gid");
        goto err_qp;
    }
    
    printf("RDMA device initialized: %s\n", device_name);
    printf("  Mode: %s\n", use_event ? "EVENT (interrupt)" : "POLLING");
    printf("  QPN: %u, LID: %u, PSN: %u\n", rdma_ctx->qpn, rdma_ctx->lid, rdma_ctx->psn);
    
    return 0;

err_qp:
    ibv_destroy_qp(rdma_ctx->qp);
err_mr:
    ibv_dereg_mr(rdma_ctx->mr);
err_buf:
    free(rdma_ctx->buf);
err_cq:
    ibv_destroy_cq(rdma_ctx->cq);
err_channel:
    if (rdma_ctx->comp_channel)
        ibv_destroy_comp_channel(rdma_ctx->comp_channel);
err_pd:
    ibv_dealloc_pd(rdma_ctx->pd);
err_close:
    ibv_close_device(rdma_ctx->ctx);
    return -1;
}

/* Transition QP to INIT state */
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

/* Transition QP to RTR (Ready to Receive) */
static int modify_qp_to_rtr(struct rdma_context *rdma_ctx,
                            struct conn_info *remote)
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

/* Transition QP to RTS (Ready to Send) */
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

/* Post receive work request */
static int post_recv(struct rdma_context *rdma_ctx)
{
    struct ibv_sge sge = {
        .addr = (uint64_t)rdma_ctx->buf,
        .length = MSG_SIZE,
        .lkey = rdma_ctx->mr->lkey,
    };
    
    struct ibv_recv_wr wr = {
        .wr_id = 1,
        .next = NULL,
        .sg_list = &sge,
        .num_sge = 1,
    };
    
    struct ibv_recv_wr *bad_wr;
    if (ibv_post_recv(rdma_ctx->qp, &wr, &bad_wr)) {
        perror("ibv_post_recv");
        return -1;
    }
    
    return 0;
}

/* Post send work request */
static int post_send(struct rdma_context *rdma_ctx, int opcode)
{
    struct ibv_sge sge = {
        .addr = (uint64_t)rdma_ctx->buf,
        .length = MSG_SIZE,
        .lkey = rdma_ctx->mr->lkey,
    };
    
    struct ibv_send_wr wr = {
        .wr_id = 2,
        .opcode = opcode,
        .send_flags = IBV_SEND_SIGNALED,
        .next = NULL,
        .sg_list = &sge,
        .num_sge = 1,
    };
    
    struct ibv_send_wr *bad_wr;
    if (ibv_post_send(rdma_ctx->qp, &wr, &bad_wr)) {
        perror("ibv_post_send");
        return -1;
    }
    
    return 0;
}

/* POLLING MODE: Poll CQ for completion */
static int poll_cq(struct rdma_context *rdma_ctx, uint64_t *poll_time)
{
    struct ibv_wc wc;
    int ret;
    uint64_t start = get_usec();
    int poll_count = 0;
    
    do {
        ret = ibv_poll_cq(rdma_ctx->cq, 1, &wc);
        if (ret < 0) {
            fprintf(stderr, "ibv_poll_cq failed\n");
            return -1;
        }
        poll_count++;
    } while (ret == 0);
    
    uint64_t end = get_usec();
    *poll_time = end - start;
    
    if (wc.status != IBV_WC_SUCCESS) {
        fprintf(stderr, "Work completion error: %s (vendor_err: 0x%x)\n",
                ibv_wc_status_str(wc.status), wc.vendor_err);
        return -1;
    }
    
    return 0;
}

/* EVENT MODE: Wait for completion notification (interrupt) */
static int wait_cq_event(struct rdma_context *rdma_ctx, uint64_t *wait_time)
{
    struct ibv_cq *ev_cq;
    void *ev_ctx;
    struct ibv_wc wc;
    int ret;
    uint64_t start = get_usec();
    
    /* Wait for completion notification (blocks, kernel interrupts) */
    ret = ibv_get_cq_event(rdma_ctx->comp_channel, &ev_cq, &ev_ctx);
    if (ret) {
        perror("ibv_get_cq_event");
        return -1;
    }
    
    /* Acknowledge the event */
    ibv_ack_cq_events(rdma_ctx->cq, 1);
    
    /* Request next notification */
    if (ibv_req_notify_cq(rdma_ctx->cq, 0)) {
        perror("ibv_req_notify_cq");
        return -1;
    }
    
    /* Now poll to get the actual completion */
    do {
        ret = ibv_poll_cq(rdma_ctx->cq, 1, &wc);
    } while (ret == 0);
    
    uint64_t end = get_usec();
    *wait_time = end - start;
    
    if (wc.status != IBV_WC_SUCCESS) {
        fprintf(stderr, "Work completion error: %s\n",
                ibv_wc_status_str(wc.status));
        return -1;
    }
    
    return 0;
}

/* Cleanup RDMA resources */
static void cleanup_rdma(struct rdma_context *rdma_ctx)
{
    if (rdma_ctx->qp)
        ibv_destroy_qp(rdma_ctx->qp);
    if (rdma_ctx->mr)
        ibv_dereg_mr(rdma_ctx->mr);
    if (rdma_ctx->buf)
        free(rdma_ctx->buf);
    if (rdma_ctx->cq)
        ibv_destroy_cq(rdma_ctx->cq);
    if (rdma_ctx->comp_channel)
        ibv_destroy_comp_channel(rdma_ctx->comp_channel);
    if (rdma_ctx->pd)
        ibv_dealloc_pd(rdma_ctx->pd);
    if (rdma_ctx->ctx)
        ibv_close_device(rdma_ctx->ctx);
}

/* Run ping-pong test */
static int run_pingpong(struct rdma_context *rdma_ctx, int sockfd)
{
    struct conn_info remote_info;
    int i;
    uint64_t total_time = 0, total_wait = 0;
    uint64_t start, end;
    
    /* Exchange connection info */
    if (exchange_conn_info(sockfd, rdma_ctx, &remote_info) < 0)
        return -1;
    
    close(sockfd);
    
    /* Setup QP */
    if (modify_qp_to_init(rdma_ctx) < 0)
        return -1;
    
    if (modify_qp_to_rtr(rdma_ctx, &remote_info) < 0)
        return -1;
    
    if (modify_qp_to_rts(rdma_ctx) < 0)
        return -1;
    
    printf("\n=== Starting ping-pong test ===\n");
    printf("Iterations: %d, Mode: %s\n\n", iterations,
           rdma_ctx->use_event ? "EVENT (interrupt)" : "POLLING");
    
    start = get_usec();
    
    if (is_server) {
        /* Server: receive then send */
        for (i = 0; i < iterations; i++) {
            uint64_t wait_time;
            
            /* Post receive */
            if (post_recv(rdma_ctx) < 0)
                return -1;
            
            /* Wait for receive completion */
            if (rdma_ctx->use_event) {
                if (wait_cq_event(rdma_ctx, &wait_time) < 0)
                    return -1;
            } else {
                if (poll_cq(rdma_ctx, &wait_time) < 0)
                    return -1;
            }
            total_wait += wait_time;
            
            /* Prepare and send response */
            snprintf(rdma_ctx->buf, MSG_SIZE, "PONG %d", i);
            if (post_send(rdma_ctx, IBV_WR_SEND) < 0)
                return -1;
            
            if (rdma_ctx->use_event) {
                if (wait_cq_event(rdma_ctx, &wait_time) < 0)
                    return -1;
            } else {
                if (poll_cq(rdma_ctx, &wait_time) < 0)
                    return -1;
            }
            total_wait += wait_time;
        }
    } else {
        /* Client: send then receive */
        for (i = 0; i < iterations; i++) {
            uint64_t wait_time;
            
            /* Prepare and send */
            snprintf(rdma_ctx->buf, MSG_SIZE, "PING %d", i);
            if (post_send(rdma_ctx, IBV_WR_SEND) < 0)
                return -1;
            
            if (rdma_ctx->use_event) {
                if (wait_cq_event(rdma_ctx, &wait_time) < 0)
                    return -1;
            } else {
                if (poll_cq(rdma_ctx, &wait_time) < 0)
                    return -1;
            }
            total_wait += wait_time;
            
            /* Post receive and wait for response */
            if (post_recv(rdma_ctx) < 0)
                return -1;
            
            if (rdma_ctx->use_event) {
                if (wait_cq_event(rdma_ctx, &wait_time) < 0)
                    return -1;
            } else {
                if (poll_cq(rdma_ctx, &wait_time) < 0)
                    return -1;
            }
            total_wait += wait_time;
        }
    }
    
    end = get_usec();
    total_time = end - start;
    
    /* Print results */
    printf("\n=== Results ===\n");
    printf("Total time:     %.3f ms\n", total_time / 1000.0);
    printf("Avg latency:    %.3f us\n", (double)total_time / (iterations * 2));
    printf("Total wait/CQ:  %.3f ms\n", total_wait / 1000.0);
    printf("Messages:       %d sent, %d received\n", iterations, iterations);
    printf("Throughput:     %.2f msg/sec\n", (double)(iterations * 2) * 1000000 / total_time);
    
    return 0;
}

int main(int argc, char **argv)
{
    struct rdma_context rdma_ctx;
    int sockfd, connfd;
    int ret = 0;
    
#if USE_POLLING
    int use_event = 0;
#else
    int use_event = 1;
#endif
    
    int event_opt = parse_args(argc, argv);
    if (event_opt < 0)
        return 1;
    
    /* Command line -e overrides compile-time default */
    if (event_opt)
        use_event = 1;
    
    srand48(getpid() * time(NULL));
    
    /* Initialize RDMA */
    if (init_rdma(&rdma_ctx, use_event) < 0)
        return 1;
    
    /* Setup TCP connection */
    if (is_server) {
        int listen_fd = tcp_server_listen(18515);
        if (listen_fd < 0) {
            cleanup_rdma(&rdma_ctx);
            return 1;
        }
        connfd = tcp_accept(listen_fd);
        close(listen_fd);
        if (connfd < 0) {
            cleanup_rdma(&rdma_ctx);
            return 1;
        }
        sockfd = connfd;
    } else {
        sockfd = tcp_client_connect(server_ip, 18515);
        if (sockfd < 0) {
            cleanup_rdma(&rdma_ctx);
            return 1;
        }
    }
    
    /* Run ping-pong test */
    if (run_pingpong(&rdma_ctx, sockfd) < 0)
        ret = 1;
    
    cleanup_rdma(&rdma_ctx);
    return ret;
}
