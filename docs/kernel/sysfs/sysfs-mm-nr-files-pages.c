/*
 * 验证 tmpfs/shmem 与 /proc/meminfo 的关系
 *
 * 测试内容：
 * 1. Shmem 是否包含 tmpfs 文件
 * 2. Cached 是否包含 tmpfs 文件
 * 3. NR_FILE_PAGES 是否包含 shmem (通过 Cached 间接验证)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>

#define MB (1024 * 1024)
#define TEST_SIZE (50 * MB)  /* 50MB 测试 */

struct meminfo {
    long cached;      /* kB */
    long buffers;     /* kB */
    long shmem;       /* kB */
    long swapcached;  /* kB */
};

/* 从 /proc/meminfo 读取数值 */
static long read_meminfo_value(const char *name)
{
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("fopen /proc/meminfo");
        return -1;
    }

    char line[256];
    long value = -1;
    size_t namelen = strlen(name);

    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, name, namelen) == 0 && line[namelen] == ':') {
            sscanf(line + namelen + 1, "%ld", &value);
            break;
        }
    }

    fclose(fp);
    return value;
}

/* 读取所有关心的内存指标 */
static void read_meminfo(struct meminfo *mi)
{
    mi->cached = read_meminfo_value("Cached");
    mi->buffers = read_meminfo_value("Buffers");
    mi->shmem = read_meminfo_value("Shmem");
    mi->swapcached = read_meminfo_value("SwapCached");
}

/* 打印内存信息 */
static void print_meminfo(const char *title, const struct meminfo *mi)
{
    printf("\n=== %s ===\n", title);
    printf("Cached:      %8ld kB\n", mi->cached);
    printf("Buffers:     %8ld kB\n", mi->buffers);
    printf("Shmem:       %8ld kB\n", mi->shmem);
    printf("SwapCached:  %8ld kB\n", mi->swapcached);
}

/* 计算差值 */
static void calc_diff(const struct meminfo *before, const struct meminfo *after,
                      struct meminfo *diff)
{
    diff->cached = after->cached - before->cached;
    diff->buffers = after->buffers - before->buffers;
    diff->shmem = after->shmem - before->shmem;
    diff->swapcached = after->swapcached - before->swapcached;
}

/* 打印差值 */
static void print_diff(const struct meminfo *diff)
{
    printf("\n=== 变化量 ===\n");
    printf("Cached:      %8ld kB (%+.1f MB)\n", diff->cached, diff->cached / 1024.0);
    printf("Buffers:     %8ld kB (%+.1f MB)\n", diff->buffers, diff->buffers / 1024.0);
    printf("Shmem:       %8ld kB (%+.1f MB)\n", diff->shmem, diff->shmem / 1024.0);
    printf("SwapCached:  %8ld kB (%+.1f MB)\n", diff->swapcached, diff->swapcached / 1024.0);
}

/* 测试1: System V 共享内存 */
static void test_sysv_shm(void)
{
    struct meminfo before, after, diff;

    printf("\n========================================");
    printf("\n测试1: System V 共享内存 (shmget/shmat)");
    printf("\n========================================\n");

    read_meminfo(&before);
    print_meminfo("分配前", &before);

    /* 创建共享内存段 */
    int shmid = shmget(IPC_PRIVATE, TEST_SIZE, IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("shmget");
        return;
    }

    /* 附加到进程地址空间 */
    void *addr = shmat(shmid, NULL, 0);
    if (addr == (void *)-1) {
        perror("shmat");
        shmctl(shmid, IPC_RMID, NULL);
        return;
    }

    /* 写入数据，确保真正分配内存 */
    memset(addr, 0xAA, TEST_SIZE);

    /* 强制同步到内存 */
    sync();
    usleep(100000); /* 100ms，让内核更新统计 */

    read_meminfo(&after);
    print_meminfo("分配 50MB SHM 后", &after);

    calc_diff(&before, &after, &diff);
    print_diff(&diff);

    /* 结论 */
    printf("\n--- 结论 ---\n");
    if (diff.shmem > 40000) {
        printf("[PASS] System V SHM 计入 Shmem (+%ld kB)\n", diff.shmem);
    } else {
        printf("[FAIL] System V SHM 未明显计入 Shmem\n");
    }

    if (diff.cached > 40000) {
        printf("[PASS] System V SHM 计入 Cached (+%ld kB)\n", diff.cached);
    } else {
        printf("[INFO] System V SHM 对 Cached 影响: %ld kB\n", diff.cached);
    }

    /* 清理 */
    shmdt(addr);
    shmctl(shmid, IPC_RMID, NULL);
}

