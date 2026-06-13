/*
 * BPF Kprobe Syscall Demo
 *
 * 使用原始 BPF syscall 实现 kprobe 探测，不依赖 libbpf/BCC
 *
 * 编译: gcc -o bpf-kprobe-syscall-demo bpf-kprobe-syscall-demo.c
 * 运行: sudo ./bpf-kprobe-syscall-demo [function_name] [duration_sec]
 *
 * 通过这个程序，做一个简单的演示，就是 bpftrace 之类的程序是如何工作的
 *
 * 记得通过 /sys/kernel/debug/tracing/trace 观察，不是通过这个程序来观察
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/syscall.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/bpf.h>
#include <linux/perf_event.h>
#include <linux/version.h>
#include <asm/unistd.h>

/* BPF syscall 封装 */
static int sys_bpf(enum bpf_cmd cmd, union bpf_attr *attr, unsigned int size) {
    return syscall(__NR_bpf, cmd, attr, size);
}

/* perf_event_open 封装 */
static int sys_perf_event_open(struct perf_event_attr *attr, pid_t pid, int cpu,
                                int group_fd, unsigned long flags) {
    return syscall(__NR_perf_event_open, attr, pid, cpu, group_fd, flags);
}

/* 读取 kprobe PMU type */
static int get_kprobe_perf_type(void) {
    int type = -1;
    FILE *f = fopen("/sys/bus/event_source/devices/kprobe/type", "r");
    if (!f) {
        return -1;
    }
    if (fscanf(f, "%d", &type) != 1) {
        type = -1;
    }
    fclose(f);
    return type;
}

/* 加载简单的 BPF 程序 (只返回 0) */
static int load_simple_bpf_program(char *log_buf, size_t log_size) {
    union bpf_attr attr = {};
    int fd;

    /* 最简单的 BPF 程序：r0 = 0; exit */
    struct bpf_insn insns[] = {
        { .code = BPF_ALU64 | BPF_MOV | BPF_K, .dst_reg = BPF_REG_0, .imm = 0 },
        { .code = BPF_JMP | BPF_EXIT },
    };

    memset(&attr, 0, sizeof(attr));
    attr.prog_type = BPF_PROG_TYPE_KPROBE;
    attr.insns = (__u64)(unsigned long)insns;
    attr.insn_cnt = sizeof(insns) / sizeof(insns[0]);
    attr.license = (__u64)(unsigned long)"GPL";
    attr.log_buf = (__u64)(unsigned long)log_buf;
    attr.log_size = log_size;
    attr.log_level = 1;
    attr.kern_version = LINUX_VERSION_CODE;

    fd = sys_bpf(BPF_PROG_LOAD, &attr, sizeof(attr));
    return fd;
}

/* 使用 perf_event_open 创建 kprobe */
static int create_kprobe_event(const char *func_name, bool is_retprobe) {
    struct perf_event_attr attr = {};
    int type = get_kprobe_perf_type();
    int fd;

    if (type < 0)
        return -1;

    memset(&attr, 0, sizeof(attr));
    attr.type = type;
    attr.size = sizeof(attr);
    attr.config = is_retprobe ? 1 : 0;
    attr.kprobe_func = (__u64)(unsigned long)func_name;
    attr.probe_offset = 0;
    attr.sample_period = 1;
    attr.sample_type = PERF_SAMPLE_IP;
    attr.disabled = 1;

    fd = sys_perf_event_open(&attr, -1, 0, -1, 0);
    if (fd < 0 && errno == ENOENT) {
        fprintf(stderr, "Function '%s' not found (check /proc/kallsyms)\n", func_name);
    }
    return fd;
}

/* 将 BPF 程序附加到 perf event */
static int attach_bpf_to_perf_event(int prog_fd, int perf_fd) {
    int ret;

    ret = ioctl(perf_fd, PERF_EVENT_IOC_SET_BPF, prog_fd);
    if (ret < 0) {
        perror("PERF_EVENT_IOC_SET_BPF");
        return -1;
    }

    ret = ioctl(perf_fd, PERF_EVENT_IOC_ENABLE, 0);
    if (ret < 0) {
        perror("PERF_EVENT_IOC_ENABLE");
        return -1;
    }

    return 0;
}

/* 读取 perf event 计数 */
static uint64_t read_perf_counter(int fd) {
    uint64_t count = 0;
    ssize_t n = read(fd, &count, sizeof(count));
    if (n != sizeof(count)) {
        if (n < 0) perror("read perf counter");
        return 0;
    }
    return count;
}

int main(int argc, char *argv[]) {
    const char *func_name = (argc > 1) ? argv[1] : "schedule";
    bool is_retprobe = false;
    int prog_fd = -1;
    int perf_fd = -1;
    int ret;
    int duration_sec = (argc > 2) ? atoi(argv[2]) : 5;
    char log_buf[8192] = {};

    /* 检查是否是 kretprobe */
    if (strncmp(func_name, "ret:", 4) == 0) {
        func_name += 4;
        is_retprobe = true;
    }

    printf("========================================\n");
    printf("BPF Kprobe Syscall Demo\n");
    printf("========================================\n");
    printf("Target: %s%s\n", is_retprobe ? "ret:" : "", func_name);
    printf("Duration: %d seconds\n", duration_sec);
    printf("\n");

    /* 步骤 1: 加载 BPF 程序 */
    printf("[Step 1] Loading BPF program...\n");
    prog_fd = load_simple_bpf_program(log_buf, sizeof(log_buf));
    if (prog_fd < 0) {
        fprintf(stderr, "Failed to load BPF program:\n%s\n", log_buf);
        return 1;
    }
    printf("         BPF program loaded (fd=%d)\n", prog_fd);

    /* 步骤 2: 创建 kprobe perf event */
    printf("[Step 2] Creating kprobe event...\n");
    perf_fd = create_kprobe_event(func_name, is_retprobe);
    if (perf_fd < 0) {
        fprintf(stderr, "Failed to create kprobe event\n");
        close(prog_fd);
        return 1;
    }
    printf("         Kprobe event created (fd=%d)\n", perf_fd);

    /* 步骤 3: 附加 BPF 程序 */
    printf("[Step 3] Attaching BPF program...\n");
    ret = attach_bpf_to_perf_event(prog_fd, perf_fd);
    if (ret < 0) {
        fprintf(stderr, "Failed to attach BPF program\n");
        close(perf_fd);
        close(prog_fd);
        return 1;
    }
    printf("         BPF program attached and enabled\n");

    /* 步骤 4: 监控事件 */
    printf("\n");
    printf("Monitoring %s%s for %d seconds...\n", is_retprobe ? "ret:" : "", func_name, duration_sec);
    printf("(Generating some load with sleep commands)\n");
    printf("\n");

    uint64_t last_count = 0;

    for (int i = 0; i < duration_sec; i++) {
        /* 每秒钟生成一些调度事件 */
        if (i % 2 == 0) {
            system("for j in 1 2 3 4 5; do sleep 0.01 & done; wait 2>/dev/null");
        }

        uint64_t count = read_perf_counter(perf_fd);
        uint64_t delta = count - last_count;
        last_count = count;

        printf("  [%d/%d] Total calls: %lu, Last second: %lu\n", i + 1, duration_sec, count, delta);
        sleep(1);
    }

    /* 最终计数 */
    uint64_t final_count = read_perf_counter(perf_fd);

    printf("\n");
    printf("========================================\n");
    printf("Final result: %lu events captured\n", final_count);
    printf("========================================\n");
    printf("BPF kprobe demo completed successfully!\n");

    /* 清理 */
    close(perf_fd);
    close(prog_fd);

    return 0;
}