/* 测试2: POSIX 共享内存 (shm_open) */
static void test_posix_shm(void)
{
    struct meminfo before, after, diff;
    const char *shm_name = "/test_shmem_verify";

    printf("\n========================================");
    printf("\n测试2: POSIX 共享内存 (shm_open/mmap)");
    printf("\n========================================\n");

    /* 清理可能存在的旧共享内存 */
    shm_unlink(shm_name);

    read_meminfo(&before);
    print_meminfo("分配前", &before);

    /* 创建 POSIX 共享内存对象 */
    int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("shm_open");
        return;
    }

    /* 设置大小 */
    if (ftruncate(fd, TEST_SIZE) < 0) {
        perror("ftruncate");
        close(fd);
        shm_unlink(shm_name);
        return;
    }

    /* mmap 映射 */
    void *addr = mmap(NULL, TEST_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        shm_unlink(shm_name);
        return;
    }

    /* 写入数据 */
    memset(addr, 0xBB, TEST_SIZE);

    sync();
    usleep(100000);

    read_meminfo(&after);
    print_meminfo("分配 50MB POSIX SHM 后", &after);

    calc_diff(&before, &after, &diff);
    print_diff(&diff);

    printf("\n--- 结论 ---\n");
    if (diff.shmem > 40000) {
        printf("[PASS] POSIX SHM 计入 Shmem (+%ld kB)\n", diff.shmem);
    } else {
        printf("[FAIL] POSIX SHM 未明显计入 Shmem\n");
    }

    if (diff.cached > 40000) {
        printf("[PASS] POSIX SHM 计入 Cached (+%ld kB)\n", diff.cached);
    } else {
        printf("[INFO] POSIX SHM 对 Cached 影响: %ld kB\n", diff.cached);
    }

    /* 清理 */
    munmap(addr, TEST_SIZE);
    close(fd);
    shm_unlink(shm_name);
}

/* 测试3: tmpfs 文件 */
static void test_tmpfs_file(void)
{
    struct meminfo before, after, diff;
    const char *testfile = "/tmp/test_shmem_verify_file";

    printf("\n========================================");
    printf("\n测试3: tmpfs 文件 (/tmp 是 tmpfs)");
    printf("\n========================================\n");

    /* 清理 */
    unlink(testfile);

    read_meminfo(&before);
    print_meminfo("创建文件前", &before);

    /* 创建文件并写入 */
    int fd = open(testfile, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("open");
        return;
    }

    /* 写入 50MB */
    char buf[4096];
    memset(buf, 0xCC, sizeof(buf));
    for (int i = 0; i < TEST_SIZE / sizeof(buf); i++) {
        if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
            perror("write");
            close(fd);
            return;
        }
    }

    /* 确保数据写入 */
    fsync(fd);
    close(fd);

    sync();
    usleep(100000);

    read_meminfo(&after);
    print_meminfo("创建 50MB tmpfs 文件后", &after);

    calc_diff(&before, &after, &diff);
    print_diff(&diff);

    printf("\n--- 结论 ---\n");
    if (diff.shmem > 40000) {
        printf("[PASS] tmpfs 文件计入 Shmem (+%ld kB)\n", diff.shmem);
    } else {
        printf("[FAIL] tmpfs 文件未明显计入 Shmem\n");
    }

    if (diff.cached > 40000) {
        printf("[PASS] tmpfs 文件计入 Cached (+%ld kB)\n", diff.cached);
    } else {
        printf("[INFO] tmpfs 文件对 Cached 影响: %ld kB\n", diff.cached);
    }

    /* 清理 */
    unlink(testfile);
}

/* 测试4: 普通文件（用于对比） */
static void test_regular_file(void)
{
    struct meminfo before, after, diff;
    const char *testfile = "/var/tmp/test_regular_verify_file";

    printf("\n========================================");
    printf("\n测试4: 普通文件 (/var/tmp 是普通文件系统)");
    printf("\n========================================\n");

    unlink(testfile);

    read_meminfo(&before);
    print_meminfo("创建文件前", &before);

    int fd = open(testfile, O_CREAT | O_RDWR, 0666);
    if (fd < 0) {
        perror("open");
        return;
    }

    char buf[4096];
    memset(buf, 0xDD, sizeof(buf));
    for (int i = 0; i < TEST_SIZE / sizeof(buf); i++) {
        if (write(fd, buf, sizeof(buf)) != sizeof(buf)) {
            perror("write");
            close(fd);
            return;
        }
    }

    fsync(fd);
    close(fd);

    sync();
    usleep(100000);

    read_meminfo(&after);
    print_meminfo("创建 50MB 普通文件后", &after);

    calc_diff(&before, &after, &diff);
    print_diff(&diff);

    printf("\n--- 结论 ---\n");
    if (diff.shmem < 10000) {
        printf("[PASS] 普通文件不计入 Shmem (+%ld kB)\n", diff.shmem);
    } else {
        printf("[INFO] 普通文件对 Shmem 影响: %ld kB\n", diff.shmem);
    }

    if (diff.cached > 40000) {
        printf("[PASS] 普通文件计入 Cached (+%ld kB)\n", diff.cached);
    } else {
        printf("[INFO] 普通文件对 Cached 影响: %ld kB\n", diff.cached);
    }

    unlink(testfile);
}

int main(void)
{
    printf("========================================\n");
    printf("验证: tmpfs/shmem 与 /proc/meminfo 的关系\n");
    printf("========================================\n");
    printf("\n原理:\n");
    printf("- NR_FILE_PAGES 统计所有 file-backed 页面\n");
    printf("- /proc/meminfo 'Cached' = NR_FILE_PAGES - SwapCached - Buffers\n");
    printf("- Shmem (即 sharedram) = NR_SHMEM\n");
    printf("- tmpfs/shmem 同时计入 NR_FILE_PAGES 和 NR_SHMEM\n");

    test_sysv_shm();
    test_posix_shm();
    test_tmpfs_file();
    test_regular_file();

    printf("\n========================================\n");
    printf("最终结论\n");
    printf("========================================\n");
    printf("1. tmpfs/shmem 既计入 Shmem，也计入 Cached\n");
    printf("2. 因此 NR_FILE_PAGES 包含 tmpfs/shmem\n");
    printf("3. 普通文件只计入 Cached，不计入 Shmem\n");
    printf("4. Shmem 和 Cached 有重叠部分 (tmpfs/shmem)\n");

    return 0;
}
